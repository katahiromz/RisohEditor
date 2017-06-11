// MRadWindow --- RADical Development Window
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MRADWINDOW_HPP_
#define MZC4_MRADWINDOW_HPP_    5   /* Version 5 */

#include "MWindowBase.hpp"
#include "MRubberBand.hpp"
#include "DialogRes.hpp"

//////////////////////////////////////////////////////////////////////////////

struct MSubclassedControl : MWindowBase
{
    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
            HANDLE_MSG(hwnd, WM_NCDESTROY, OnNCDestroy);
        }
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        if (fDown)
        {
            FORWARD_WM_KEYDOWN(GetParent(hwnd), vk, cRepeat, flags, SendMessage);
        }
    }

    void OnNCDestroy(HWND hwnd)
    {
        delete this;
    }
};

//////////////////////////////////////////////////////////////////////////////

struct MRadDialog : MDialogBase
{
    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
        }
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
            HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
        }
        return CallWindowProcDx(hwnd, uMsg, wParam, lParam);
    }

    void OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
    {
    }

    void OnNCLButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
    {
    }

    void OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
    {
    }

    void OnNCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
    {
        FORWARD_WM_CONTEXTMENU(GetParent(hwnd), hwnd, x, y, SendMessage);
    }

    void OnNCMouseMove(HWND hwnd, int x, int y, UINT codeHitTest)
    {
    }

    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        if (fDown)
        {
            FORWARD_WM_KEYDOWN(GetParent(hwnd), vk, cRepeat, flags, SendMessage);
        }
    }

    void DoSubclass(HWND hCtrl)
    {
        MSubclassedControl *pCtrl = new MSubclassedControl;
        pCtrl->SubclassDx(hCtrl);

        DoSubclassChildren(hCtrl);
    }

    void DoSubclassChildren(HWND hwnd)
    {
        HWND hCtrl = GetTopWindow(hwnd);
        while (hCtrl)
        {
            DoSubclass(hCtrl);
            hCtrl = GetWindow(hCtrl, GW_HWNDNEXT);
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        POINT pt = { 0, 0 };
        SetWindowPosDx(&pt);
        SubclassDx(hwnd);

        DoSubclassChildren(hwnd);
        return FALSE;
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        FORWARD_WM_CONTEXTMENU(GetParent(hwnd), hwndContext, xPos, yPos, SendMessage);
    }
};

//////////////////////////////////////////////////////////////////////////////

struct MRadWindow : MWindowBase
{
    INT         m_xDialogBaseUnit;
    INT         m_yDialogBaseUnit;
    HHOOK       m_mouse_hook;
    MRadDialog   m_rad_dialog;
    RubberBand  m_rubber_band;
    DialogRes   m_dialog_res;
    static MRadWindow *s_p_rad_window;

    MRadWindow() : m_xDialogBaseUnit(0), m_yDialogBaseUnit(0),
                  m_mouse_hook(NULL)
    {
    }

    ~MRadWindow()
    {
        UnhookMouse();
    }

    static HWND GetPrimaryControl(HWND hwnd, HWND hwndDialog)
    {
        for (;;)
        {
            if (GetParent(hwnd) == NULL || GetParent(hwnd) == hwndDialog)
                return hwnd;

            hwnd = GetParent(hwnd);
        }
    }

    static LRESULT CALLBACK
    MouseProc(INT nCode, WPARAM wParam, LPARAM lParam)
    {
        if (s_p_rad_window == NULL)
            return 0;
        if (nCode < 0)
            return CallNextHookEx(s_p_rad_window->m_mouse_hook, nCode, wParam, lParam);

        MOUSEHOOKSTRUCT *pmhs = (MOUSEHOOKSTRUCT *)lParam;
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN)
        {
            RECT rc;
            GetWindowRect(s_p_rad_window->m_rad_dialog, &rc);
            POINT pt = pmhs->pt;
            if (PtInRect(&rc, pt))
            {
                ScreenToClient(s_p_rad_window->m_rad_dialog, &pt);
                BOOL IsVisible = IsWindowVisible(s_p_rad_window->m_rubber_band);
                DestroyWindow(s_p_rad_window->m_rubber_band);
                HWND hwnd = ChildWindowFromPointEx(s_p_rad_window->m_rad_dialog, pt, CWP_ALL);
                s_p_rad_window->m_rubber_band.CreateDx(s_p_rad_window->m_rad_dialog, FALSE);
                if (wParam == WM_RBUTTONDOWN)
                {
                    PostMessage(s_p_rad_window->m_rad_dialog, WM_CONTEXTMENU,
                        (WPARAM)hwnd, MAKELPARAM(pmhs->pt.x, pmhs->pt.y));
                    if (IsVisible)
                    {
                        ShowWindow(s_p_rad_window->m_rubber_band, SW_SHOWNOACTIVATE);
                    }
                    return TRUE;
                }
                else if (wParam == WM_LBUTTONDOWN)
                {
                    if (hwnd && s_p_rad_window->m_rad_dialog.m_hwnd != hwnd)
                    {
                        hwnd = GetPrimaryControl(hwnd, s_p_rad_window->m_rad_dialog);
                        s_p_rad_window->m_rubber_band.SetTarget(hwnd);
                        s_p_rad_window->m_rubber_band.FitToTarget();
                        ShowWindow(s_p_rad_window->m_rubber_band, SW_SHOWNOACTIVATE);
                        return TRUE;
                    }
                }
            }
        }

        return CallNextHookEx(s_p_rad_window->m_mouse_hook, nCode, wParam, lParam);
    }

    BOOL HookMouse(HWND hwnd)
    {
        s_p_rad_window = this;
        DWORD dwThreadID = GetCurrentThreadId();
        m_mouse_hook = SetWindowsHookEx(WH_MOUSE, MouseProc, NULL, dwThreadID);
        return (m_mouse_hook != NULL);
    }

    void UnhookMouse()
    {
        if (m_mouse_hook)
        {
            UnhookWindowsHookEx(m_mouse_hook);
            m_mouse_hook = NULL;
        }
    }

    void FitToRadDialog()
    {
        RECT Rect;
        GetWindowRect(m_rad_dialog, &Rect);
        SIZE Size;
        Size.cx = Rect.right - Rect.left;
        Size.cy = Rect.bottom - Rect.top;

        DWORD style = GetWindowLong(m_hwnd, GWL_STYLE);
        DWORD exstyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);
        SetRect(&Rect, 0, 0, Size.cx, Size.cy);
        AdjustWindowRectEx(&Rect, style, FALSE, exstyle);
        OffsetRect(&Rect, -Rect.left, -Rect.top);

        MoveWindow(m_hwnd, 0, 0, Rect.right, Rect.bottom, TRUE);
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("katahiromz's MRadWindow Class");
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hIcon = NULL;
        wcx.hbrBackground = GetStockBrush(DKGRAY_BRUSH);
        wcx.hIconSm = NULL;
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        if (!HookMouse(hwnd))
        {
            return FALSE;
        }

        m_dialog_res.Fixup(FALSE);
        std::vector<BYTE> data = m_dialog_res.data();
        m_dialog_res.Fixup(TRUE);

        if (!m_rad_dialog.CreateDialogIndirectDx(hwnd, &data[0]))
        {
            return FALSE;
        }

        if (!m_rubber_band.CreateDx(m_rad_dialog, FALSE))
        {
            return FALSE;
        }

        FitToRadDialog();

        ShowWindow(m_rad_dialog, SW_SHOWNORMAL);
        UpdateWindow(m_rad_dialog);

        return TRUE;
    }

    void OnDestroy(HWND hwnd)
    {
        UnhookMouse();
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
            HANDLE_MSG(hwnd, WM_MOVE, OnMove);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
            HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
            HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
            case WM_EXITSIZEMOVE:
                m_rubber_band.InvalidateClient();
                InvalidateRect(m_rubber_band, NULL, FALSE);
                return 0;
        }
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        HWND hwndPrev, hwndNext;
        RECT rc;
        if (fDown)
        {
            MWindowBase& dialog = m_rad_dialog;
            MWindowBase& target = m_rubber_band.m_target;
            if (!target)
                return;

            switch (vk)
            {
            case VK_TAB:
                if (GetKeyState(VK_SHIFT) < 0)
                {
                    hwndPrev = GetWindow(target, GW_HWNDPREV);
                    if (hwndPrev == NULL)
                    {
                        hwndPrev = GetWindow(target, GW_HWNDLAST);
                    }
                    m_rubber_band.SetTarget(hwndPrev);
                }
                else
                {
                    hwndNext = GetWindow(target, GW_HWNDNEXT);
                    if (hwndNext == NULL)
                    {
                        hwndNext = GetWindow(target, GW_HWNDFIRST);
                    }
                    m_rubber_band.SetTarget(hwndNext);
                }
                break;
            case VK_UP:
                if (GetKeyState(VK_SHIFT) < 0)
                {
                    GetWindowRect(target, &rc);
                    MapWindowRect(NULL, dialog, &rc);
                    SIZE siz = SizeFromRectDx(&rc);
                    siz.cy -= 1;
                    target.SetWindowPosDx(NULL, &siz);
                }
                else
                {
                    GetWindowRect(target, &rc);
                    MapWindowRect(NULL, dialog, &rc);
                    rc.top -= 1;
                    POINT pt = { rc.left, rc.top };
                    target.SetWindowPosDx(&pt);
                }
                break;
            case VK_DOWN:
                if (GetKeyState(VK_SHIFT) < 0)
                {
                    GetWindowRect(target, &rc);
                    MapWindowRect(NULL, dialog, &rc);
                    SIZE siz = SizeFromRectDx(&rc);
                    siz.cy += 1;
                    target.SetWindowPosDx(NULL, &siz);
                }
                else
                {
                    GetWindowRect(target, &rc);
                    MapWindowRect(NULL, dialog, &rc);
                    rc.top += 1;
                    POINT pt = { rc.left, rc.top };
                    target.SetWindowPosDx(&pt);
                }
                break;
            case VK_LEFT:
                if (GetKeyState(VK_SHIFT) < 0)
                {
                    GetWindowRect(target, &rc);
                    MapWindowRect(NULL, dialog, &rc);
                    SIZE siz = SizeFromRectDx(&rc);
                    siz.cx -= 1;
                    target.SetWindowPosDx(NULL, &siz);
                }
                else
                {
                    GetWindowRect(target, &rc);
                    MapWindowRect(NULL, dialog, &rc);
                    rc.left -= 1;
                    POINT pt = { rc.left, rc.top };
                    target.SetWindowPosDx(&pt);
                }
                break;
            case VK_RIGHT:
                if (GetKeyState(VK_SHIFT) < 0)
                {
                    GetWindowRect(target, &rc);
                    MapWindowRect(NULL, dialog, &rc);
                    SIZE siz = SizeFromRectDx(&rc);
                    siz.cx += 1;
                    target.SetWindowPosDx(NULL, &siz);
                }
                else
                {
                    GetWindowRect(target, &rc);
                    MapWindowRect(NULL, dialog, &rc);
                    rc.right += 1;
                    POINT pt = { rc.left, rc.top };
                    target.SetWindowPosDx(&pt);
                }
                break;
            default:
                return;
            }
            m_rubber_band.FitToTarget();
        }
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        UnhookMouse();
        BOOL IsVisible = IsWindowVisible(m_rubber_band);
        ShowWindow(m_rubber_band, SW_HIDE);
        switch (id)
        {
        case IDM_TEST2:
            MsgBoxDx(TEXT("Test2"));
            break;
        case IDM_TEST3:
            MsgBoxDx(TEXT("Test3"));
            break;
        }
        if (IsVisible)
            ShowWindow(m_rubber_band, SW_SHOWNOACTIVATE);
        HookMouse(hwnd);
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(2));
        HMENU hSubMenu = GetSubMenu(hMenu, 0);

        m_rubber_band.InvalidateClient();
 
        LPRECT prc = NULL;
        RECT rc;
        if (m_rubber_band.m_target)
        {
            GetWindowRect(m_rubber_band.m_target, &rc);
            prc = &rc;
        }

        UnhookMouse();
        TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
            xPos, yPos, 0, hwnd, prc);
        PostMessage(hwnd, WM_NULL, 0, 0);
        HookMouse(hwnd);

        m_rubber_band.InvalidateClient();
    }

    BOOL GetBaseUnits(INT& xDialogBaseUnit, INT& yDialogBaseUnit)
    {
        if (m_xDialogBaseUnit == 0)
        {
            m_xDialogBaseUnit = m_dialog_res.GetBaseUnits(m_yDialogBaseUnit);
            if (m_xDialogBaseUnit == 0)
            {
                return FALSE;
            }
        }

        xDialogBaseUnit = m_xDialogBaseUnit;
        yDialogBaseUnit = m_yDialogBaseUnit;

        return TRUE;
    }

    virtual void Update(HWND hwnd)
    {
    }

    void OnMove(HWND hwnd, int x, int y)
    {
        m_rubber_band.FitToTarget();
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        m_dialog_res.Update();

        INT xDialogBaseUnit, yDialogBaseUnit;
        if (!GetBaseUnits(xDialogBaseUnit, yDialogBaseUnit))
            return;

        RECT Rect1;
        GetClientRect(m_hwnd, &Rect1);

        INT cxPixels = Rect1.right - Rect1.left;
        INT cyPixels = Rect1.bottom - Rect1.top;
        MoveWindow(m_rad_dialog, 0, 0, cxPixels, cyPixels, TRUE);

        RECT Rect2;
        GetClientRect(m_rad_dialog, &Rect2);

        INT cxDialog = MulDiv((Rect2.right - Rect2.left), 4, xDialogBaseUnit);
        INT cyDialog = MulDiv((Rect2.bottom - Rect2.top), 8, yDialogBaseUnit);

        m_dialog_res.m_siz.cx = cxDialog;
        m_dialog_res.m_siz.cy = cyDialog;

        cxPixels = MulDiv(cxDialog, xDialogBaseUnit, 4);
        cyPixels = MulDiv(cyDialog, yDialogBaseUnit, 8);
        SetRect(&Rect2, 0, 0, cxPixels, cyPixels);

        DWORD style = GetWindowStyle(m_rad_dialog);
        DWORD exstyle = GetWindowExStyle(m_rad_dialog);
        AdjustWindowRectEx(&Rect2, style, FALSE, exstyle);
        OffsetRect(&Rect2, -Rect2.left, -Rect2.top);
        cxPixels = Rect2.right;
        cyPixels = Rect2.bottom;

        MoveWindow(m_rad_dialog, 0, 0, cxPixels, cyPixels, TRUE);

        Update(hwnd);
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MRADWINDOW_HPP_

//////////////////////////////////////////////////////////////////////////////
