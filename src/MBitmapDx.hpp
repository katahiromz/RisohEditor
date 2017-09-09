// MBitmapDx.hpp --- GDI+ Bitmap wrapper                        -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MBITMAPDX_HPP_
#define MZC4_MBITMAPDX_HPP_

#include <initguid.h>

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif

#include <gdiplus.h>
#pragma comment (lib, "gdiplus.lib")

#include <vector>

INT GetEncoderClsid(const WCHAR *format, CLSID *pClsid);
class MBitmapDx;

//////////////////////////////////////////////////////////////////////////////

class MBitmapDx
{
public:
    MBitmapDx();
    ~MBitmapDx();

    Gdiplus::Bitmap *GetBitmap() const;
    BOOL SetBitmap(Gdiplus::Bitmap *pBitmap);
    BOOL CreateFromMemory(const void *pvData, DWORD dwSize);

    COLORREF GetBackColor() const;
    void SetBackColor(COLORREF rgbBack);

    UINT GetFrameIndex() const;
    void SetFrameIndex(UINT nIndex);

    UINT GetFrameCount() const;
    UINT GetLoopIndex() const;
    UINT GetLoopCount() const;
    DWORD GetFrameDelay(UINT nFrameIndex) const;

    HBITMAP GetHBITMAP(LONG& cx, LONG& cy);

    void Destroy();
    void FreeBitmap();
    void FreeDelayPropertyItem();
    BOOL Step(DWORD& dwDelay);

protected:
    Gdiplus::Bitmap        *m_pBitmap;
    COLORREF                m_rgbBack;
    UINT                    m_nFrameIndex;
    UINT                    m_nFrameCount;
    UINT                    m_nLoopIndex;
    UINT                    m_nLoopCount;
    Gdiplus::PropertyItem  *m_pDelayItem;
    HGLOBAL                 m_hGlobal;

    BOOL CreateInternal();

private:
    // NOTE: MBitmapDx should not be copyed.
    MBitmapDx(const MBitmapDx&);
    MBitmapDx& operator=(const MBitmapDx&);
};

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

//////////////////////////////////////////////////////////////////////////////

inline MBitmapDx::MBitmapDx()
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

inline MBitmapDx::~MBitmapDx()
{
    Destroy();
}

inline void MBitmapDx::FreeBitmap()
{
    if (m_pBitmap)
    {
        delete m_pBitmap;
        m_pBitmap = NULL;
    }
}

inline void MBitmapDx::FreeDelayPropertyItem()
{
    if (m_pDelayItem)
    {
        std::free(m_pDelayItem);
        m_pDelayItem = NULL;
    }
}

inline void MBitmapDx::Destroy()
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

inline Gdiplus::Bitmap *MBitmapDx::GetBitmap() const
{
    return m_pBitmap;
}

inline COLORREF MBitmapDx::GetBackColor() const
{
    return m_rgbBack;
}

inline void MBitmapDx::SetBackColor(COLORREF rgbBack)
{
    m_rgbBack = rgbBack;
}

inline BOOL MBitmapDx::CreateInternal()
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

inline BOOL MBitmapDx::SetBitmap(Gdiplus::Bitmap *pBitmap)
{
    Destroy();

    m_pBitmap = pBitmap;
    return m_pBitmap && CreateInternal();
}

inline BOOL MBitmapDx::CreateFromMemory(const void *pvData, DWORD dwSize)
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

inline DWORD MBitmapDx::GetFrameDelay(UINT nFrameIndex) const
{
    if (nFrameIndex < m_nFrameCount && m_pDelayItem)
    {
        return ((DWORD *)m_pDelayItem->value)[m_nFrameIndex] * 10;
    }
    return 0;
}

inline void MBitmapDx::SetFrameIndex(UINT nFrameIndex)
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

inline HBITMAP MBitmapDx::GetHBITMAP(LONG& cx, LONG& cy)
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

inline UINT MBitmapDx::GetLoopIndex() const
{
    return m_nLoopIndex;
}

inline UINT MBitmapDx::GetLoopCount() const
{
    return m_nLoopCount;
}

inline UINT MBitmapDx::GetFrameIndex() const
{
    return m_nFrameIndex;
}

inline UINT MBitmapDx::GetFrameCount() const
{
    return m_nFrameCount;
}

inline BOOL MBitmapDx::Step(DWORD& dwDelay)
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

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MBITMAPDX_HPP_
