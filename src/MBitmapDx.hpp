// MBitmapDx.hpp --- GDI+ Bitmap wrapper                        -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <initguid.h>
#ifndef _INC_WINDOWS
	#include <windows.h>
#endif
#include <vector>
#include <gdiplus.h>

#include "PackedDIB.hpp"

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
	HBITMAP GetHBITMAP32(LONG& cx, LONG& cy);

	void Destroy();
	void FreeBitmap();
	void FreeDelayPropertyItem();
	BOOL Step(DWORD& dwDelay);

	UINT GetWidth();
	UINT GetHeight();

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
// Trivial inline accessors

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

inline DWORD MBitmapDx::GetFrameDelay(UINT nFrameIndex) const
{
	if (nFrameIndex < m_nFrameCount && m_pDelayItem)
	{
		return ((DWORD *)m_pDelayItem->value)[m_nFrameIndex] * 10;
	}
	return 0;
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

inline UINT MBitmapDx::GetWidth()
{
	if (m_pBitmap)
	{
		return m_pBitmap->GetWidth();
	}
	return 0;
}

inline UINT MBitmapDx::GetHeight()
{
	if (m_pBitmap)
	{
		return m_pBitmap->GetHeight();
	}
	return 0;
}
