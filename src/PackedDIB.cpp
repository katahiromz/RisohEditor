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
	DWORD ColorCount = 0;
	if (HeaderSize == sizeof(bc))
	{
		CopyMemory(&bc, pPackedDIB, sizeof(bc));

		switch (bc.bcBitCount)
		{
		case 1:     ColorCount = 2;     break;
		case 4:     ColorCount = 16;    break;
		case 8:     ColorCount = 256;   break;
		case 24:    break;
		default:
			assert(0);
			return 0;   // failure
		}

		Ret = bc.bcSize + ColorCount * sizeof(RGBTRIPLE);
		return (Ret <= dwSize) ? Ret : 0;
	}

	if (HeaderSize < sizeof(bi))
	{
		return 0;       // failure
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
		if (bi.biCompression == BI_BITFIELDS)
		{
			ColorCount = 3;
		}
		break;
	case 24:
		break;
	default:
		return 0;   // failure
	}

	Ret = bi.biSize + ColorCount * sizeof(RGBQUAD);
	if (Ret > dwSize)
	{
		assert(0);
		Ret = 0;
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
	DWORD Offset = PackedDIB_GetBitsOffset(pPackedDIB, dwSize);
	if (Offset == 0)
		return nullptr;

	LPBYTE pb = (LPBYTE)pPackedDIB + Offset;
	DWORD cbBits = dwSize - Offset;

	// NOTE: BITMAPINFO only has room for a single RGBQUAD in bmiColors.
	// Copying into a local BITMAPINFO truncates the color table for
	// 1/4/8-bpp DIBs, which corrupts the resulting bitmap's palette.
	// Use the original buffer directly instead, since it already
	// contains the full color table.
	LPBITMAPINFO pbi = (LPBITMAPINFO)pPackedDIB;

	// CreateDIBSection (the DIB_RGB_COLORS path below) hands back a raw
	// pixel buffer sized straight from biWidth/biBitCount -- it does not
	// decode BI_RLE4/BI_RLE8, so copying compressed source bytes into it
	// would just corrupt the image. BITMAPCOREHEADER (OS/2) packed DIBs
	// have no biCompression field at all -- never RLE -- and always take
	// that CreateDIBSection path, same as before.
	DWORD dwHeaderSize = *(const DWORD *)pPackedDIB;
	BOOL fCompressed = (dwHeaderSize >= sizeof(BITMAPINFOHEADER)) &&
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
#ifdef _MSC_VER
			// Win2k3 ieframe.dll BITMAP 214 causes exception
			__try
			{
				CopyMemory(pBits, pb, cbBits);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				;
			}
#else
			CopyMemory(pBits, pb, cbBits);
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
	// PackedDIB_* works on a "packed DIB" -- BITMAPINFOHEADER + color
	// table + bits, with NO BITMAPFILEHEADER -- exactly how RT_BITMAP
	// resources are stored. It's an easy mistake to instead pass the raw
	// bytes of an actual .bmp *file*, which does start with a 14-byte
	// BITMAPFILEHEADER ("BM" + bfSize/bfOffBits/...). Detect that case and
	// skip it transparently, rather than letting PackedDIB_GetBitsOffset
	// misread the file header as (garbage) BITMAPINFOHEADER fields and
	// fail outright.
	const BYTE *pb0 = (const BYTE *)ptr;
	if (siz > sizeof(BITMAPFILEHEADER) && pb0[0] == 'B' && pb0[1] == 'M')
	{
		ptr = pb0 + sizeof(BITMAPFILEHEADER);
		siz -= sizeof(BITMAPFILEHEADER);
	}

	HBITMAP hbm = PackedDIB_CreateBitmap(ptr, DWORD(siz));
	if (hbm)
	{
		HBITMAP hbmCopy = (HBITMAP)CopyImage(hbm, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG | LR_CREATEDIBSECTION);
		DeleteObject(hbm);
		return hbmCopy;
	}

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
