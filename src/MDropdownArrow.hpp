#pragma once

#include "MWindowBase.hpp"
#include "resource.h"

#define MYWM_SETITEMRECT (WM_USER + 100)
#define MYWM_COMPLEMENT (WM_USER + 112)
#define MYWM_CLOSELIST (WM_USER + 103)

BOOL InitLangListBox(HWND hwnd);

class MDropdownListDlg : public MDialogBase
{
public:
    HWND m_lst1;
    HWND m_arrow;
    WNDPROC m_fnListOldWndProc;

    static LRESULT CALLBACK
    ListWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result;
        MDropdownListDlg *parent = (MDropdownListDlg *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        WNDPROC fn = (WNDPROC)parent->m_fnListOldWndProc;
        switch (uMsg)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            result = SendMessage(hwnd, LB_ITEMFROMPOINT, 0, lParam);
            if (0 && HIWORD(result) == 0)
            {
                if (uMsg == WM_LBUTTONDOWN)
                {
                    ListBox_SetCurSel(hwnd, LOWORD(result));
                    PostMessage(parent->m_arrow, MYWM_COMPLEMENT, VK_RETURN, 0);
                }
                else if (uMsg == WM_LBUTTONDBLCLK)
                {
                    ListBox_SetCurSel(hwnd, LOWORD(result));
                    PostMessage(parent->m_arrow, MYWM_COMPLEMENT, VK_RETURN, 0);
                }
            }
            break;
        }
        return CallWindowProc(fn, hwnd, uMsg, wParam, lParam);
    }

    MDropdownListDlg() : MDialogBase(IDD_DROPDOWNPOPUP)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        ShowWindow(hwnd, SW_HIDE);

        m_lst1 = GetDlgItem(hwnd, lst1);
        m_fnListOldWndProc = SubclassWindow(m_lst1, ListWindowProc);
        SetWindowLongPtr(m_lst1, GWLP_USERDATA, (LONG_PTR)this);

        InitList(hwnd);
        return TRUE;
    }

    void InitList(HWND hwnd)
    {
        InitLangListBox(GetDlgItem(hwnd, lst1));

        INT nCount = (INT)SendDlgItemMessageW(hwnd, lst1, LB_GETCOUNT, 0, 0);

        RECT rc;
        SendDlgItemMessageW(hwnd, lst1, LB_GETITEMRECT, 0, (LPARAM)&rc);
        LONG cy = rc.bottom - rc.top;

        if (nCount > 10)
            nCount = 10;

        GetClientRect(hwnd, &rc);
        rc.bottom = rc.top + nCount * cy;
        DWORD style = GetWindowStyle(hwnd);
        DWORD exstyle = GetWindowExStyle(hwnd);
        AdjustWindowRectEx(&rc, style, FALSE, exstyle);

        HWND hList = GetDlgItem(hwnd, lst1);
        style = GetWindowStyle(hList);
        exstyle = GetWindowExStyle(hList);
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

    MDropdownArrow()
    {
    }

    virtual ~MDropdownArrow()
    {
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("MZC4 Dropdown Arrow");
    }

    virtual VOID ModifyWndClassDx(WNDCLASSEX& wcx)
    {
    }

    BOOL GetIndex(INT& nIndex) const
    {
        INT index = (INT)SendMessage(m_dialog.m_lst1, LB_GETCURSEL, 0, 0);
        if (index >= 0)
        {
            nIndex = index;
            return TRUE;
        }
        return FALSE;
    }

    BOOL DoComplement(HWND hwnd, WPARAM wParam)
    {
        INT nIndex = (INT)SendMessage(m_dialog.m_lst1, LB_GETCURSEL, 0, 0);
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
        }

        m_bDown = bShow;
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
