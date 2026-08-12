// MRadWindow.cpp --- RADical development window
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "MRadWindow.hpp"
#include "MAddCtrlDlg.hpp"
#include "MDlgPropDlg.hpp"
#include "MCtrlPropDlg.hpp"
#include "ConstantsDB.hpp"
#include "PackedDIB.hpp"
#include "IconRes.hpp"
#include <memory>
#include <climits>

//////////////////////////////////////////////////////////////////////////////
// MRadCtrl --- the RADical controls
// NOTE: An MRadCtrl is a dialog control, subclassed by MRadDialog.

// constructor
MRadCtrl::MRadCtrl()
	: m_dwMagic(0xDEADFACE)
	, m_bTopCtrl(FALSE)
	, m_bMoving(FALSE)
	, m_bSizing(FALSE)
	, m_bLocking(FALSE)
	, m_nIndex(-1)
{
	m_pt.x = m_pt.y = -1;
	m_nImageType = 0;
}

// the default icon
HICON& MRadCtrl::Icon()
{
	static HICON s_hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICO));
	return s_hIcon;
}

// the default bitmap
HBITMAP& MRadCtrl::Bitmap()
{
	static HBITMAP s_hbm = LoadBitmap(GetModuleHandle(NULL),
									  MAKEINTRESOURCE(IDB_BMP));
	return s_hbm;
}

// is the window a group box?
BOOL MRadCtrl::IsGroupBox(HWND hCtrl)
{
	WCHAR szClass[8];
	GetClassNameW(hCtrl, szClass, _countof(szClass));
	if (lstrcmpiW(szClass, L"BUTTON") == 0)
	{
		return (GetWindowStyle(hCtrl) & BS_TYPEMASK) == BS_GROUPBOX;
	}
	return FALSE;
}

// call me after subclassing
void MRadCtrl::PostSubclass()
{
	SIZE siz = { 0, 0 };
	TCHAR szClass[MAX_PATH];
	GetWindowPosDx(m_hwnd, NULL, &siz);
	GetClassName(m_hwnd, szClass, _countof(szClass));
	if (lstrcmpi(szClass, TEXT("STATIC")) == 0)
	{
		// static control
		DWORD style = GetWindowStyle(m_hwnd);
		if ((style & SS_TYPEMASK) == SS_ICON)
		{
			m_nImageType = 1;   // icon
			HICON hIcon = Icon();
			SendMessage(m_hwnd, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
			SetWindowPosDx(m_hwnd, NULL, &siz);
		}
		else if ((style & SS_TYPEMASK) == SS_BITMAP)
		{
			m_nImageType = 2;   // bitmap
			HBITMAP hbm = Bitmap();
			SendMessage(m_hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
			SetWindowPosDx(m_hwnd, NULL, &siz);
		}
		return;
	}
}

// the targets (the selected window handles)
MRadCtrl::set_type& MRadCtrl::GetTargets()
{
	static set_type s_set;
	return s_set;
}

// the last selection (the selected window handle)
HWND& MRadCtrl::GetLastSel()
{
	static HWND s_hwnd = NULL;
	return s_hwnd;
}

// get the target control indexes
std::unordered_set<INT> MRadCtrl::GetTargetIndeces()
{
	set_type targets = MRadCtrl::GetTargets();

	std::unordered_set<INT> indeces;
	for (auto target : targets)
	{
		auto pCtrl = MRadCtrl::GetRadCtrl(target);
		if (pCtrl)
			indeces.insert(pCtrl->m_nIndex);
	}
	return indeces;
}

// the index-to-control mapping
MRadCtrl::map_type& MRadCtrl::IndexToCtrlMap()
{
	static map_type s_map;
	return s_map;
}

// get the rubber band that is associated to the MRadCtrl
MRubberBand *MRadCtrl::GetRubberBand()
{
	return m_hwndRubberBand.m_hwnd ? &m_hwndRubberBand : NULL;
}

// get the MRadCtrl from a window handle
MRadCtrl *MRadCtrl::GetRadCtrl(HWND hwnd)
{
	MWindowBase *base = GetUserData(hwnd);
	if (base)
	{
		auto pCtrl = static_cast<MRadCtrl *>(base);
		if (pCtrl->m_dwMagic == 0xDEADFACE)
			return pCtrl;
	}
	return NULL;
}

// is the user dragging on the dialog face
BOOL& MRadCtrl::GetRangeSelect(void)
{
	static BOOL s_bRangeSelect = FALSE;
	return s_bRangeSelect;
}

// Keep every group box pinned at the very back of the Z order. Group
// boxes in this app paint a solid opaque fill over their whole client
// rect (see OnEraseBkgnd below), so if an ordinary control ever ends up
// Z-order-behind one, that control gets completely hidden -- calling
// this after any Z-order change that sends something to HWND_BOTTOM
// guarantees a group box can never be "passed" by a regular control.
//
// If a group box happens to be selected (has its own rubber band) when
// this runs, its band is kept directly above it -- just enough for the
// selection outline to still be visible over the group box's own
// opaque fill -- but still below every ordinary control. Selecting a
// group box must never visually cover anything else, the same as the
// group box's own body never does.
void MRadCtrl::PinGroupBoxesToBack(HWND hwndParent)
{
	if (!hwndParent || !IsWindow(hwndParent))
		return;

	// collect the group boxes in front-to-back order first, since
	// reordering while walking the sibling chain would disturb the
	// walk itself
	std::vector<HWND> groupBoxes;
	for (HWND hCtrl = GetTopWindow(hwndParent); hCtrl; hCtrl = GetWindow(hCtrl, GW_HWNDNEXT))
	{
		auto pCtrl = GetRadCtrl(hCtrl);
		if (pCtrl && pCtrl->m_bTopCtrl && IsGroupBox(hCtrl))
			groupBoxes.push_back(hCtrl);
	}

	// send them to the very bottom, processing back-to-front so their
	// relative order among themselves is preserved (the one that was
	// frontmost among group boxes ends up frontmost among group boxes
	// again, just now below every non-group-box control)
	for (auto it = groupBoxes.rbegin(); it != groupBoxes.rend(); ++it)
	{
		HWND hGroupBox = *it;

		// if this group box is currently selected, sink its own band
		// first, so that after the group box itself is sunk right
		// after, the band ends up directly ABOVE the group box (a
		// SetWindowPos(..., HWND_BOTTOM) always becomes the new
		// absolute bottom, pushing whatever was already there up by
		// exactly one slot)
		auto pCtrl = GetRadCtrl(hGroupBox);
		if (pCtrl)
		{
			if (auto pBand = pCtrl->GetRubberBand())
			{
				::SetWindowPos(*pBand, HWND_BOTTOM, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			}
		}

		::SetWindowPos(hGroupBox, HWND_BOTTOM, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	// group-box sinking must not leave the index labels buried under
	// whatever was pushed up in the process. Post rather than paint
	// synchronously: coalesces with any pending MYWM_REDRAW and avoids
	// a full erase/repaint on every Z-order tweak.
	::PostMessage(hwndParent, MYWM_REDRAW, 0, 0);
}

// Undo the Z-order elevation that Select()/OnNCLButtonDown() gave a
// control while it was selected. Without this, a control that was once
// selected (then deselected, e.g. by Ctrl-click, or by clicking
// elsewhere) stays parked at the very top of the Z order forever, and
// can end up covering another control's rubber band later even though
// it isn't selected itself anymore -- that's the "sometimes the rubber
// band gets partially hidden" bug.
void MRadCtrl::UnelevateZOrder(HWND hwnd)
{
	if (hwnd && IsWindow(hwnd) && !MRadCtrl::IsGroupBox(hwnd))
	{
		::SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		// we just sent a regular control to the very bottom, which
		// would otherwise put it *behind* any group box -- re-sink the
		// group box(es) below it again right away
		PinGroupBoxesToBack(GetParent(hwnd));
	}
}

// deselect the selection
BOOL MRadCtrl::DeselectSelection()
{
	BOOL bFound = FALSE;    // not found yet

	for (auto target : GetTargets())
	{
		MRadCtrl *pCtrl = GetRadCtrl(target);
		if (pCtrl)
		{
			bFound = TRUE;  // found

			// destroy the rubber band of the control
			DestroyWindow(pCtrl->m_hwndRubberBand);
			pCtrl->m_hwndRubberBand.m_hwnd = NULL;

			// give up the Z-order elevation now that it's deselected
			UnelevateZOrder(target);
		}
	}

	// clear the target set
	GetTargets().clear();

	// clear the last selection
	GetLastSel() = NULL;

	return bFound;  // found or not
}

LRESULT MRadCtrl::DoSendMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ::SendMessage(hwnd, uMsg, wParam, lParam);
}

// delete the selection
void MRadCtrl::DeleteSelection()
{
	if (GetTargets().empty())
		return;

	// send MYWM_DELETESEL message to the parent of the target
	auto it = GetTargets().begin();
	DoSendMessage(GetParent(*it), MYWM_DELETESEL, 0, 0);
}

// deselect this control
void MRadCtrl::Deselect()
{
	// delete the rubber band of this control
	MRubberBand *band = GetRubberBand();
	if (band)
		DestroyWindow(*band);
	m_hwndRubberBand.m_hwnd = NULL;

	// remove the control from targets
	GetTargets().erase(m_hwnd);

	// clear the last selection
	GetLastSel() = NULL;

	// give up the Z-order elevation now that it's deselected
	UnelevateZOrder(m_hwnd);
}

// is it selected?
BOOL MRadCtrl::IsSelected() const
{
	return IsWindow(m_hwndRubberBand);
}

// select the control
void MRadCtrl::Select(HWND hwnd)
{
	// get the RADical control instance
	auto pCtrl = GetRadCtrl(hwnd);
	if (pCtrl == NULL)
		return;

	// destroy any stale rubber band
	::DestroyWindow(pCtrl->m_hwndRubberBand);

	// Bring the selected control itself to the front first. A selected
	// control (and its rubber band) must never be hidden behind other,
	// unselected sibling controls -- that was the old HWND_BOTTOM
	// behavior, and it's exactly backwards from what the user sees:
	// "selected" should mean "on top", not "sent to the back".
	// A group box is left alone since it's meant to sit behind the
	// controls it visually contains.
	if (!MRadCtrl::IsGroupBox(hwnd))
	{
		// NOTE: SetWindowPosDx() treats a NULL hwndInsertAfter as "no
		// z-order change requested" and quietly adds SWP_NOZORDER --
		// but HWND_TOP is defined as ((HWND)0), i.e. it IS NULL, so
		// SetWindowPosDx(hwnd, NULL, NULL, HWND_TOP) would silently do
		// nothing. Call the raw API directly here to actually move it.
		::SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	// Create the rubber band for the control. Newly created child
	// windows are placed at the top of the Z order by default, so the
	// rubber band naturally ends up in front of the control it wraps
	// (which we just brought to the front above).
	pCtrl->m_hwndRubberBand.CreateDx(GetParent(hwnd), hwnd, TRUE);

	if (MRadCtrl::IsGroupBox(hwnd))
	{
		// The band we just created defaults to the very top of the Z
		// order, same as for any other control -- but a group box (and
		// so its own selection outline doesn't cover anything else
		// either) must stay at the very back. Sink both the group box
		// and its brand-new band back down together.
		// (PinGroupBoxesToBack also re-raises the index labels.)
		PinGroupBoxesToBack(GetParent(hwnd));
	}
	else
	{
		// rubber band creation puts a new sibling on top of everything;
		// re-overwrite the index labels so they are never covered.
		// Post so multiple Select() calls (range-select, SelectAll) share
		// a single paint rather than flashing once per control.
		::PostMessage(GetParent(hwnd), MYWM_REDRAW, 0, 0);
	}

	// add the handle to the targets
	GetTargets().insert(hwnd);

	// set the handle to the last selection
	GetLastSel() = hwnd;
}

void MRadCtrl::SelectByIndex(INT nIndex)
{
	auto it = IndexToCtrlMap().find(nIndex);
	if (it != IndexToCtrlMap().end())
	{
		HWND hwndCtrl = it->second;
		assert(MRadCtrl::GetRadCtrl(hwndCtrl));
		if (auto pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl))
		{
			assert(pCtrl->m_nIndex == nIndex);
		}
		Select(it->second);
	}
}

// move the selected RADical controls
void MRadCtrl::MoveSelection(HWND hwnd, INT dx, INT dy)
{
	auto& targets = GetTargets();
	if (targets.empty())
		return;

	for (auto target : targets)
	{
		if (hwnd == target)
			continue;
		if (auto pCtrl = GetRadCtrl(target))
			pCtrl->m_bMoving = TRUE;
	}

	HDWP hdwp = BeginDeferWindowPos(static_cast<int>(targets.size()));
	if (hdwp)
	{
		for (auto target : targets)
		{
			if (hwnd == target)
				continue;

			if (auto pCtrl = GetRadCtrl(target))
			{
				RECT rc;
				GetWindowRect(*pCtrl, &rc);
				MapWindowRect(NULL, ::GetParent(*pCtrl), &rc);
				OffsetRect(&rc, dx, dy);

				hdwp = DeferWindowPos(hdwp, *pCtrl, NULL,
									  rc.left, rc.top, 0, 0,
									  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
				if (!hdwp)
					break;
			}
		}

		if (hdwp)
			EndDeferWindowPos(hdwp);
	}

	for (auto target : targets)
	{
		if (hwnd == target)
			continue;
		if (auto pCtrl = GetRadCtrl(target))
			pCtrl->m_bMoving = FALSE;
	}

	PostMessage(GetParent(hwnd), MYWM_REDRAW, 0, 0);
}

// resize the selected RADical controls
void MRadCtrl::ResizeSelection(HWND hwnd, INT cx, INT cy)
{
	auto& targets = GetTargets();
	if (targets.empty())
		return;

	for (auto target : targets)
	{
		if (hwnd == target)
			continue;
		if (auto pCtrl = GetRadCtrl(target))
			pCtrl->m_bSizing = TRUE;
	}

	HDWP hdwp = BeginDeferWindowPos(static_cast<int>(targets.size()));
	if (hdwp)
	{
		for (auto target : targets)
		{
			if (hwnd == target)
				continue;

			if (auto pCtrl = GetRadCtrl(target))
			{
				hdwp = DeferWindowPos(hdwp, *pCtrl, NULL,
									  0, 0, cx, cy,
									  SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				if (!hdwp)
					break;
			}
		}

		if (hdwp)
			EndDeferWindowPos(hdwp);
	}

	for (auto target : targets)
	{
		if (hwnd == target)
			continue;

		if (auto pCtrl = GetRadCtrl(target))
		{
			if (auto band = pCtrl->GetRubberBand())
				band->FitToTarget();

			pCtrl->m_bSizing = FALSE;
		}
	}

	PostMessage(GetParent(hwnd), MYWM_REDRAW, 0, 0);
}

// callback function for DoRangeSelect
BOOL CALLBACK
MRadCtrl::RangeSelectProc(HWND hwnd, LPARAM lParam)
{
	auto prs = (RANGE_SELECT *)lParam;
	RECT *prc = &prs->rc;
	RECT rc;

	// is it a RADical control?
	if (auto pCtrl = GetRadCtrl(hwnd))
	{
		// is it a top control?
		if (!pCtrl->m_bTopCtrl)
			return TRUE;    // continue

		// get the window rectangle of the control
		GetWindowRect(*pCtrl, &rc);

		// the control in range?
		if (prc->left <= rc.left && prc->top <= rc.top &&
			rc.right <= prc->right && rc.bottom <= prc->bottom)
		{
			// is [Ctrl] key down?
			if (prs->bCtrlDown)
			{
				// is the control selected?
				if (pCtrl->IsSelected())
				{
					// deselect
					pCtrl->Deselect();
				}
				else
				{
					// select
					MRadCtrl::Select(*pCtrl);
				}
			}
			else
			{
				// is the control not selected?
				if (!pCtrl->IsSelected())
				{
					// select
					MRadCtrl::Select(*pCtrl);
				}
			}
		}
	}

	return TRUE;    // continue
}

// do range selection
void MRadCtrl::DoRangeSelect(HWND hwndParent, const RECT *prc, BOOL bCtrlDown)
{
	RANGE_SELECT rs;
	rs.rc = *prc;
	rs.bCtrlDown = bCtrlDown;
	::EnumChildWindows(hwndParent, RangeSelectProc, (LPARAM)&rs);
}

// the window procedure of MRadCtrl
LRESULT CALLBACK
MRadCtrl::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG(hwnd, WM_NCHITTEST, OnNCHitTest);
		HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
		HANDLE_MSG(hwnd, WM_NCLBUTTONDOWN, OnNCLButtonDown);
		HANDLE_MSG(hwnd, WM_NCLBUTTONDBLCLK, OnNCLButtonDown);
		HANDLE_MSG(hwnd, WM_NCMOUSEMOVE, OnNCMouseMove);
		HANDLE_MSG(hwnd, WM_NCLBUTTONUP, OnNCLButtonUp);
		HANDLE_MSG(hwnd, WM_MOVE, OnMove);
		HANDLE_MSG(hwnd, WM_SIZE, OnSize);
		HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, OnLButtonDown);
		HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
		HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
		HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
		HANDLE_MSG(hwnd, WM_NCRBUTTONDOWN, OnNCRButtonDown);
		HANDLE_MSG(hwnd, WM_NCRBUTTONDBLCLK, OnNCRButtonDown);
		HANDLE_MSG(hwnd, WM_NCRBUTTONUP, OnNCRButtonUp);
		HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
		HANDLE_MESSAGE(hwnd, MYWM_REDRAW, OnRedraw);
		case WM_MOVING: case WM_SIZING:
			return 0;
	}
	return DefaultProcDx(hwnd, uMsg, wParam, lParam);
}

// MRadCtrl MYWM_REDRAW
LRESULT MRadCtrl::OnRedraw(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	PostMessage(GetParent(hwnd), MYWM_REDRAW, 0, 0);
	return 0;
}

// MRadCtrl WM_ERASEBKGND
BOOL MRadCtrl::OnEraseBkgnd(HWND hwnd, HDC hdc)
{
	// get the window class name of MRadCtrl
	WCHAR szClass[MAX_PATH];
	GetClassNameW(hwnd, szClass, _countof(szClass));

	// special rendering for specific classes
	if (lstrcmpiW(szClass, TOOLBARCLASSNAMEW) == 0 ||
		lstrcmpiW(szClass, REBARCLASSNAMEW) == 0 ||
		lstrcmpiW(szClass, WC_PAGESCROLLERW) == 0 ||
		IsGroupBox(hwnd))
	{
		RECT rc;
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, (HBRUSH)(COLOR_3DFACE + 1));
		return TRUE;
	}

	// otherwise default processing
	return (BOOL)DefaultProcDx();
}

// MRadCtrl WM_NCRBUTTONDOWN/WM_NCRBUTTONDBLCLK
void MRadCtrl::OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
	if (fDoubleClick)
		return;

	// emulate clicking to select the control
	OnNCLButtonDown(hwnd, FALSE, x, y, codeHitTest);
	OnNCLButtonUp(hwnd, x, y, codeHitTest);

	// send WM_NCRBUTTONDOWN to the parent
	DoSendMessage(GetParent(hwnd), WM_NCRBUTTONDOWN, (WPARAM)hwnd, MAKELPARAM(x, y));
}

// MRadCtrl WM_NCRBUTTONUP
void MRadCtrl::OnNCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
{
	// eat
}

// MRadCtrl WM_LBUTTONDOWN/WM_LBUTTONDBLCLK
void MRadCtrl::OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	// eat
}

// MRadCtrl WM_LBUTTONUP
void MRadCtrl::OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
	// eat
}

// MRadCtrl WM_MOUSEMOVE
void MRadCtrl::OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
	// eat
}

// MRadCtrl WM_MOVE
void MRadCtrl::OnMove(HWND hwnd, int x, int y)
{
	if (!m_bTopCtrl)
	{
		// if not a top control, do default processing
		DefaultProcDx(hwnd, WM_MOVE, 0, MAKELPARAM(x, y));
		return;
	}

	// if not locked
	if (!m_bLocking)
	{
		if (!m_bMoving)
		{
			// move the selected controls by the difference of positions
			POINT pt;
			GetCursorPos(&pt);
			MoveSelection(hwnd, pt.x - m_pt.x, pt.y - m_pt.y);
			m_pt = pt;  // remember the position
		}

		// Re-assert the front-of-Z-order position on every move tick,
		// not just once at mouse-down. During a plain "press and drag
		// in one motion" (the most common gesture), a very large
		// number of WM_MOVE messages can fire back-to-back while the
		// button stays down, and depending on exactly how that
		// interacts with sibling controls' own repaints, the one-time
		// elevation from OnNCLButtonDown()/Select() isn't always
		// enough to keep the band fully on top the whole time it's
		// being dragged. Doing it every tick is cheap (SetWindowPos
		// with SWP_NOMOVE|SWP_NOSIZE is a no-op position-wise) and
		// makes the "stay on top" guarantee hold continuously rather
		// than only at the start/end of the drag.
		if (!IsGroupBox(hwnd))
		{
			::SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}

		// move the rubber band
		if (auto band = GetRubberBand())
		{
			band->FitToTarget();
			::SetWindowPos(*band, HWND_TOP, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}

		// Index labels are refreshed via the single MYWM_REDRAW that
		// OnCtrlMove posts after UpdateRes -- do not PaintIndexLabels
		// here; a full RDW_ERASE redraw on every WM_MOVE is the main
		// source of label flicker while dragging.

		// redraw
		RECT rc;
		GetClientRect(hwnd, &rc);
		InvalidateRect(hwnd, &rc, FALSE);

		// send MYWM_CTRLMOVE to the parent
		DoSendMessage(GetParent(hwnd), MYWM_CTRLMOVE, (WPARAM)hwnd, 0);
	}
}

// MRadCtrl WM_SIZE
void MRadCtrl::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	// default processing
	DefaultProcDx(hwnd, WM_SIZE, state, MAKELPARAM(cx, cy));

	if (!m_bTopCtrl)
		return;     // not a top control

	// is it not locked
	if (!m_bLocking)
	{
		// resize if necessary
		if (!m_bSizing)
			ResizeSelection(hwnd, cx, cy);

		// re-assert the front-of-Z-order position and keep the band
		// synced to the new size, for the same reason as in OnMove()
		if (!IsGroupBox(hwnd))
		{
			::SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
		if (auto band = GetRubberBand())
		{
			band->FitToTarget();
			::SetWindowPos(*band, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}

		// Index labels are refreshed via the single MYWM_REDRAW that
		// OnCtrlSize posts after UpdateRes -- avoid a second full
		// dialog erase/repaint from every WM_SIZE.

		// send MYWM_CTRLSIZE to the parent
		DoSendMessage(GetParent(hwnd), MYWM_CTRLSIZE, (WPARAM)hwnd, 0);

		// redraw
		InvalidateRect(hwnd, NULL, FALSE);
	}
}

// MRadCtrl WM_KEYDOWN/WM_KEYUP
void MRadCtrl::OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	if (fDown)
	{
		// send WM_KEYDOWN to the parent
		FORWARD_WM_KEYDOWN(GetParent(hwnd), vk, cRepeat, flags, DoSendMessage);
	}
}

// MRadCtrl WM_NCLBUTTONDOWN/WM_NCLBUTTONDBLCLK
void MRadCtrl::OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
	if (hwnd != m_hwnd)
		return;     // invalid

	// update the position
	GetCursorPos(&m_pt);

	if (fDoubleClick)
	{
		// send MYWM_RADDBLCLICK to the parent
		DoSendMessage(GetParent(hwnd), MYWM_RADDBLCLICK, 0, (LPARAM)hwnd);
		return;
	}

	// ignore if not on the caption
	if (codeHitTest != HTCAPTION)
		return;

	if (GetKeyState(VK_CONTROL) < 0)    // [Ctrl] key is down
	{
		if (m_hwndRubberBand)
		{
			// deselect this control
			Deselect();
			return;     // finish
		}
	}
	else if (GetKeyState(VK_SHIFT) < 0)     // [Shift] key is down
	{
		if (m_hwndRubberBand)
		{
			return;     // finish
		}
	}
	else
	{
		if (!m_hwndRubberBand)
		{
			// deselect the selected controls
			DeselectSelection();
		}
	}

	// if no rubber band that is associated to this control
	if (m_hwndRubberBand == NULL)
	{
		// select this control
		Select(hwnd);
	}

	// Bring the control (and, in front of it, its rubber band) to the
	// front of the Z order right now, immediately on mouse-down --
	// BEFORE starting the drag below. DefWindowProc(..., HTCAPTION, ...)
	// below enters a modal loop that doesn't return until the mouse
	// button is released, so anything placed after it only takes
	// effect once the drag/click is already over. Doing it here
	// instead means a plain press-and-drag (no separate click first)
	// already has the control on top for the very first frame of the
	// drag, not just from the second WM_MOVE onward.
	if (!IsGroupBox(hwnd) && IsWindow(hwnd))
	{
		// Use the raw API, not SetWindowPosDx() -- see the NOTE in
		// Select() about HWND_TOP being indistinguishable from NULL to
		// that wrapper.
		::SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		HWND hwndBand = NULL;
		if (auto pBand = GetRubberBand())
		{
			hwndBand = *pBand;
			::SetWindowPos(hwndBand, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}

		// SetWindowPos() only *requests* a repaint; the actual WM_PAINT
		// is normally delivered later through the ordinary message
		// queue. But DefWindowProc(..., HTCAPTION, ...) right below
		// enters its own modal loop the instant we call it, and that
		// loop won't necessarily get around to painting before the
		// user starts moving the mouse. So force it to happen right
		// now, synchronously, while we're still in plain control here
		// -- otherwise the very first on-screen frame after the click
		// (before any movement) can still show the old stacking order.
		RECT rc;
		GetWindowRect(hwnd, &rc);
		if (hwndBand)
		{
			RECT rcBand;
			GetWindowRect(hwndBand, &rcBand);
			UnionRect(&rc, &rc, &rcBand);
		}
		InflateRect(&rc, 4, 4);
		HWND hwndParent = GetParent(hwnd);
		MapWindowRect(NULL, hwndParent, &rc);
		RedrawWindow(hwndParent, &rc, NULL,
			RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_ERASE);

		// re-overwrite the index labels so the first painted frame
		// already has them on top of the control/band we just raised
		MRadDialog::PaintIndexLabels(hwndParent);
	}

	// enable dragging by emulating the title bar dragging
	DefWindowProc(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(x, y));
}

// MRadCtrl WM_NCMOUSEMOVE
void MRadCtrl::OnNCMouseMove(HWND hwnd, int x, int y, UINT codeHitTest)
{
	// default processing
	DefWindowProc(hwnd, WM_NCMOUSEMOVE, codeHitTest, MAKELPARAM(x, y));
}

// MRadCtrl WM_NCLBUTTONUP
void MRadCtrl::OnNCLButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
{
	// finish moving
	m_bMoving = FALSE;

	// clear the position
	m_pt.x = -1;
	m_pt.y = -1;

	// default processing
	DefWindowProc(hwnd, WM_NCLBUTTONUP, codeHitTest, MAKELPARAM(x, y));
}

// the helper function for hittest: just collect every overlapping
// descendant that could matter, in Z-order (EnumChildWindows visits
// front-to-back, i.e. topmost/frontmost window first).
BOOL CALLBACK MRadCtrl::EnumHitTestChildProc(HWND hwnd, LPARAM lParam)
{
	// NOTE: EnumChildWindows scans not only children but descendants.

	auto pmht = (MYHITTEST *)lParam;

	// get the window rectangle
	RECT rc;
	GetWindowRect(hwnd, &rc);

	// the point in the rectangle?
	if (!PtInRect(&rc, pmht->pt))
		return TRUE;    // if not, ignore

	if (MRubberBand::GetRubberBand(hwnd) || MRadCtrl::GetRadCtrl(hwnd))
	{
		pmht->matches.push_back(hwnd);
	}

	return TRUE;    // continue
}

// resolve the collected matches into hCandidate/hLast/hGroupBox, giving
// priority to the frontmost (topmost in Z-order) one. Selected controls
// are kept at the front of the Z order (see Select() and
// OnNCLButtonDown()), so the frontmost match is the one the user
// actually sees on top and expects clicks to go to.
void MRadCtrl::ResolveHitTestMatches(MYHITTEST *pmht)
{
	// walk the matches back-to-front (rearmost first) so that the
	// frontmost one is applied last and "wins" -- this reuses the exact
	// same per-node logic that used to run directly inside
	// EnumChildWindows, just applied in the opposite order.
	for (auto it = pmht->matches.rbegin(); it != pmht->matches.rend(); ++it)
	{
		HWND hwnd = *it;

		// the control has a rubber band?
		if (auto pBand = MRubberBand::GetRubberBand(hwnd))
		{
			// the rubber band's target is the control?
			auto pCtrl = MRadCtrl::GetRadCtrl(pBand->m_hwndTarget);
			if (pCtrl && pCtrl->m_bTopCtrl)
			{
				// clear the candidate
				pmht->hCandidate = NULL;

				// if the window is a group box
				if (IsGroupBox(*pCtrl))
				{
					// set to hGroupBox
					pmht->hGroupBox = *pCtrl;
				}
				else
				{
					// otherwise, set to the last (i.e. frontmost so far) target
					pmht->hLast = pBand->m_hwndTarget;
				}
			}
		}
		else if (auto pCtrl = MRadCtrl::GetRadCtrl(hwnd))   // is it a RADical control?
		{
			if (pCtrl->m_bTopCtrl)  // a top control
			{
				// set to the last (i.e. frontmost so far) target
				pmht->hLast = hwnd;

				// is it not group box?
				if (!IsGroupBox(hwnd))
				{
					// it's a candidate
					pmht->hCandidate = hwnd;
				}
			}
		}
	}
}

// MRadCtrl WM_NCHITTEST
UINT MRadCtrl::OnNCHitTest(HWND hwnd, int x, int y)
{
	if (m_bTopCtrl)     // a top control?
	{
		// get the window rectangle
		RECT rc;
		GetWindowRect(hwnd, &rc);

		// if the position is out of rectangle, ignore it
		POINT pt = { x, y };
		if (!PtInRect(&rc, pt))
			return HTTRANSPARENT;

		// it has a rubber band?
		if (m_hwndRubberBand)
		{
			// get the window rectangle of the rubber band
			RECT rcBand;
			GetWindowRect(m_hwndRubberBand, &rcBand);

			// create the region object
			HRGN hRgn = CreateRectRgn(0, 0, 0, 0);
			GetWindowRgn(m_hwndRubberBand, hRgn);
			OffsetRgn(hRgn, rcBand.left, rcBand.top);

			if (PtInRegion(hRgn, x, y))     // if in the region
			{
				// delete the region object
				DeleteObject(hRgn);

				return HTTRANSPARENT;   // ignore
			}

			// delete the region object
			DeleteObject(hRgn);
		}

		// initialize a MYHITTEST
		MYHITTEST mht;
		mht.hParent = GetParent(hwnd);
		mht.hCandidate = NULL;
		mht.hLast = NULL;
		mht.hGroupBox = NULL;
		mht.pt = pt;

		// collect every overlapping descendant, then resolve them
		// giving priority to the frontmost one
		EnumChildWindows(mht.hParent, EnumHitTestChildProc, (LPARAM)&mht);
		ResolveHitTestMatches(&mht);

		//if (GetAsyncKeyState(VK_RBUTTON) < 0)
		//    DebugBreak();

		if (mht.hCandidate == hwnd && hwnd != mht.hGroupBox)
		{
			// there is a candidate and it is not group box
			return HTCAPTION;   // emulate caption hittest
		}

		if (!mht.hCandidate && mht.hLast == hwnd)
		{
			// there is no candidate, but last one is this control
			return HTCAPTION;   // emulate caption hittest
		}

		if (mht.hCandidate == hwnd)
		{
			// there is candidate
			return HTCAPTION;   // emulate caption hittest
		}
	}

	// ignore
	return HTTRANSPARENT;
}

void MRadCtrl::DoTest()
{
	WCHAR szText[256];
	MStringW str = GetWindowTextW();
	auto it = IndexToCtrlMap().find(m_nIndex);
	if (it != IndexToCtrlMap().end()) {
		StringCchPrintfW(szText, _countof(szText),
			L"MRadCtrl:%p, m_hwnd:%p, m_dwMagic:0x%08X, m_bTopCtrl:%d, m_nIndex:%d, "
			L"str:%s, it->second: %p",
			this, m_hwnd, m_dwMagic, m_bTopCtrl, m_nIndex,
			str.c_str(), it->second);
	}

	MessageBoxW(NULL, szText, L"MRadCtrl::DoTest()", MB_ICONINFORMATION);
}

//////////////////////////////////////////////////////////////////////////////
// MRadDialog --- RADical dialog

// contructor
MRadDialog::MRadDialog()
	: m_index_visible(FALSE)
	, m_hFontLabel(NULL)
	, m_bMovingSizing(FALSE)
{
	// get the dialog base units
	m_xDialogBaseUnit = LOWORD(GetDialogBaseUnits());
	m_yDialogBaseUnit = HIWORD(GetDialogBaseUnits());

	// reset the clicked position
	m_ptClicked.x = m_ptClicked.y = -1;

	// create the label font
	HFONT hFont = GetStockFont(DEFAULT_GUI_FONT);
	LOGFONT lf;
	GetObject(hFont, sizeof(lf), &lf);
	lf.lfHeight = 14;
	m_hFontLabel = ::CreateFontIndirect(&lf);

	// create the background brush
	m_hbrBack = NULL;
	ReCreateBackBrush();
}

MRadDialog::~MRadDialog()
{
	DeleteBackBrush();

	if (m_hFontLabel)
	{
		DeleteObject(m_hFontLabel);
		m_hFontLabel = NULL;
	}
}

void MRadDialog::DeleteBackBrush()
{
	if (m_hbrBack)
	{
		DeleteObject(m_hbrBack);
		m_hbrBack = nullptr;
	}
}

// EnumChildWindows' callback function to get the target
BOOL CALLBACK MRadDialog::GetTargetProc(HWND hwnd, LPARAM lParam)
{
	auto get_target = (GET_TARGET *)lParam;
	if (auto pCtrl = MRadCtrl::GetRadCtrl(hwnd))
	{
		if (pCtrl->m_dwMagic != 0xDEADFACE)
			return TRUE;    // invalid

		if (!pCtrl->m_bTopCtrl)
			return TRUE;    // not a top control?

		// get the target and the index
		switch (get_target->target)
		{
		case TARGET_PREV:
			if (get_target->m_nCurrentIndex > pCtrl->m_nIndex &&
				pCtrl->m_nIndex > get_target->m_nIndex)
			{
				get_target->m_nIndex = pCtrl->m_nIndex;
				get_target->hwndTarget = pCtrl->m_hwnd;
			}
			break;

		case TARGET_NEXT:
			if (get_target->m_nCurrentIndex < pCtrl->m_nIndex &&
				pCtrl->m_nIndex < get_target->m_nIndex)
			{
				get_target->m_nIndex = pCtrl->m_nIndex;
				get_target->hwndTarget = pCtrl->m_hwnd;
			}
			break;

		case TARGET_FIRST:
			if (pCtrl->m_nIndex < get_target->m_nIndex)
			{
				get_target->m_nIndex = pCtrl->m_nIndex;
				get_target->hwndTarget = pCtrl->m_hwnd;
			}
			break;

		case TARGET_LAST:
			if (pCtrl->m_nIndex > get_target->m_nIndex)
			{
				get_target->m_nIndex = pCtrl->m_nIndex;
				get_target->hwndTarget = pCtrl->m_hwnd;
			}
			break;
		}
	}

	return TRUE;    // continue
}

HWND MRadDialog::GetNextCtrl(HWND hwndCtrl) const
{
	if (auto pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl))
	{
		GET_TARGET get_target;
		get_target.target = TARGET_NEXT;
		get_target.hwndTarget = NULL;
		get_target.m_nIndex = 0x7FFFFFFF;
		get_target.m_nCurrentIndex = pCtrl->m_nIndex;
		EnumChildWindows(GetParent(hwndCtrl), GetTargetProc, (LPARAM)&get_target);
		return get_target.hwndTarget;
	}
	return NULL;
}

HWND MRadDialog::GetPrevCtrl(HWND hwndCtrl) const
{
	if (auto pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl))
	{
		GET_TARGET get_target;
		get_target.target = TARGET_PREV;
		get_target.hwndTarget = NULL;
		get_target.m_nIndex = -1;
		get_target.m_nCurrentIndex = pCtrl->m_nIndex;
		EnumChildWindows(GetParent(hwndCtrl), GetTargetProc, (LPARAM)&get_target);
		return get_target.hwndTarget;
	}
	return NULL;
}

HWND MRadDialog::GetFirstCtrl(HWND hwndParent)
{
	GET_TARGET get_target;
	get_target.target = TARGET_FIRST;
	get_target.hwndTarget = NULL;
	get_target.m_nIndex = 0x7FFFFFFF;
	EnumChildWindows(hwndParent, GetTargetProc, (LPARAM)&get_target);
	return get_target.hwndTarget;
}

HWND MRadDialog::GetLastCtrl(HWND hwndParent)
{
	GET_TARGET get_target;
	get_target.target = TARGET_LAST;
	get_target.hwndTarget = NULL;
	get_target.m_nIndex = -1;
	EnumChildWindows(hwndParent, GetTargetProc, (LPARAM)&get_target);
	return get_target.hwndTarget;
}

// the dialog procedure of MRadDialog
INT_PTR CALLBACK MRadDialog::DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
		HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
		HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, OnLButtonDown);
		HANDLE_MSG(hwnd, WM_RBUTTONDOWN, OnRButtonDown);
		HANDLE_MSG(hwnd, WM_RBUTTONDBLCLK, OnRButtonDown);
		HANDLE_MESSAGE(hwnd, MYWM_SELCHANGE, OnSelChange);
		HANDLE_MESSAGE(hwnd, MYWM_REDRAW, OnRedraw);
	}
	return 0;
}

LRESULT MRadDialog::DoSendMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ::SendMessage(hwnd, uMsg, wParam, lParam);
}

// MRadDialog WM_RBUTTONDOWN
void MRadDialog::OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	// clicked on the MRadDialog

	// if [Shift] and/or [Ctrl] was pressed, then ignore
	if (::GetKeyState(VK_SHIFT) < 0 || ::GetKeyState(VK_CONTROL) < 0)
		return;

	// update the clicked position
	m_ptClicked.x = x;
	m_ptClicked.y = y;

	// deselect the controls
	MRadCtrl::DeselectSelection();

	// notify MYWM_SELCHANGE to the parent
	DoSendMessage(GetParent(hwnd), MYWM_SELCHANGE, 0, 0);
}

// MRadDialog MYWM_REDRAW
LRESULT MRadDialog::OnRedraw(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// PaintIndexLabels already forces a synchronous update of the
	// dialog and its children before drawing the numbers; a prior
	// InvalidateRect only added an extra paint pass and contributed
	// to index-label flicker under rapid MYWM_REDRAW posting.
	PaintIndexLabels(hwnd);
	return 0;
}

// MRadDialog MYWM_SELCHANGE
LRESULT MRadDialog::OnSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// notify MYWM_SELCHANGE to the parent
	DoSendMessage(GetParent(hwnd), MYWM_SELCHANGE, wParam, lParam);
	return 0;
}

// get the normalized rectangle from two points
void MRadDialog::NormalizeRect(RECT *prc, POINT pt0, POINT pt1)
{
	if (pt0.x < pt1.x)
	{
		prc->left = pt0.x;
		prc->right = pt1.x;
	}
	else
	{
		prc->left = pt1.x;
		prc->right = pt0.x;
	}

	if (pt0.y < pt1.y)
	{
		prc->top = pt0.y;
		prc->bottom = pt1.y;
	}
	else
	{
		prc->top = pt1.y;
		prc->bottom = pt0.y;
	}
}

// draw the dragging rectangle
void MRadDialog::DrawDragSelect(HWND hwnd)
{
	if (HDC hDC = GetDC(hwnd))
	{
		RECT rc;
		NormalizeRect(&rc, m_ptClicked, m_ptDragging);

		DrawFocusRect(hDC, &rc);

		ReleaseDC(hwnd, hDC);
	}
}

// MRadDialog WM_LBUTTONDOWN/WM_LBUTTONDBLCLK
void MRadDialog::OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	// update the clicked position
	m_ptClicked.x = x;
	m_ptClicked.y = y;

	// if not [Shift] nor [Ctrl] pressed
	if (::GetKeyState(VK_SHIFT) >= 0 && ::GetKeyState(VK_CONTROL) >= 0)
	{
		if (fDoubleClick)
		{
			// send MYWM_RADDBLCLICK to the parent
			DoSendMessage(GetParent(hwnd), MYWM_RADDBLCLICK, 0, (LPARAM)NULL);
			return;
		}
		else
		{
			// deselect the controls
			MRadCtrl::DeselectSelection();
		}
	}

	// if not range selection
	if (!MRadCtrl::GetRangeSelect())
	{
		// update the dragging position
		m_ptDragging = m_ptClicked;

		// draw the dragging selection
		DrawDragSelect(hwnd);

		// enable the range selection
		MRadCtrl::GetRangeSelect() = TRUE;

		// start mouse capturing
		SetCapture(hwnd);
	}

	// notify MYWM_SELCHANGE to the parent
	DoSendMessage(GetParent(hwnd), MYWM_SELCHANGE, 0, 0);
}

// MRadDialog WM_MOUSEMOVE
void MRadDialog::OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
	if (MRadCtrl::GetRangeSelect())     // in range selection
	{
		// erase the previous dragging selection
		DrawDragSelect(hwnd);

		// update the dragging position
		m_ptDragging.x = x;
		m_ptDragging.y = y;

		// draw the new dragging selection
		DrawDragSelect(hwnd);
	}
}

// MRadDialog WM_CAPTURECHANGED
void MRadDialog::OnCaptureChanged(HWND hwnd)
{
	if (MRadCtrl::GetRangeSelect())     // in range selection
	{
		// erase the previous dragging selection
		DrawDragSelect(hwnd);

		// disable range selection
		MRadCtrl::GetRangeSelect() = FALSE;
	}
}

// MRadDialog WM_LBUTTONUP
void MRadDialog::OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
	if (MRadCtrl::GetRangeSelect())     // in range selection
	{
		// get the rectangle from two points
		RECT rc;
		NormalizeRect(&rc, m_ptClicked, m_ptDragging);

		// convert it to the screen coordinates
		MapWindowRect(hwnd, NULL, &rc);

		// if [Shift] and [Ctrl] keys are not pressed
		if (GetAsyncKeyState(VK_SHIFT) >= 0 &&
			GetAsyncKeyState(VK_CONTROL) >= 0)
		{
			// deselect the controls
			MRadCtrl::DeselectSelection();
		}

		// release the capture
		ReleaseCapture();

		// disable range selection
		MRadCtrl::GetRangeSelect() = FALSE;

		// is [Ctrl] key down?
		BOOL bCtrlDown = GetAsyncKeyState(VK_CONTROL) < 0;

		// update the selection status of the controls
		MRadCtrl::DoRangeSelect(hwnd, &rc, bCtrlDown);
	}

	// notify MYWM_SELCHANGE to the parent
	DoSendMessage(GetParent(hwnd), MYWM_SELCHANGE, 0, 0);
}

// MRadDialog WM_ERASEBKGND
BOOL MRadDialog::OnEraseBkgnd(HWND hwnd, HDC hdc)
{
	// create the background brush if necessary
	if (m_hbrBack == NULL)
		ReCreateBackBrush();

	RECT rc;
	if (!GetUpdateRect(hwnd, &rc, FALSE))
		GetClientRect(hwnd, &rc);

	// NOTE: hwnd now has WS_CLIPCHILDREN (see OnInitDialog), so the
	// window manager already excludes every child window's rectangle
	// (rubber bands included) from hdc's clip region for us. We used to
	// do that by hand here with GetWindowRect/ExcludeClipRect for every
	// single child on every single erase, which is both slow (an
	// EnumChildWindows-scale scan on every redraw while dragging) and a
	// visible source of flicker, since the manual exclusion could lag a
	// frame behind the child that just moved. Just fill; the clipping
	// is free and always correct now.
	FillRect(hdc, &rc, m_hbrBack);
	return TRUE;    // processed
}

// Stably "overwrite" the index-number overlay on top of hwndDialog.
// There is no separate window for the numbers, no hit-testing, and no
// Z-order to maintain here -- we simply force hwndDialog and every one
// of its children to finish painting themselves first (which also
// erases whatever numbers a previous call drew, since it's a real
// repaint through WM_ERASEBKGND), and then, if the owning MRadDialog's
// m_index_visible is TRUE, draw the numbers straight onto hwndDialog's
// own client DC.
//
// hwndDialog has WS_CLIPCHILDREN (see OnInitDialog), so a plain
// GetDC(hwndDialog) already has every child control's rectangle
// excluded from its visible region -- SelectClipRgn(hdc, NULL) alone
// cannot undo that, since it only resets an application's own *extra*
// clip region, not the clipping baked in by the window's style.
// GetDC() is really just GetDCEx(hwnd, NULL, DCX_USESTYLE | DCX_CACHE)
// under the hood -- DCX_USESTYLE is what makes it consult the window's
// WS_CLIPCHILDREN/WS_CLIPSIBLINGS bits. Calling GetDCEx() ourselves
// without DCX_USESTYLE (and without DCX_CLIPCHILDREN) skips that, so
// the returned DC paints over every child instead of excluding them.
//
// NOTE: we deliberately do NOT use GetDC(NULL) (the screen DC) here,
// even though that also bypasses WS_CLIPCHILDREN. Under DWM (on by
// default since Vista), each window is composited from its own
// off-screen surface; writing straight to the screen DC lands outside
// that surface, so the very next time DWM recomposites the area our
// pixels vanish, and until then they can linger as ghosting or flash
// an uncomposited black frame. Drawing into hwndDialog's own DC keeps
// the numbers part of the window's real content, so they behave like
// any other GDI drawing under composition.
/*static*/ void MRadDialog::PaintIndexLabels(HWND hwndDialog)
{
	if (!hwndDialog || !IsWindow(hwndDialog))
		return;

	MRadDialog *pDialog = static_cast<MRadDialog *>(MDialogBase::GetUserData(hwndDialog));
	if (!pDialog)
		return;

	// When labels are hidden we only need children/background to finish
	// painting (to erase any previous numbers). Skip RDW_ERASE -- the
	// window already has a valid background and erasing here is a major
	// flicker source during selection/Z-order changes with labels off.
	// When labels are shown we still need a full erase so leftover blue
	// number cells at previous control positions do not ghost.
	UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN;
	if (pDialog->m_index_visible)
		flags |= RDW_ERASE;
	::RedrawWindow(hwndDialog, NULL, NULL, flags);

	if (!pDialog->m_index_visible)
		return;

	// a client-area DC for hwndDialog that, unlike plain GetDC(), does
	// NOT exclude the area covered by child windows -- omitting
	// DCX_USESTYLE (and DCX_CLIPCHILDREN) is what makes the difference
	HDC hdc = ::GetDCEx(hwndDialog, NULL, DCX_CACHE);
	if (!hdc)
		return;

	// make sure no leftover application clip region restricts where we
	// can draw within that (already unclipped-from-children) DC
	::SelectClipRgn(hdc, NULL);

	HGDIOBJ hFontOld = ::SelectObject(hdc, pDialog->m_hFontLabel);
	::SetTextColor(hdc, RGB(255, 255, 255));
	::SetBkMode(hdc, OPAQUE);
	::SetBkColor(hdc, RGB(0, 0, 255));

	for (auto& pair : MRadCtrl::IndexToCtrlMap())
	{
		RECT rc;
		::GetWindowRect(pair.second, &rc);
		::MapWindowRect(NULL, hwndDialog, &rc);   // -> hwndDialog client coords

		TCHAR szText[32];
		StringCchPrintf(szText, _countof(szText), TEXT("%d"), pair.first);
		::TextOut(hdc, rc.left, rc.top, szText, lstrlen(szText));
	}

	::SelectObject(hdc, hFontOld);
	::ReleaseDC(hwndDialog, hdc);
}

// the window procedure of MRadDialog
LRESULT CALLBACK MRadDialog::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG(hwnd, WM_NCLBUTTONDBLCLK, OnNCLButtonDown);
		HANDLE_MSG(hwnd, WM_NCLBUTTONDOWN, OnNCLButtonDown);
		HANDLE_MSG(hwnd, WM_NCLBUTTONUP, OnNCLButtonUp);
		HANDLE_MSG(hwnd, WM_NCRBUTTONDBLCLK, OnNCRButtonDown);
		HANDLE_MSG(hwnd, WM_NCRBUTTONDOWN, OnNCRButtonDown);
		HANDLE_MSG(hwnd, WM_NCRBUTTONUP, OnNCRButtonUp);
		HANDLE_MSG(hwnd, WM_NCMOUSEMOVE, OnNCMouseMove);
		HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
		HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
		HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
		HANDLE_MSG(hwnd, WM_SIZE, OnSize);
		HANDLE_MSG(hwnd, WM_SYSCOLORCHANGE, OnSysColorChange);
		HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
		HANDLE_MESSAGE(hwnd, MYWM_CTRLMOVE, OnCtrlMove);
		HANDLE_MESSAGE(hwnd, MYWM_CTRLSIZE, OnCtrlSize);
		HANDLE_MESSAGE(hwnd, MYWM_DELETESEL, OnDeleteSel);
		HANDLE_MESSAGE(hwnd, MYWM_RADDBLCLICK, OnRadDblClick);
		case WM_CAPTURECHANGED:
			OnCaptureChanged(hwnd);
			break;
	}
	return CallWindowProcDx(hwnd, uMsg, wParam, lParam);
}

// MRadDialog MYWM_RADDBLCLICK
LRESULT MRadDialog::OnRadDblClick(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// send MYWM_RADDBLCLICK to the parent
	DoSendMessage(GetParent(hwnd), MYWM_RADDBLCLICK, wParam, lParam);
	return 0;
}

// MRadDialog WM_SIZE
void MRadDialog::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	// While ReCreateRadDialog/CreateDx is building the dialog it sets
	// m_bMovingSizing so intermediate size changes do not push
	// MYWM_UPDATEDLGRES (and the deferred SetWindowText on m_hCodeEditor).
	// Without this, opening GUI edit rewrites the code pane for an
	// unchanged dialog and shows up as flicker.
	if (m_bMovingSizing)
		return;

	// send MYWM_DLGSIZE to the parent
	DoSendMessage(GetParent(hwnd), MYWM_DLGSIZE, 0, 0);
}

// MRadDialog MYWM_DELETESEL
LRESULT MRadDialog::OnDeleteSel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// send MYWM_DELETESEL to the parent
	DoSendMessage(GetParent(hwnd), MYWM_DELETESEL, wParam, lParam);
	return 0;
}

// MRadDialog MYWM_CTRLMOVE
LRESULT MRadDialog::OnCtrlMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// send MYWM_CTRLMOVE to the parent
	DoSendMessage(GetParent(hwnd), MYWM_CTRLMOVE, wParam, lParam);
	return 0;
}

// MRadDialog MYWM_CTRLSIZE
LRESULT MRadDialog::OnCtrlSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// send MYWM_CTRLSIZE to the parent
	DoSendMessage(GetParent(hwnd), MYWM_CTRLSIZE, wParam, lParam);
	return 0;
}

// MRadDialog WM_NCLBUTTONDOWN/WM_NCLBUTTONDBLCLK
void MRadDialog::OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
	// eat
}

// MRadDialog WM_NCLBUTTONUP
void MRadDialog::OnNCLButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
{
	// eat
}

// MRadDialog WM_NCRBUTTONDOWN/WM_NCRBUTTONDBLCLK
void MRadDialog::OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
	if (fDoubleClick)
		return;

	// update the clicked position
	m_ptClicked.x = x;
	m_ptClicked.y = y;
	// into screen coordinates
	ScreenToClient(hwnd, &m_ptClicked);

	// send WM_CONTEXTMENU to the parent
	FORWARD_WM_CONTEXTMENU(GetParent(hwnd), hwnd, x, y, DoSendMessage);
}

// MRadDialog WM_NCRBUTTONUP
void MRadDialog::OnNCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
{
	// eat
}

// MRadDialog WM_NCMOUSEMOVE
void MRadDialog::OnNCMouseMove(HWND hwnd, int x, int y, UINT codeHitTest)
{
	// eat
}

// MRadDialog WM_KEYDOWN/WM_KEYUP
void MRadDialog::OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	if (fDown)
	{
		// send it to the parent
		FORWARD_WM_KEYDOWN(GetParent(hwnd), vk, cRepeat, flags, DoSendMessage);
	}
}

// NOTE: We have to do subclassing all the children controls and their descendants
//       to modify the hittesting.

// do subclassing a control and its descendants
void MRadDialog::DoSubclass(HWND hCtrl, INT nIndex)
{
	// make it an MRadCtrl
	auto pCtrl = std::make_shared<MRadCtrl>();
	pCtrl->SubclassDx(hCtrl);
	pCtrl->m_bTopCtrl = (nIndex != -1);
	pCtrl->m_nIndex = nIndex;
	m_rad_ctrls.push_back(pCtrl);

	if (nIndex != -1)   // a top control?
	{
		// update the index-to-control mapping
		MRadCtrl::IndexToCtrlMap()[nIndex] = hCtrl;

		// Top controls are free-standing siblings that the user can
		// overlap (e.g. move one on top of another) while editing, so
		// give each of them WS_CLIPSIBLINGS: without it, an overlapping
		// sibling and the control underneath can each repaint over the
		// other's area, which is another visible flicker source
		// (especially noticeable while dragging a selection over other
		// controls).
		pCtrl->ModifyStyleDx(0, WS_CLIPSIBLINGS);
	}

	pCtrl->PostSubclass();

#ifndef NDEBUG
	MString text = GetWindowText(hCtrl);
	MTRACE(TEXT("MRadCtrl::DoSubclass: %p, %d, '%s'\n"), hCtrl, nIndex, text.c_str());
#endif

	// do subclassing its children
	DoSubclassChildren(hCtrl);
}

// do subclassing the children
void MRadDialog::DoSubclassChildren(HWND hwnd, BOOL bTop)
{
	HWND hCtrl = GetTopWindow(hwnd);
	if (bTop)   // a top control?
	{
		INT nIndex = 0;
		while (hCtrl)
		{
			// do subclassing
			DoSubclass(hCtrl, nIndex);

			// increment the index
			++nIndex;

			// get the next control
			hCtrl = GetWindow(hCtrl, GW_HWNDNEXT);
		}
	}
	else    // not a top control
	{
		while (hCtrl)
		{
			// subclass the non-top control
			DoSubclass(hCtrl, -1);

			// get the next
			hCtrl = GetWindow(hCtrl, GW_HWNDNEXT);
		}
	}
}

// create the background brush
BOOL MRadDialog::ReCreateBackBrush()
{
	DeleteBackBrush();

	// 3D face collor
	COLORREF rgb = GetSysColor(COLOR_3DFACE);

	// calculate another color
	DWORD dwTotal = GetRValue(rgb) + GetGValue(rgb) + GetBValue(rgb);
	rgb = (dwTotal < 255 * 3 / 2) ? RGB(255, 255, 255) : RGB(0, 0, 0);

	// an 8x8-pixel rectangle
	RECT rc8x8 = { 0, 0, 8, 8 };

	// create an 8x8 bitmap
	HBITMAP hbm8x8 = Create24BppBitmapDx(8, 8);
	if (HDC hDC = CreateCompatibleDC(NULL))
	{
		HGDIOBJ hbmOld = SelectObject(hDC, hbm8x8);
		{
			FillRect(hDC, &rc8x8, (HBRUSH)(COLOR_3DFACE + 1));
			if (g_settings.bShowDotsOnDialog)
			{
				for (int y = 0; y < 8; y += 4)
				{
					for (int x = 0; x < 8; x += 4)
					{
						SetPixelV(hDC, x, y, rgb);
					}
				}
			}
		}
		SelectObject(hDC, hbmOld);
		DeleteDC(hDC);
	}

	// create a packed DIB
	std::vector<BYTE> data;
	PackedDIB_CreateFromHandle(data, hbm8x8);
	DeleteObject(hbm8x8);

	// create the brush
	m_hbrBack = CreateDIBPatternBrushPt(&data[0], DIB_RGB_COLORS);
	return m_hbrBack != NULL;
}

// MRadDialog WM_SYSCOLORCHANGE
void MRadDialog::OnSysColorChange(HWND hwnd)
{
	// recreate the back brush
	ReCreateBackBrush();

	// redraw
	InvalidateRect(hwnd, NULL, TRUE);
}

// MRadDialog WM_INITDIALOG
BOOL MRadDialog::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	// update the background brush
	OnSysColorChange(hwnd);

	// initialize
	MRadCtrl::GetTargets().clear();
	MRadCtrl::GetLastSel() = NULL;
	MRadCtrl::IndexToCtrlMap().clear();

	// move this dialog
	POINT pt = { 0, 0 };
	SetWindowPosDx(hwnd, &pt);

	// Let the OS clip the children out of our own background painting
	// instead of us manually excluding every child rectangle by hand in
	// OnEraseBkgnd on every single redraw (see OnEraseBkgnd below). This
	// is a big part of fixing the flicker: without it, the pattern-brush
	// background gets drawn, then every child repaints on top of it,
	// which is visibly a two-step (flickering) redraw.
	// NOTE: 'hwnd' is used here explicitly (not the ModifyStyleDx()
	// helper / m_hwnd) because this dialog isn't subclassed yet at this
	// point -- SubclassDx(hwnd) only happens further down below.
	SetWindowLongPtr(hwnd, GWL_STYLE,
		GetWindowLongPtr(hwnd, GWL_STYLE) | WS_CLIPCHILDREN);

	// do subclassing the children of this dialog
	DoSubclassChildren(hwnd, TRUE);

	// pin any group boxes to the very back right away, so a control
	// that just happens to sit in front of one in the dialog template's
	// original order doesn't get hidden by it later
	MRadCtrl::PinGroupBoxesToBack(hwnd);

	// if indeces are visible
	if (m_index_visible)
	{
		ShowIndexLabels(TRUE);   // show the labels
	}

	// do subclassing this dialog
	SubclassDx(hwnd);

	return FALSE;
}

// show/hide the index-number overlay. There is no separate window to
// create or destroy any more -- just flip the flag and let
// PaintIndexLabels() do the actual (re)painting: a real repaint erases
// whatever numbers were drawn before, and then draws fresh ones only
// if bShow is TRUE.
void MRadDialog::ShowIndexLabels(BOOL bShow)
{
	m_index_visible = bShow;
	PaintIndexLabels(m_hwnd);
}

//////////////////////////////////////////////////////////////////////////////
// MRadWindow --- the RADical window
// NOTE: An MRadWindow contains an MRadDialog.

// constructor
MRadWindow::MRadWindow()
	: m_xDialogBaseUnit(0)
	, m_yDialogBaseUnit(0)
	, m_hIcon(NULL)
	, m_hIconSm(NULL)
	, m_clipboard(m_dialog_res)
{
}

// destructor
MRadWindow::~MRadWindow()
{
	// delete the icons
	if (m_hIcon)
	{
		DestroyIcon(m_hIcon);
		m_hIcon = NULL;
	}
	if (m_hIconSm)
	{
		DestroyIcon(m_hIconSm);
		m_hIconSm = NULL;
	}

	// clear the mappings
	clear_maps();
}

// create the mappings
void MRadWindow::create_maps(LANGID lang)
{
	// for all the dialog items
	for (size_t i = 0; i < m_dialog_res.size(); ++i)
	{
		auto& item = m_dialog_res[i];
		// is it a STATIC control?
		if (item.m_class == 0x0082 ||
			lstrcmpiW(item.m_class.c_str(), L"STATIC") == 0)
		{
			if ((item.m_style & SS_TYPEMASK) == SS_ICON)
			{
				// icon
				g_res.do_icon(m_title_to_icon, item, lang);
			}
			else if ((item.m_style & SS_TYPEMASK) == SS_BITMAP)
			{
				// bitmap
				g_res.do_bitmap(m_title_to_bitmap, item, lang);
			}
		}
	}
}

// clear the mappings
void MRadWindow::clear_maps()
{
	for (auto& pair : m_title_to_bitmap)
	{
		DeleteObject(pair.second);
	}
	m_title_to_bitmap.clear();

	for (auto& pair : m_title_to_icon)
	{
		DestroyIcon(pair.second);
	}
	m_title_to_icon.clear();
}

VOID MRadWindow::DestroyWindow()
{
	::DestroyWindow(m_hwnd);
}

// create an MRadWindow window
BOOL MRadWindow::CreateDx(HWND hwndParent)
{
	// lock the moving/resizing
	BOOL bMovingSizing = m_rad_dialog.m_bMovingSizing;
	m_rad_dialog.m_bMovingSizing = TRUE;

	// create the window
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
	if (CreateWindowDx(hwndParent, MAKEINTRESOURCE(IDS_RADWINDOW), style))
	{
		// show it
		CenterWindowDx();
		ShowWindow(m_hwnd, SW_SHOWNORMAL);
		UpdateWindow(m_hwnd);

		// resume the moving/resizing flag
		m_rad_dialog.m_bMovingSizing = bMovingSizing;

		return TRUE;    // success
	}

	// resume the moving/resizing flag
	m_rad_dialog.m_bMovingSizing = bMovingSizing;

	return FALSE;   // failure
}

// convert the coordinates
void MRadWindow::ClientToDialog(POINT *ppt)
{
	GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
	ppt->x = MulDiv(ppt->x, 4, m_xDialogBaseUnit);
	ppt->y = MulDiv(ppt->y, 8, m_yDialogBaseUnit);
}

// convert the coordinates
void MRadWindow::ClientToDialog(SIZE *psiz)
{
	GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
	psiz->cx = MulDiv(psiz->cx, 4, m_xDialogBaseUnit);
	psiz->cy = MulDiv(psiz->cy, 8, m_yDialogBaseUnit);
}

// convert the coordinates
void MRadWindow::ClientToDialog(RECT *prc)
{
	GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
	prc->left = MulDiv(prc->left, 4, m_xDialogBaseUnit);
	prc->right = MulDiv(prc->right, 4, m_xDialogBaseUnit);
	prc->top = MulDiv(prc->top, 8, m_yDialogBaseUnit);
	prc->bottom = MulDiv(prc->bottom, 8, m_yDialogBaseUnit);
}

// convert the coordinates
void MRadWindow::DialogToClient(POINT *ppt)
{
	GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
	ppt->x = (ppt->x * m_xDialogBaseUnit + 2) / 4;
	ppt->y = (ppt->y * m_yDialogBaseUnit + 4) / 8;
}

// convert the coordinates
void MRadWindow::DialogToClient(SIZE *psiz)
{
	GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
	psiz->cx = (psiz->cx * m_xDialogBaseUnit + 2) / 4;
	psiz->cy = (psiz->cy * m_yDialogBaseUnit + 4) / 8;
}

// convert the coordinates
void MRadWindow::DialogToClient(RECT *prc)
{
	GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
	prc->left = (prc->left * m_xDialogBaseUnit + 2) / 4;
	prc->right = (prc->right * m_xDialogBaseUnit + 2) / 4;
	prc->top = (prc->top * m_yDialogBaseUnit + 4) / 8;
	prc->bottom = (prc->bottom * m_yDialogBaseUnit + 4) / 8;
}

HWND MRadWindow::GetPrimaryControl(HWND hwnd, HWND hwndDialog)
{
	for (;;)
	{
		if (GetParent(hwnd) == NULL || GetParent(hwnd) == hwndDialog)
			return hwnd;

		hwnd = GetParent(hwnd);
	}
}

// adjust MRadWindow's position and size to MRadDialog's client area
void MRadWindow::FitToRadDialog()
{
	// get the window rectangle
	RECT rc;
	GetWindowRect(m_rad_dialog, &rc);
	SIZE siz = SizeFromRectDx(&rc);

	// adjust the rectangle
	SetRect(&rc, 0, 0, siz.cx, siz.cy);
	DWORD style = GetWindowLong(m_hwnd, GWL_STYLE);
	DWORD exstyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);
	AdjustWindowRectEx(&rc, style, FALSE, exstyle);

	// resize the MRadWindow
	siz = SizeFromRectDx(&rc);
	SetWindowPosDx(NULL, &siz);
}

// the window class name
LPCTSTR MRadWindow::GetWndClassNameDx() const
{
	return TEXT("katahiromz's MRadWindow Class");
}

void MRadWindow::ModifyWndClassDx(WNDCLASSEX& wcx)
{
	// no class icon
	wcx.hIcon = NULL;
	wcx.hIconSm = NULL;

	// dark gray background
	wcx.hbrBackground = GetStockBrush(DKGRAY_BRUSH);
}

LRESULT MRadWindow::DoSendMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ::SendMessage(hwnd, uMsg, wParam, lParam);
}

// recreate the MRadDialog window
BOOL MRadWindow::ReCreateRadDialog(HWND hwnd, INT nSelectStartIndex)
{
	assert(IsWindow(hwnd));

	// lock moving/resizing
	BOOL bMovingSizingOld = m_rad_dialog.m_bMovingSizing;
	m_rad_dialog.m_bMovingSizing = TRUE;

	// destroy MRadDialog window if any
	if (m_rad_dialog)
	{
		::DestroyWindow(m_rad_dialog);
	}

	m_pOleHost = std::make_shared<MOleHost>();
	DoSetActiveOleHost(m_pOleHost.get());

	// get the resource data
	m_dialog_res.FixupForRad(false);
	std::vector<BYTE> data = m_dialog_res.data();
#if 0
	MFile file(TEXT("modified.bin"), TRUE);
	DWORD cbWritten;
	file.WriteFile(&data[0], (DWORD)data.size(), &cbWritten);
#endif
	m_dialog_res.FixupForRad(true);

	// create the MRadDialog window from data
	if (!m_rad_dialog.CreateDialogIndirectDx(hwnd, &data[0]))
	{
		m_rad_dialog.m_bMovingSizing = bMovingSizingOld;
		return FALSE;
	}
	assert(IsWindow(m_rad_dialog));

	// adjust the MRadWindow's size to MRadDialog
	FitToRadDialog();

	// Keep m_bMovingSizing locked across ShowWindow/UpdateWindow so the
	// resulting WM_SIZE does not fire MYWM_DLGSIZE → UpdateRes → the
	// deferred m_hCodeEditor SetWindowText path (visible as a flash when
	// opening or refreshing the GUI editor).
	ShowWindow(m_rad_dialog, SW_SHOWNOACTIVATE);
	UpdateWindow(m_rad_dialog);

	// make it foreground
	SetForegroundWindow(hwnd);

	// update the mappings
	update_maps();

	// unlock after the window is up
	m_rad_dialog.m_bMovingSizing = bMovingSizingOld;

	// select the RADical control of the specified index
	if (nSelectStartIndex != -1)
	{
		HWND hwndNext = MRadDialog::GetFirstCtrl(hwnd);
		while (hwndNext)
		{
			if (auto pCtrl = MRadCtrl::GetRadCtrl(hwndNext))
			{
				if (pCtrl->m_nIndex >= nSelectStartIndex)
					MRadCtrl::Select(hwndNext);
			}
			hwndNext = m_rad_dialog.GetNextCtrl(hwndNext);
		}
	}

	// Only notify when a selection was actually restored. On a plain open
	// (nSelectStartIndex == -1, empty targets) MYWM_SELCHANGE would only
	// clear line marks and invalidate m_hCodeEditor for no visible change.
	if (nSelectStartIndex != -1 || !MRadCtrl::GetTargets().empty())
	{
		DoSendMessage(GetParent(hwnd), MYWM_SELCHANGE, 0, 0);
	}

	return TRUE;
}

// update the mappings
void MRadWindow::update_maps()
{
	GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);

	// for all the RADical controls
	for (HWND hCtrl = GetTopWindow(m_rad_dialog);
		 hCtrl;
		 hCtrl = GetNextWindow(hCtrl, GW_HWNDNEXT))
	{
		// is it a RADical control?
		if (auto pCtrl = MRadCtrl::GetRadCtrl(hCtrl))
		{
			// get the size
			SIZE siz = { 0, 0 };
			GetWindowPosDx(hCtrl, NULL, &siz);

			if (pCtrl->m_nImageType == 1)
			{
				// icon
				MIdOrString title = m_dialog_res[pCtrl->m_nIndex].m_title;
				if (HICON hIcon = m_title_to_icon[title])
				{
					DoSendMessage(hCtrl, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
					DWORD style = GetWindowStyle(hCtrl);
					if (style & SS_REALSIZEIMAGE)
					{
						ICONINFO info;
						GetIconInfo(hIcon, &info);
						BITMAP bm;
						GetObject(info.hbmColor, sizeof(BITMAP), &bm);
						siz.cx = bm.bmWidth;
						siz.cy = bm.bmHeight;
						DeleteObject(info.hbmMask);
						DeleteObject(info.hbmColor);
					}
					else if (style & SS_REALSIZECONTROL)
					{
						siz.cx = m_dialog_res[pCtrl->m_nIndex].m_siz.cx * m_xDialogBaseUnit / 4;
						siz.cy = m_dialog_res[pCtrl->m_nIndex].m_siz.cy * m_yDialogBaseUnit / 8;
					}
					if (siz.cx <= 8)
						siz.cx = 8;
					if (siz.cy <= 8)
						siz.cy = 8;

					// resize
					SetWindowPosDx(hCtrl, NULL, &siz);
				}
			}
			if (pCtrl->m_nImageType == 2)
			{
				// bitmap
				MIdOrString title = m_dialog_res[pCtrl->m_nIndex].m_title;
				if (HBITMAP hbm = m_title_to_bitmap[title])
				{
					DoSendMessage(hCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
					DWORD style = GetWindowStyle(hCtrl);
					if (style & SS_REALSIZECONTROL)
					{
						siz.cx = m_dialog_res[pCtrl->m_nIndex].m_siz.cx * m_xDialogBaseUnit / 4;
						siz.cy = m_dialog_res[pCtrl->m_nIndex].m_siz.cy * m_yDialogBaseUnit / 8;
					}
					else
					{
						BITMAP bm;
						GetObject(hbm, sizeof(BITMAP), &bm);
						siz.cx = bm.bmWidth;
						siz.cy = bm.bmHeight;
					}
					if (siz.cx <= 8)
						siz.cx = 8;
					if (siz.cy <= 8)
						siz.cy = 8;

					// resize
					SetWindowPosDx(hCtrl, NULL, &siz);
				}
			}
		}
	}
}

// MRadWindow WM_CREATE
BOOL MRadWindow::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	// create the icons
	m_hIcon = LoadIconDx(IDI_SMILY);
	m_hIconSm = LoadSmallIconDx(IDI_SMILY);

	// set the icons
	DoSendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
	DoSendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

	// resume the window position if necessary
	if (g_settings.bResumeWindowPos)
	{
		if (g_settings.nRadLeft != CW_USEDEFAULT)
		{
			POINT pt = { g_settings.nRadLeft, g_settings.nRadTop };
			SetWindowPosDx(&pt);
		}
		else
		{
			CenterWindowDx(hwnd);
		}
	}
	else
	{
		CenterWindowDx(hwnd);
	}

	// create the RADical dialog (MRadDialog)
	return ReCreateRadDialog(hwnd);
}

// MRadWindow WM_DESTROY
void MRadWindow::OnDestroy(HWND hwnd)
{
	// send ID_DESTROYRAD to the owner
	HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
	DoSendMessage(hwndOwner, WM_COMMAND, ID_DESTROYRAD, 0);

	MRadCtrl::GetTargets().clear();
	MRadCtrl::GetLastSel() = NULL;
	MRadCtrl::IndexToCtrlMap().clear();

	// destroy the icons
	if (m_hIcon)
	{
		DestroyIcon(m_hIcon);
		m_hIcon = NULL;
	}
	if (m_hIconSm)
	{
		DestroyIcon(m_hIconSm);
		m_hIconSm = NULL;
	}
}

// the window procedure of MRadWindow
LRESULT CALLBACK
MRadWindow::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		DO_MSG(WM_CREATE, OnCreate);
		DO_MSG(WM_MOVE, OnMove);
		DO_MSG(WM_SIZE, OnSize);
		DO_MSG(WM_DESTROY, OnDestroy);
		DO_MSG(WM_CONTEXTMENU, OnContextMenu);
		DO_MSG(WM_KEYDOWN, OnKey);
		DO_MSG(WM_COMMAND, OnCommand);
		DO_MESSAGE(MYWM_CTRLMOVE, OnCtrlMove);
		DO_MESSAGE(MYWM_CTRLSIZE, OnCtrlSize);
		DO_MESSAGE(MYWM_DLGSIZE, OnDlgSize);
		DO_MESSAGE(MYWM_DELETESEL, OnDeleteSel);
		DO_MESSAGE(MYWM_SELCHANGE, OnSelChange);
		DO_MESSAGE(MYWM_GETUNITS, OnGetUnits);
		DO_MESSAGE(MYWM_RADDBLCLICK, OnRadDblClick);
		DO_MSG(WM_INITMENUPOPUP, OnInitMenuPopup);
		DO_MSG(WM_ACTIVATE, OnActivate);
		DO_MSG(WM_SYSCOLORCHANGE, OnSysColorChange);
	}
	return DefaultProcDx();
}

// MRadWindow WM_SYSCOLORCHANGE
void MRadWindow::OnSysColorChange(HWND hwnd)
{
	m_rad_dialog.SendMessageDx(WM_SYSCOLORCHANGE);
}

// MRadWindow MYWM_RADDBLCLICK
LRESULT MRadWindow::OnRadDblClick(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
	DoSendMessage(hwndOwner, MYWM_RADDBLCLICK, wParam, lParam);
	return 0;
}

// MRadWindow MYWM_GETUNITS
LRESULT MRadWindow::OnGetUnits(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// store the dialog base units
	GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
	return 0;
}

// MRadWindow WM_ACTIVATE
void MRadWindow::OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
	if (state == WA_ACTIVE || state == WA_CLICKACTIVE)
	{
		// check whether compilation requires or not
		HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
		if (!DoSendMessage(hwndOwner, MYWM_COMPILECHECK, (WPARAM)hwnd, 0))
		{
			return;
		}
	}

	// default processing
	FORWARD_WM_ACTIVATE(hwnd, state, hwndActDeact, fMinimized, DefWindowProc);
}

// MRadWindow MYWM_SELCHANGE
LRESULT MRadWindow::OnSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// get the owner window
	HWND hwndOwner = GetWindow(hwnd, GW_OWNER);

	// is there the last selection?
	HWND hwndSel = MRadCtrl::GetLastSel();
	if (hwndSel == NULL)    // no
	{
		// report the selection change to the owner window
		DoSendMessage(hwndOwner, MYWM_SELCHANGE, 0, 0);

		// clear the status
		DoSendMessage(hwndOwner, MYWM_CLEARSTATUS, 0, 0);

		return 0;
	}

	// get the MRadCtrl pointer
	auto pCtrl = MRadCtrl::GetRadCtrl(hwndSel);
	if (pCtrl == NULL)
	{
		// report the selection change to the owner window
		DoSendMessage(hwndOwner, MYWM_SELCHANGE, 0, 0);

		// clear the status
		DoSendMessage(hwndOwner, MYWM_CLEARSTATUS, 0, 0);
		return 0;
	}

	// check the index
	if (size_t(pCtrl->m_nIndex) < m_dialog_res.m_items.size())
	{
		// report the position and size of the index
		DialogItem& item = m_dialog_res[pCtrl->m_nIndex];
		DoSendMessage(hwndOwner, MYWM_MOVESIZEREPORT,
			MAKEWPARAM(item.m_pt.x, item.m_pt.y),
			MAKELPARAM(item.m_siz.cx, item.m_siz.cy));
	}

	// report the selection change to the owner window
	DoSendMessage(hwndOwner, MYWM_SELCHANGE, 0, 0);
	return 0;
}

// MRadWindow WM_INITMENUPOPUP
void MRadWindow::OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
{
	auto set = MRadCtrl::GetTargets();
	if (set.empty())
	{
		EnableMenuItem(hMenu, ID_DELCTRL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CTRLPROP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_TOPALIGN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_BOTTOMALIGN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_LEFTALIGN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_RIGHTALIGN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FITTOGRID, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CUT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPY, MF_GRAYED);
	}
	else if (set.size() == 1)
	{
		EnableMenuItem(hMenu, ID_DELCTRL, MF_ENABLED);
		EnableMenuItem(hMenu, ID_CTRLPROP, MF_ENABLED);
		EnableMenuItem(hMenu, ID_TOPALIGN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_BOTTOMALIGN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_LEFTALIGN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_RIGHTALIGN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FITTOGRID, MF_ENABLED);
		EnableMenuItem(hMenu, ID_CUT, MF_ENABLED);
		EnableMenuItem(hMenu, ID_COPY, MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hMenu, ID_DELCTRL, MF_ENABLED);
		EnableMenuItem(hMenu, ID_CTRLPROP, MF_ENABLED);
		EnableMenuItem(hMenu, ID_TOPALIGN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_BOTTOMALIGN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_LEFTALIGN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_RIGHTALIGN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_FITTOGRID, MF_ENABLED);
		EnableMenuItem(hMenu, ID_CUT, MF_ENABLED);
		EnableMenuItem(hMenu, ID_COPY, MF_ENABLED);
	}

	EnableMenuItem(hMenu, ID_CTRLINDEXTOP, CanIndexTop() ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenu, ID_CTRLINDEXBOTTOM, CanIndexBottom() ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenu, ID_CTRLINDEXMINUS, CanIndexMinus() ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenu, ID_CTRLINDEXPLUS, CanIndexPlus() ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenu, ID_PASTE, m_clipboard.IsAvailable() ? MF_ENABLED : MF_GRAYED);
}

// MRadWindow MYWM_DELETESEL
LRESULT MRadWindow::OnDeleteSel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// delete the selected dialog items from m_dialog_res
	auto indeces = MRadCtrl::GetTargetIndeces();
	for (size_t i = m_dialog_res.size(); i > 0;)
	{
		--i;
		if (indeces.find(INT(i)) != indeces.end())
		{
			m_dialog_res.m_items.erase(m_dialog_res.m_items.begin() + i);
			--m_dialog_res.m_cItems;
		}
	}

	// recreate the MRadDialog
	ReCreateRadDialog(hwnd);

	// update the resource
	UpdateRes();

	return 0;
}

// MRadWindow MYWM_CTRLMOVE
LRESULT MRadWindow::OnCtrlMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	HWND hwndCtrl = (HWND)wParam;

	auto pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl);
	if (pCtrl == NULL)
		return 0;   // invalid

	// check the index
	if (pCtrl->m_nIndex < 0 || m_dialog_res.m_cItems <= pCtrl->m_nIndex)
		return 0;   // invalid

	MTRACEA("OnCtrlMove: %d\n", pCtrl->m_nIndex);

	// get the rectangle of the control in dialog coordinates
	RECT rc;
	GetWindowRect(*pCtrl, &rc);
	MapWindowRect(NULL, m_rad_dialog, &rc);
	ClientToDialog(&rc);

	// update DialogItem position
	DialogItem& item = m_dialog_res[pCtrl->m_nIndex];
	item.m_pt.x = rc.left;
	item.m_pt.y = rc.top;

	// update resource
	UpdateRes();

	// notify the position/size change to the owner
	HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
	DoSendMessage(hwndOwner, MYWM_MOVESIZEREPORT,
		MAKEWPARAM(item.m_pt.x, item.m_pt.y),
		MAKELPARAM(item.m_siz.cx, item.m_siz.cy));

	// redraw
	PostMessage(m_rad_dialog, MYWM_REDRAW, 0, 0);

	return 0;
}

// MRadWindow MYWM_CTRLSIZE
LRESULT MRadWindow::OnCtrlSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	HWND hwndCtrl = (HWND)wParam;
	auto pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl);
	if (pCtrl == NULL)
		return 0;   // invalid

	// check the index
	if (pCtrl->m_nIndex < 0 || m_dialog_res.m_cItems <= pCtrl->m_nIndex)
		return 0;   // invalid

	MTRACEA("OnCtrlSize: %d\n", pCtrl->m_nIndex);

	// get the rectangle of the control in dialog coordinates
	RECT rc;
	GetWindowRect(*pCtrl, &rc);
	MapWindowRect(NULL, m_rad_dialog, &rc);
	ClientToDialog(&rc);

	// update DialogItem size
	DialogItem& item = m_dialog_res[pCtrl->m_nIndex];
	item.m_siz.cx = rc.right - rc.left;
	item.m_siz.cy = rc.bottom - rc.top;

	// if it was combobox, then apply the settings
	TCHAR szClass[MAX_PATH];
	GetClassName(hwndCtrl, szClass, _countof(szClass));
	if (lstrcmpi(szClass, TEXT("COMBOBOX")) == 0 ||
		lstrcmpi(szClass, WC_COMBOBOXEX) == 0)
	{
		item.m_siz.cy = g_settings.nComboHeight;
	}

	// update the resource
	UpdateRes();

	// notify the position/size change to the owner
	HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
	DoSendMessage(hwndOwner, MYWM_MOVESIZEREPORT,
		MAKEWPARAM(item.m_pt.x, item.m_pt.y),
		MAKELPARAM(item.m_siz.cx, item.m_siz.cy));

	// redraw
	PostMessage(m_rad_dialog, MYWM_REDRAW, 0, 0);

	return 0;
}

// update the resource
void MRadWindow::UpdateRes()
{
	// notify the update of dialog resource to the owner window
	HWND hwndOwner = ::GetWindow(m_hwnd, GW_OWNER);
	DoSendMessage(hwndOwner, MYWM_UPDATEDLGRES, 0, 0);

	// NOTE: UpdateRes is called continuously while dragging/resizing a
	// control (once per mouse-move). Do NOT call PaintIndexLabels here
	// -- OnCtrlMove/OnCtrlSize already post a single MYWM_REDRAW that
	// refreshes the index-number overlay. Painting synchronously on
	// every mouse-move (with RDW_ERASE) was the main cause of index
	// label flicker during drag.
}

// MRadWindow MYWM_DLGSIZE
LRESULT MRadWindow::OnDlgSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// get the rectangle of the dialog in dialog coordinates
	RECT rc;
	GetWindowRect(m_rad_dialog, &rc);
	ClientToDialog(&rc);

	// update m_dialog_res
	m_dialog_res.m_siz.cx = rc.right - rc.left;
	m_dialog_res.m_siz.cy = rc.bottom - rc.top;
	UpdateRes();

	// notify the position/size change to the owner
	HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
	DoSendMessage(hwndOwner, MYWM_MOVESIZEREPORT,
		MAKEWPARAM(rc.left, rc.top),
		MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));

	return 0;
}

// called from MMainWnd WM_COMMAND ID_ADDCTRL
void MRadWindow::OnAddCtrl(HWND hwnd)
{
	// get the client area
	RECT rc;
	GetClientRect(hwnd, &rc);

	// use the clicked position
	POINT pt = m_rad_dialog.m_ptClicked;

	// adjust the position
	if (pt.x < 0 || pt.y < 0)
		pt.x = pt.y = 0;
	if (rc.right - 30 < pt.x)
		pt.x = rc.right - 30;
	if (rc.bottom - 30 < pt.y)
		pt.y = rc.bottom - 30;

	ClientToDialog(&pt);

	// show the dialog
	MAddCtrlDlg dialog(m_dialog_res, pt);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add an RT_DLGINIT entry if necesssary
		MByteStreamEx::data_type data;
		if (m_dialog_res.SaveDlgInitData(data))
		{
			if (!data.empty())
			{
				g_res.add_lang_entry(RT_DLGINIT, m_dialog_res.m_name, m_dialog_res.m_lang, data);
			}
		}

		// refresh
		OnRefresh(hwnd);
	}
}

// called from MMainWnd WM_COMMAND ID_CTRLPROP
void MRadWindow::OnCtrlProp(HWND hwnd)
{
	// show the dialog
	MCtrlPropDlg dialog(m_dialog_res, MRadCtrl::GetTargetIndeces());
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		auto type = RT_DLGINIT;
		auto name = m_dialog_res.m_name;
		auto lang = m_dialog_res.m_lang;

		// add or delete an RT_DLGINIT entry if necesssary
		MByteStreamEx::data_type data;
		if (m_dialog_res.SaveDlgInitData(data))
		{
			if (data.empty())
			{
				g_res.search_and_delete(ET_LANG, type, name, lang);
			}
			else
			{
				g_res.add_lang_entry(type, name, lang, data);
			}
		}
		else
		{
			g_res.search_and_delete(ET_LANG, type, name, lang);
		}

		// refresh
		OnRefresh(hwnd);
	}
}

// called from MMainWnd WM_COMMAND ID_DLGPROP
void MRadWindow::OnDlgProp(HWND hwnd)
{
	// show the dialog
	MDlgPropDlg dialog(m_dialog_res);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// refresh
		OnRefresh(hwnd);
	}
}

// refresh
void MRadWindow::OnRefresh(HWND hwnd)
{
	// recreate MRadDialog
	ReCreateRadDialog(hwnd);

	// update the resource
	UpdateRes();
}

// show/hide the indeces
void MRadWindow::OnShowIndex(HWND hwnd)
{
	m_rad_dialog.m_index_visible = !m_rad_dialog.m_index_visible;
	m_rad_dialog.ShowIndexLabels(m_rad_dialog.m_index_visible);
}

// get the selected dialog items
BOOL MRadWindow::GetSelectedItems(DialogItems& items)
{
	for (INT index : MRadCtrl::GetTargetIndeces())
		items.push_back(m_dialog_res[index]);
	return !items.empty();
}

// MRadWindow WM_COMMAND
void MRadWindow::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	DialogItems items;
	static INT s_nShift = 0;

	switch (id)
	{
	case ID_CUT:
		if (GetSelectedItems(items))
		{
			m_clipboard.Copy(hwnd, items);
			MRadCtrl::DeleteSelection();
			s_nShift = 0;
		}
		return;

	case ID_COPY:
		if (GetSelectedItems(items))
		{
			m_clipboard.Copy(hwnd, items);
			s_nShift = 0;
		}
		return;

	case ID_PASTE:
		if (m_clipboard.Paste(hwnd, items))
		{
			s_nShift += 5;
			for (auto& item : items)
			{
				item.m_pt.x += s_nShift;
				item.m_pt.y += s_nShift;
				m_dialog_res.m_items.push_back(item);
				++m_dialog_res.m_cItems;
			}
			OnRefresh(hwnd);
		}
		return;
	}

	// notify WM_COMMAND to the owner window
	HWND hwndOwner = ::GetWindow(m_hwnd, GW_OWNER);
	FORWARD_WM_COMMAND(hwndOwner, id, hwndCtl, codeNotify, DoSendMessage);
}

void MRadWindow::AlignSelection(AlignEdge edge)
{
	auto& targets = MRadCtrl::GetTargets();
	if (targets.size() < 2)
		return;

	RECT rc;
	INT extreme = (edge == ALIGN_TOP || edge == ALIGN_LEFT) ? INT_MAX : INT_MIN;

	for (HWND hwndCtrl : targets)
	{
		GetWindowRect(hwndCtrl, &rc);
		MapWindowRect(NULL, m_rad_dialog, &rc);
		switch (edge)
		{
			case ALIGN_TOP:    if (rc.top < extreme) extreme = rc.top; break;
			case ALIGN_BOTTOM: if (extreme < rc.bottom) extreme = rc.bottom; break;
			case ALIGN_LEFT:   if (rc.left < extreme) extreme = rc.left; break;
			case ALIGN_RIGHT:  if (extreme < rc.right) extreme = rc.right; break;
		}
	}

	for (HWND hwndCtrl : targets)
	{
		auto pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl);
		GetWindowRect(hwndCtrl, &rc);
		MapWindowRect(NULL, m_rad_dialog, &rc);

		INT x = rc.left, y = rc.top;
		switch (edge)
		{
			case ALIGN_TOP:    y = extreme; break;
			case ALIGN_BOTTOM: y = extreme - (rc.bottom - rc.top); break;
			case ALIGN_LEFT:   x = extreme; break;
			case ALIGN_RIGHT:  x = extreme - (rc.right - rc.left); break;
		}

		pCtrl->m_bMoving = TRUE;
		SetWindowPos(hwndCtrl, NULL, x, y, 0, 0,
			SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
		pCtrl->m_bMoving = FALSE;
	}
}

void MRadWindow::OnTopAlign(HWND hwnd)    { AlignSelection(ALIGN_TOP); }
void MRadWindow::OnBottomAlign(HWND hwnd) { AlignSelection(ALIGN_BOTTOM); }
void MRadWindow::OnLeftAlign(HWND hwnd)   { AlignSelection(ALIGN_LEFT); }
void MRadWindow::OnRightAlign(HWND hwnd)  { AlignSelection(ALIGN_RIGHT); }

// able to make it top index? (true if any selected item follows an unselected one)
BOOL MRadWindow::CanIndexTop() const
{
	auto indeces = MRadCtrl::GetTargetIndeces();
	if (indeces.empty())
		return FALSE;

	BOOL saw_unselected = FALSE;
	for (UINT i = 0; i < m_dialog_res.m_cItems; ++i)
	{
		if (indeces.find(i) != indeces.end())
		{
			if (saw_unselected)
				return TRUE;
		}
		else
		{
			saw_unselected = TRUE;
		}
	}
	return FALSE;
}

// partition items into (unselected, selected) preserving relative order
void MRadWindow::PartitionBySelection(DialogItems& unselected, DialogItems& selected) const
{
	auto indeces = MRadCtrl::GetTargetIndeces();
	for (UINT i = 0; i < m_dialog_res.m_cItems; ++i)
	{
		if (indeces.find(i) == indeces.end())
			unselected.push_back(m_dialog_res[i]);
		else
			selected.push_back(m_dialog_res[i]);
	}
}

// make it top index
void MRadWindow::IndexTop(HWND hwnd)
{
	if (MRadCtrl::GetTargetIndeces().empty())
		return;

	DialogItems unselected, selected;
	PartitionBySelection(unselected, selected);
	m_dialog_res.m_items = std::move(unselected);
	m_dialog_res.m_items.insert(m_dialog_res.m_items.begin(), selected.begin(), selected.end());
	OnRefresh(hwnd);
}

// able to make it bottom index? (true if any selected item precedes an unselected one)
BOOL MRadWindow::CanIndexBottom() const
{
	auto indeces = MRadCtrl::GetTargetIndeces();
	if (indeces.empty())
		return FALSE;

	BOOL saw_unselected = FALSE;
	for (INT i = m_dialog_res.m_cItems - 1; i >= 0; --i)
	{
		if (indeces.find(i) != indeces.end())
		{
			if (saw_unselected)
				return TRUE;
		}
		else
		{
			saw_unselected = TRUE;
		}
	}
	return FALSE;
}

// make it bottom index
void MRadWindow::IndexBottom(HWND hwnd)
{
	if (MRadCtrl::GetTargetIndeces().empty())
		return;

	DialogItems unselected, selected;
	PartitionBySelection(unselected, selected);
	m_dialog_res.m_items = std::move(unselected);
	m_dialog_res.m_items.insert(m_dialog_res.m_items.end(), selected.begin(), selected.end());
	OnRefresh(hwnd);
}

// able to decrement the control index?
BOOL MRadWindow::CanIndexMinus() const
{
	auto indeces = MRadCtrl::GetTargetIndeces();
	return !indeces.empty() && indeces.count(0) == 0;
}

// decrement the control index
void MRadWindow::IndexMinus(HWND hwnd)
{
	auto indeces = MRadCtrl::GetTargetIndeces();
	if (indeces.empty())
		return;

	// move the dialog items
	for (INT i = 0; i < m_dialog_res.m_cItems - 1; ++i)
	{
		if (indeces.find(i + 1) != indeces.end())
		{
			std::swap(m_dialog_res[i], m_dialog_res[i + 1]);
		}
	}

	// refresh
	OnRefresh(hwnd);
}

// able to increment the control index?
BOOL MRadWindow::CanIndexPlus() const
{
	auto indeces = MRadCtrl::GetTargetIndeces();
	return !indeces.empty() && indeces.count(m_dialog_res.m_cItems - 1) == 0;
}

// increment the control index
void MRadWindow::IndexPlus(HWND hwnd)
{
	auto indeces = MRadCtrl::GetTargetIndeces();
	if (indeces.empty())
		return;

	// move the dialog items
	for (UINT i = m_dialog_res.m_cItems - 1; i > 0; --i)
	{
		if (indeces.find(i - 1) != indeces.end())
		{
			std::swap(m_dialog_res[i], m_dialog_res[i - 1]);
		}
	}

	// refresh
	OnRefresh(hwnd);
}

// MRadWindow WM_KEYDOWN/WM_KEYUP
void MRadWindow::OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	RECT rc;
	if (!fDown)
		return;     // ignore WM_KEYUP

	// get the target
	HWND hwndTarget = MRadCtrl::GetLastSel();
	if (hwndTarget == NULL && !MRadCtrl::GetTargets().empty())
	{
		hwndTarget = *MRadCtrl::GetTargets().begin();
	}

	// get the target control
	auto pCtrl = MRadCtrl::GetRadCtrl(hwndTarget);

	// for each case of virtual key
	switch (vk)
	{
	case VK_TAB:
		if (GetKeyState(VK_SHIFT) < 0)
		{
			// Shift+Tab
			HWND hwndNext = NULL;
			if (!hwndTarget)
			{
				hwndNext = MRadDialog::GetLastCtrl(hwnd);
			}
			else
			{
				hwndNext = m_rad_dialog.GetPrevCtrl(hwndTarget);
			}
			if (!hwndNext)
			{
				hwndNext = MRadDialog::GetLastCtrl(hwnd);
			}
			MRadCtrl::DeselectSelection();
			MRadCtrl::Select(hwndNext);
		}
		else
		{
			// Tab
			HWND hwndNext = NULL;
			if (!hwndTarget)
			{
				hwndNext = MRadDialog::GetFirstCtrl(hwnd);
			}
			else
			{
				hwndNext = m_rad_dialog.GetNextCtrl(hwndTarget);
			}
			if (!hwndNext)
			{
				hwndNext = MRadDialog::GetFirstCtrl(hwnd);
			}
			MRadCtrl::DeselectSelection();
			MRadCtrl::Select(hwndNext);
		}
		break;

	case VK_UP:
		if (pCtrl == NULL)
		{
			return;
		}
		if (GetKeyState(VK_SHIFT) < 0)
		{
			// Shift+Up
			GetWindowRect(*pCtrl, &rc);
			MapWindowRect(NULL, m_rad_dialog, &rc);
			SIZE siz = SizeFromRectDx(&rc);
			siz.cy -= 1;
			MRadCtrl::ResizeSelection(NULL, siz.cx, siz.cy);
		}
		else
		{
			// Up
			MRadCtrl::MoveSelection(NULL, 0, -1);
		}
		break;

	case VK_DOWN:
		if (pCtrl == NULL)
		{
			return;
		}
		if (GetKeyState(VK_SHIFT) < 0)
		{
			// Shift+Down
			GetWindowRect(*pCtrl, &rc);
			MapWindowRect(NULL, m_rad_dialog, &rc);
			SIZE siz = SizeFromRectDx(&rc);
			siz.cy += 1;
			MRadCtrl::ResizeSelection(NULL, siz.cx, siz.cy);
		}
		else
		{
			// Down
			MRadCtrl::MoveSelection(NULL, 0, +1);
		}
		break;

	case VK_LEFT:
		if (pCtrl == NULL)
		{
			return;
		}
		if (GetKeyState(VK_SHIFT) < 0)
		{
			// Shift+Left
			GetWindowRect(*pCtrl, &rc);
			MapWindowRect(NULL, m_rad_dialog, &rc);
			SIZE siz = SizeFromRectDx(&rc);
			siz.cx -= 1;
			MRadCtrl::ResizeSelection(NULL, siz.cx, siz.cy);
		}
		else
		{
			// Left
			MRadCtrl::MoveSelection(NULL, -1, 0);
		}
		break;

	case VK_RIGHT:
		if (pCtrl == NULL)
		{
			return;
		}
		if (GetKeyState(VK_SHIFT) < 0)
		{
			// Shift+Right
			GetWindowRect(*pCtrl, &rc);
			MapWindowRect(NULL, m_rad_dialog, &rc);
			SIZE siz = SizeFromRectDx(&rc);
			siz.cx += 1;
			MRadCtrl::ResizeSelection(NULL, siz.cx, siz.cy);
		}
		else
		{
			// Right
			MRadCtrl::MoveSelection(NULL, +1, 0);
		}
		break;

	case VK_DELETE: // Del
		MRadCtrl::DeleteSelection();
		break;

	case 'A':
		if (GetAsyncKeyState(VK_CONTROL) < 0)
		{
			// Ctrl+A
			SelectAll(hwnd);
		}
		break;

	case 'C':
		if (GetAsyncKeyState(VK_CONTROL) < 0)
		{
			// Ctrl+C
			SendMessageDx(WM_COMMAND, ID_COPY);
		}
		break;

	case 'D':
		if (GetAsyncKeyState(VK_CONTROL) < 0)
		{
			// Ctrl+D
			SendMessageDx(WM_COMMAND, ID_SHOWINDEXLABELS);
		}
		break;

	case 'V':
		if (GetAsyncKeyState(VK_CONTROL) < 0)
		{
			// Ctrl+V
			SendMessageDx(WM_COMMAND, ID_PASTE);
		}
		break;

	case 'X':
		if (GetAsyncKeyState(VK_CONTROL) < 0)
		{
			// Ctrl+X
			SendMessageDx(WM_COMMAND, ID_CUT);
		}
		break;

	case 'L':
		if (pCtrl)
			pCtrl->DoTest();
		break;

	default:
		return;
	}
}

// select all the RADical controls
void MRadWindow::SelectAll(HWND hwnd)
{
	MRadCtrl::DeselectSelection();

	// select all
	for (HWND hwndNext = MRadDialog::GetFirstCtrl(hwnd);
		 hwndNext;
		 hwndNext = m_rad_dialog.GetNextCtrl(hwndNext))
	{
		MRadCtrl::Select(hwndNext);
	}
}

// MRadWindow WM_CONTEXTMENU
void MRadWindow::OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
	// show the popup menu
	PopupMenuDx(hwnd, hwndContext, IDR_POPUPMENUS, 1, xPos, yPos);
}

// get the dialog base units
BOOL MRadWindow::GetBaseUnits(INT& xDialogBaseUnit, INT& yDialogBaseUnit)
{
	// use m_dialog_res.GetBaseUnits
	m_xDialogBaseUnit = m_dialog_res.GetBaseUnits(m_yDialogBaseUnit);
	if (m_xDialogBaseUnit == 0)
	{
		return FALSE;
	}

	// update
	xDialogBaseUnit = m_xDialogBaseUnit;
	yDialogBaseUnit = m_yDialogBaseUnit;
	m_rad_dialog.m_xDialogBaseUnit = m_xDialogBaseUnit;
	m_rad_dialog.m_yDialogBaseUnit = m_yDialogBaseUnit;

	return TRUE;
}

// MRadWindow WM_MOVE
void MRadWindow::OnMove(HWND hwnd, int x, int y)
{
	RECT rc;
	GetWindowRect(hwnd, &rc);

	// remember the position
	g_settings.nRadLeft = rc.left;
	g_settings.nRadTop = rc.top;
}

// MRadWindow WM_SIZE
void MRadWindow::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	if (m_rad_dialog.m_bMovingSizing)
		return;     // in locking

	// get the client rectangle of MRadWindow
	RECT rc;
	GetClientRect(m_hwnd, &rc);
	SIZE siz = SizeFromRectDx(&rc);

	// resize m_rad_dialog
	m_rad_dialog.m_bMovingSizing = TRUE;
	MoveWindow(m_rad_dialog, 0, 0, siz.cx, siz.cy, TRUE);
	m_rad_dialog.m_bMovingSizing = FALSE;

	// get the client rectangle of MRadDialog
	GetClientRect(m_rad_dialog, &rc);
	siz = SizeFromRectDx(&rc);

	// Change m_dialog_res.m_siz
	ClientToDialog(&siz);
	m_dialog_res.m_siz = siz;

	// update the resource
	UpdateRes();
}

// fit the coordinates to the grid
void MRadWindow::FitToGrid(POINT *ppt)
{
	ppt->x = (ppt->x + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
	ppt->y = (ppt->y + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
}
void MRadWindow::FitToGrid(SIZE *psiz)
{
	psiz->cx = (psiz->cx + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
	psiz->cy = (psiz->cy + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
}
void MRadWindow::FitToGrid(RECT *prc)
{
	prc->left = (prc->left + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
	prc->top = (prc->top + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
	prc->right = (prc->right + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
	prc->bottom = (prc->bottom + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
}
void MRadWindow::OnFitToGrid(HWND hwnd)
{
	for (auto& target : MRadCtrl::GetTargets())
	{
		// ignore if target was m_rad_dialog
		if (target == m_rad_dialog)
			continue;

		// get the target control
		auto pCtrl = MRadCtrl::GetRadCtrl(target);
		if (pCtrl == NULL)
			continue;

		// check the index
		if (pCtrl->m_nIndex < 0 || m_dialog_res.m_cItems <= pCtrl->m_nIndex)
			continue;

		// get the dialog item
		DialogItem& item = m_dialog_res[pCtrl->m_nIndex];
		FitToGrid(&item.m_pt);
		FitToGrid(&item.m_siz);

		// get the position and size in client coordinates
		POINT pt = item.m_pt;
		SIZE siz = item.m_siz;
		MTRACEA("PTSIZ: %d, %d, %d, %d\n", pt.x, pt.y, siz.cx, siz.cy);
		DialogToClient(&pt);
		DialogToClient(&siz);

		// move and resize the control
		pCtrl->m_bLocking = TRUE;
		pCtrl->SetWindowPosDx(&pt, &siz);
		auto pBand = pCtrl->GetRubberBand();
		if (pBand)
		{
			pBand->FitToTarget();
		}
		pCtrl->m_bLocking = FALSE;

		// report moving/resizing to the owner window
		HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
		DoSendMessage(hwndOwner, MYWM_MOVESIZEREPORT,
			MAKEWPARAM(item.m_pt.x, item.m_pt.y),
			MAKELPARAM(item.m_siz.cx, item.m_siz.cy));
	}

	// update the resource
	UpdateRes();
}
