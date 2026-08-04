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
	DWORD Offset = PackedDIB_GetBitsOffset(pPackedDIB, dwSize);
	if (Offset == 0)
		return FALSE;   // failure

	const BYTE *pb = (const BYTE *)pPackedDIB;
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
	dwSize -= Offset;

	// NOTE: BITMAPINFO only has room for a single RGBQUAD in bmiColors.
	// Copying into a local BITMAPINFO truncates the color table for
	// 1/4/8-bpp DIBs, which corrupts the resulting bitmap's palette.
	// Use the original buffer directly instead, since it already
	// contains the full color table.
	LPBITMAPINFO pbi = (LPBITMAPINFO)pPackedDIB;
	LPVOID pBits;

	HBITMAP hbm;
	HDC hDC = CreateCompatibleDC(nullptr);
	if (!hDC)
		return nullptr;
	hbm = CreateDIBSection(hDC, pbi, DIB_RGB_COLORS, &pBits, nullptr, 0);
	DeleteDC(hDC);

	if (hbm)
	{
#ifdef _MSC_VER
		// Win2k3 ieframe.dll BITMAP 214 causes exception
		__try
		{
			CopyMemory(pBits, pb, dwSize);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			;
		}
#else
		CopyMemory(pBits, pb, dwSize);
#endif
	}

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

BOOL
PackedDIB_Extract(LPCWSTR FileName, const void *ptr, size_t siz, BOOL WritePNG)
{
	BITMAPFILEHEADER FileHeader;

	if (WritePNG)
	{
		BOOL ret = FALSE;
		HBITMAP hbm = PackedDIB_CreateBitmap(ptr, DWORD(siz));
		if (hbm == nullptr)
			return FALSE;

		Gdiplus::Bitmap *pBitmap = Gdiplus::Bitmap::FromHBITMAP(hbm, nullptr);
		if (pBitmap && pBitmap->GetLastStatus() == Gdiplus::Ok)
		{
			CLSID cls;
			if (GetEncoderClsid(L"image/png", &cls) != -1)
			{
				ret = pBitmap->Save(FileName, &cls, nullptr) == Gdiplus::Ok;
			}
		}
		delete pBitmap;
		DeleteObject(hbm);
		return ret;
	}

	FileHeader.bfType = 0x4d42;
	FileHeader.bfSize = (DWORD)(sizeof(FileHeader) + siz);
	FileHeader.bfReserved1 = 0;
	FileHeader.bfReserved2 = 0;

	DWORD dwOffset = PackedDIB_GetBitsOffset(ptr, DWORD(siz));
	if (dwOffset == 0)
		return FALSE;

	FileHeader.bfOffBits = sizeof(FileHeader) + dwOffset;

	MByteStreamEx bs;
	if (!bs.WriteRaw(FileHeader) || !bs.WriteData(ptr, siz))
		return FALSE;

	return bs.SaveToFile(FileName);
}

HBITMAP PackedDIB_CreateBitmapFromMemory(const void *ptr, size_t siz)
{
	HBITMAP hbm = nullptr;

	// Try a dirty hack for BI_RLE4, BI_RLE8, ...
	WCHAR szPath[MAX_PATH], szTempFile[MAX_PATH];
	GetTempPathW(_countof(szPath), szPath);
	GetTempFileNameW(szPath, L"reb", 0, szTempFile);

	if (PackedDIB_Extract(szTempFile, ptr, siz, FALSE))
	{
		hbm = (HBITMAP)LoadImageW(nullptr, szTempFile, IMAGE_BITMAP, 0, 0,
								  LR_LOADFROMFILE | LR_COLOR);
	}

	DeleteFileW(szTempFile);

	if (hbm == nullptr)
		hbm = PackedDIB_CreateBitmap(ptr, DWORD(siz));

	return hbm;
}
