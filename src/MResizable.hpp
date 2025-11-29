// MResizable.hpp -- Win32 dynamic layout resizable window/dialog -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MRESIZABLE_HPP_
#define MZC4_MRESIZABLE_HPP_        4   /* Version 4 */

struct MCtrlLayout;
class MResizable;

////////////////////////////////////////////////////////////////////////////

#include "MPointSizeRect.hpp"

////////////////////////////////////////////////////////////////////////////
// layout anchors for MResizable::SetLayoutAnchor

#define mzcLA_NO_ANCHOR        MSize(-1, -1)    // don't resize
#define mzcLA_TOP_LEFT         MSize(0, 0)      // upper left
#define mzcLA_TOP_CENTER       MSize(50, 0)     // upper center
#define mzcLA_TOP_RIGHT        MSize(100, 0)    // upper right
#define mzcLA_MIDDLE_LEFT      MSize(0, 50)     // middle left
#define mzcLA_MIDDLE_CENTER    MSize(50, 50)    // middle center
#define mzcLA_MIDDLE_RIGHT     MSize(100, 50)   // middle right
#define mzcLA_BOTTOM_LEFT      MSize(0, 100)    // lower left
#define mzcLA_BOTTOM_CENTER    MSize(50, 100)   // lower center
#define mzcLA_BOTTOM_RIGHT     MSize(100, 100)  // lower right

////////////////////////////////////////////////////////////////////////////
// MResizable

#include "MScrollBar.hpp"   // for MSizeGrip
#include <vector>           // for std::vector

class MResizable
{
protected:
	struct MCtrlLayout
	{
		HWND m_hwndCtrl;
		MSize m_sizLA_1, m_sizMargin1, m_sizLA_2, m_sizMargin2;

		MCtrlLayout() = default;

		MCtrlLayout(HWND hwndCtrl, SIZE sizLA_1, SIZE sizMargin1,
			SIZE sizLA_2, SIZE sizMargin2) : m_hwndCtrl(hwndCtrl),
			m_sizLA_1(sizLA_1), m_sizMargin1(sizMargin1),
			m_sizLA_2(sizLA_2), m_sizMargin2(sizMargin2)
		{
		}
	};
public:
	typedef MCtrlLayout                 layout_type;
	typedef std::vector<MCtrlLayout>    layouts_type;

	MResizable();
	virtual ~MResizable();

	// NOTE: Please call OnParentCreate after parent's WM_CREATE or WM_INITDIALOG.
	VOID OnParentCreate(HWND hwnd, BOOL bEnableResize = TRUE,
						BOOL bShowSizeGrip = TRUE);

	// NOTE: Please call OnSize on parent's WM_SIZE.
	VOID OnSize(const RECT *prcClient = NULL);

	// NOTE: Please call SetLayoutAnchor after OnParentCreate.
	// NOTE: sizLA_1 is upper left anchor, sizLA_2 is lower right anchor.
	VOID SetLayoutAnchor(HWND hwndCtrl,
						 MSize sizLA_1, MSize sizLA_2 = mzcLA_NO_ANCHOR);
	VOID SetLayoutAnchor(UINT nCtrlID,
						 MSize sizLA_1, MSize sizLA_2 = mzcLA_NO_ANCHOR);

		  layout_type *CtrlLayout(HWND hwndCtrl);
		  layout_type *CtrlLayout(UINT nCtrlID);
	const layout_type *CtrlLayout(HWND hwndCtrl) const;
	const layout_type *CtrlLayout(UINT nCtrlID) const;

	BOOL IsResizeEnabled() const;
	VOID EnableResize(BOOL bEnableResize, BOOL bShowSizeGrip = TRUE);
	VOID ClearLayouts();

protected:
	HWND            m_hwndParent;
	BOOL            m_bResizeEnabled;
	MSizeGrip       m_size_grip;
	layouts_type    m_layouts;

	VOID ShowSizeGrip(BOOL bShow = TRUE);
	VOID ModifyParentStyle(BOOL bEnableResize);
	VOID MoveSizeGrip();
	VOID ArrangeLayout(const RECT *prc);

private:
	// NOTE: MResizable is not copyable
	MResizable(const MResizable& rsz);
	MResizable& operator=(const MResizable& rsz);
};

////////////////////////////////////////////////////////////////////////////

inline MResizable::MResizable()
	: m_hwndParent(NULL), m_bResizeEnabled(FALSE)
{
}

inline /*virtual*/ MResizable::~MResizable()
{
}

inline VOID MResizable::ClearLayouts()
{
	m_layouts.clear();
}

inline VOID MResizable::SetLayoutAnchor(
	UINT nCtrlID, MSize sizLA_1, MSize sizLA_2/* = mzcLA_NO_ANCHOR*/)
{
	assert(m_hwndParent);
	assert(::IsWindow(m_hwndParent));
	assert(sizLA_1 != mzcLA_NO_ANCHOR);
	MResizable::SetLayoutAnchor(
		GetDlgItem(m_hwndParent, nCtrlID), sizLA_1, sizLA_2);
}

inline VOID MResizable::OnSize(const RECT *prcClient/* = NULL*/)
{
	assert(m_hwndParent);
	assert(::IsWindow(m_hwndParent));

	ArrangeLayout(prcClient);
	MoveSizeGrip();
}

inline BOOL MResizable::IsResizeEnabled() const
{
	return m_bResizeEnabled;
}

inline VOID MResizable::ShowSizeGrip(BOOL bShow/* = TRUE*/)
{
	assert(m_hwndParent);
	assert(::IsWindow(m_hwndParent));

	if (bShow)
	{
		// size grip aleady exists?
		if (!m_size_grip)
		{
			// create size grip
			MRect ClientRect;
			GetClientRect(m_hwndParent, &ClientRect);
			INT cx = ::GetSystemMetrics(SM_CXVSCROLL);
			INT cy = ::GetSystemMetrics(SM_CYHSCROLL);
			m_size_grip.CreateWindowDx(m_hwndParent, NULL,
				WS_CHILD | WS_CLIPSIBLINGS | SBS_SIZEGRIP, 0,
				ClientRect.right - cx, ClientRect.bottom - cy,
				cx, cy, reinterpret_cast<HMENU>(123456789));
		}

		MoveSizeGrip();
		ShowWindow(m_size_grip, SW_SHOWNOACTIVATE);
	}
	else
	{
		if (::IsWindow(m_size_grip))
			ShowWindow(m_size_grip, SW_HIDE);
	}
}

inline VOID MResizable::MoveSizeGrip()
{
	assert(m_hwndParent);
	assert(::IsWindow(m_hwndParent));

	if (m_size_grip)
	{
		// move it
		MRect ClientRect;
		GetClientRect(m_hwndParent, &ClientRect);
		INT cx = ::GetSystemMetrics(SM_CXVSCROLL);
		INT cy = ::GetSystemMetrics(SM_CYHSCROLL);
		SetWindowPos(m_size_grip, NULL,
			ClientRect.right - cx, ClientRect.bottom - cy,
			cx, cy, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	}
}

inline VOID MResizable::ModifyParentStyle(BOOL bEnableResize)
{
	// style
	DWORD style = ::GetWindowLong(m_hwndParent, GWL_STYLE);
	if (bEnableResize)
	{
		style &= ~DS_MODALFRAME;
		style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
		SetWindowLong(m_hwndParent, GWL_STYLE, style);
	}
	else
	{
		style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		style |= DS_MODALFRAME;
		SetWindowLong(m_hwndParent, GWL_STYLE, style);
	}
	// ex.style
	style = ::GetWindowLong(m_hwndParent, GWL_EXSTYLE);
	if (bEnableResize)
	{
		style &= ~WS_EX_DLGMODALFRAME;
		style |= 0;
		SetWindowLong(m_hwndParent, GWL_EXSTYLE, style);
	}
	else
	{
		style &= ~0;
		style |= WS_EX_DLGMODALFRAME;
		SetWindowLong(m_hwndParent, GWL_EXSTYLE, style);
	}
	if (bEnableResize)
	{
		GetSystemMenu(m_hwndParent, TRUE);
	}
	else
	{
		HMENU hSysMenu;
		hSysMenu = ::GetSystemMenu(m_hwndParent, FALSE);
		RemoveMenu(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND);
		RemoveMenu(hSysMenu, SC_SIZE, MF_BYCOMMAND);
		RemoveMenu(hSysMenu, SC_RESTORE, MF_BYCOMMAND);
	}
	RedrawWindow(m_hwndParent, NULL, NULL,
				   RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW);
	InvalidateRect(m_hwndParent, NULL, TRUE);
}

inline VOID
MResizable::EnableResize(BOOL bEnableResize, BOOL bShowSizeGrip/* = TRUE*/)
{
	ShowSizeGrip(bEnableResize && bShowSizeGrip);
	ModifyParentStyle(bEnableResize);
	m_bResizeEnabled = bEnableResize;
}

inline VOID MResizable::OnParentCreate(HWND hwndParent, BOOL bEnableResize,
									   BOOL bShowSizeGrip/* = TRUE*/)
{
	assert(hwndParent);
	assert(::IsWindow(hwndParent));

	m_hwndParent = hwndParent;

	ClearLayouts();

	// NOTE: The parent window must have initially WS_THICKFRAME style.
	assert(::GetWindowLong(hwndParent, GWL_STYLE) & WS_THICKFRAME);

	EnableResize(bEnableResize, bShowSizeGrip);
}

inline VOID MResizable::ArrangeLayout(const RECT *prc)
{
	assert(m_hwndParent);
	assert(::IsWindow(m_hwndParent));

	MRect ClientRect;
	if (prc)
	{
		ClientRect = *prc;
	}
	else
	{
		GetClientRect(m_hwndParent, &ClientRect);
	}

	const UINT count = UINT(m_layouts.size());
	if (count == 0)
		return;

	HDWP hDwp = ::BeginDeferWindowPos(count);
	if (hDwp == NULL)
		return;

	const UINT uFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREPOSITION;
	for (UINT i = 0; i < count; ++i)
	{
		const layout_type& layout = m_layouts[i];
		HWND hwndCtrl = layout.m_hwndCtrl;
		if (!::IsWindow(hwndCtrl))
			continue;

		MRect ChildRect, NewRect;

		GetWindowRect(hwndCtrl, &ChildRect);
		MapWindowPoints(NULL, m_hwndParent,
						  reinterpret_cast<LPPOINT>(&ChildRect), 2);

		NewRect.left = layout.m_sizMargin1.cx +
					   ClientRect.Width() * layout.m_sizLA_1.cx / 100;
		NewRect.top = layout.m_sizMargin1.cy +
					  ClientRect.Height() * layout.m_sizLA_1.cy / 100;
		NewRect.right = layout.m_sizMargin2.cx +
						ClientRect.Width() * layout.m_sizLA_2.cx / 100;
		NewRect.bottom = layout.m_sizMargin2.cy +
						 ClientRect.Height() * layout.m_sizLA_2.cy / 100;

		if (NewRect != ChildRect)
		{
			hDwp = ::DeferWindowPos(hDwp, hwndCtrl, NULL,
				NewRect.left, NewRect.top,
				NewRect.Width(), NewRect.Height(), uFlags);
		}

		InvalidateRect(hwndCtrl, NULL, TRUE);
	}

	EndDeferWindowPos(hDwp);
}

inline VOID MResizable::SetLayoutAnchor(
	HWND hwndCtrl, MSize sizLA_1, MSize sizLA_2/* = mzcLA_NO_ANCHOR*/)
{
	assert(m_hwndParent);
	assert(::IsWindow(m_hwndParent));
	assert(sizLA_1 != mzcLA_NO_ANCHOR);

	MRect ClientRect, ChildRect;
	GetClientRect(m_hwndParent, &ClientRect);
	GetWindowRect(hwndCtrl, &ChildRect);
	MapWindowPoints(NULL, m_hwndParent,
					reinterpret_cast<LPPOINT>(&ChildRect), 2);

	if (sizLA_2 == mzcLA_NO_ANCHOR)
		sizLA_2 = sizLA_1;

	MSize sizMargin1, sizMargin2;
	sizMargin1.cx = ChildRect.left - ClientRect.Width() * sizLA_1.cx / 100;
	sizMargin1.cy = ChildRect.top - ClientRect.Height() * sizLA_1.cy / 100;
	sizMargin2.cx = ChildRect.right - ClientRect.Width() * sizLA_2.cx / 100;
	sizMargin2.cy = ChildRect.bottom - ClientRect.Height() * sizLA_2.cy / 100;

	// search hwndCtrl
	layout_type *pLayout = CtrlLayout(hwndCtrl);
	if (pLayout)
		return;

	// if not found
	layout_type layout(hwndCtrl, sizLA_1, sizMargin1, sizLA_2, sizMargin2);
	m_layouts.push_back(layout);
}

inline MResizable::layout_type *MResizable::CtrlLayout(HWND hwndCtrl)
{
	const UINT count = UINT(m_layouts.size());
	for (UINT i = 0; i < count; ++i)
	{
		if (m_layouts[i].m_hwndCtrl == hwndCtrl)
			return &m_layouts[i];
	}
	return NULL;
}

inline MResizable::layout_type *MResizable::CtrlLayout(UINT nCtrlID)
{
	HWND hwndCtrl = ::GetDlgItem(m_hwndParent, nCtrlID);
	const UINT count = UINT(m_layouts.size());
	for (UINT i = 0; i < count; ++i)
	{
		if (m_layouts[i].m_hwndCtrl == hwndCtrl)
			return &m_layouts[i];
	}
	return NULL;
}

inline const MResizable::layout_type *MResizable::CtrlLayout(HWND hwndCtrl) const
{
	const UINT count = UINT(m_layouts.size());
	for (UINT i = 0; i < count; ++i)
	{
		if (m_layouts[i].m_hwndCtrl == hwndCtrl)
			return &m_layouts[i];
	}
	return NULL;
}

inline const MResizable::layout_type *MResizable::CtrlLayout(UINT nCtrlID) const
{
	HWND hwndCtrl = ::GetDlgItem(m_hwndParent, nCtrlID);
	const UINT count = UINT(m_layouts.size());
	for (UINT i = 0; i < count; ++i)
	{
		if (m_layouts[i].m_hwndCtrl == hwndCtrl)
			return &m_layouts[i];
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MRESIZABLE_HPP_
