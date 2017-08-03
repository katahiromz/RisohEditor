// MRadWindow
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_RADWINDOW_HPP_
#define MZC4_RADWINDOW_HPP_

#include "MWindowBase.hpp"
#include "MRubberBand.hpp"
#include "MAddCtrlDlg.hpp"
#include "MDlgPropDlg.hpp"
#include "MCtrlPropDlg.hpp"
#include "DialogRes.hpp"
#include "resource.h"
#include <set>

//////////////////////////////////////////////////////////////////////////////

#define MYWM_CTRLMOVE           (WM_USER + 100)
#define MYWM_CTRLSIZE           (WM_USER + 101)
#define MYWM_CTRLDESTROY        (WM_USER + 102)
#define MYWM_DLGSIZE            (WM_USER + 103)

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
        if (hwnd)
            GetTargets().insert(hwnd);
        GetLastSel() = hwnd;
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
            HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
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

        OnNCLButtonDown(hwnd, FALSE, x, y, codeHitTest);
        OnNCLButtonUp(hwnd, x, y, codeHitTest);
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

        SendMessage(GetParent(hwnd), MYWM_CTRLMOVE, (WPARAM)hwnd, 0);
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

        SendMessage(GetParent(hwnd), MYWM_CTRLSIZE, (WPARAM)hwnd, 0);
    }

    void OnDestroy(HWND hwnd)
    {
        SendMessage(GetParent(hwnd), MYWM_CTRLDESTROY, (WPARAM)hwnd, 0);
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
    BOOL m_bDestroying;
    POINT m_ptClicked;

    MRadDialog() : m_bDestroying(FALSE)
    {
        m_ptClicked.x = m_ptClicked.y = -1;
    }

    void Renumber()
    {
        INT nIndex = 0;
        TCHAR szClass[64];
        HWND hwndCtrl = ::GetTopWindow(m_hwnd);
        for (;;)
        {
            HWND hwndNext = GetNextWindow(hwndCtrl, GW_HWNDNEXT);
            if (hwndNext == NULL)
            {
                break;
            }

            hwndCtrl = hwndNext;
            ::GetClassName(hwndCtrl, szClass, _countof(szClass));
            if (lstrcmpi(szClass, MRubberBand().GetWndClassNameDx()) != 0)
            {
                MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl);
                if (pCtrl)
                {
                    pCtrl->m_nIndex = nIndex++;
                }
            }
        }
    }

    HWND GetNextCtrl(HWND hwndCtrl) const
    {
        HWND hwnd = hwndCtrl;

        TCHAR szClass[64];
        for (;;)
        {
            HWND hwndNext = GetNextWindow(hwnd, GW_HWNDNEXT);
            if (hwndNext == NULL)
            {
                hwndNext = GetNextWindow(hwnd, GW_HWNDFIRST);
            }

            if (hwndNext == hwndCtrl)
                return NULL;

            hwnd = hwndNext;

            ::GetClassName(hwndNext, szClass, _countof(szClass));
            if (lstrcmpi(szClass, MRubberBand().GetWndClassNameDx()) != 0)
                break;
        }

        return hwnd;
    }

    HWND GetPrevCtrl(HWND hwndCtrl) const
    {
        HWND hwnd = hwndCtrl;

        TCHAR szClass[64];
        for (;;)
        {
            HWND hwndPrev = GetNextWindow(hwnd, GW_HWNDPREV);
            if (hwndPrev == NULL)
            {
                hwndPrev = GetNextWindow(hwnd, GW_HWNDLAST);
            }

            if (hwndPrev == hwndCtrl)
                return NULL;

            hwnd = hwndPrev;

            ::GetClassName(hwndPrev, szClass, _countof(szClass));
            if (lstrcmpi(szClass, MRubberBand().GetWndClassNameDx()) != 0)
                break;
        }

        return hwnd;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
            HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, OnLButtonDown);
            HANDLE_MSG(hwnd, WM_RBUTTONDOWN, OnRButtonDown);
            HANDLE_MSG(hwnd, WM_RBUTTONDBLCLK, OnRButtonDown);
        }
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    void OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        if (::GetKeyState(VK_SHIFT) < 0 || ::GetKeyState(VK_CONTROL) < 0)
            return;

        m_ptClicked.x = x;
        m_ptClicked.y = y;

        MRadCtrl::DeselectSelection();
    }

    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        if (::GetKeyState(VK_SHIFT) < 0 || ::GetKeyState(VK_CONTROL) < 0)
            return;

        m_ptClicked.x = x;
        m_ptClicked.y = y;

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
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
            HANDLE_MESSAGE(hwnd, MYWM_CTRLMOVE, OnCtrlMove);
            HANDLE_MESSAGE(hwnd, MYWM_CTRLSIZE, OnCtrlSize);
            HANDLE_MESSAGE(hwnd, MYWM_CTRLDESTROY, OnCtrlDestroy);
        }
        return CallWindowProcDx(hwnd, uMsg, wParam, lParam);
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        SendMessage(GetParent(hwnd), MYWM_DLGSIZE, 0, 0);
    }

    LRESULT OnCtrlMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        SendMessage(GetParent(hwnd), MYWM_CTRLMOVE, wParam, lParam);
        return 0;
    }

    LRESULT OnCtrlSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        SendMessage(GetParent(hwnd), MYWM_CTRLSIZE, wParam, lParam);
        return 0;
    }

    LRESULT OnCtrlDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        if (!m_bDestroying)
        {
            SendMessage(GetParent(hwnd), MYWM_CTRLDESTROY, wParam, lParam);
        }
        return 0;
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

        m_ptClicked.x = x;
        m_ptClicked.y = y;
        ::ScreenToClient(hwnd, &m_ptClicked);

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
    MRadDialog  m_rad_dialog;
    DialogRes   m_dialog_res;
    static MRadWindow *s_p_rad_window;

    MRadWindow() : m_xDialogBaseUnit(0), m_yDialogBaseUnit(0)
    {
    }

    ~MRadWindow()
    {
    }

    void ClientToDialog(POINT *ppt)
    {
        ppt->x = (ppt->x * 4) / m_xDialogBaseUnit;
        ppt->y = (ppt->y * 8) / m_yDialogBaseUnit;
    }

    void ClientToDialog(SIZE *psiz)
    {
        psiz->cx = (psiz->cx * 4) / m_xDialogBaseUnit;
        psiz->cy = (psiz->cy * 8) / m_yDialogBaseUnit;
    }

    void ClientToDialog(RECT *prc)
    {
        prc->left = (prc->left * 4) / m_xDialogBaseUnit;
        prc->right = (prc->right * 4) / m_xDialogBaseUnit;
        prc->top = (prc->top * 8) / m_yDialogBaseUnit;
        prc->bottom = (prc->bottom * 8) / m_yDialogBaseUnit;
    }

    void DialogToClient(POINT *ppt)
    {
        ppt->x = (ppt->x * m_xDialogBaseUnit) / 4;
        ppt->y = (ppt->y * m_yDialogBaseUnit) / 8;
    }

    void DialogToClient(SIZE *psiz)
    {
        psiz->cx = (psiz->cx * m_xDialogBaseUnit) / 4;
        psiz->cy = (psiz->cy * m_yDialogBaseUnit) / 8;
    }

    void DialogToClient(RECT *prc)
    {
        prc->left = (prc->left * m_xDialogBaseUnit) / 4;
        prc->right = (prc->right * m_xDialogBaseUnit) / 4;
        prc->top = (prc->top * m_yDialogBaseUnit) / 8;
        prc->bottom = (prc->bottom * m_yDialogBaseUnit) / 8;
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

        m_rad_dialog.m_bDestroying = FALSE;
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
        m_rad_dialog.m_bDestroying = TRUE;
        HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
        PostMessage(hwndOwner, WM_COMMAND, ID_DESTROYRAD, 0);
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
            HANDLE_MESSAGE(hwnd, MYWM_CTRLMOVE, OnCtrlMove);
            HANDLE_MESSAGE(hwnd, MYWM_CTRLSIZE, OnCtrlSize);
            HANDLE_MESSAGE(hwnd, MYWM_CTRLDESTROY, OnCtrlDestroy);
            HANDLE_MESSAGE(hwnd, MYWM_DLGSIZE, OnDlgSize);
        }
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    LRESULT OnCtrlMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        HWND hwndCtrl = (HWND)wParam;
        MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl);
        if (pCtrl == NULL)
            return 0;

        if (pCtrl->m_nIndex == -1)
            return 0;

        RECT rc;
        GetWindowRect(*pCtrl, &rc);
        MapWindowRect(NULL, m_rad_dialog, &rc);

        ClientToDialog(&rc);

        m_dialog_res.Items[pCtrl->m_nIndex].m_pt.x = rc.left;
        m_dialog_res.Items[pCtrl->m_nIndex].m_pt.y = rc.top;

        UpdateRes();

        return 0;
    }

    LRESULT OnCtrlSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        HWND hwndCtrl = (HWND)wParam;
        MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl);
        if (pCtrl == NULL)
            return 0;

        if (pCtrl->m_nIndex == -1)
            return 0;

        RECT rc;
        GetWindowRect(*pCtrl, &rc);
        MapWindowRect(NULL, m_rad_dialog, &rc);

        ClientToDialog(&rc);

        m_dialog_res.Items[pCtrl->m_nIndex].m_siz.cx = rc.right - rc.left;
        m_dialog_res.Items[pCtrl->m_nIndex].m_siz.cy = rc.bottom - rc.top;

        UpdateRes();

        return 0;
    }

    void UpdateRes()
    {
        HWND hwndOwner = ::GetWindow(m_hwnd, GW_OWNER);
        PostMessage(hwndOwner, WM_COMMAND, ID_UPDATERES, 0);
    }

    LRESULT OnCtrlDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        HWND hwndCtrl = (HWND)wParam;
        MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl);
        if (pCtrl == NULL)
            return 0;

        if (pCtrl->m_nIndex == -1)
            return 0;

        m_dialog_res.Items.erase(m_dialog_res.Items.begin() + pCtrl->m_nIndex);
        m_dialog_res.m_cItems--;
        m_rad_dialog.Renumber();
        UpdateRes();

        return 0;
    }

    LRESULT OnDlgSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        RECT rc;
        GetClientRect(m_rad_dialog, &rc);
        ClientToDialog(&rc);

        m_dialog_res.m_pt.x = rc.left;
        m_dialog_res.m_pt.y = rc.top;
        m_dialog_res.m_siz.cx = rc.right - rc.left;
        m_dialog_res.m_siz.cy = rc.bottom - rc.top;
        UpdateRes();

        return 0;
    }

    void OnAddCtrl(HWND hwnd)
    {
        POINT pt = m_rad_dialog.m_ptClicked;
        ClientToDialog(&pt);
        if (pt.x < 0 || pt.y < 0)
            pt.x = pt.y = 0;

        MAddCtrlDlg dialog(m_dialog_res, pt);
        dialog.DialogBoxDx(hwnd);
    }

    void OnCtrlProp(HWND hwnd)
    {
    }

    void OnDlgProp(HWND hwnd)
    {
        MDlgPropDlg dialog(m_dialog_res);
        dialog.DialogBoxDx(hwnd);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case ID_DELCTRL:
            MRadCtrl::DeleteSelection();
            break;
        case ID_ADDCTRL:
            OnAddCtrl(hwnd);
            break;
        case ID_CTRLPROP:
            OnCtrlProp(hwnd);
            break;
        case ID_DLGPROP:
            OnDlgProp(hwnd);
            break;
        }
    }

    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
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
        HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(3));
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

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_RADWINDOW_HPP_
