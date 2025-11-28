// MBitmapDx.cpp --- GDI+ Bitmap wrapper                        -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
//////////////////////////////////////////////////////////////////////////////

#include "MBitmapDx.hpp"

//////////////////////////////////////////////////////////////////////////////

MBitmapDx::MBitmapDx()
{
    m_pBitmap = NULL;
    m_rgbBack = RGB(255, 255, 255);
    m_nFrameIndex = 0;
    m_nFrameCount = 0;
    m_pDelayItem = NULL;
    m_nLoopIndex = 0;
    m_nLoopCount = (UINT)-1;
    m_hGlobal = NULL;
}

MBitmapDx::~MBitmapDx()
{
    Destroy();
}

void MBitmapDx::FreeBitmap()
{
    if (m_pBitmap)
    {
        delete m_pBitmap;
        m_pBitmap = NULL;
    }
}

void MBitmapDx::FreeDelayPropertyItem()
{
    if (m_pDelayItem)
    {
        std::free(m_pDelayItem);
        m_pDelayItem = NULL;
    }
}

void MBitmapDx::Destroy()
{
    m_nFrameIndex = 0;
    m_nFrameCount = 0;
    m_nLoopIndex = 0;
    m_nLoopCount = (UINT)-1;

    FreeBitmap();
    FreeDelayPropertyItem();

    if (m_hGlobal)
    {
        GlobalFree(m_hGlobal);
        m_hGlobal = NULL;
    }
}

BOOL MBitmapDx::CreateInternal()
{
    using namespace Gdiplus;
    UINT nDimCount = m_pBitmap->GetFrameDimensionsCount();

    if (nDimCount)
    {
        std::vector<GUID> dims(nDimCount);
        m_pBitmap->GetFrameDimensionsList(&dims[0], nDimCount);
        m_nFrameCount = m_pBitmap->GetFrameCount(&dims[0]);
    }

    UINT cbItem;

    FreeDelayPropertyItem();
    cbItem = m_pBitmap->GetPropertyItemSize(PropertyTagFrameDelay);
    if (cbItem)
    {
        m_pDelayItem = (PropertyItem *)std::malloc(cbItem);
        m_pBitmap->GetPropertyItem(PropertyTagFrameDelay, cbItem, m_pDelayItem);
    }
    else
    {
        m_pDelayItem = NULL;
    }

    m_nLoopIndex = 0;
    m_nLoopCount = (UINT)-1;
    cbItem = m_pBitmap->GetPropertyItemSize(PropertyTagLoopCount);
    if (cbItem)
    {
        PropertyItem *pItem = (PropertyItem *)std::malloc(cbItem);
        if (pItem)
        {
            if (m_pBitmap->GetPropertyItem(PropertyTagLoopCount, cbItem, pItem) == Ok)
            {
                m_nLoopCount = *(WORD *)pItem->value;
            }
            std::free(pItem);
        }
    }

    return TRUE;
}

BOOL MBitmapDx::SetBitmap(Gdiplus::Bitmap *pBitmap)
{
    Destroy();

    m_pBitmap = pBitmap;
    return m_pBitmap && CreateInternal();
}

BOOL MBitmapDx::CreateFromMemory(const void *pvData, DWORD dwSize)
{
    Destroy();

    Gdiplus::Bitmap *pBitmap = NULL;

    m_hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwSize);
    if (m_hGlobal)
    {
        LPVOID pv = GlobalLock(m_hGlobal);
        if (pv)
        {
            CopyMemory(pv, pvData, dwSize);
            GlobalUnlock(m_hGlobal);
        }

        IStream *pStream = NULL;
        if (CreateStreamOnHGlobal(m_hGlobal, FALSE, &pStream) == S_OK)
        {
            pBitmap = Gdiplus::Bitmap::FromStream(pStream);
            pStream->Release();
        }
    }

    m_pBitmap = pBitmap;
    return m_pBitmap && CreateInternal();
}

void MBitmapDx::SetFrameIndex(UINT nFrameIndex)
{
    if (nFrameIndex < m_nFrameCount)
    {
        GUID guid = Gdiplus::FrameDimensionTime;
        if (Gdiplus::Ok != m_pBitmap->SelectActiveFrame(&guid, nFrameIndex))
        {
            guid = Gdiplus::FrameDimensionPage;
            m_pBitmap->SelectActiveFrame(&guid, nFrameIndex);
        }
    }
    m_nFrameIndex = nFrameIndex;
}

HBITMAP MBitmapDx::GetHBITMAP(LONG& cx, LONG& cy)
{
    cx = (LONG)m_pBitmap->GetWidth();
    cy = (LONG)m_pBitmap->GetHeight();

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    LPVOID pvBits;
    HBITMAP hbm = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pvBits, 
                                   NULL, 0);
    if (hbm == NULL)
        return NULL;

    HDC hDC = CreateCompatibleDC(NULL);
    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        RECT rc = { 0, 0, cx, cy };

        HBRUSH hbr = CreateSolidBrush(m_rgbBack);
        FillRect(hDC, &rc, hbr);
        DeleteObject(hbr);

        Gdiplus::Graphics g(hDC);
        g.DrawImage(m_pBitmap, 0, 0, (INT)cx, (INT)cy);
    }
    SelectObject(hDC, hbmOld);
    DeleteDC(hDC);

    return hbm;
}

HBITMAP MBitmapDx::GetHBITMAP32(LONG& cx, LONG& cy)
{
    using namespace Gdiplus;
    if (!(m_pBitmap->GetPixelFormat() & PixelFormatAlpha))
    {
        return GetHBITMAP(cx, cy);
    }

    cx = m_pBitmap->GetWidth();
    cy = m_pBitmap->GetHeight();

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = -cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    LPVOID pvBits;
    HBITMAP hbm;
    HDC hdc = CreateCompatibleDC(NULL);
    hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    DeleteDC(hdc);
    if (hbm == NULL)
        return NULL;

    for (LONG y = 0; y < cy; ++y)
    {
        for (LONG x = 0; x < cx; ++x)
        {
            Color color;
            m_pBitmap->GetPixel(x, y, &color);
            BYTE a = color.GetA();
            BYTE r = color.GetR();
            BYTE g = color.GetG();
            BYTE b = color.GetB();
            ((LPBYTE)pvBits)[(x + y * cx) * 4 + 0] = b;
            ((LPBYTE)pvBits)[(x + y * cx) * 4 + 1] = g;
            ((LPBYTE)pvBits)[(x + y * cx) * 4 + 2] = r;
            ((LPBYTE)pvBits)[(x + y * cx) * 4 + 3] = a;
        }
    }

    return hbm;
}

BOOL MBitmapDx::Step(DWORD& dwDelay)
{
    dwDelay = INFINITE;
    if (m_nLoopCount == (UINT)-1)
        return FALSE;

    if (m_nFrameIndex + 1 < m_nFrameCount)
    {
        dwDelay = GetFrameDelay(m_nFrameIndex);
        SetFrameIndex(m_nFrameIndex);
        ++m_nFrameIndex;
        return TRUE;
    }

    if (m_nLoopCount == 0 || m_nLoopIndex < m_nLoopCount)
    {
        dwDelay = GetFrameDelay(m_nFrameIndex);
        SetFrameIndex(m_nFrameIndex);
        m_nFrameIndex = 0;
        ++m_nLoopIndex;
        return TRUE;
    }

    return FALSE;
}
