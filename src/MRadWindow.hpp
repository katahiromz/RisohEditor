// MRadWindow --- RADical Development Window
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MRADWINDOW_HPP_
#define MZC4_MRADWINDOW_HPP_    5   /* Version 5 */

#include "MWindowBase.hpp"
#include "MRubberBand.hpp"
#include "DialogRes.hpp"
#include "MDlgPropDlg.hpp"

class MSubclassedControl;
class MRadDialog;
class MRadWindow;

//////////////////////////////////////////////////////////////////////////////

class MSubclassedControl : public MWindowBase
{
public:
    HWND    m_hwndDialog;
    HWND    m_hwndRubberBand;

    MSubclassedControl(HWND hwndDialog, HWND hwndRubberBand)
        : m_hwndDialog(hwndDialog), m_hwndRubberBand(hwndRubberBand)
    {
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
            HANDLE_MSG(hwnd, WM_NCDESTROY, OnNCDestroy);
            HANDLE_MSG(hwnd, WM_MOVE, OnMove);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
            HANDLE_MSG(hwnd, WM_SETCURSOR, OnSetCursor);
            //HANDLE_MSG(hwnd, WM_MOUSEACTIVATE, OnMouseActivate);
            HANDLE_MSG(hwnd, WM_NCHITTEST, OnNCHitTest);
            HANDLE_MSG(hwnd, WM_NCLBUTTONDOWN, OnNCLButtonDown);
            HANDLE_MSG(hwnd, WM_NCMOUSEMOVE, OnNCMouseMove);
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

    BOOL SendMouseMsgToParent(HWND hwnd, UINT uMsg, WPARAM wParam, BOOL ScreenCoord)
    {
        HWND hwndParent = GetParent(hwnd);
        if (hwndParent)
        {
            POINT pt;
            GetCursorPos(&pt);
            if (!ScreenCoord)
                ScreenToClient(hwnd, &pt);
            SendMessage(hwndParent, uMsg, wParam, MAKELPARAM(pt.x, pt.y));
            return TRUE;
        }
        return FALSE;
    }

    void OnMove(HWND hwnd, int x, int y)
    {
        InvalidateRect(hwnd, NULL, TRUE);
        SendMessage(m_hwndRubberBand, WM_COMMAND, 666, (LPARAM)m_hwnd);
        DefaultProcDx();
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        InvalidateRect(hwnd, NULL, TRUE);
        SendMessage(m_hwndRubberBand, WM_COMMAND, 666, (LPARAM)m_hwnd);
        DefaultProcDx();
    }

    BOOL OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;
    }

    //int OnMouseActivate(HWND hwnd, HWND hwndTopLevel, UINT codeHitTest, UINT msg)
    //{
    //    return MA_NOACTIVATE;
    //}

    UINT OnNCHitTest(HWND hwnd, int x, int y)
    {
        HWND hwndParent = GetParent(hwnd);
        if (GetClassWord(hwndParent, GCW_ATOM) != 0x8002)
        {
            return HTTRANSPARENT;
        }
        else
        {
            return HTCAPTION;
        }
    }

    void OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
    {
        DefaultProcDx(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(x, y));
    }

    void OnNCMouseMove(HWND hwnd, int x, int y, UINT codeHitTest)
    {
        DefaultProcDx(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, MAKELPARAM(x, y));
    }
};

//////////////////////////////////////////////////////////////////////////////

class MRadDialog : public MDialogBase
{
public:
    MRadDialog()
    {
    }

    INT GetCtrlIndex(HWND hwndTargetCtrl) const
    {
        INT index = 0;
        for (HWND hwndCtrl = ::GetTopWindow(m_hwnd); hwndCtrl;
             hwndCtrl = ::GetWindow(hwndCtrl, GW_HWNDNEXT))
        {
            if (hwndCtrl == hwndTargetCtrl)
                return index;
            ++index;
        }
        return -1;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
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

    void DoSubclass(HWND hCtrl, HWND hwndRubberBand)
    {
        if (hCtrl == hwndRubberBand)
            return;

        MSubclassedControl *pCtrl = new MSubclassedControl(m_hwnd, hwndRubberBand);
        pCtrl->SubclassDx(hCtrl);

        DoSubclassChildren(hCtrl, hwndRubberBand);
    }

    void DoSubclassChildren(HWND hwnd, HWND hwndRubberBand)
    {
        HWND hCtrl = GetTopWindow(hwnd);
        while (hCtrl)
        {
            DoSubclass(hCtrl, hwndRubberBand);
            hCtrl = GetWindow(hCtrl, GW_HWNDNEXT);
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        POINT pt = { 0, 0 };
        SetWindowPosDx(&pt);
        return FALSE;
    }
};

//////////////////////////////////////////////////////////////////////////////

class MRadWindow : public MWindowBase
{
protected:
    HHOOK           m_mouse_hook;
    MRubberBand     m_rubber_band;
public:
    INT             m_xDialogBaseUnit;
    INT             m_yDialogBaseUnit;
    MRadDialog      m_rad_dialog;
    DialogRes       m_dialog_res;

    static MRadWindow*& Singleton()
    {
        // FIXME: support multiple MRadWindow
        static MRadWindow *p_rad_window;
        return p_rad_window;
    }

    MRadWindow() : m_mouse_hook(NULL),
                   m_xDialogBaseUnit(0), m_yDialogBaseUnit(0)
    {
    }

    ~MRadWindow()
    {
        UnhookMouse();
    }

    INT GetCtrlIndex(HWND hwndTargetCtrl) const
    {
        return m_rad_dialog.GetCtrlIndex(hwndTargetCtrl);
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

    void SetTarget(HWND hwndTarget)
    {
        m_rubber_band.SetTarget(hwndTarget);
    }

    static LRESULT CALLBACK
    MouseProc(INT nCode, WPARAM wParam, LPARAM lParam)
    {
        if (Singleton() == NULL)
            return 0;
        if (nCode < 0)
            return CallNextHookEx(Singleton()->m_mouse_hook, nCode, wParam, lParam);

        MOUSEHOOKSTRUCT *pmhs = (MOUSEHOOKSTRUCT *)lParam;
        if (wParam == WM_NCLBUTTONDOWN || wParam == WM_RBUTTONDOWN)
        {
            RECT rc;
            GetWindowRect(Singleton()->m_rad_dialog, &rc);
            POINT pt = pmhs->pt;
            if (PtInRect(&rc, pt))
            {
                HWND hwnd = WindowFromPoint(pt);
                if (hwnd != Singleton()->m_rubber_band.m_hwnd)
                {
                    ScreenToClient(Singleton()->m_rad_dialog, &pt);
                    hwnd = ChildWindowFromPointEx(Singleton()->m_rad_dialog, pt, CWP_ALL);
                    if (wParam == WM_RBUTTONDOWN)
                    {
                        PostMessage(Singleton()->m_hwnd, WM_CONTEXTMENU, (WPARAM)hwnd,
                                    MAKELPARAM(pmhs->pt.x, pmhs->pt.y));
                        return TRUE;
                    }
                    if (hwnd)
                    {
                        HWND hwndCtrl = GetPrimaryControl(hwnd, Singleton()->m_rad_dialog);
                        Singleton()->SetTarget(hwndCtrl);
                    }
                }
            }
            else
            {
                Singleton()->SetTarget(NULL);
            }
        }

        return CallNextHookEx(Singleton()->m_mouse_hook, nCode, wParam, lParam);
    }

    BOOL HookMouse(HWND hwnd)
    {
        if (m_mouse_hook == NULL)
        {
            Singleton() = this;
            DWORD dwThreadID = GetCurrentThreadId();
            m_mouse_hook = SetWindowsHookEx(WH_MOUSE, MouseProc, NULL, dwThreadID);
        }
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

    BOOL ReCreate(HWND hwnd)
    {
        UnhookMouse();

        if (m_rad_dialog)
            DestroyWindow(m_rad_dialog);
        if (m_rubber_band)
            DestroyWindow(m_rubber_band);

        m_dialog_res.Fixup(FALSE);
        std::vector<BYTE> data = m_dialog_res.data();
        m_dialog_res.Fixup(TRUE);

        if (!m_rad_dialog.CreateDialogIndirectDx(hwnd, &data[0]))
        {
            return FALSE;
        }

        if (!m_rubber_band.CreateDx(m_hwnd, FALSE))
        {
            return FALSE;
        }

        m_rad_dialog.SubclassDx(m_rad_dialog);
        m_rad_dialog.DoSubclassChildren(m_rad_dialog, m_rubber_band);

        if (!HookMouse(hwnd))
        {
            return FALSE;
        }

        FitToRadDialog();

        ShowWindow(m_rad_dialog, SW_SHOWNORMAL);
        UpdateWindow(m_rad_dialog);
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        return ReCreate(hwnd);
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
            HANDLE_MSG(hwnd, WM_INITMENUPOPUP, OnInitMenuPopup);
            case WM_EXITSIZEMOVE:
                SetTarget(m_rubber_band.m_target);
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
                    SetTarget(hwndPrev);
                }
                else
                {
                    hwndNext = GetWindow(target, GW_HWNDNEXT);
                    if (hwndNext == NULL)
                    {
                        hwndNext = GetWindow(target, GW_HWNDFIRST);
                    }
                    SetTarget(hwndNext);
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

    void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
    {
        if (m_rubber_band.m_target)
        {
            EnableMenuItem(hMenu, ID_CTRLPROP, MF_ENABLED);
            EnableMenuItem(hMenu, ID_DELCTRL, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_CTRLPROP, MF_GRAYED);
            EnableMenuItem(hMenu, ID_DELCTRL, MF_GRAYED);
        }
    }

    void OnAddCtrl(HWND hwnd)
    {
        // FIXME
    }

    void OnDelCtrl(HWND hwnd)
    {
        HWND hCtrl = m_rubber_band.m_target;
        if (hCtrl == NULL)
            return;

        UINT nIndex = GetCtrlIndex(hCtrl);
        if (nIndex < m_dialog_res.Items.size())
        {
            m_dialog_res.Items.erase(m_dialog_res.Items.begin() + nIndex);
            m_dialog_res.m_cItems--;
            ReCreate(hwnd);
        }
    }

    void OnCtrlProp(HWND hwnd)
    {
        // FIXME
    }

    void OnDlgProp(HWND hwnd)
    {
        MDlgPropDlg dialog(m_dialog_res);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            // FIXME
        }
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        UnhookMouse();
        BOOL IsVisible = IsWindowVisible(m_rubber_band);
        ShowWindow(m_rubber_band, SW_HIDE);

        switch (id)
        {
        case ID_ADDCTRL:
            OnAddCtrl(hwnd);
            break;
        case ID_DELCTRL:
            OnDelCtrl(hwnd);
            break;
        case ID_CTRLPROP:
            OnCtrlProp(hwnd);
            break;
        case ID_DLGPROP:
            OnDlgProp(hwnd);
            break;
        }

        if (IsVisible && ::IsWindow(m_rubber_band))
            ShowWindow(m_rubber_band, SW_SHOWNOACTIVATE);
        HookMouse(hwnd);
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        UnhookMouse();

        POINT pt;
        pt.x = xPos;
        pt.y = yPos;
        ScreenToClient(m_rad_dialog, &pt);
        HWND hCtrl = ChildWindowFromPointEx(m_rad_dialog, pt, CWP_ALL);
        hCtrl = GetPrimaryControl(hCtrl, m_rad_dialog);
        if (hCtrl != m_rad_dialog.m_hwnd && hCtrl != m_hwnd)
        {
            SetTarget(hCtrl);
        }
        else
        {
            SetTarget(NULL);
        }

        HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(3));
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
 
        LPRECT prc = NULL;
        RECT rc;
        if (m_rubber_band.m_target)
        {
            GetWindowRect(m_rubber_band.m_target, &rc);
            prc = &rc;
        }

        INT nCmd = TrackPopupMenu(hSubMenu,
            TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
            xPos, yPos, 0, hwnd, prc);
        PostMessage(hwnd, WM_NULL, 0, 0);
        PostMessage(hwnd, WM_COMMAND, nCmd, 0);
        HookMouse(hwnd);
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
