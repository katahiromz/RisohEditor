// PackedDIB.cpp --- Packed DIB
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Win32API resource editor
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

#include "stdafx.hpp"

#define WIDTHBYTES(i) (((i) + 31) / 32 * 4)

//////////////////////////////////////////////////////////////////////////////

DWORD
PackedDIB_GetBitsOffset(const void *pPackedDIB, DWORD Size)
{
    BITMAPCOREHEADER bc;
    BITMAPINFOHEADER bi;
    DWORD Ret;

    if (pPackedDIB == NULL || Size < sizeof(bc))
    {
        assert(0);
        return 0;   // failure
    }

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
        return (Ret <= Size) ? Ret : 0;
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
    if (Ret > Size)
    {
        assert(0);
        Ret = 0;
    }
    return Ret;
}

BOOL
PackedDIB_GetInfo(const void *pPackedDIB, DWORD Size, BITMAP& bm)
{
    DWORD Offset = PackedDIB_GetBitsOffset(pPackedDIB, Size);
    if (Offset == 0)
    {
        assert(0);
        return FALSE;   // failure
    }

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
PackedDIB_CreateBitmap(const void *pPackedDIB, DWORD Size)
{
    DWORD Offset = PackedDIB_GetBitsOffset(pPackedDIB, Size);
    if (Offset == 0)
        return NULL;

    LPBYTE pb = (LPBYTE)pPackedDIB + Offset;
    Size -= Offset;

    BITMAPINFO bi = *(const BITMAPINFO *)pPackedDIB;
    //BITMAPINFOHEADER *pbmih = &bi.bmiHeader;
    LPVOID pBits;

    HBITMAP hbm;
    HDC hDC = CreateCompatibleDC(NULL);
    hbm = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
    DeleteDC(hDC);

    // FIXME: BI_RLE4 and BI_RLE8
    if (hbm)
    {
        CopyMemory(pBits, pb, Size);
    }

    return hbm;
}

HICON
PackedDIB_CreateIcon(const void *pPackedDIB, DWORD Size, BITMAP& bm, BOOL bIcon)
{
    LPBYTE pb = (LPBYTE)(void *)pPackedDIB;

    //int xHotSpot = 0, yHotSpot = 0;
    if (!bIcon)
    {
        //xHotSpot = ((LPWORD)pb)[0];
        //yHotSpot = ((LPWORD)pb)[1];
        pb += 2 * sizeof(WORD);
        Size -= 2 * sizeof(WORD);
    }

    if (!PackedDIB_GetInfo(pb, Size, bm))
    {
        assert(0);
        return NULL;
    }
    bm.bmHeight /= 2;

    if (!bIcon)
    {
        pb -= 2 * sizeof(WORD);
        Size += 2 * sizeof(WORD);
    }

    HICON hIcon;
    hIcon = CreateIconFromResourceEx(pb, Size, bIcon, 0x00030000,
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
PackedDIB_CreateFromHandle(std::vector<BYTE>& Data, HBITMAP hbm)
{
    Data.clear();

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
        Data = bs.data();
        return TRUE;
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
