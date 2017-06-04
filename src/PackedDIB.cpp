// PackedDIB.cpp --- Packed DIB
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

    LPVOID pBits;
    HDC hDC = CreateCompatibleDC(NULL);
    HBITMAP hbm = CreateDIBSection(hDC, (LPBITMAPINFO)pPackedDIB,
                                   DIB_RGB_COLORS, &pBits, NULL, 0);
    DeleteDC(hDC);

    if (hbm)
    {
        LPBYTE pb = (LPBYTE)pPackedDIB;
        CopyMemory(pBits, pb + Offset, Size - Offset);
    }
    return hbm;
}

HICON
PackedDIB_CreateIcon(const void *pPackedDIB, DWORD Size, BITMAP& bm, BOOL bIcon)
{
    LPBYTE pb = (LPBYTE)(void *)pPackedDIB;

    int xHotSpot = 0, yHotSpot = 0;
    if (Size >= 4 && !bIcon)
    {
        xHotSpot = ((LPWORD)pb)[0];
        yHotSpot = ((LPWORD)pb)[1];
        pb += 2 * sizeof(WORD);
        Size -= 2 * sizeof(WORD);
    }

    if (!PackedDIB_GetInfo(pb, Size, bm))
    {
        assert(0);
        return NULL;
    }
    bm.bmHeight /= 2;

    HICON hIcon;
    if (bIcon)
    {
        hIcon = CreateIconFromResourceEx(pb, Size, bIcon, 0x00030000,
                                         bm.bmWidth, bm.bmHeight, 0);
        assert(hIcon);
    }
    else
    {
        LPBYTE pbBits = (LPBYTE)bm.bmBits;
        LPBYTE pbXor = pbBits;
        LPBYTE pbAnd = pbBits + bm.bmWidthBytes * bm.bmHeight;
        hIcon = CreateCursor(GetModuleHandle(NULL), xHotSpot, yHotSpot,
                             bm.bmWidth, bm.bmHeight, pbAnd, pbXor);
        assert(hIcon);
    }
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

    ByteStream bs;
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
