// MDropdownArrow.hpp --- Language drop-down list and arrow
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "Common.hpp"

#define MYWM_SETITEMRECT (WM_USER + 100)
#define MYWM_COMPLEMENT (WM_USER + 112)
#define MYWM_CLOSELIST (WM_USER + 103)

class MDropdownListDlg : public MDialogBase
{
public:
    HWND m_lst1;
    HWND m_arrow;

    MDropdownListDlg() : MDialogBase(IDD_DROPDOWNPOPUP)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        ShowWindow(hwnd, SW_HIDE);
        m_lst1 = GetDlgItem(hwnd, lst1);
        InitList(hwnd);
        return TRUE;
    }

    void InitList(HWND hwnd)
    {
        InitLangListBox(m_lst1);

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

        SetTimer(hwnd, 999, 250, NULL);
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
        HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
        HWND hwndFocus = GetFocus();
        if (m_arrow != hwndFocus && hwnd != hwndFocus &&
            hwndOwner != hwndFocus && m_lst1 != hwndFocus)
        {
            KillTimer(hwnd, 999);
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

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
        HANDLE_MSG(hwnd, WM_VKEYTOITEM, OnVKeyToItem);
        default:
            return DefaultProcDx();
        }
    }
};

class MDropdownArrow : public MWindowBase
{
public:
    BOOL m_bDown;
    RECT m_rcItem;
    MDropdownListDlg m_dialog;
    HWND m_hwndMain;
    LANGID m_wLangId;

    MDropdownArrow() : m_wLangId(0)
    {
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("MZC4 Dropdown Arrow");
    }

    BOOL ChooseLang(LANGID wLangId)
    {
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

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

            ChooseLangListBoxLang(m_dialog.m_lst1, m_wLangId);
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
