// MBitmapDx.cpp --- GDI+ Bitmap wrapper                        -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
//////////////////////////////////////////////////////////////////////////////

#include "MBitmapDx.hpp"

MBitmapDx::MBitmapDx()
{
	m_pBitmap = nullptr;
	m_rgbBack = RGB(255, 255, 255);
	m_nFrameIndex = 0;
	m_nFrameCount = 0;
	m_pDelayItem = nullptr;
	m_nLoopIndex = 0;
	m_nLoopCount = (UINT)-1;
	m_hGlobal = nullptr;
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
		m_pBitmap = nullptr;
	}
}

void MBitmapDx::FreeDelayPropertyItem()
{
	if (m_pDelayItem)
	{
		std::free(m_pDelayItem);
		m_pDelayItem = nullptr;
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
		m_hGlobal = nullptr;
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
		m_pDelayItem = nullptr;
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
	if (!m_pBitmap)
		return FALSE;

	if (!CreateInternal())
	{
		delete m_pBitmap;
		m_pBitmap = nullptr;
		return FALSE;
	}
	return TRUE;
}

BOOL MBitmapDx::CreateFromMemory(const void *pvData, DWORD dwSize)
{
	Destroy();

	Gdiplus::Bitmap *pBitmap = nullptr;
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwSize);
	if (hGlobal)
	{
		LPVOID pv = GlobalLock(hGlobal);
		if (pv)
		{
			CopyMemory(pv, pvData, dwSize);
			GlobalUnlock(hGlobal);

			IStream *pStream = nullptr;
			if (CreateStreamOnHGlobal(hGlobal, FALSE, &pStream) == S_OK)
			{
				try
				{
					pBitmap = Gdiplus::Bitmap::FromStream(pStream);

					INT cx = pBitmap->GetWidth();
					INT cy = pBitmap->GetHeight();
					if (!cx || !cy)
					{
						delete pBitmap;
						pBitmap = nullptr;
					}
				}
				catch (...)
				{
					pBitmap = nullptr;
				}
				pStream->Release();
			}
		}
	}

	if (pBitmap)
	{
		m_hGlobal = hGlobal;
		m_pBitmap = pBitmap;
		if (!CreateInternal())
		{
			Destroy();
			return FALSE;
		}
		return TRUE;
	}
	else
	{
		if (hGlobal)
			GlobalFree(hGlobal);
		return FALSE;
	}
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
	HBITMAP hbm = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &pvBits,
								   nullptr, 0);
	if (hbm == nullptr)
		return nullptr;

	HDC hDC = CreateCompatibleDC(nullptr);
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

	if (!m_pBitmap)
		return nullptr;

	if (!(m_pBitmap->GetPixelFormat() & PixelFormatAlpha))
		return GetHBITMAP(cx, cy);

	cx = static_cast<LONG>(m_pBitmap->GetWidth());
	cy = static_cast<LONG>(m_pBitmap->GetHeight());
	if (cx <= 0 || cy <= 0)
		return nullptr;

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = cx;
	bmi.bmiHeader.biHeight = -cy; // top-down DIB
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;

	LPVOID pvBits = nullptr;
	HDC hdc = CreateCompatibleDC(nullptr);
	HBITMAP hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
	DeleteDC(hdc);
	if (!hbm || !pvBits)
		return nullptr;

	BitmapData bitmapData = {};
	Rect rect(0, 0, cx, cy);
	Status status = m_pBitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);
	if (status != Ok)
	{
		DeleteObject(hbm);
		return nullptr;
	}

	const BYTE* pSrc = static_cast<const BYTE*>(bitmapData.Scan0);
	BYTE* pDst = static_cast<BYTE*>(pvBits);
	const INT srcStride = bitmapData.Stride;
	const INT dstStride = cx * 4;

	for (INT y = 0; y < cy; ++y)
	{
		memcpy(pDst, pSrc, dstStride);
		pSrc += srcStride;
		pDst += dstStride;
	}

	m_pBitmap->UnlockBits(&bitmapData);
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
