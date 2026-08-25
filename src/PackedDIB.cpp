// PackedDIB.cpp --- Packed DIB
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "PackedDIB.hpp"
#include "MIdOrString.hpp"
#include "MByteStreamEx.hpp"
#include <strsafe.h>
#include <gdiplus.h>
#include <cassert>

#define WIDTHBYTES(i) (((i) + 31) / 32 * 4)

#ifndef BI_ALPHABITFIELDS
#define BI_ALPHABITFIELDS 6
#endif

//////////////////////////////////////////////////////////////////////////////
// DIB header kind detection.
//
// A "packed DIB" can start with any of several header structs, all of which
// begin with a DWORD giving the header's own size (bcSize/biSize). GDI (and
// this code) dispatches on that size to know which layout follows:
//
//   12  bytes -- BITMAPCOREHEADER  (used by BITMAPCOREINFO, OS/2 1.x)
//   16  bytes -- OS22XBITMAPHEADER, short form (OS/2 2.x)
//   40  bytes -- BITMAPINFOHEADER  (used by BITMAPINFO, the common Win32 case)
//   52  bytes -- BITMAPV2INFOHEADER (undocumented; adds RGB masks)
//   56  bytes -- BITMAPV3INFOHEADER (undocumented; adds an alpha mask)
//   64  bytes -- OS22XBITMAPHEADER, full form (OS/2 2.x)
//   108 bytes -- BITMAPV4HEADER    (adds RGBA masks, color space, gamma)
//   124 bytes -- BITMAPV5HEADER    (adds ICC profile info)
//
// Anything else is a header size this code doesn't recognize -- rather than
// guessing at its layout (and risking misreading garbage as width/height/bit
// count), callers should fail gracefully so the caller can fall back to a
// more general decoder (see PackedDIB_CreateBitmapFromMemory's GDI+ path).
enum DIB_HEADER_KIND
{
	DIBHDR_UNKNOWN,
	DIBHDR_CORE,    // BITMAPCOREHEADER / BITMAPCOREINFO
	DIBHDR_OS22,    // OS22XBITMAPHEADER (16 or 64 bytes)
	DIBHDR_INFO,    // BITMAPINFOHEADER / BITMAPINFO
	DIBHDR_V2,      // BITMAPV2INFOHEADER
	DIBHDR_V3,      // BITMAPV3INFOHEADER
	DIBHDR_V4,      // BITMAPV4HEADER
	DIBHDR_V5,      // BITMAPV5HEADER
};

static DIB_HEADER_KIND
PackedDIB_GetHeaderKind(DWORD dwHeaderSize)
{
	switch (dwHeaderSize)
	{
	case sizeof(BITMAPCOREHEADER): return DIBHDR_CORE;   // 12
	case 16:                       return DIBHDR_OS22;
	case sizeof(BITMAPINFOHEADER): return DIBHDR_INFO;   // 40
	case 52:                       return DIBHDR_V2;
	case 56:                       return DIBHDR_V3;
	case 64:                       return DIBHDR_OS22;
	case sizeof(BITMAPV4HEADER):   return DIBHDR_V4;     // 108
	case sizeof(BITMAPV5HEADER):   return DIBHDR_V5;     // 124
	default:                       return DIBHDR_UNKNOWN;
	}
}

//////////////////////////////////////////////////////////////////////////////

INT GetEncoderClsid(const WCHAR *format, CLSID *pClsid)
{
	UINT nCount = 0, cbItem = 0;

	Gdiplus::GetImageEncodersSize(&nCount, &cbItem);
	if (cbItem == 0)
		return -1;  // Failure

	Gdiplus::ImageCodecInfo *pInfo = nullptr;
	pInfo = (Gdiplus::ImageCodecInfo *)std::malloc(cbItem);
	if (pInfo == nullptr)
		return -1;  // Failure

	GetImageEncoders(nCount, cbItem, pInfo);

	for (UINT k = 0; k < nCount; ++k)
	{
		if (lstrcmpW(pInfo[k].MimeType, format) == 0)
		{
			*pClsid = pInfo[k].Clsid;
			std::free(pInfo);
			return k;  // Success
		}
	}

	std::free(pInfo);
	return -1;  // Failure
}

DWORD
PackedDIB_GetBitsOffset(const void *pPackedDIB, DWORD dwSize)
{
	BITMAPCOREHEADER bc;
	BITMAPINFOHEADER bi;
	DWORD Ret;

	if (pPackedDIB == nullptr || dwSize < sizeof(bc))
	{
		assert(0);
		return 0;   // failure
	}

	if (memcmp(pPackedDIB, "\x89\x50\x4E\x47", 4) == 0)
		return 0;   // PNG

	DWORD HeaderSize = *(DWORD *)pPackedDIB;
	if (HeaderSize > dwSize)
		return 0;

	DIB_HEADER_KIND Kind = PackedDIB_GetHeaderKind(HeaderSize);
	if (Kind == DIBHDR_UNKNOWN)
	{
		// A header size we don't recognize. Rather than misreading garbage
		// as width/height/bit-count fields, fail so the caller can fall
		// back to a more general decoder.
		return 0;   // failure
	}

	DWORD ColorCount = 0;
	if (Kind == DIBHDR_CORE)
	{
		CopyMemory(&bc, pPackedDIB, sizeof(bc));

		switch (bc.bcBitCount)
		{
		case 1:     ColorCount = 2;     break;
		case 4:     ColorCount = 16;    break;
		case 8:     ColorCount = 256;   break;
		case 24:    break;
		default:
			// Not a bit depth BITMAPCOREHEADER supports -- treat as an
			// unrecognized/malformed DIB rather than asserting.
			return 0;   // failure
		}

		Ret = bc.bcSize + ColorCount * sizeof(RGBTRIPLE);
		return (Ret <= dwSize) ? Ret : 0;
	}

	// DIBHDR_OS22 / DIBHDR_INFO / DIBHDR_V2 / DIBHDR_V3 / DIBHDR_V4 / DIBHDR_V5
	// all share the same first 40 bytes as BITMAPINFOHEADER (width, height,
	// planes, bit count, compression, sizes, color-table sizes, ...), so a
	// plain BITMAPINFOHEADER-shaped read of those first bytes is valid for
	// every one of them; the extra fields each adds (masks, gamma, ICC
	// profile info, ...) live after that and don't affect this calculation.
	if (HeaderSize < sizeof(bi))
	{
		return 0;       // failure (e.g. the 16-byte short OS22XBITMAPHEADER)
	}

	CopyMemory(&bi, pPackedDIB, sizeof(bi));

	switch (bi.biBitCount)
	{
	case 1:
		ColorCount = (bi.biClrUsed ? bi.biClrUsed : 2);
		break;
	case 4:
		ColorCount = (bi.biClrUsed ? bi.biClrUsed : 16);
		break;
	case 8:
		ColorCount = (bi.biClrUsed ? bi.biClrUsed : 256);
		break;
	case 16: case 32:
		if (Kind == DIBHDR_INFO &&
		    (bi.biCompression == BI_BITFIELDS || bi.biCompression == BI_ALPHABITFIELDS))
		{
			// Classic (40-byte) BITMAPINFOHEADER stores the R/G/B(/A) masks
			// as extra DWORDs appended right after the header, taking the
			// place of a color table. BITMAPV2/V3/V4/V5 headers embed those
			// same masks as fields *inside* the (already larger) header, so
			// there is nothing extra to skip for those -- adding this
			// adjustment for them would push the offset past the real
			// start of the pixel data.
			ColorCount = (bi.biCompression == BI_ALPHABITFIELDS) ? 4 : 3;
		}
		else if (bi.biClrUsed)
		{
			// Uncommon, but 16/32bpp DIBs may still carry an optional color
			// table (e.g. for a palette hint); honor biClrUsed if present.
			ColorCount = bi.biClrUsed;
		}
		break;
	case 24:
		if (bi.biClrUsed)
			ColorCount = bi.biClrUsed;
		break;
	default:
		return 0;   // failure -- not a bit depth we recognize
	}

	Ret = bi.biSize + ColorCount * sizeof(RGBQUAD);
	if (Ret > dwSize)
	{
		// Header claims a color table that doesn't fit in the buffer we
		// were given -- treat as malformed/unrecognized input rather than
		// asserting, so callers can fail over to another decoder.
		return 0;
	}
	return Ret;
}

BOOL
PackedDIB_GetInfo(const void *pPackedDIB, DWORD dwSize, BITMAP& bm)
{
	const BYTE* pb = (const BYTE*)pPackedDIB;
	if (dwSize > sizeof(BITMAPFILEHEADER) && pb[0] == 'B' && pb[1] == 'M')
	{
		pb += sizeof(BITMAPFILEHEADER);
		pPackedDIB = pb;
		dwSize -= sizeof(BITMAPFILEHEADER);
	}

	DWORD Offset = PackedDIB_GetBitsOffset(pPackedDIB, dwSize);
	if (Offset == 0)
		return FALSE;   // failure

	DWORD HeaderSize = *(const DWORD *)pPackedDIB;
	if (HeaderSize == sizeof(BITMAPCOREHEADER))
	{
		BITMAPCOREHEADER *pbc = (BITMAPCOREHEADER *)pPackedDIB;
		bm.bmType = 0;
		bm.bmWidth = pbc->bcWidth;
		bm.bmHeight = pbc->bcHeight;
		bm.bmWidthBytes = WIDTHBYTES(pbc->bcWidth * pbc->bcBitCount);
		bm.bmPlanes = 1;
		bm.bmBitsPixel = pbc->bcBitCount;
		bm.bmBits = (LPVOID)(LPBYTE)(pb + Offset);
		return TRUE;
	}

	if (HeaderSize >= sizeof(BITMAPINFOHEADER))
	{
		BITMAPINFOHEADER *pbi = (BITMAPINFOHEADER *)pPackedDIB;
		bm.bmType = 0;
		bm.bmWidth = pbi->biWidth;
		bm.bmHeight = pbi->biHeight;
		bm.bmWidthBytes = WIDTHBYTES(pbi->biWidth * pbi->biBitCount);
		bm.bmPlanes = 1;
		bm.bmBitsPixel = pbi->biBitCount;
		bm.bmBits = (LPVOID)(LPBYTE)(pb + Offset);
		return TRUE;
	}

	assert(0);
	return FALSE;    // failure
}

HBITMAP
PackedDIB_CreateBitmap(const void *pPackedDIB, DWORD dwSize)
{
	if (pPackedDIB == nullptr || dwSize < sizeof(DWORD))
		return nullptr;

	// Identify which DIB header this is up front. Header sizes this code
	// doesn't recognize (not BITMAPCOREHEADER/OS22XBITMAPHEADER/
	// BITMAPINFOHEADER/BITMAPV2..V5HEADER) are rejected here rather than
	// guessed at, so the caller (PackedDIB_CreateBitmapFromMemory) can fall
	// back to its GDI+ / LoadImage decoding paths instead.
	DWORD dwHeaderSize = *(const DWORD *)pPackedDIB;
	DIB_HEADER_KIND Kind = PackedDIB_GetHeaderKind(dwHeaderSize);
	if (Kind == DIBHDR_UNKNOWN)
		return nullptr;

	DWORD Offset = PackedDIB_GetBitsOffset(pPackedDIB, dwSize);
	if (Offset == 0)
		return nullptr;

	LPBYTE pb = (LPBYTE)pPackedDIB + Offset;
	DWORD cbBits = dwSize - Offset;

	// NOTE: BITMAPINFO only has room for a single RGBQUAD in bmiColors.
	// Copying into a local BITMAPINFO truncates the color table for
	// 1/4/8-bpp DIBs, which corrupts the resulting bitmap's palette.
	// Use the original buffer directly instead, since it already
	// contains the full color table. This is also required for
	// BITMAPV4HEADER/BITMAPV5HEADER: GDI dispatches on the leading
	// biSize field itself (CreateDIBSection, SetDIBits, StretchDIBits,
	// ... all understand BITMAPCOREHEADER, BITMAPINFOHEADER, and the V4/V5
	// headers natively), so passing the original buffer straight through
	// is correct -- and necessary -- for every header kind here.
	LPBITMAPINFO pbi = (LPBITMAPINFO)pPackedDIB;

	// CreateDIBSection (the DIB_RGB_COLORS path below) hands back a raw
	// pixel buffer sized straight from biWidth/biBitCount -- it does not
	// decode BI_RLE4/BI_RLE8, so copying compressed source bytes into it
	// would just corrupt the image. BI_RLE4/BI_RLE8 are only meaningful
	// for BITMAPINFOHEADER-and-newer headers (BITMAPCOREHEADER and
	// OS22XBITMAPHEADER packed DIBs have no such compression here) and
	// always take the plain CreateDIBSection path below, same as before.
	BOOL fCompressed = (Kind != DIBHDR_CORE && Kind != DIBHDR_OS22) &&
	                   (pbi->bmiHeader.biCompression == BI_RLE4 ||
	                    pbi->bmiHeader.biCompression == BI_RLE8);

	HDC hDC = CreateCompatibleDC(nullptr);
	if (!hDC)
		return nullptr;

	HBITMAP hbm;
	if (fCompressed)
	{
		// For RLE data, decode straight into a DIB section of our own
		// choosing (24-bit BI_RGB) via SetDIBits, instead of going through
		// CreateDIBitmap. CreateDIBitmap builds a device-dependent bitmap
		// matching the *current display*, which means GDI has to color-
		// match every decoded pixel against whatever the display can
		// currently show -- on anything other than a plain truecolor
		// desktop (a palettized/limited-color device, a remote session,
		// ...) that shows up as washed-out/blotchy colors. Targeting an
		// explicit 24-bit DIB section instead means GDI decodes the RLE
		// and converts each palette entry straight to its exact RGB value
		// with no device-dependent approximation involved.
		BITMAPINFOHEADER bmihDst = { sizeof(bmihDst) };
		bmihDst.biWidth = pbi->bmiHeader.biWidth;
		bmihDst.biHeight = pbi->bmiHeader.biHeight;
		bmihDst.biPlanes = 1;
		bmihDst.biBitCount = 24;
		bmihDst.biCompression = BI_RGB;

		LPVOID pDstBits = nullptr;
		hbm = CreateDIBSection(hDC, (LPBITMAPINFO)&bmihDst, DIB_RGB_COLORS, &pDstBits, nullptr, 0);
		if (hbm)
		{
			// lpbmi here is still the *source* description (pbi, with its
			// original biBitCount/biCompression/color table) -- SetDIBits
			// decodes from that format into hbm's (the 24-bit target's).
			INT nLines = SetDIBits(hDC, hbm, 0, pbi->bmiHeader.biHeight, pb, pbi, DIB_RGB_COLORS);
			if (nLines == 0)
			{
				DeleteObject(hbm);
				hbm = nullptr;
			}
		}
	}
	else
	{
		LPVOID pBits;
		hbm = CreateDIBSection(hDC, pbi, DIB_RGB_COLORS, &pBits, nullptr, 0);

		if (hbm)
		{
			DWORD cbNeeded = 0;
			if (pbi->bmiHeader.biSizeImage != 0)
				cbNeeded = pbi->bmiHeader.biSizeImage;
			else
				cbNeeded = WIDTHBYTES(pbi->bmiHeader.biWidth * pbi->bmiHeader.biBitCount) *
				           abs(pbi->bmiHeader.biHeight);

			DWORD cbCopy = (cbBits < cbNeeded) ? cbBits : cbNeeded;

#ifdef _MSC_VER
			// Win2k3 ieframe.dll BITMAP 214 causes exception
			__try
			{
				CopyMemory(pBits, pb, cbCopy);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				;
			}
#else
			CopyMemory(pBits, pb, cbCopy);
#endif
		}
	}

	DeleteDC(hDC);

	return hbm;
}

HICON
PackedDIB_CreateIcon(const void *pPackedDIB, DWORD dwSize, BITMAP& bm, BOOL bIcon)
{
	LPBYTE pb = (LPBYTE)(void *)pPackedDIB;

	//int xHotSpot = 0, yHotSpot = 0;
	if (!bIcon)
	{
		if (dwSize < 2 * sizeof(WORD))
			return nullptr;
		//xHotSpot = ((LPWORD)pb)[0];
		//yHotSpot = ((LPWORD)pb)[1];
		pb += 2 * sizeof(WORD);
		dwSize -= 2 * sizeof(WORD);
	}

	if (!PackedDIB_GetInfo(pb, dwSize, bm))
	{
		return nullptr;
	}
	bm.bmHeight /= 2;

	if (!bIcon)
	{
		pb -= 2 * sizeof(WORD);
		dwSize += 2 * sizeof(WORD);
	}

	HICON hIcon;
	hIcon = CreateIconFromResourceEx(pb, dwSize, bIcon, 0x00030000,
									 bm.bmWidth, bm.bmHeight, 0);
	assert(hIcon);
	return hIcon;
}

typedef struct tagBITMAPINFOEX
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD          bmiColors[256];
} BITMAPINFOEX, FAR * LPBITMAPINFOEX;

BOOL
PackedDIB_CreateFromHandle(std::vector<BYTE>& vecData, HBITMAP hbm)
{
	vecData.clear();

	BITMAP bm;
	if (!GetObject(hbm, sizeof(bm), &bm))
		return FALSE;

	BITMAPINFOEX bi;
	BITMAPINFOHEADER *pbmih;
	DWORD cColors, cbColors;

	pbmih = &bi.bmiHeader;
	ZeroMemory(pbmih, sizeof(BITMAPINFOHEADER));
	pbmih->biSize             = sizeof(BITMAPINFOHEADER);
	pbmih->biWidth            = bm.bmWidth;
	pbmih->biHeight           = bm.bmHeight;
	pbmih->biPlanes           = 1;
	pbmih->biBitCount         = bm.bmBitsPixel;
	pbmih->biCompression      = BI_RGB;
	pbmih->biSizeImage        = bm.bmWidthBytes * bm.bmHeight;

	if (bm.bmBitsPixel < 16)
		cColors = 1 << bm.bmBitsPixel;
	else
		cColors = 0;
	cbColors = cColors * sizeof(RGBQUAD);

	std::vector<BYTE> Bits(pbmih->biSizeImage);
	HDC hDC = CreateCompatibleDC(nullptr);
	if (hDC == nullptr)
		return FALSE;

	LPBITMAPINFO pbi = LPBITMAPINFO(&bi);
	if (!GetDIBits(hDC, hbm, 0, bm.bmHeight, &Bits[0], pbi, DIB_RGB_COLORS))
	{
		DeleteDC(hDC);
		return FALSE;
	}

	DeleteDC(hDC);

	MByteStreamEx bs;
	if (bs.WriteRaw(*pbmih) &&
		bs.WriteData(bi.bmiColors, cbColors) &&
		bs.WriteData(&Bits[0], Bits.size()))
	{
		vecData = bs.data();
		return TRUE;
	}
	return FALSE;
}

HBITMAP PackedDIB_CreateBitmapFromMemory(const void *ptr, size_t siz)
{
	HBITMAP hbm = PackedDIB_CreateBitmap(ptr, DWORD(siz));
	if (hbm)
	{
		HBITMAP hbmCopy = (HBITMAP)CopyImage(hbm, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG | LR_CREATEDIBSECTION);
		DeleteObject(hbm);
		return hbmCopy;
	}

	// Load bitmap from memory using GDI+
	using namespace Gdiplus;
	IStream *pStream = nullptr;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, siz);
	if (hMem)
	{
		void *pData = GlobalLock(hMem);
		if (pData)
		{
			CopyMemory(pData, ptr, siz);
			GlobalUnlock(hMem);

			if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK)
			{
				Bitmap *pBmp = Bitmap::FromStream(pStream);
				if (pBmp && pBmp->GetLastStatus() == Ok)
				{
					Status st = pBmp->GetHBITMAP(Color(0,0,0,0), &hbm);
					if (st != Ok)
						hbm = nullptr;
				}
				delete pBmp;
				pStream->Release();
			}
			else
			{
				GlobalFree(hMem);
			}
		}
		else
		{
			GlobalFree(hMem);
		}
	}
	if (hbm)
		return hbm;

	// Load bitmap using temporary file
	WCHAR szPath[MAX_PATH], szTempFile[MAX_PATH];
	if (GetTempPathW(_countof(szPath), szPath) &&
		GetTempFileNameW(szPath, L"reb", 0, szTempFile))
	{
		if (FILE *fout = _wfopen(szTempFile, L"wb"))
		{
			BOOL bOK = !!fwrite(ptr, siz, 1, fout);
			fclose(fout);
			if (bOK)
			{
				hbm = (HBITMAP)LoadImageW(nullptr, szTempFile, IMAGE_BITMAP, 0, 0,
										  LR_LOADFROMFILE | LR_COLOR);
			}
		}
		DeleteFileW(szTempFile);
	}

	return hbm;
}

static inline WORD ReadU16(const std::vector<BYTE>& d, size_t off)
{
	DWORD dw0 = d[off], dw1 = d[off + 1];
	return static_cast<WORD>(dw0 | (dw1 << 8));
}

static inline DWORD ReadU32(const std::vector<BYTE>& d, size_t off)
{
	DWORD dw0 = d[off + 0], dw1 = d[off + 1], dw2 = d[off + 2], dw3 = d[off + 3];
	return static_cast<DWORD>(dw0 | (dw1 << 8) | (dw2 << 16) | (dw3 << 24));
}

BOOL Bitmap_DropFileHeader(std::vector<BYTE>& data)
{
	if (data.size() < sizeof(BITMAPFILEHEADER) || data[0] != 'B' || data[1] != 'M')
		return FALSE;

	BITMAPFILEHEADER fileHeader;
	memcpy(&fileHeader, data.data(), sizeof(BITMAPFILEHEADER));

	if (fileHeader.bfOffBits < sizeof(BITMAPFILEHEADER) || fileHeader.bfOffBits > data.size())
		return FALSE;

	data.erase(data.begin(), data.begin() + sizeof(BITMAPFILEHEADER));
	return TRUE;
}

BOOL Bitmap_AddFileHeader(std::vector<BYTE>& data)
{
	if (data.size() < 4)
		return FALSE;

	const DWORD biSize = ReadU32(data, 0);
	if (data.size() < biSize)
		return FALSE;

	WORD  biBitCount = 0;
	DWORD biCompression = BI_RGB;
	DWORD biClrUsed = 0;
	DWORD paletteEntrySize = sizeof(RGBQUAD);
	DWORD headerSize = biSize;
	DWORD extraMaskBytes = 0;

	if (biSize == sizeof(BITMAPCOREHEADER))
	{
		if (data.size() < sizeof(BITMAPCOREHEADER))
			return FALSE;
		biBitCount = ReadU16(data, 10);
		biCompression = 0;
		biClrUsed = 0;
		paletteEntrySize = sizeof(RGBTRIPLE);
	}
	else if (biSize >= sizeof(BITMAPINFOHEADER))
	{
		biBitCount = ReadU16(data, 14);
		biCompression = ReadU32(data, 16);
		biClrUsed = ReadU32(data, 32);
		paletteEntrySize = sizeof(RGBQUAD);

		if (biSize == sizeof(BITMAPINFOHEADER))
		{
#ifndef BI_ALPHABITFIELDS
	#define BI_ALPHABITFIELDS 6
#endif
			if (biCompression == BI_BITFIELDS)
				extraMaskBytes = 12; // R,G,B
			else if (biCompression == BI_ALPHABITFIELDS)
				extraMaskBytes = 16; // R,G,B,A
		}
	}
	else
	{
		biBitCount = 0;
		biCompression = 0;
		biClrUsed  = 0;
		paletteEntrySize = 4;
	}

	if (biClrUsed > 256)
		biClrUsed = 0;

	DWORD colorTableEntries = 0;
	if (biClrUsed != 0)
		colorTableEntries = biClrUsed;
	else if (biBitCount != 0 && biBitCount <= 8)
		colorTableEntries = (1u << biBitCount);
	const DWORD colorTableBytes = colorTableEntries * paletteEntrySize;

	const DWORD fileHeaderSize = sizeof(BITMAPFILEHEADER);
	DWORD offBits = fileHeaderSize + headerSize + extraMaskBytes + colorTableBytes;

	if (offBits - fileHeaderSize > static_cast<DWORD>(data.size()))
		offBits = fileHeaderSize + headerSize;

	const DWORD fileSize = fileHeaderSize + static_cast<DWORD>(data.size());

	BITMAPFILEHEADER header;
	header.bfType      = 0x4D42; // "BM"
	header.bfSize      = fileSize;
	header.bfReserved1 = 0;
	header.bfReserved2 = 0;
	header.bfOffBits   = offBits;
	const BYTE* pb = reinterpret_cast<const BYTE*>(&header);
	data.insert(data.begin(), pb, pb + sizeof(header));
	return TRUE;
}
