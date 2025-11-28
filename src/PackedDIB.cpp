// PackedDIB.cpp --- Packed DIB
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#include "PackedDIB.hpp"
#include "MIdOrString.hpp"
#include "MByteStreamEx.hpp"
#include <strsafe.h>
#include <gdiplus.h>

#define WIDTHBYTES(i) (((i) + 31) / 32 * 4)

//////////////////////////////////////////////////////////////////////////////

inline INT GetEncoderClsid(const WCHAR *format, CLSID *pClsid)
{
    UINT nCount = 0, cbItem = 0;

    Gdiplus::GetImageEncodersSize(&nCount, &cbItem);
    if (cbItem == 0)
        return -1;  // Failure

    Gdiplus::ImageCodecInfo *pInfo = NULL;
    pInfo = (Gdiplus::ImageCodecInfo *)std::malloc(cbItem);
    if (pInfo == NULL)
        return -1;  // Failure

    GetImageEncoders(nCount, cbItem, pInfo);

    for (UINT k = 0; k < nCount; ++k)
    {
        if (lstrcmpW(pInfo[k].MimeType, format) == 0)
        {
            *pClsid = pInfo[k].Clsid;
            free(pInfo);
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

    if (pPackedDIB == NULL || dwSize < sizeof(bc))
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
        assert(0);
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
        assert(0);
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
        return NULL;

    LPBYTE pb = (LPBYTE)pPackedDIB + Offset;
    dwSize -= Offset;

    BITMAPINFO bi = *(const BITMAPINFO *)pPackedDIB;
    //BITMAPINFOHEADER *pbmih = &bi.bmiHeader;
    LPVOID pBits;

    HBITMAP hbm;
    HDC hDC = CreateCompatibleDC(NULL);
    hbm = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
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
        //xHotSpot = ((LPWORD)pb)[0];
        //yHotSpot = ((LPWORD)pb)[1];
        pb += 2 * sizeof(WORD);
        dwSize -= 2 * sizeof(WORD);
    }

    if (!PackedDIB_GetInfo(pb, dwSize, bm))
    {
        return NULL;
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
    HDC hDC = CreateCompatibleDC(NULL);
    if (hDC == NULL)
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
        Gdiplus::Bitmap *pBitmap = Gdiplus::Bitmap::FromHBITMAP(hbm, NULL);
        if (pBitmap)
        {
            CLSID cls;
            if (GetEncoderClsid(L"image/png", &cls) != -1)
            {
                ret = pBitmap->Save(FileName, &cls, NULL) == Gdiplus::Ok;
            }
            delete pBitmap;
        }
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
    HBITMAP hbm = NULL;

    // Try a dirty hack for BI_RLE4, BI_RLE8, ...
    WCHAR szPath[MAX_PATH], szTempFile[MAX_PATH];
    GetTempPathW(_countof(szPath), szPath);
    GetTempFileNameW(szPath, L"reb", 0, szTempFile);

    if (PackedDIB_Extract(szTempFile, ptr, siz, FALSE))
    {
        hbm = (HBITMAP)LoadImageW(NULL, szTempFile, IMAGE_BITMAP, 0, 0,
                                  LR_LOADFROMFILE | LR_COLOR);
    }

    DeleteFileW(szTempFile);

    if (hbm == NULL)
        hbm = PackedDIB_CreateBitmap(ptr, DWORD(siz));

    return hbm;
}
