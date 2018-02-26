// MRadWindow.hpp --- RADical development window
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#ifndef MZC4_MRADWINDOW_HPP_
#define MZC4_MRADWINDOW_HPP_

#include "MWindowBase.hpp"
#include "MRubberBand.hpp"
#include "MIndexLabels.hpp"
#include "MAddCtrlDlg.hpp"
#include "MDlgPropDlg.hpp"
#include "MCtrlPropDlg.hpp"
#include "DialogRes.hpp"
#include "PackedDIB.hpp"
#include "Res.hpp"
#include "IconRes.hpp"
#include "resource.h"
#include <map>
#include <set>
#include <climits>

class MRadCtrl;
class MRadDialog;
class MRadWindow;

//////////////////////////////////////////////////////////////////////////////

#define MYWM_CTRLMOVE           (WM_USER + 100)
#define MYWM_CTRLSIZE           (WM_USER + 101)
#ifndef MYWM_SELCHANGE
    #define MYWM_SELCHANGE      (WM_USER + 102)
#endif
#define MYWM_DLGSIZE            (WM_USER + 103)
#define MYWM_DELETESEL          (WM_USER + 104)
#define MYWM_MOVESIZEREPORT     (WM_USER + 105)
#define MYWM_CLEARSTATUS        (WM_USER + 106)
#define MYWM_COMPILECHECK       (WM_USER + 107)
#define MYWM_REOPENRAD          (WM_USER + 108)
#define MYWM_GETUNITS           (WM_USER + 109)

#define GRID_SIZE 5

class MRadCtrl : public MWindowBase
{
public:
    DWORD           m_dwMagic;
    BOOL            m_bTopCtrl;
    HWND            m_hwndRubberBand;
    BOOL            m_bMoving;
    BOOL            m_bSizing;
    BOOL            m_bLocking;
    INT             m_nIndex;
    ConstantsDB&    m_db;
    RisohSettings&  m_settings;
    POINT           m_pt;
    INT             m_nImageType;

    MRadCtrl(ConstantsDB& db, RisohSettings& settings) :
        m_dwMagic(0xDEADFACE), m_bTopCtrl(FALSE), m_hwndRubberBand(NULL),
        m_bMoving(FALSE), m_bSizing(FALSE), m_bLocking(FALSE),
        m_nIndex(-1), m_db(db), m_settings(settings)
    {
        m_pt.x = m_pt.y = -1;
        m_nImageType = 0;
    }

    static HICON& Icon()
    {
        static HICON s_hIcon = LoadIcon(GetModuleHandle(NULL),
                                        MAKEINTRESOURCE(IDI_ICO));
        return s_hIcon;
    }
    static HBITMAP& Bitmap()
    {
        static HBITMAP s_hbm = LoadBitmap(GetModuleHandle(NULL),
                                          MAKEINTRESOURCE(IDB_BMP));
        return s_hbm;
    }

    static BOOL IsGroupBox(HWND hCtrl)
    {
        WCHAR szClass[8];
        GetClassNameW(hCtrl, szClass, _countof(szClass));
        if (lstrcmpiW(szClass, L"BUTTON") == 0)
        {
            return (GetWindowStyle(hCtrl) & BS_TYPEMASK) == BS_GROUPBOX;
        }
        return FALSE;
    }

    void EndSubclass()
    {
        SIZE siz;
        TCHAR szClass[16];
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

    static std::set<INT> GetTargetIndeces()
    {
        set_type targets = MRadCtrl::GetTargets();

        std::set<INT> indeces;
        set_type::iterator it, end = targets.end();
        for (it = targets.begin(); it != end; ++it)
        {
            MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(*it);
            indeces.insert(pCtrl->m_nIndex);
        }
        return indeces;
    }

    typedef std::map<INT, HWND> map_type;
    static map_type& IndexToCtrlMap()
    {
        static map_type s_map;
        return s_map;
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
            if (pCtrl->m_dwMagic == 0xDEADFACE)
                return pCtrl;
        }
        return NULL;
    }

    static BOOL& GetRangeSelect(void)
    {
        static BOOL s_bRangeSelect = FALSE;
        return s_bRangeSelect;
    }

    static BOOL DeselectSelection()
    {
        BOOL bFound = FALSE;
        set_type::iterator it, end = GetTargets().end();
        for (it = GetTargets().begin(); it != end; ++it)
        {
            MRadCtrl *pCtrl = GetRadCtrl(*it);
            if (pCtrl)
            {
                ::DestroyWindow(pCtrl->m_hwndRubberBand);
                pCtrl->m_hwndRubberBand = NULL;
                bFound = TRUE;
            }
        }
        GetTargets().clear();
        GetLastSel() = NULL;
        return bFound;
    }

    static void DeleteSelection()
    {
        if (GetTargets().empty())
            return;

        set_type::iterator it = GetTargets().begin();
        PostMessage(GetParent(*it), MYWM_DELETESEL, 0, 0);
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

    BOOL IsSelected()
    {
        return IsWindow(m_hwndRubberBand);
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

        if (!MRadCtrl::IsGroupBox(hwnd))
        {
            SetWindowPosDx(hwnd, NULL, NULL, HWND_BOTTOM);
        }

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

    struct RANGE_SELECT
    {
        RECT rc;
        BOOL bCtrlDown;
    };

    static BOOL CALLBACK
    RangeSelectProc(HWND hwnd, LPARAM lParam)
    {
        RANGE_SELECT *prs = (RANGE_SELECT *)lParam;
        RECT *prc = &prs->rc;
        RECT rc;

        if (MRadCtrl *pCtrl = GetRadCtrl(hwnd))
        {
            if (pCtrl->m_bTopCtrl)
            {
                GetWindowRect(*pCtrl, &rc);

                if (prc->left <= rc.left && prc->top <= rc.top &&
                    rc.right <= prc->right && rc.bottom <= prc->bottom)
                {
                    if (prs->bCtrlDown)
                    {
                        if (pCtrl->IsSelected())
                        {
                            pCtrl->Deselect();
                        }
                        else
                        {
                            MRadCtrl::Select(*pCtrl);
                        }
                    }
                    else
                    {
                        if (!pCtrl->IsSelected())
                        {
                            MRadCtrl::Select(*pCtrl);
                        }
                    }
                }
            }
        }
        return TRUE;
    }

    static void DoRangeSelect(HWND hwndParent, const RECT *prc, BOOL bCtrlDown)
    {
        RANGE_SELECT rs;
        rs.rc = *prc;
        rs.bCtrlDown = bCtrlDown;
        EnumChildWindows(hwndParent, RangeSelectProc, (LPARAM)&rs);
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
            HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
            case WM_MOVING: case WM_SIZING:
                return 0;
        }
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
    {
        WCHAR szClass[64];
        GetClassNameW(hwnd, szClass, 64);
        if (lstrcmpiW(szClass, TOOLBARCLASSNAMEW) == 0 ||
            lstrcmpiW(szClass, REBARCLASSNAMEW) == 0 ||
            lstrcmpiW(szClass, WC_PAGESCROLLERW) == 0)
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, (HBRUSH)(COLOR_3DFACE + 1));
            return TRUE;
        }

        return (BOOL)DefaultProcDx();
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

        if (!m_bLocking)
        {
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
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        DefaultProcDx(hwnd, WM_SIZE, state, MAKELPARAM(cx, cy));
        if (!m_bTopCtrl)
        {
            return;
        }

        if (!m_bLocking)
        {
            if (!m_bSizing)
                ResizeSelection(hwnd, cx, cy);

            SendMessage(GetParent(hwnd), MYWM_CTRLSIZE, (WPARAM)hwnd, 0);
            InvalidateRect(hwnd, NULL, TRUE);
        }
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

        ::DefWindowProc(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(x, y));
        if (!IsGroupBox(hwnd))
            SetWindowPosDx(hwnd, NULL, NULL, HWND_BOTTOM);
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

    struct MYHITTEST
    {
        HWND hParent;
        HWND hCandidate;
        HWND hLast;
        POINT pt;
    };

    static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
    {
        MYHITTEST *pmht = (MYHITTEST *)lParam;
        RECT rc;
        GetWindowRect(hwnd, &rc);
        if (PtInRect(&rc, pmht->pt))
        {
            // NOTE: EnumChildWindows scans not only children but descendants.
            if (MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwnd))
            {
                if (pCtrl->m_bTopCtrl)
                {
                    pmht->hLast = hwnd;
                    if (!IsGroupBox(hwnd))
                    {
                        pmht->hCandidate = hwnd;
                    }
                }
            }
        }
        return TRUE;
    }

    UINT OnNCHitTest(HWND hwnd, int x, int y)
    {
        if (m_bTopCtrl)
        {
            RECT rc;
            GetWindowRect(hwnd, &rc);

            POINT pt = { x, y };
            if (m_hwndRubberBand && PtInRect(&rc, pt))
                return HTCAPTION;

            MYHITTEST mht;
            mht.hParent = GetParent(hwnd);
            mht.hCandidate = NULL;
            mht.hLast = NULL;
            mht.pt = pt;
            EnumChildWindows(mht.hParent, EnumChildProc, (LPARAM)&mht);

            if (mht.hCandidate == hwnd ||
                (!mht.hCandidate && mht.hLast == hwnd))
            {
                return HTCAPTION;
            }
        }
        return HTTRANSPARENT;
    }

    virtual void PostNcDestroy()
    {
        if (m_bTopCtrl)
        {
            DebugPrintDx("MRadCtrl::PostNcDestroy: %p\n", m_hwnd);
        }
        MWindowBase::PostNcDestroy();
        delete this;
    }
};

class MRadDialog : public MDialogBase
{
public:
    BOOL            m_index_visible;
    ConstantsDB&    m_db;
    POINT           m_ptClicked;
    POINT           m_ptDragging;
    MIndexLabels    m_labels;
    RisohSettings&  m_settings;
    BOOL            m_bMovingSizing;
    INT             m_xDialogBaseUnit;
    INT             m_yDialogBaseUnit;
    HBRUSH          m_hbrBack;

    MRadDialog(RisohSettings& settings, ConstantsDB& db)
        : m_index_visible(FALSE), m_db(db), m_settings(settings),
          m_bMovingSizing(FALSE)
    {
        m_xDialogBaseUnit = LOWORD(GetDialogBaseUnits());
        m_yDialogBaseUnit = HIWORD(GetDialogBaseUnits());

        m_ptClicked.x = m_ptClicked.y = -1;

        HFONT hFont = GetStockFont(DEFAULT_GUI_FONT);
        LOGFONT lf;
        GetObject(hFont, sizeof(lf), &lf);
        lf.lfHeight = 14;
        hFont = ::CreateFontIndirect(&lf);
        m_labels.m_hFont = hFont;

        m_hbrBack = NULL;
        ReCreateBackBrush();
    }

    ~MRadDialog()
    {
        DeleteObject(m_hbrBack);
    }

    enum TARGET
    {
        TARGET_NEXT,
        TARGET_PREV,
        TARGET_FIRST,
        TARGET_LAST
    };

    struct GET_TARGET
    {
        TARGET target;
        HWND hwndTarget;
        INT m_nIndex;
        INT m_nCurrentIndex;
    };

    static BOOL CALLBACK GetTargetProc(HWND hwnd, LPARAM lParam)
    {
        GET_TARGET *get_target = (GET_TARGET *)lParam;
        if (MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwnd))
        {
            if (pCtrl->m_dwMagic != 0xDEADFACE || !pCtrl->m_bTopCtrl)
            {
                return TRUE;
            }
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
        return TRUE;
    }

    HWND GetNextCtrl(HWND hwndCtrl) const
    {
        if (MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl))
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

    HWND GetPrevCtrl(HWND hwndCtrl) const
    {
        if (MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl))
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

    static HWND GetFirstCtrl(HWND hwndParent)
    {
        GET_TARGET get_target;
        get_target.target = TARGET_FIRST;
        get_target.hwndTarget = NULL;
        get_target.m_nIndex = 0x7FFFFFFF;
        EnumChildWindows(hwndParent, GetTargetProc, (LPARAM)&get_target);
        return get_target.hwndTarget;
    }

    static HWND GetLastCtrl(HWND hwndParent)
    {
        GET_TARGET get_target;
        get_target.target = TARGET_LAST;
        get_target.hwndTarget = NULL;
        get_target.m_nIndex = -1;
        EnumChildWindows(hwndParent, GetTargetProc, (LPARAM)&get_target);
        return get_target.hwndTarget;
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
            HANDLE_MESSAGE(hwnd, MYWM_SELCHANGE, OnSelChange);
        }
        return 0;
    }

    void OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        if (::GetKeyState(VK_SHIFT) < 0 || ::GetKeyState(VK_CONTROL) < 0)
            return;

        m_ptClicked.x = x;
        m_ptClicked.y = y;

        MRadCtrl::DeselectSelection();
    }

    LRESULT OnSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        PostMessage(GetParent(hwnd), MYWM_SELCHANGE, wParam, lParam);
        return 0;
    }

    void NormalizeRect(RECT *prc, POINT pt0, POINT pt1)
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

    void DrawDragSelect(HWND hwnd)
    {
        if (HDC hDC = GetDC(hwnd))
        {
            RECT rc;
            NormalizeRect(&rc, m_ptClicked, m_ptDragging);

            DrawFocusRect(hDC, &rc);

            ReleaseDC(hwnd, hDC);
        }
    }

    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        m_ptClicked.x = x;
        m_ptClicked.y = y;

        if (::GetKeyState(VK_SHIFT) >= 0 && ::GetKeyState(VK_CONTROL) >= 0)
        {
            MRadCtrl::DeselectSelection();
        }

        if (!MRadCtrl::GetRangeSelect())
        {
            m_ptDragging = m_ptClicked;

            DrawDragSelect(hwnd);

            MRadCtrl::GetRangeSelect() = TRUE;
            ::SetCapture(hwnd);
        }
    }

    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
        if (MRadCtrl::GetRangeSelect())
        {
            DrawDragSelect(hwnd);

            m_ptDragging.x = x;
            m_ptDragging.y = y;

            DrawDragSelect(hwnd);
        }
    }

    void OnCaptureChanged(HWND hwnd)
    {
        if (MRadCtrl::GetRangeSelect())
        {
            DrawDragSelect(hwnd);
            MRadCtrl::GetRangeSelect() = FALSE;
        }
    }

    void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
    {
        if (MRadCtrl::GetRangeSelect())
        {
            RECT rc;
            NormalizeRect(&rc, m_ptClicked, m_ptDragging);

            MapWindowRect(hwnd, NULL, &rc);

            if (GetAsyncKeyState(VK_SHIFT) >= 0 &&
                GetAsyncKeyState(VK_CONTROL) >= 0)
            {
                MRadCtrl::DeselectSelection();
            }

            ReleaseCapture();
            MRadCtrl::GetRangeSelect() = FALSE;

            BOOL bCtrlDown = GetAsyncKeyState(VK_CONTROL) < 0;
            MRadCtrl::DoRangeSelect(hwnd, &rc, bCtrlDown);
        }
    }

    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
    {
        if (m_hbrBack == NULL)
            ReCreateBackBrush();

        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, m_hbrBack);

        return TRUE;
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
            HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
            HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
            HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
            HANDLE_MSG(hwnd, WM_SYSCOLORCHANGE, OnSysColorChange);
            HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
            HANDLE_MESSAGE(hwnd, MYWM_CTRLMOVE, OnCtrlMove);
            HANDLE_MESSAGE(hwnd, MYWM_CTRLSIZE, OnCtrlSize);
            HANDLE_MESSAGE(hwnd, MYWM_DELETESEL, OnDeleteSel);
            case WM_CAPTURECHANGED:
                OnCaptureChanged(hwnd);
                break;
        }
        return CallWindowProcDx(hwnd, uMsg, wParam, lParam);
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        if (m_bMovingSizing)
            return;

        SendMessage(GetParent(hwnd), MYWM_DLGSIZE, 0, 0);
    }

    LRESULT OnDeleteSel(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        SendMessage(GetParent(hwnd), MYWM_DELETESEL, wParam, lParam);
        return 0;
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

    void DoSubclass(HWND hCtrl, INT nIndex)
    {
        MRadCtrl *pCtrl = new MRadCtrl(m_db, m_settings);
        pCtrl->SubclassDx(hCtrl);
        pCtrl->m_bTopCtrl = (nIndex != -1);
        pCtrl->m_nIndex = nIndex;

        if (nIndex != -1)
        {
            MRadCtrl::IndexToCtrlMap()[nIndex] = hCtrl;
        }

        pCtrl->EndSubclass();

        MString text = GetWindowText(hCtrl);
        DebugPrintDx(TEXT("MRadCtrl::DoSubclass: %p, %d, '%s'\n"), hCtrl, nIndex, text.c_str());

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
                DoSubclass(hCtrl, nIndex++);

                hCtrl = GetWindow(hCtrl, GW_HWNDNEXT);
            }
        }
        else
        {
            while (hCtrl)
            {
                DoSubclass(hCtrl, -1);
                hCtrl = GetWindow(hCtrl, GW_HWNDNEXT);
            }
        }
    }

    BOOL ReCreateBackBrush()
    {
        if (m_hbrBack)
        {
            DeleteObject(m_hbrBack);
            m_hbrBack = NULL;
        }

        COLORREF rgb = GetSysColor(COLOR_3DFACE);
        DWORD dwTotal = GetRValue(rgb) + GetGValue(rgb) + GetBValue(rgb);
        rgb = (dwTotal < 255) ? RGB(255, 255, 255) : RGB(0, 0, 0);

        RECT rc8x8 = { 0, 0, 8, 8 };

        HBITMAP hbm8x8 = Create24BppBitmapDx(8, 8);
        HDC hDC = CreateCompatibleDC(NULL);
        HGDIOBJ hbmOld = SelectObject(hDC, hbm8x8);
        {
            FillRect(hDC, &rc8x8, (HBRUSH)(COLOR_3DFACE + 1));
            if (m_settings.bShowDotsOnDialog)
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
        std::vector<BYTE> data;
        PackedDIB_CreateFromHandle(data, hbm8x8);
        DeleteObject(hbm8x8);

        m_hbrBack = CreateDIBPatternBrushPt(&data[0], DIB_RGB_COLORS);
        return m_hbrBack != NULL;
    }

    void OnSysColorChange(HWND hwnd)
    {
        ReCreateBackBrush();
        InvalidateRect(hwnd, NULL, TRUE);
    }

    void GroupBoxDown(HWND hwnd)
    {
        HWND hwndFirstGroupBox = NULL;
    retry:
        for (HWND hCtrl = GetTopWindow(hwnd); hCtrl;
             hCtrl = GetWindow(hCtrl, GW_HWNDNEXT))
        {
            if (MRadCtrl::IsGroupBox(hCtrl))
            {
                if (hwndFirstGroupBox == NULL)
                {
                    hwndFirstGroupBox = hCtrl;
                }
                else if (hwndFirstGroupBox == hCtrl)
                {
                    break;
                }
                SetWindowPos(hCtrl, HWND_BOTTOM, 0, 0, 0, 0,
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOREPOSITION);
                goto retry;
            }
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        OnSysColorChange(hwnd);

        MRadCtrl::GetTargets().clear();
        MRadCtrl::GetLastSel() = NULL;
        MRadCtrl::IndexToCtrlMap().clear();

        POINT pt = { 0, 0 };
        SetWindowPosDx(hwnd, &pt);

        DoSubclassChildren(hwnd, TRUE);

        if (m_index_visible)
            ShowHideLabels(TRUE);

        SubclassDx(hwnd);

        GroupBoxDown(hwnd);

        return FALSE;
    }

    void ShowHideLabels(BOOL bShow = TRUE)
    {
        m_index_visible = bShow;
        if (bShow)
            m_labels.ReCreate(m_hwnd, MRadCtrl::IndexToCtrlMap());
        else
            m_labels.Destroy();
    }
};

class MRadWindow : public MWindowBase
{
public:
    ResEntries&     m_entries;
    INT             m_xDialogBaseUnit;
    INT             m_yDialogBaseUnit;
    MRadDialog      m_rad_dialog;
    DialogRes       m_dialog_res;
    RisohSettings&  m_settings;
    HICON           m_hIcon;
    HICON           m_hIconSm;
    MTitleToBitmap  m_title_to_bitmap;
    MTitleToIcon    m_title_to_icon;

    MRadWindow(ResEntries& entries, ConstantsDB& db, RisohSettings& settings)
        : m_entries(entries),
          m_xDialogBaseUnit(0), m_yDialogBaseUnit(0), m_rad_dialog(settings, db),
          m_dialog_res(db), m_settings(settings), m_hIcon(NULL), m_hIconSm(NULL)
    {
    }

    void create_maps(WORD lang)
    {
        for (size_t i = 0; i < m_dialog_res.size(); ++i)
        {
            DialogItem& item = m_dialog_res[i];
            if (item.m_class == 0x0082 ||
                lstrcmpiW(item.m_class.c_str(), L"STATIC") == 0)
            {
                // static control
                if ((item.m_style & SS_TYPEMASK) == SS_ICON)
                {
                    Res_DoIcon(m_entries, m_title_to_icon, item, lang);
                }
                else if ((item.m_style & SS_TYPEMASK) == SS_BITMAP)
                {
                    Res_DoBitmap(m_entries, m_title_to_bitmap, item, lang);
                }
            }
        }
    }

    ~MRadWindow()
    {
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
        clear_maps();
    }

    void clear_maps()
    {
        ClearMaps(m_title_to_bitmap, m_title_to_icon);
    }

    BOOL CreateDx(HWND hwndParent)
    {
        BOOL bMovingSizing = m_rad_dialog.m_bMovingSizing;
        m_rad_dialog.m_bMovingSizing = TRUE;
        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
        if (CreateWindowDx(hwndParent, MAKEINTRESOURCE(IDS_RADWINDOW), style))
        {
            ShowWindow(m_hwnd, SW_SHOWNORMAL);
            UpdateWindow(m_hwnd);
            m_rad_dialog.m_bMovingSizing = bMovingSizing;
            return TRUE;
        }
        m_rad_dialog.m_bMovingSizing = bMovingSizing;

        return FALSE;
    }

    void ClientToDialog(POINT *ppt)
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        ppt->x = (ppt->x * 4) / m_xDialogBaseUnit;
        ppt->y = (ppt->y * 8) / m_yDialogBaseUnit;
    }

    void ClientToDialog(SIZE *psiz)
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        psiz->cx = (psiz->cx * 4) / m_xDialogBaseUnit;
        psiz->cy = (psiz->cy * 8) / m_yDialogBaseUnit;
    }

    void ClientToDialog(RECT *prc)
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        prc->left = (prc->left * 4) / m_xDialogBaseUnit;
        prc->right = (prc->right * 4) / m_xDialogBaseUnit;
        prc->top = (prc->top * 8) / m_yDialogBaseUnit;
        prc->bottom = (prc->bottom * 8) / m_yDialogBaseUnit;
    }

    void DialogToClient(POINT *ppt)
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        ppt->x = (ppt->x * m_xDialogBaseUnit + 2) / 4;
        ppt->y = (ppt->y * m_yDialogBaseUnit + 4) / 8;
    }

    void DialogToClient(SIZE *psiz)
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        psiz->cx = (psiz->cx * m_xDialogBaseUnit + 2) / 4;
        psiz->cy = (psiz->cy * m_yDialogBaseUnit + 4) / 8;
    }

    void DialogToClient(RECT *prc)
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        prc->left = (prc->left * m_xDialogBaseUnit + 2) / 4;
        prc->right = (prc->right * m_xDialogBaseUnit + 2) / 4;
        prc->top = (prc->top * m_yDialogBaseUnit + 4) / 8;
        prc->bottom = (prc->bottom * m_yDialogBaseUnit + 4) / 8;
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
        RECT rc;
        GetWindowRect(m_rad_dialog, &rc);
        SIZE siz;
        siz.cx = rc.right - rc.left;
        siz.cy = rc.bottom - rc.top;

        DWORD style = GetWindowLong(m_hwnd, GWL_STYLE);
        DWORD exstyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);
        SetRect(&rc, 0, 0, siz.cx, siz.cy);
        AdjustWindowRectEx(&rc, style, FALSE, exstyle);

        siz.cx = rc.right - rc.left;
        siz.cy = rc.bottom - rc.top;
        SetWindowPosDx(NULL, &siz);
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

    BOOL ReCreateRadDialog(HWND hwnd)
    {
        assert(IsWindow(hwnd));

        BOOL bMovingSizingOld = m_rad_dialog.m_bMovingSizing;
        m_rad_dialog.m_bMovingSizing = TRUE;
        if (m_rad_dialog)
        {
            DestroyWindow(m_rad_dialog);
        }

        m_dialog_res.Fixup(FALSE);
        std::vector<BYTE> data = m_dialog_res.data();
#if 0
        MFile file(TEXT("modified.bin"), TRUE);
        DWORD cbWritten;
        file.WriteFile(&data[0], (DWORD)data.size(), &cbWritten);
#endif
        m_dialog_res.Fixup(TRUE);

        if (!m_rad_dialog.CreateDialogIndirectDx(hwnd, &data[0]))
        {
            m_rad_dialog.m_bMovingSizing = bMovingSizingOld;
            return FALSE;
        }
        assert(IsWindow(m_rad_dialog));

        FitToRadDialog();
        m_rad_dialog.m_bMovingSizing = bMovingSizingOld;

        ShowWindow(m_rad_dialog, SW_SHOWNOACTIVATE);
        UpdateWindow(m_rad_dialog);

        SetForegroundWindow(hwnd);

        update_maps();

        return TRUE;
    }

    void update_maps()
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);

        HWND hCtrl = GetTopWindow(m_rad_dialog);
        while (hCtrl)
        {
            if (MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hCtrl))
            {
                SIZE siz;
                GetWindowPosDx(hCtrl, NULL, &siz);

                if (pCtrl->m_nImageType == 1)
                {
                    // icon
                    WORD wID = m_dialog_res[pCtrl->m_nIndex].m_title.m_id;
                    if (HICON hIcon = m_title_to_icon[wID])
                    {
                        SendMessage(hCtrl, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
                        DWORD style = GetWindowStyle(hCtrl);
                        if (style & SS_REALSIZEIMAGE)
                        {
                            ICONINFO info;
                            GetIconInfo(hIcon, &info);
                            BITMAP bm;
                            GetObject(info.hbmColor, sizeof(BITMAP), &bm);
                            siz.cx = bm.bmWidth;
                            siz.cy = bm.bmHeight;
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
                        SetWindowPosDx(hCtrl, NULL, &siz);
                    }
                }
                if (pCtrl->m_nImageType == 2)
                {
                    // bitmap
                    WORD wID = m_dialog_res[pCtrl->m_nIndex].m_title.m_id;
                    if (HBITMAP hbm = m_title_to_bitmap[wID])
                    {
                        SendMessage(hCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
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
                        SetWindowPosDx(hCtrl, NULL, &siz);
                    }
                }
            }

            hCtrl = GetNextWindow(hCtrl, GW_HWNDNEXT);
        }
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

        if (m_settings.bResumeWindowPos)
        {
            if (m_settings.nRadLeft != CW_USEDEFAULT)
            {
                POINT pt = { m_settings.nRadLeft, m_settings.nRadTop };
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

        return ReCreateRadDialog(hwnd);
    }

    void OnDestroy(HWND hwnd)
    {
        HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
        PostMessage(hwndOwner, WM_COMMAND, CMDID_DESTROYRAD, 0);

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

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
            DO_MSG(WM_INITMENUPOPUP, OnInitMenuPopup);
            DO_MSG(WM_ACTIVATE, OnActivate);
            DO_MSG(WM_SYSCOLORCHANGE, OnSysColorChange);
        }
        return DefaultProcDx();
    }

    void OnSysColorChange(HWND hwnd)
    {
        m_rad_dialog.SendMessageDx(WM_SYSCOLORCHANGE);
    }

    LRESULT OnGetUnits(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        return 0;
    }

    void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
    {
        if (state == WA_ACTIVE || state == WA_CLICKACTIVE)
        {
            HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
            if (!SendMessage(hwndOwner, MYWM_COMPILECHECK, (WPARAM)hwnd, 0))
            {
                return;
            }
            PostMessage(hwndOwner, MYWM_REOPENRAD, 0, 0);
        }
        FORWARD_WM_ACTIVATE(hwnd, state, hwndActDeact, fMinimized, DefWindowProc);
    }

    LRESULT OnSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        HWND hwndOwner = GetWindow(hwnd, GW_OWNER);

        HWND hwndSel = MRadCtrl::GetLastSel();
        if (hwndSel == NULL)
        {
            PostMessage(hwndOwner, MYWM_CLEARSTATUS, 0, 0);
            return 0;
        }
        MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwndSel);
        if (pCtrl == NULL)
        {
            PostMessage(hwndOwner, MYWM_CLEARSTATUS, 0, 0);
            return 0;
        }

        if (size_t(pCtrl->m_nIndex) < m_dialog_res.m_items.size())
        {
            DialogItem& item = m_dialog_res.m_items[pCtrl->m_nIndex];
            PostMessage(hwndOwner, MYWM_MOVESIZEREPORT, 
                MAKEWPARAM(item.m_pt.x, item.m_pt.y),
                MAKELPARAM(item.m_siz.cx, item.m_siz.cy));
        }
        return 0;
    }

    void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
    {
        MRadCtrl::set_type set = MRadCtrl::GetTargets();
        if (set.empty())
        {
            ::EnableMenuItem(hMenu, CMDID_DELCTRL, MF_GRAYED);
            ::EnableMenuItem(hMenu, CMDID_CTRLPROP, MF_GRAYED);
            ::EnableMenuItem(hMenu, CMDID_TOPALIGN, MF_GRAYED);
            ::EnableMenuItem(hMenu, CMDID_BOTTOMALIGN, MF_GRAYED);
            ::EnableMenuItem(hMenu, CMDID_LEFTALIGN, MF_GRAYED);
            ::EnableMenuItem(hMenu, CMDID_RIGHTALIGN, MF_GRAYED);
            ::EnableMenuItem(hMenu, CMDID_FITTOGRID, MF_GRAYED);
        }
        else if (set.size() == 1)
        {
            ::EnableMenuItem(hMenu, CMDID_DELCTRL, MF_ENABLED);
            ::EnableMenuItem(hMenu, CMDID_CTRLPROP, MF_ENABLED);
            ::EnableMenuItem(hMenu, CMDID_TOPALIGN, MF_GRAYED);
            ::EnableMenuItem(hMenu, CMDID_BOTTOMALIGN, MF_GRAYED);
            ::EnableMenuItem(hMenu, CMDID_LEFTALIGN, MF_GRAYED);
            ::EnableMenuItem(hMenu, CMDID_RIGHTALIGN, MF_GRAYED);
            ::EnableMenuItem(hMenu, CMDID_FITTOGRID, MF_ENABLED);
        }
        else
        {
            ::EnableMenuItem(hMenu, CMDID_DELCTRL, MF_ENABLED);
            ::EnableMenuItem(hMenu, CMDID_CTRLPROP, MF_ENABLED);
            ::EnableMenuItem(hMenu, CMDID_TOPALIGN, MF_ENABLED);
            ::EnableMenuItem(hMenu, CMDID_BOTTOMALIGN, MF_ENABLED);
            ::EnableMenuItem(hMenu, CMDID_LEFTALIGN, MF_ENABLED);
            ::EnableMenuItem(hMenu, CMDID_RIGHTALIGN, MF_ENABLED);
            ::EnableMenuItem(hMenu, CMDID_FITTOGRID, MF_ENABLED);
        }

        if (CanIndexTop())
        {
            ::EnableMenuItem(hMenu, CMDID_CTRLINDEXTOP, MF_ENABLED);
        }
        else
        {
            ::EnableMenuItem(hMenu, CMDID_CTRLINDEXTOP, MF_GRAYED);
        }

        if (CanIndexBottom())
        {
            ::EnableMenuItem(hMenu, CMDID_CTRLINDEXBOTTOM, MF_ENABLED);
        }
        else
        {
            ::EnableMenuItem(hMenu, CMDID_CTRLINDEXBOTTOM, MF_GRAYED);
        }

        if (CanIndexMinus())
        {
            ::EnableMenuItem(hMenu, CMDID_CTRLINDEXMINUS, MF_ENABLED);
        }
        else
        {
            ::EnableMenuItem(hMenu, CMDID_CTRLINDEXMINUS, MF_GRAYED);
        }

        if (CanIndexPlus())
        {
            ::EnableMenuItem(hMenu, CMDID_CTRLINDEXPLUS, MF_ENABLED);
        }
        else
        {
            ::EnableMenuItem(hMenu, CMDID_CTRLINDEXPLUS, MF_GRAYED);
        }
    }

    LRESULT OnDeleteSel(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        std::set<INT> indeces = MRadCtrl::GetTargetIndeces();
        std::set<INT>::reverse_iterator rit, rend = indeces.rend();
        for (rit = indeces.rbegin(); rit != rend; ++rit)
        {
            m_dialog_res.m_items.erase(m_dialog_res.m_items.begin() + *rit);
            m_dialog_res.m_cItems--;
        }

        ReCreateRadDialog(hwnd);
        UpdateRes();

        return 0;
    }

    LRESULT OnCtrlMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        HWND hwndCtrl = (HWND)wParam;
        MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl);
        if (pCtrl == NULL)
            return 0;

        if (pCtrl->m_nIndex < 0 || m_dialog_res.m_cItems <= pCtrl->m_nIndex)
            return 0;

        DebugPrintDx("OnCtrlMove: %d\n", pCtrl->m_nIndex);

        RECT rc;
        GetWindowRect(*pCtrl, &rc);
        MapWindowRect(NULL, m_rad_dialog, &rc);

        ClientToDialog(&rc);

        DialogItem& item = m_dialog_res.m_items[pCtrl->m_nIndex];
        item.m_pt.x = rc.left;
        item.m_pt.y = rc.top;

        UpdateRes();

        HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
        PostMessage(hwndOwner, MYWM_MOVESIZEREPORT,
            MAKEWPARAM(item.m_pt.x, item.m_pt.y),
            MAKELPARAM(item.m_siz.cx, item.m_siz.cy));

        InvalidateRect(m_rad_dialog, NULL, TRUE);

        return 0;
    }

    LRESULT OnCtrlSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        HWND hwndCtrl = (HWND)wParam;
        MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl);
        if (pCtrl == NULL)
            return 0;

        if (pCtrl->m_nIndex < 0 || m_dialog_res.m_cItems <= pCtrl->m_nIndex)
            return 0;

        DebugPrintDx("OnCtrlSize: %d\n", pCtrl->m_nIndex);

        RECT rc;
        GetWindowRect(*pCtrl, &rc);
        MapWindowRect(NULL, m_rad_dialog, &rc);

        ClientToDialog(&rc);

        DialogItem& item = m_dialog_res.m_items[pCtrl->m_nIndex];
        item.m_siz.cx = rc.right - rc.left;
        item.m_siz.cy = rc.bottom - rc.top;

        TCHAR szClass[64];
        GetClassName(hwndCtrl, szClass, _countof(szClass));
        if (lstrcmpi(szClass, TEXT("COMBOBOX")) == 0 ||
            lstrcmpi(szClass, WC_COMBOBOXEX) == 0)
        {
            item.m_siz.cy = m_settings.nComboHeight;
        }

        UpdateRes();

        HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
        PostMessage(hwndOwner, MYWM_MOVESIZEREPORT,
            MAKEWPARAM(item.m_pt.x, item.m_pt.y),
            MAKELPARAM(item.m_siz.cx, item.m_siz.cy));

        InvalidateRect(m_rad_dialog, NULL, TRUE);

        return 0;
    }

    void UpdateRes()
    {
        HWND hwndOwner = ::GetWindow(m_hwnd, GW_OWNER);
        PostMessage(hwndOwner, WM_COMMAND, CMDID_UPDATEDLGRES, 0);

        m_rad_dialog.ShowHideLabels(m_rad_dialog.m_index_visible);
    }

    LRESULT OnCtrlDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        HWND hwndCtrl = (HWND)wParam;
        MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl);
        if (pCtrl == NULL)
            return 0;

        if (pCtrl->m_nIndex == -1)
            return 0;

        m_dialog_res.m_items.erase(m_dialog_res.m_items.begin() + pCtrl->m_nIndex);
        m_dialog_res.m_cItems--;
        UpdateRes();

        return 0;
    }

    LRESULT OnDlgSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        RECT rc;
        GetWindowRect(m_rad_dialog, &rc);
        ClientToDialog(&rc);

        m_dialog_res.m_siz.cx = rc.right - rc.left;
        m_dialog_res.m_siz.cy = rc.bottom - rc.top;
        UpdateRes();

        HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
        PostMessage(hwndOwner, MYWM_MOVESIZEREPORT,
            MAKEWPARAM(rc.left, rc.top),
            MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));

        return 0;
    }

    void OnAddCtrl(HWND hwnd)
    {
        POINT pt = m_rad_dialog.m_ptClicked;
        ClientToDialog(&pt);
        if (pt.x < 0 || pt.y < 0)
            pt.x = pt.y = 0;

        RECT rc;
        GetClientRect(hwnd, &rc);
        if (rc.right - 30 < pt.x)
            pt.x = rc.right - 30;
        if (rc.bottom - 30 < pt.y)
            pt.y = rc.bottom - 30;

        MAddCtrlDlg dialog(m_dialog_res, m_rad_dialog.m_db, pt, m_settings);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            ReCreateRadDialog(hwnd);
            UpdateRes();
        }
    }

    void OnCtrlProp(HWND hwnd)
    {
        MCtrlPropDlg dialog(m_dialog_res, MRadCtrl::GetTargetIndeces(),
                            m_rad_dialog.m_db);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            ReCreateRadDialog(hwnd);
            UpdateRes();
        }
    }

    void OnDlgProp(HWND hwnd)
    {
        MDlgPropDlg dialog(m_dialog_res, m_rad_dialog.m_db);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            ReCreateRadDialog(hwnd);
            UpdateRes();
        }
    }

    void OnRefresh(HWND hwnd)
    {
        ReCreateRadDialog(hwnd);
        UpdateRes();
    }

    void OnShowHideIndex(HWND hwnd)
    {
        m_rad_dialog.m_index_visible = !m_rad_dialog.m_index_visible;
        m_rad_dialog.ShowHideLabels(m_rad_dialog.m_index_visible);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        HWND hwndOwner = ::GetWindow(m_hwnd, GW_OWNER);
        FORWARD_WM_COMMAND(hwndOwner, id, hwndCtl, codeNotify, SendMessage);
    }

    void OnTopAlign(HWND hwnd)
    {
        MRadCtrl::set_type set = MRadCtrl::GetTargets();
        if (set.size() < 2)
            return;

        INT nUp = INT_MAX;
        RECT rc;
        MRadCtrl::set_type::iterator it, end = set.end();
        for (it = set.begin(); it != end; ++it)
        {
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);
            if (rc.top < nUp)
                nUp = rc.top;
        }
        for (it = set.begin(); it != end; ++it)
        {
            MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(*it);
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);
            pCtrl->m_bMoving = TRUE;
            SetWindowPos(*it, NULL, rc.left, nUp, 0, 0,
                SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            pCtrl->m_bMoving = FALSE;
        }
    }

    void OnBottomAlign(HWND hwnd)
    {
        MRadCtrl::set_type set = MRadCtrl::GetTargets();
        if (set.size() < 2)
            return;

        INT nDown = INT_MIN;
        RECT rc;
        MRadCtrl::set_type::iterator it, end = set.end();
        for (it = set.begin(); it != end; ++it)
        {
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);
            if (nDown < rc.bottom)
                nDown = rc.bottom;
        }
        for (it = set.begin(); it != end; ++it)
        {
            MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(*it);
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);
            INT cy = rc.bottom - rc.top;
            pCtrl->m_bMoving = TRUE;
            SetWindowPos(*it, NULL, rc.left, nDown - cy, 0, 0,
                SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            pCtrl->m_bMoving = FALSE;
        }
    }

    void OnLeftAlign(HWND hwnd)
    {
        MRadCtrl::set_type set = MRadCtrl::GetTargets();
        if (set.size() < 2)
            return;

        INT nLeft = INT_MAX;
        RECT rc;
        MRadCtrl::set_type::iterator it, end = set.end();
        for (it = set.begin(); it != end; ++it)
        {
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);
            if (rc.left < nLeft)
                nLeft = rc.left;
        }
        for (it = set.begin(); it != end; ++it)
        {
            MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(*it);
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);
            pCtrl->m_bMoving = TRUE;
            SetWindowPos(*it, NULL, nLeft, rc.top, 0, 0,
                SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            pCtrl->m_bMoving = FALSE;
        }
    }

    void OnRightAlign(HWND hwnd)
    {
        MRadCtrl::set_type set = MRadCtrl::GetTargets();
        if (set.size() < 2)
            return;

        INT nRight = INT_MIN;
        RECT rc;
        MRadCtrl::set_type::iterator it, end = set.end();
        for (it = set.begin(); it != end; ++it)
        {
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);
            if (nRight < rc.right)
                nRight = rc.right;
        }
        for (it = set.begin(); it != end; ++it)
        {
            MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(*it);
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);
            INT cx = rc.right - rc.left;
            pCtrl->m_bMoving = TRUE;
            SetWindowPos(*it, NULL, nRight - cx, rc.top, 0, 0,
                SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            pCtrl->m_bMoving = FALSE;
        }
    }

    BOOL CanIndexTop() const
    {
        std::set<INT> indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty())
            return FALSE;

        INT iSelected = -1;
        INT iUnselected = -1;
        for (INT i = 0; i < m_dialog_res.m_cItems; ++i)
        {
            if (indeces.find(i) != indeces.end())
            {
                if (iUnselected != -1)
                    return TRUE;

                iSelected = i;
            }
            else
            {
                iUnselected = i;
            }
        }

        return FALSE;
    }

    void IndexTop(HWND hwnd)
    {
        std::set<INT> indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty())
            return;

        DialogItems items1, items2;
        for (INT i = 0; i < m_dialog_res.m_cItems; ++i)
        {
            if (indeces.find(i) == indeces.end())
            {
                items1.push_back(m_dialog_res.m_items[i]);
            }
            else
            {
                items2.push_back(m_dialog_res.m_items[i]);
            }
        }
        m_dialog_res.m_items = items1;
        m_dialog_res.m_items.insert(m_dialog_res.m_items.begin(), items2.begin(), items2.end());

        ReCreateRadDialog(hwnd);
        UpdateRes();
    }

    BOOL CanIndexBottom() const
    {
        std::set<INT> indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty())
            return FALSE;

        INT iSelected = -1;
        INT iUnselected = -1;
        for (INT i = m_dialog_res.m_cItems - 1; i >= 0; --i)
        {
            if (indeces.find(i) != indeces.end())
            {
                if (iUnselected != -1)
                    return TRUE;

                iSelected = i;
            }
            else
            {
                iUnselected = i;
            }
        }

        return FALSE;
    }

    void IndexBottom(HWND hwnd)
    {
        std::set<INT> indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty())
            return;

        DialogItems items1, items2;
        for (INT i = 0; i < m_dialog_res.m_cItems; ++i)
        {
            if (indeces.find(i) == indeces.end())
            {
                items1.push_back(m_dialog_res.m_items[i]);
            }
            else
            {
                items2.push_back(m_dialog_res.m_items[i]);
            }
        }
        m_dialog_res.m_items = items1;
        m_dialog_res.m_items.insert(m_dialog_res.m_items.end(), items2.begin(), items2.end());

        ReCreateRadDialog(hwnd);
        UpdateRes();
    }

    BOOL CanIndexMinus() const
    {
        std::set<INT> indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty() || indeces.count(0) > 0)
            return FALSE;

        return TRUE;
    }

    void IndexMinus(HWND hwnd)
    {
        std::set<INT> indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty())
            return;

        for (INT i = 0; i < m_dialog_res.m_cItems - 1; ++i)
        {
            if (indeces.find(i + 1) != indeces.end())
            {
                std::swap(m_dialog_res.m_items[i], m_dialog_res.m_items[i + 1]);
            }
        }

        ReCreateRadDialog(hwnd);
        UpdateRes();
    }

    BOOL CanIndexPlus() const
    {
        std::set<INT> indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty() || indeces.count(m_dialog_res.m_cItems - 1) > 0)
            return FALSE;

        return TRUE;
    }

    void IndexPlus(HWND hwnd)
    {
        std::set<INT> indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty())
            return;

        for (INT i = m_dialog_res.m_cItems - 1; i > 0; --i)
        {
            if (indeces.find(i - 1) != indeces.end())
            {
                std::swap(m_dialog_res.m_items[i], m_dialog_res.m_items[i - 1]);
            }
        }

        ReCreateRadDialog(hwnd);
        UpdateRes();
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
        HMENU hSubMenu = GetSubMenu(hMenu, 1);

        ::SetForegroundWindow(hwnd);
        ::TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
            xPos, yPos, 0, hwnd, NULL);
        ::PostMessage(hwnd, WM_NULL, 0, 0);
    }

    BOOL GetBaseUnits(INT& xDialogBaseUnit, INT& yDialogBaseUnit)
    {
        m_xDialogBaseUnit = m_dialog_res.GetBaseUnits(m_yDialogBaseUnit);
        if (m_xDialogBaseUnit == 0)
        {
            return FALSE;
        }

        xDialogBaseUnit = m_xDialogBaseUnit;
        yDialogBaseUnit = m_yDialogBaseUnit;
        m_rad_dialog.m_xDialogBaseUnit = m_xDialogBaseUnit;
        m_rad_dialog.m_yDialogBaseUnit = m_yDialogBaseUnit;

        return TRUE;
    }

    void OnMove(HWND hwnd, int x, int y)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);

        m_settings.nRadLeft = rc.left;
        m_settings.nRadTop = rc.top;
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        if (m_rad_dialog.m_bMovingSizing)
            return;

        m_dialog_res.Update();

        RECT rc;

        GetClientRect(m_hwnd, &rc);
        SIZE siz = SizeFromRectDx(&rc);
        m_rad_dialog.m_bMovingSizing = TRUE;
        MoveWindow(m_rad_dialog, 0, 0, siz.cx, siz.cy, TRUE);
        m_rad_dialog.m_bMovingSizing = FALSE;

        GetClientRect(m_rad_dialog, &rc);
        siz = SizeFromRectDx(&rc);
        ClientToDialog(&siz);
        m_dialog_res.m_siz = siz;

        UpdateRes();
    }

    void FitToGrid(POINT *ppt)
    {
        ppt->x = (ppt->x + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
        ppt->y = (ppt->y + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
    }
    void FitToGrid(SIZE *psiz)
    {
        psiz->cx = (psiz->cx + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
        psiz->cy = (psiz->cy + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
    }
    void FitToGrid(RECT *prc)
    {
        prc->left = (prc->left + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
        prc->top = (prc->top + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
        prc->right = (prc->right + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
        prc->bottom = (prc->bottom + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE;
    }

    void OnFitToGrid(HWND hwnd)
    {
        MRadCtrl::set_type::iterator it, end = MRadCtrl::GetTargets().end();
        for (it = MRadCtrl::GetTargets().begin(); it != end; ++it)
        {
            if (*it == m_rad_dialog)
                continue;

            MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(*it);
            if (pCtrl == NULL)
                continue;

            if (pCtrl->m_nIndex < 0 || m_dialog_res.m_cItems <= pCtrl->m_nIndex)
                continue;

            DialogItem& item = m_dialog_res.m_items[pCtrl->m_nIndex];
            FitToGrid(&item.m_pt);
            FitToGrid(&item.m_siz);

            POINT pt = item.m_pt;
            SIZE siz = item.m_siz;
            DebugPrintDx("PTSIZ: %d, %d, %d, %d\n", pt.x, pt.y, siz.cx, siz.cy);
            DialogToClient(&pt);
            DialogToClient(&siz);

            pCtrl->m_bLocking = TRUE;
            pCtrl->SetWindowPosDx(&pt, &siz);
            MRubberBand *pBand = pCtrl->GetRubberBand();
            if (pBand)
            {
                pBand->FitToTarget();
            }
            pCtrl->m_bLocking = FALSE;

            HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
            PostMessage(hwndOwner, MYWM_MOVESIZEREPORT,
                MAKEWPARAM(item.m_pt.x, item.m_pt.y),
                MAKELPARAM(item.m_siz.cx, item.m_siz.cy));
        }
        UpdateRes();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MRADWINDOW_HPP_
