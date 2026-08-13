// MDropdownArrow.hpp --- Language drop-down list and arrow
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "Utils.h"

#define MYWM_SETITEMRECT (WM_USER + 100)
#define MYWM_COMPLEMENT (WM_USER + 112)
#define MYWM_CLOSELIST (WM_USER + 103)
#define MYWM_AUTOCOMPLETE (WM_USER + 116)
#define MYWM_AUTOCOMPLETEDONE (WM_USER + 117)

enum ARROW_TARGET_TYPE
{
	TARGET_TYPE_TYPE,
	TARGET_TYPE_NAME,
	TARGET_TYPE_LANG,
};

class MDropdownListDlg : public MDialogBase
{
public:
	HWND m_lst1;
	HWND m_arrow;
	ARROW_TARGET_TYPE m_target_type = TARGET_TYPE_LANG;
	enum { TIMER_ID = 999 };

	MDropdownListDlg()
		: MDialogBase(IDD_DROPDOWNPOPUP)
		, m_lst1(NULL)
		, m_arrow(NULL)
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		ShowWindow(hwnd, SW_HIDE);
		m_lst1 = GetDlgItem(hwnd, lst1);
		InitList(hwnd);
		return TRUE;
	}

	void SetTargetType(ARROW_TARGET_TYPE target_type)
	{
		if (m_target_type == target_type)
			return;
		// Only manipulate the list box when the dialog is active and m_lst1 is valid.
		// Using a stale (destroyed) m_lst1 handle is dangerous: Windows may recycle the
		// handle value for an unrelated window, causing SendMessage to block for seconds.
		if (m_lst1 != NULL)
		{
			if (target_type == TARGET_TYPE_TYPE)
				InitTypeListBox(m_lst1);
			else if (target_type == TARGET_TYPE_NAME)
				InitNameListBox(m_lst1);
			else if (target_type == TARGET_TYPE_LANG)
				InitLangListBox(m_lst1);
		}
		m_target_type = target_type;
	}

	void InitList(HWND hwnd)
	{
		if (m_target_type == TARGET_TYPE_TYPE)
			InitTypeListBox(m_lst1);
		else if (m_target_type == TARGET_TYPE_NAME)
			InitNameListBox(m_lst1);
		else if (m_target_type == TARGET_TYPE_LANG)
			InitLangListBox(m_lst1);
		else
			return;

		INT nCount = ListBox_GetCount(m_lst1);

		RECT rc;
		ListBox_GetItemRect(m_lst1, 0, &rc);
		LONG cy = rc.bottom - rc.top;

		if (nCount > 10)
			nCount = 10;

		GetClientRect(hwnd, &rc);
		rc.bottom = rc.top + nCount * cy;
		DWORD style = GetWindowStyle(hwnd);
		DWORD exstyle = GetWindowExStyle(hwnd);
		AdjustWindowRectEx(&rc, style, FALSE, exstyle);

		style = GetWindowStyle(m_lst1);
		exstyle = GetWindowExStyle(m_lst1);
		AdjustWindowRectEx(&rc, style, FALSE, exstyle);

		MoveWindow(hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

		SetTimer(hwnd, TIMER_ID, 250, NULL);
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case lst1:
			if (codeNotify == LBN_DBLCLK)
			{
				PostMessage(m_arrow, MYWM_COMPLEMENT, VK_RETURN, 0);
			}
			break;
		}
	}

	void OnSize(HWND hwnd, UINT state, int cx, int cy)
	{
		MoveWindow(GetDlgItem(hwnd, lst1), 0, 0, cx, cy, TRUE);
	}

	void OnTimer(HWND hwnd, UINT id)
	{
		if (id != TIMER_ID)
			return;

		HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
		HWND hwndFocus = GetFocus();
		if (m_arrow != hwndFocus && hwnd != hwndFocus &&
			hwndOwner != hwndFocus && m_lst1 != hwndFocus)
		{
			KillTimer(hwnd, TIMER_ID);
			PostMessage(m_arrow, MYWM_CLOSELIST, 0, 0);
		}
	}

	int OnVKeyToItem(HWND hwnd, UINT vk, HWND hwndListbox, int iCaret)
	{
		switch (vk)
		{
		case VK_DOWN:
		case VK_UP:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_HOME:
		case VK_END:
			return -1;
		case VK_RETURN:
			PostMessage(m_arrow, MYWM_COMPLEMENT, VK_RETURN, 0);
			return -2;
		}
		return 0;
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		HANDLE_MSG(hwnd, WM_SIZE, OnSize);
		HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
		HANDLE_MSG(hwnd, WM_VKEYTOITEM, OnVKeyToItem);
		default:
			return 0;
		}
	}
};

class MDropdownArrow : public MWindowBase
{
public:
	BOOL m_bDown = FALSE;
	RECT m_rcItem = {};
	MDropdownListDlg m_dialog;
	HWND m_hwndMain = nullptr;
	ARROW_TARGET_TYPE m_target_type = TARGET_TYPE_LANG;
	MIdOrString m_type;
	MIdOrString m_name;
	LANGID m_wLangId = 0;

	LPCTSTR GetWndClassNameDx() const override
	{
		return TEXT("MZC4 Dropdown Arrow");
	}

	BOOL ChooseType()
	{
		m_target_type = TARGET_TYPE_TYPE;
		m_dialog.SetTargetType(TARGET_TYPE_TYPE);
		m_type = BAD_TYPE;
		m_name = BAD_NAME;
		return TRUE;
	}

	BOOL ChooseName(const MIdOrString& type, const MIdOrString& name)
	{
		m_target_type = TARGET_TYPE_NAME;
		m_dialog.SetTargetType(TARGET_TYPE_NAME);
		m_type = type;
		m_name = name;
		return TRUE;
	}

	BOOL ChooseLang(LANGID wLangId)
	{
		m_target_type = TARGET_TYPE_LANG;
		m_dialog.SetTargetType(TARGET_TYPE_LANG);
		m_wLangId = wLangId;
		return TRUE;
	}

	BOOL DoComplement(HWND hwnd, WPARAM wParam)
	{
		INT nIndex = ListBox_GetCurSel(m_dialog.m_lst1);
		switch (wParam)
		{
		case VK_RETURN:
			PostMessageW(m_hwndMain, MYWM_COMPLEMENT, nIndex, 0);
			ShowDropDownList(hwnd, FALSE);
			return TRUE;
		case VK_ESCAPE:
			ShowDropDownList(hwnd, FALSE);
			return TRUE;
		}
		return FALSE;
	}

	LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
		HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
		HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, OnLButtonDown);
		case MYWM_SETITEMRECT:
			OnSetItemRect(hwnd, (LPRECT)lParam);
			break;
		case MYWM_COMPLEMENT:
			DoComplement(hwnd, wParam);
			break;
		case MYWM_CLOSELIST:
			ShowDropDownList(hwnd, FALSE);
			break;
		default:
			return DefaultProcDx();
		}
		return 0;
	}

	void ShowDropDownList(HWND hwnd, BOOL bShow)
	{
		if (IsWindow(m_dialog))
		{
			DestroyWindow(m_dialog);
			// Clear stale handles so SetTargetType cannot accidentally send messages
			// to a recycled (foreign) window and block the UI thread for seconds.
			m_dialog.m_lst1 = NULL;
			m_dialog.m_arrow = NULL;
		}

		if (bShow)
		{
			assert(IsWindow(hwnd));
			m_dialog.CreateDialogDx(hwnd);
			m_dialog.m_arrow = hwnd;

			RECT rc;
			GetWindowRect(m_dialog, &rc);
			LONG cy = rc.bottom - rc.top;

			RECT rcItem = m_rcItem;
			MapWindowRect(GetParent(hwnd), NULL, &rcItem);

			POINT pt = { rcItem.left, rcItem.bottom };
			HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

			MONITORINFO mi = { sizeof(mi) };
			GetMonitorInfo(hMonitor, &mi);
			RECT& rcWork = mi.rcWork;

			LONG y;
			if (pt.y + cy < rcWork.bottom)
				y = pt.y;
			else
				y = rcItem.top - cy;

			SetWindowPos(m_dialog, NULL, rcItem.left, y, 0, 0,
				SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

			switch (m_target_type)
			{
			case TARGET_TYPE_TYPE:
				ChooseTypeListBoxType(m_dialog.m_lst1, m_type);
				break;
			case TARGET_TYPE_NAME:
				ChooseNameListBoxName(m_dialog.m_lst1, m_type, m_name);
				break;
			case TARGET_TYPE_LANG:
				ChooseLangListBoxLang(m_dialog.m_lst1, m_wLangId);
				break;
			}
		}

		m_bDown = bShow;

		if (hwnd)
			InvalidateRect(hwnd, NULL, TRUE);
	}

	SIZE GetArrowSize(LPCRECT prc) const
	{
		SIZE ret;
		ret.cx = prc->bottom - prc->top;
		ret.cy = prc->bottom - prc->top;
		return ret;
	}

protected:
	BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
	{
		m_bDown = FALSE;
		m_hwndMain = NULL;
		return TRUE;
	}

	void OnSetItemRect(HWND hwnd, LPCRECT prc)
	{
		m_rcItem = *prc;
		SIZE siz = GetArrowSize(prc);
		SetWindowPos(hwnd, NULL, 0, 0, siz.cx, siz.cy,
			SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	}

	void OnPaint(HWND hwnd)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);

		PAINTSTRUCT ps;
		if (HDC hdc = BeginPaint(hwnd, &ps))
		{
			if (m_bDown)
				DrawFrameControl(hdc, &rc, DFC_SCROLL, DFCS_SCROLLCOMBOBOX | DFCS_PUSHED);
			else
				DrawFrameControl(hdc, &rc, DFC_SCROLL, DFCS_SCROLLCOMBOBOX);

			EndPaint(hwnd, &ps);
		}
	}

	void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
	{
		m_bDown = !m_bDown;
		InvalidateRect(hwnd, NULL, TRUE);

		SetFocus(GetParent(hwnd));
		ShowDropDownList(hwnd, m_bDown);
	}
};
