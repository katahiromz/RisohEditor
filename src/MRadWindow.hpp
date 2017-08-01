#ifndef RADWINDOW_HPP_
#define RADWINDOW_HPP_

#include "MWindowBase.hpp"
#include "MRubberBand.hpp"
#include "DialogRes.hpp"
#include <set>
#include <map>

class MRadCtrl : public MWindowBase
{
public:
    BOOL m_bTopCtrl;
    HWND m_hwndRubberBand;
    BOOL m_bMoving;
    BOOL m_bSizing;
    INT m_nIndex;
    POINT m_pt;

    MRadCtrl() : m_bTopCtrl(FALSE), m_hwndRubberBand(NULL),
                 m_bMoving(FALSE), m_bSizing(FALSE), m_nIndex(-1)
    {
        m_pt.x = m_pt.y = -1;
    }

    typedef std::set<HWND> set_type;
    static set_type& GetTargets()
    {
        static set_type s_set;
        return s_set;
    }

    static HWND& GetLastSel()
    {
        static HWND s_hwnd = NULL;
        return s_hwnd;
    }

    MRubberBand *GetRubberBand()
    {
        MWindowBase *base = GetUserData(m_hwndRubberBand);
        if (base)
        {
            MRubberBand *band = static_cast<MRubberBand *>(base);
            return band;
        }
        return NULL;
    }

    static MRadCtrl *GetRadCtrl(HWND hwnd)
    {
        MWindowBase *base = GetUserData(hwnd);
        if (base)
        {
            MRadCtrl *pCtrl;
            pCtrl = static_cast<MRadCtrl *>(base);
            return pCtrl;
        }
        return NULL;
    }

    static void DeselectSelection()
    {
        set_type::iterator it, end = GetTargets().end();
        for (it = GetTargets().begin(); it != end; ++it)
        {
            MRadCtrl *pCtrl = GetRadCtrl(*it);
            if (pCtrl)
            {
                ::DestroyWindow(pCtrl->m_hwndRubberBand);
                pCtrl->m_hwndRubberBand = NULL;
            }
        }
        GetTargets().clear();
        GetLastSel() = NULL;
    }

    static void DeleteSelection(HWND hwnd = NULL)
    {
        set_type::iterator it, end = GetTargets().end();
        for (it = GetTargets().begin(); it != end; ++it)
        {
            if (hwnd == *it)
                continue;

            MRadCtrl *pCtrl = GetRadCtrl(*it);
            if (pCtrl)
            {
                ::DestroyWindow(pCtrl->m_hwndRubberBand);
                pCtrl->m_hwndRubberBand = NULL;
                ::DestroyWindow(*pCtrl);
            }
        }
        GetTargets().clear();
        GetLastSel() = NULL;
    }

    void Deselect()
    {
        MRubberBand *band = GetRubberBand();
        if (band)
        {
            ::DestroyWindow(*band);
        }
        GetTargets().erase(m_hwnd);
        m_hwndRubberBand = NULL;
        GetLastSel() = NULL;
    }

    static void Select(HWND hwnd)
    {
        if (hwnd == NULL)
            return;

        MRadCtrl *pCtrl = GetRadCtrl(hwnd);
        if (pCtrl == NULL)
            return;

        MRubberBand *band = new MRubberBand;
        band->CreateDx(GetParent(hwnd), hwnd, TRUE);

        pCtrl->m_hwndRubberBand = *band;
        GetTargets().insert(hwnd);
        GetLastSel() = hwnd;
    }

    static void MoveSelection(HWND hwnd, INT dx, INT dy)
    {
        set_type::iterator it, end = GetTargets().end();
        for (it = GetTargets().begin(); it != end; ++it)
        {
            if (hwnd == *it)
                continue;

            MRadCtrl *pCtrl = GetRadCtrl(*it);
            if (pCtrl)
            {
                RECT rc;
                ::GetWindowRect(*pCtrl, &rc);
                ::MapWindowPoints(NULL, ::GetParent(*pCtrl), (LPPOINT)&rc, 2);
                OffsetRect(&rc, dx, dy);

                pCtrl->m_bMoving = TRUE;
                pCtrl->SetWindowPosDx((LPPOINT)&rc);
                pCtrl->m_bMoving = FALSE;
            }
        }
    }

    static void ResizeSelection(HWND hwnd, INT cx, INT cy)
    {
        set_type::iterator it, end = GetTargets().end();
        for (it = GetTargets().begin(); it != end; ++it)
        {
            if (hwnd == *it)
                continue;

            MRadCtrl *pCtrl = GetRadCtrl(*it);
            if (pCtrl && !pCtrl->m_bSizing)
            {
                pCtrl->m_bSizing = TRUE;
                SIZE siz = { cx , cy };
                pCtrl->SetWindowPosDx(NULL, &siz);
                pCtrl->m_bSizing = FALSE;

                MRubberBand *band = pCtrl->GetRubberBand();
                if (band)
                {
                    band->FitToTarget();
                }
            }
        }
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
            case WM_MOVING: case WM_SIZING:
                return 0;
        }
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    void OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
    {
        if (fDoubleClick)
            return;

        SendMessage(GetParent(hwnd), WM_NCRBUTTONDOWN, (WPARAM)hwnd, MAKELPARAM(x, y));
    }

    void OnNCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
    {
    }

    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
    }

    void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
    {
    }

    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
    }

    void OnMove(HWND hwnd, int x, int y)
    {
        if (!m_bTopCtrl)
        {
            DefaultProcDx(hwnd, WM_MOVE, 0, MAKELPARAM(x, y));
            return;
        }

        if (!m_bMoving)
        {
            POINT pt;
            ::GetCursorPos(&pt);
            MoveSelection(hwnd, pt.x - m_pt.x, pt.y - m_pt.y);
            m_pt = pt;
        }

        MRubberBand *band = GetRubberBand();
        if (band)
        {
            band->FitToTarget();
        }

        RECT rc;
        ::GetClientRect(hwnd, &rc);
        ::InvalidateRect(hwnd, &rc, TRUE);
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        DefaultProcDx(hwnd, WM_SIZE, 0, MAKELPARAM(cx, cy));
        if (!m_bTopCtrl)
        {
            return;
        }

        if (!m_bSizing)
            ResizeSelection(hwnd, cx, cy);
    }

    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        if (fDown)
        {
            FORWARD_WM_KEYDOWN(GetParent(hwnd), vk, cRepeat, flags, SendMessage);
        }
    }

    void OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
    {
        ::GetCursorPos(&m_pt);

        if (fDoubleClick)
            return;

        if (codeHitTest != HTCAPTION)
            return;

        if (GetKeyState(VK_CONTROL) < 0)
        {
            if (m_hwndRubberBand)
            {
                Deselect();
                return;
            }
        }
        else if (GetKeyState(VK_SHIFT) < 0)
        {
            if (m_hwndRubberBand)
            {
                return;
            }
        }
        else
        {
            if (m_hwndRubberBand)
            {
                ;
            }
            else
            {
                DeselectSelection();
            }
        }

        if (m_hwndRubberBand == NULL)
        {
            Select(hwnd);
        }

        HWND hwndPrev = GetWindow(hwnd, GW_HWNDPREV);
        ::DefWindowProc(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(x, y));
        SetWindowPosDx(NULL, NULL, hwndPrev);
    }

    void OnNCMouseMove(HWND hwnd, int x, int y, UINT codeHitTest)
    {
        ::DefWindowProc(hwnd, WM_NCMOUSEMOVE, codeHitTest, MAKELPARAM(x, y));
    }

    void OnNCLButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
    {
        m_bMoving = FALSE;
        m_pt.x = -1;
        m_pt.y = -1;
        ::DefWindowProc(hwnd, WM_NCLBUTTONUP, codeHitTest, MAKELPARAM(x, y));
    }

    UINT OnNCHitTest(HWND hwnd, int x, int y)
    {
        if (m_bTopCtrl)
        {
            return HTCAPTION;
        }
        return HTTRANSPARENT;
    }

    virtual void PostNcDestroy()
    {
        m_hwnd = NULL;
        delete this;
    }
};

class MRadDialog : public MDialogBase
{
public:
    HWND GetNextCtrl(HWND hwndCtrl) const
    {
        HWND hwndNext = GetNextWindow(hwndCtrl, GW_HWNDNEXT);
        if (hwndNext == NULL)
        {
            hwndNext = GetNextWindow(hwndCtrl, GW_HWNDFIRST);
        }

        TCHAR szClass[64];
        while (hwndNext)
        {
            ::GetClassName(hwndNext, szClass, _countof(szClass));
            if (lstrcmpi(szClass, MRubberBand().GetWndClassNameDx()) != 0)
                break;

            hwndNext = GetNextWindow(hwndNext, GW_HWNDNEXT);
        }
        if (hwndNext == NULL)
        {
            hwndNext = GetNextWindow(hwndCtrl, GW_HWNDFIRST);
        }

        return hwndNext;
    }

    HWND GetPrevCtrl(HWND hwndCtrl) const
    {
        HWND hwndPrev = GetNextWindow(hwndCtrl, GW_HWNDPREV);
        if (hwndPrev == NULL)
        {
            hwndPrev = GetNextWindow(hwndCtrl, GW_HWNDLAST);
        }

        TCHAR szClass[64];
        while (hwndPrev)
        {
            ::GetClassName(hwndPrev, szClass, _countof(szClass));
            if (lstrcmpi(szClass, MRubberBand().GetWndClassNameDx()) != 0)
                break;

            hwndPrev = GetNextWindow(hwndPrev, GW_HWNDPREV);
        }
        if (hwndPrev == NULL)
        {
            hwndPrev = GetNextWindow(hwndCtrl, GW_HWNDLAST);
        }

        return hwndPrev;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
            HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, OnLButtonDown);
        }
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        if (::GetKeyState(VK_SHIFT) < 0 || ::GetKeyState(VK_CONTROL) < 0)
            return;

        MRadCtrl::DeselectSelection();
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
        if (fDoubleClick)
            return;

        FORWARD_WM_CONTEXTMENU(GetParent(hwnd), hwnd, x, y, SendMessage);
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

    void DoSubclass(HWND hCtrl, BOOL bTop)
    {
        MRadCtrl *pCtrl = new MRadCtrl;
        pCtrl->SubclassDx(hCtrl);
        pCtrl->m_bTopCtrl = bTop;

        DoSubclassChildren(hCtrl);
    }

    void DoSubclassChildren(HWND hwnd, BOOL bTop = FALSE)
    {
        HWND hCtrl = GetTopWindow(hwnd);
        if (bTop)
        {
            INT nIndex = 0;
            while (hCtrl)
            {
                DoSubclass(hCtrl, bTop);

                MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hCtrl);
                pCtrl->m_nIndex = nIndex;

                hCtrl = GetWindow(hCtrl, GW_HWNDNEXT);
                ++nIndex;
            }
        }
        else
        {
            while (hCtrl)
            {
                DoSubclass(hCtrl, bTop);
                hCtrl = GetWindow(hCtrl, GW_HWNDNEXT);
            }
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        POINT pt = { 0, 0 };
        SetWindowPosDx(&pt);
        SubclassDx(hwnd);

        DoSubclassChildren(hwnd, TRUE);

        return FALSE;
    }
};

struct MRadWindow : MWindowBase
{
    INT         m_xDialogBaseUnit;
    INT         m_yDialogBaseUnit;
    HHOOK       m_mouse_hook;
    MRadDialog  m_rad_dialog;
    DialogRes   m_dialog_res;
    static MRadWindow *s_p_rad_window;

    MRadWindow() : m_xDialogBaseUnit(0), m_yDialogBaseUnit(0),
                   m_mouse_hook(NULL)
    {
    }

    ~MRadWindow()
    {
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
        m_dialog_res.Fixup(FALSE);
        std::vector<BYTE> data = m_dialog_res.data();
        m_dialog_res.Fixup(TRUE);

        if (!m_rad_dialog.CreateDialogIndirectDx(hwnd, &data[0]))
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
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
            HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
            HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
            HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        ;
    }

    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        HWND hwndPrev, hwndNext;
        RECT rc;
        if (!fDown)
            return;

        HWND hwndTarget = MRadCtrl::GetLastSel();
        if (hwndTarget == NULL && !MRadCtrl::GetTargets().empty())
        {
            hwndTarget = *MRadCtrl::GetTargets().begin();
        }

        MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwndTarget);

        switch (vk)
        {
        case VK_TAB:
            if (GetKeyState(VK_SHIFT) < 0)
            {
                if (hwndTarget == NULL)
                {
                    hwndTarget = GetWindow(m_rad_dialog, GW_HWNDLAST);
                }
                else
                {
                    hwndTarget = m_rad_dialog.GetPrevCtrl(hwndTarget);
                }
                MRadCtrl::DeselectSelection();
                MRadCtrl::Select(hwndTarget);
            }
            else
            {
                if (hwndTarget == NULL)
                {
                    hwndTarget = GetWindow(m_rad_dialog, GW_HWNDFIRST);
                }
                else
                {
                    hwndTarget = m_rad_dialog.GetNextCtrl(hwndTarget);
                }
                MRadCtrl::DeselectSelection();
                MRadCtrl::Select(hwndTarget);
            }
            break;
        case VK_UP:
            if (pCtrl == NULL)
            {
                return;
            }
            if (GetKeyState(VK_SHIFT) < 0)
            {
                GetWindowRect(*pCtrl, &rc);
                MapWindowRect(NULL, m_rad_dialog, &rc);
                SIZE siz = SizeFromRectDx(&rc);
                siz.cy -= 1;
                MRadCtrl::ResizeSelection(NULL, siz.cx, siz.cy);
            }
            else
            {
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
                GetWindowRect(*pCtrl, &rc);
                MapWindowRect(NULL, m_rad_dialog, &rc);
                SIZE siz = SizeFromRectDx(&rc);
                siz.cy += 1;
                MRadCtrl::ResizeSelection(NULL, siz.cx, siz.cy);
            }
            else
            {
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
                GetWindowRect(*pCtrl, &rc);
                MapWindowRect(NULL, m_rad_dialog, &rc);
                SIZE siz = SizeFromRectDx(&rc);
                siz.cx -= 1;
                MRadCtrl::ResizeSelection(NULL, siz.cx, siz.cy);
            }
            else
            {
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
                GetWindowRect(*pCtrl, &rc);
                MapWindowRect(NULL, m_rad_dialog, &rc);
                SIZE siz = SizeFromRectDx(&rc);
                siz.cx += 1;
                MRadCtrl::ResizeSelection(NULL, siz.cx, siz.cy);
            }
            else
            {
                MRadCtrl::MoveSelection(NULL, +1, 0);
            }
            break;
        case VK_DELETE:
            MRadCtrl::DeleteSelection();
            break;
        default:
            return;
        }
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(2));
        HMENU hSubMenu = GetSubMenu(hMenu, 0);

        ::SetForegroundWindow(hwnd);
        ::TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
            xPos, yPos, 0, hwnd, NULL);
        ::PostMessage(hwnd, WM_NULL, 0, 0);
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

#endif  // ndef RADWINDOW_HPP_
