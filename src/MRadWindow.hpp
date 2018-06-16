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
// constants

// user-defined window messages
#define MYWM_CTRLMOVE           (WM_USER + 100)     // control was moved
#define MYWM_CTRLSIZE           (WM_USER + 101)     // control was resized
#ifndef MYWM_SELCHANGE
    #define MYWM_SELCHANGE      (WM_USER + 102)     // selection was changed
#endif
#define MYWM_DLGSIZE            (WM_USER + 103)     // dialog was resized
#define MYWM_DELETESEL          (WM_USER + 104)     // selection was deleted
#define MYWM_MOVESIZEREPORT     (WM_USER + 105)     // report moving/resizing
#define MYWM_CLEARSTATUS        (WM_USER + 106)     // clear status
#define MYWM_COMPILECHECK       (WM_USER + 107)     // compilation check
#define MYWM_REOPENRAD          (WM_USER + 108)     // reopen the RADical window
#define MYWM_GETUNITS           (WM_USER + 109)     // get the dialog base units

#define GRID_SIZE   5   // grid size

//////////////////////////////////////////////////////////////////////////////
// MRadCtrl --- the RADical controls

class MRadCtrl : public MWindowBase
{
public:
    DWORD           m_dwMagic;          // magic number to verify the instance
    BOOL            m_bTopCtrl;         // is it a top control?
    HWND            m_hwndRubberBand;   // the rubber band window
    BOOL            m_bMoving;          // is it moving?
    BOOL            m_bSizing;          // is it resizing?
    BOOL            m_bLocking;         // is it locked?
    INT             m_nIndex;           // the control index
    POINT           m_pt;               // the position
    INT             m_nImageType;       // the image type

    // constructor
    MRadCtrl() : m_dwMagic(0xDEADFACE), m_bTopCtrl(FALSE), m_hwndRubberBand(NULL),
                 m_bMoving(FALSE), m_bSizing(FALSE), m_bLocking(FALSE), m_nIndex(-1)
    {
        m_pt.x = m_pt.y = -1;
        m_nImageType = 0;
    }

    // the default icon
    static HICON& Icon()
    {
        static HICON s_hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICO));
        return s_hIcon;
    }

    // the default bitmap
    static HBITMAP& Bitmap()
    {
        static HBITMAP s_hbm = LoadBitmap(GetModuleHandle(NULL),
                                          MAKEINTRESOURCE(IDB_BMP));
        return s_hbm;
    }

    // is the window a group box?
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

    // call me after subclassing
    void PostSubclass()
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

    // the targets (the selected window handles)
    typedef std::set<HWND> set_type;
    static set_type& GetTargets()
    {
        static set_type s_set;
        return s_set;
    }

    // the last selection (the selected window handle)
    static HWND& GetLastSel()
    {
        static HWND s_hwnd = NULL;
        return s_hwnd;
    }

    // get the target control indexes
    static std::set<INT> GetTargetIndeces()
    {
        set_type targets = MRadCtrl::GetTargets();

        std::set<INT> indeces;
        for (auto target : targets)
        {
            auto pCtrl = MRadCtrl::GetRadCtrl(target);
            indeces.insert(pCtrl->m_nIndex);
        }
        return indeces;
    }

    // the index-to-control mapping
    typedef std::map<INT, HWND> map_type;
    static map_type& IndexToCtrlMap()
    {
        static map_type s_map;
        return s_map;
    }

    // get the rubber band that is associated to the MRadCtrl
    MRubberBand *GetRubberBand()
    {
        MWindowBase *base = GetUserData(m_hwndRubberBand);
        if (base)
        {
            return static_cast<MRubberBand *>(base);
        }
        return NULL;
    }

    // get the MRadCtrl from a window handle
    static MRadCtrl *GetRadCtrl(HWND hwnd)
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
    static BOOL& GetRangeSelect(void)
    {
        static BOOL s_bRangeSelect = FALSE;
        return s_bRangeSelect;
    }

    // deselect the selection
    static BOOL DeselectSelection()
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
                pCtrl->m_hwndRubberBand = NULL;
            }
        }

        // clear the target set
        GetTargets().clear();

        // clear the last selection
        GetLastSel() = NULL;

        return bFound;  // found or not
    }

    // delete the selection
    static void DeleteSelection()
    {
        if (GetTargets().empty())
            return;

        // send MYWM_DELETESEL message to the parent of the target
        auto it = GetTargets().begin();
        PostMessage(GetParent(*it), MYWM_DELETESEL, 0, 0);
    }

    // deselect this control
    void Deselect()
    {
        // delete the rubber band of this control
        MRubberBand *band = GetRubberBand();
        if (band)
            DestroyWindow(*band);
        m_hwndRubberBand = NULL;

        // remove the control from targets
        GetTargets().erase(m_hwnd);

        // clear the last selection
        GetLastSel() = NULL;
    }

    // is it selected?
    BOOL IsSelected() const
    {
        return IsWindow(m_hwndRubberBand);
    }

    // select the control
    static void Select(HWND hwnd)
    {
        // get the RADical control instance
        auto pCtrl = GetRadCtrl(hwnd);
        if (pCtrl == NULL)
            return;

        // create the rubber band for the control
        auto band = new MRubberBand;
        band->CreateDx(GetParent(hwnd), hwnd, TRUE);
        pCtrl->m_hwndRubberBand = *band;

        // if not group box
        if (!MRadCtrl::IsGroupBox(hwnd))
        {
            // go to bottom! (for better hittest)
            SetWindowPosDx(hwnd, NULL, NULL, HWND_BOTTOM);
        }

        // add the handle to the targets
        GetTargets().insert(hwnd);

        // set the handle to the last selection
        GetLastSel() = hwnd;
    }

    // move the selected RADical controls
    static void MoveSelection(HWND hwnd, INT dx, INT dy)
    {
        // for each target
        for (auto target : GetTargets())
        {
            if (hwnd == target)
                continue;   // care the others only

            if (auto pCtrl = GetRadCtrl(target))    // RADical control?
            {
                // get the window rectangle relative to the parent
                RECT rc;
                GetWindowRect(*pCtrl, &rc);
                MapWindowPoints(NULL, ::GetParent(*pCtrl), (LPPOINT)&rc, 2);

                // move the offset by dx and dy
                OffsetRect(&rc, dx, dy);

                // move it
                pCtrl->m_bMoving = TRUE;
                pCtrl->SetWindowPosDx((LPPOINT)&rc);
                pCtrl->m_bMoving = FALSE;
            }
        }
    }

    // resize the selected RADical controls
    static void ResizeSelection(HWND hwnd, INT cx, INT cy)
    {
        // for each target
        for (auto target : GetTargets())
        {
            if (hwnd == target)
                continue;   // care the others only

            // is it a resizing RADical control?
            auto pCtrl = GetRadCtrl(target);
            if (pCtrl && !pCtrl->m_bSizing)
            {
                // resize it
                pCtrl->m_bSizing = TRUE;
                SIZE siz = { cx , cy };
                pCtrl->SetWindowPosDx(NULL, &siz);
                pCtrl->m_bSizing = FALSE;

                // also move the rubber band
                if (auto band = pCtrl->GetRubberBand();)
                {
                    band->FitToTarget();
                }
            }
        }
    }

    // range selection
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

        // is it a RADical control?
        if (auto pCtrl = GetRadCtrl(hwnd))
        {
            // is it a top control?
            if (!pCtrl->m_bTopCtrl)
                return;

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
    static void DoRangeSelect(HWND hwndParent, const RECT *prc, BOOL bCtrlDown)
    {
        RANGE_SELECT rs;
        rs.rc = *prc;
        rs.bCtrlDown = bCtrlDown;
        EnumChildWindows(hwndParent, RangeSelectProc, (LPARAM)&rs);
    }

    // the window procedure of MRadCtrl
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

    // MRadCtrl WM_ERASEBKGND
    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
    {
        // get the window class name of MRadCtrl
        WCHAR szClass[64];
        GetClassNameW(hwnd, szClass, 64);

        // special rendering for specific classes
        if (lstrcmpiW(szClass, TOOLBARCLASSNAMEW) == 0 ||
            lstrcmpiW(szClass, REBARCLASSNAMEW) == 0 ||
            lstrcmpiW(szClass, WC_PAGESCROLLERW) == 0)
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
    void OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
    {
        if (fDoubleClick)
            return;

        // emulate clicking to select the control
        OnNCLButtonDown(hwnd, FALSE, x, y, codeHitTest);
        OnNCLButtonUp(hwnd, x, y, codeHitTest);

        // send WM_NCRBUTTONDOWN to the parent
        SendMessage(GetParent(hwnd), WM_NCRBUTTONDOWN, (WPARAM)hwnd, MAKELPARAM(x, y));
    }

    // MRadCtrl WM_NCRBUTTONUP
    void OnNCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
    {
        // consume
    }

    // MRadCtrl WM_NCLBUTTONDOWN/WM_NCLBUTTONDBLCLK
    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        // consume
    }

    // MRadCtrl WM_LBUTTONUP
    void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
    {
        // consume
    }

    // MRadCtrl WM_MOUSEMOVE
    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
        // consume
    }

    // MRadCtrl WM_MOVE
    void OnMove(HWND hwnd, int x, int y)
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

            // move the rubber band
            if (auto band = GetRubberBand())
            {
                band->FitToTarget();
            }

            // redraw
            RECT rc;
            GetClientRect(hwnd, &rc);
            InvalidateRect(hwnd, &rc, TRUE);

            // send MYWM_CTRLMOVE to the parent
            SendMessage(GetParent(hwnd), MYWM_CTRLMOVE, (WPARAM)hwnd, 0);
        }
    }

    // MRadCtrl WM_SIZE
    void OnSize(HWND hwnd, UINT state, int cx, int cy)
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

            // send MYWM_CTRLSIZE to the parent
            SendMessage(GetParent(hwnd), MYWM_CTRLSIZE, (WPARAM)hwnd, 0);

            // redraw
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }

    // MRadCtrl WM_KEYDOWN/WM_KEYUP
    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        if (fDown)
        {
            // send WM_KEYDOWN to the parent
            FORWARD_WM_KEYDOWN(GetParent(hwnd), vk, cRepeat, flags, SendMessage);
        }
    }

    // MRadCtrl WM_NCLBUTTONDOWN/WM_NCLBUTTONDBLCLK
    void OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
    {
        if (hwnd != m_hwnd)
            return;     // invalid

        // update the position
        GetCursorPos(&m_pt);

        if (fDoubleClick)
            return;     // ignore double clicks

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

        // enable dragging by emulating the title bar dragging
        DefWindowProc(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(x, y));

        // if not group box
        if (!IsGroupBox(hwnd))
        {
            // go to bottom (for better hittest)
            SetWindowPosDx(hwnd, NULL, NULL, HWND_BOTTOM);
        }
    }

    // MRadCtrl WM_NCMOUSEMOVE
    void OnNCMouseMove(HWND hwnd, int x, int y, UINT codeHitTest)
    {
        // default processing
        DefWindowProc(hwnd, WM_NCMOUSEMOVE, codeHitTest, MAKELPARAM(x, y));
    }

    // MRadCtrl WM_NCLBUTTONUP
    void OnNCLButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
    {
        // finish moving
        m_bMoving = FALSE;

        // clear the position
        m_pt.x = -1;
        m_pt.y = -1;

        // default processing
        DefWindowProc(hwnd, WM_NCLBUTTONUP, codeHitTest, MAKELPARAM(x, y));
    }

    struct MYHITTEST
    {
        HWND hParent;
        HWND hCandidate;
        HWND hLast;
        HWND hGroupBox;
        POINT pt;
    };

    // the helper function for hittest
    static BOOL CALLBACK EnumHitTestChildProc(HWND hwnd, LPARAM lParam)
    {
        // NOTE: EnumChildWindows scans not only children but descendants.

        auto pmht = (MYHITTEST *)lParam;

        // get the window rectangle
        RECT rc;
        GetWindowRect(hwnd, &rc);

        // the point in the rectangle?
        if (!PtInRect(&rc, pmht->pt))
            return TRUE;    // if not, ignore

        // the control has a rubber band?
        if (auto pBand = MRubberBand::GetRubberBand(hwnd))
        {
            // the rubber band's target is the control?
            auto pCtrl = MRadCtrl::GetRadCtrl(pBand->m_hwndTarget);
            if (pCtrl && pCtrl->m_bTopCtrl)
            {
                // clear the candidate
                pmht->hCandidate = NULL;

                // if the window is sgroupbox
                if (IsGroupBox(*pCtrl))
                {
                    // set to hGroupBox
                    pmht->hGroupBox = *pCtrl;
                }
                else
                {
                    // otherwise, set to the last target
                    pmht->hLast = pBand->m_hwndTarget;
                }
            }
        }
        else if (auto pCtrl = MRadCtrl::GetRadCtrl(hwnd))   // is it a RADical control?
        {
            if (pCtrl->m_bTopCtrl)  // a top control
            {
                // set to the last target
                pmht->hLast = hwnd;

                // is it not group box?
                if (!IsGroupBox(hwnd))
                {
                    // it's a candidate
                    pmht->hCandidate = hwnd;
                }
            }
        }

        return TRUE;    // continue
    }

    // MRadCtrl WM_NCHITTEST
    UINT OnNCHitTest(HWND hwnd, int x, int y)
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

            // try to hittest
            EnumChildWindows(mht.hParent, EnumHitTestChildProc, (LPARAM)&mht);

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

    // called in WM_NCDESTROY
    virtual void PostNcDestroy()
    {
        if (m_bTopCtrl)
        {
            DebugPrintDx("MRadCtrl::PostNcDestroy: %p\n", m_hwnd);
        }
        MWindowBase::PostNcDestroy();

        // delete the MRadCtrl instance
        delete this;
    }
};

//////////////////////////////////////////////////////////////////////////////
// MRadDialog --- RADical dialog

class MRadDialog : public MDialogBase
{
public:
    BOOL            m_index_visible;        // indeces are visible
    POINT           m_ptClicked;            // the clicked position
    POINT           m_ptDragging;           // the dragging position
    MIndexLabels    m_labels;               // the labels
    BOOL            m_bMovingSizing;        // moving and/or resizing
    INT             m_xDialogBaseUnit;      // the X dialog base unit
    INT             m_yDialogBaseUnit;      // the Y dialog base unit
    HBRUSH          m_hbrBack;              // the background brush

    // contructor
    MRadDialog() : m_index_visible(FALSE), m_bMovingSizing(FALSE)
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
        hFont = ::CreateFontIndirect(&lf);
        m_labels.m_hFont = hFont;

        // create the background brush
        m_hbrBack = NULL;
        ReCreateBackBrush();
    }

    ~MRadDialog()
    {
        // delete the brush
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
        auto get_target = (GET_TARGET *)lParam;
        if (auto pCtrl = MRadCtrl::GetRadCtrl(hwnd))
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
            SetCapture(hwnd);
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
        ScreenToClient(hwnd, &m_ptClicked);

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
        MRadCtrl *pCtrl = new MRadCtrl();
        pCtrl->SubclassDx(hCtrl);
        pCtrl->m_bTopCtrl = (nIndex != -1);
        pCtrl->m_nIndex = nIndex;

        if (nIndex != -1)
        {
            MRadCtrl::IndexToCtrlMap()[nIndex] = hCtrl;
        }

        pCtrl->PostSubclass();

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
    INT             m_xDialogBaseUnit;
    INT             m_yDialogBaseUnit;
    MRadDialog      m_rad_dialog;
    DialogRes       m_dialog_res;
    HICON           m_hIcon;
    HICON           m_hIconSm;
    MTitleToBitmap  m_title_to_bitmap;
    MTitleToIcon    m_title_to_icon;
    DialogItemClipboard m_clipboard;

    MRadWindow()
        : m_xDialogBaseUnit(0), m_yDialogBaseUnit(0),
          m_hIcon(NULL), m_hIconSm(NULL),
          m_clipboard(m_dialog_res)
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
                    g_res.do_icon(m_title_to_icon, item, lang);
                }
                else if ((item.m_style & SS_TYPEMASK) == SS_BITMAP)
                {
                    g_res.do_bitmap(m_title_to_bitmap, item, lang);
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

    BOOL ReCreateRadDialog(HWND hwnd, INT nSelectStartIndex = -1)
    {
        assert(IsWindow(hwnd));

        BOOL bMovingSizingOld = m_rad_dialog.m_bMovingSizing;
        m_rad_dialog.m_bMovingSizing = TRUE;
        if (m_rad_dialog)
        {
            DestroyWindow(m_rad_dialog);
        }

        m_dialog_res.Fixup(false);
        std::vector<BYTE> data = m_dialog_res.data();
#if 0
        MFile file(TEXT("modified.bin"), TRUE);
        DWORD cbWritten;
        file.WriteFile(&data[0], (DWORD)data.size(), &cbWritten);
#endif
        m_dialog_res.Fixup(true);

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

        if (nSelectStartIndex != -1)
        {
            HWND hwndNext = MRadDialog::GetFirstCtrl(hwnd);
            while (hwndNext)
            {
                if (MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(hwndNext))
                {
                    if (pCtrl->m_nIndex >= nSelectStartIndex)
                        MRadCtrl::Select(hwndNext);
                }
                hwndNext = m_rad_dialog.GetNextCtrl(hwndNext);
            }
        }

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
                    MIdOrString title = m_dialog_res[pCtrl->m_nIndex].m_title;
                    if (HICON hIcon = m_title_to_icon[title])
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
                    MIdOrString title = m_dialog_res[pCtrl->m_nIndex].m_title;
                    if (HBITMAP hbm = m_title_to_bitmap[title])
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

        return ReCreateRadDialog(hwnd);
    }

    void OnDestroy(HWND hwnd)
    {
        HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
        PostMessage(hwndOwner, WM_COMMAND, ID_DESTROYRAD, 0);

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
            DialogItem& item = m_dialog_res[pCtrl->m_nIndex];
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

        if (CanIndexTop())
        {
            EnableMenuItem(hMenu, ID_CTRLINDEXTOP, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_CTRLINDEXTOP, MF_GRAYED);
        }

        if (CanIndexBottom())
        {
            EnableMenuItem(hMenu, ID_CTRLINDEXBOTTOM, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_CTRLINDEXBOTTOM, MF_GRAYED);
        }

        if (CanIndexMinus())
        {
            EnableMenuItem(hMenu, ID_CTRLINDEXMINUS, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_CTRLINDEXMINUS, MF_GRAYED);
        }

        if (CanIndexPlus())
        {
            EnableMenuItem(hMenu, ID_CTRLINDEXPLUS, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_CTRLINDEXPLUS, MF_GRAYED);
        }

        if (m_clipboard.IsAvailable())
        {
            EnableMenuItem(hMenu, ID_PASTE, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_PASTE, MF_GRAYED);
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

        DialogItem& item = m_dialog_res[pCtrl->m_nIndex];
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

        DialogItem& item = m_dialog_res[pCtrl->m_nIndex];
        item.m_siz.cx = rc.right - rc.left;
        item.m_siz.cy = rc.bottom - rc.top;

        TCHAR szClass[64];
        GetClassName(hwndCtrl, szClass, _countof(szClass));
        if (lstrcmpi(szClass, TEXT("COMBOBOX")) == 0 ||
            lstrcmpi(szClass, WC_COMBOBOXEX) == 0)
        {
            item.m_siz.cy = g_settings.nComboHeight;
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
        PostMessage(hwndOwner, WM_COMMAND, ID_UPDATEDLGRES, 0);

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

        MAddCtrlDlg dialog(m_dialog_res, pt);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            ReCreateRadDialog(hwnd);
            UpdateRes();
        }
    }

    void OnCtrlProp(HWND hwnd)
    {
        MCtrlPropDlg dialog(m_dialog_res, MRadCtrl::GetTargetIndeces());
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            ReCreateRadDialog(hwnd);
            UpdateRes();
        }
    }

    void OnDlgProp(HWND hwnd)
    {
        MDlgPropDlg dialog(m_dialog_res);
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

    BOOL GetSelectedItems(DialogItems& items)
    {
        std::set<INT> indeces = MRadCtrl::GetTargetIndeces();
        std::set<INT>::const_iterator it, end = indeces.end();
        for (it = indeces.begin(); it != end; ++it)
        {
            DialogItem& item = m_dialog_res[*it];
            items.push_back(item);
        }
        return !items.empty();
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
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
                for (size_t i = 0; i < items.size(); ++i)
                {
                    items[i].m_pt.x += s_nShift;
                    items[i].m_pt.y += s_nShift;
                }

                INT nIndex = INT(m_dialog_res.m_items.size());
                for (size_t i = 0; i < items.size(); ++i)
                {
                    m_dialog_res.m_cItems++;
                    m_dialog_res.m_items.push_back(items[i]);
                }
                ReCreateRadDialog(hwnd, nIndex);
                UpdateRes();
            }
            return;
        }

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
                items1.push_back(m_dialog_res[i]);
            }
            else
            {
                items2.push_back(m_dialog_res[i]);
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
                items1.push_back(m_dialog_res[i]);
            }
            else
            {
                items2.push_back(m_dialog_res[i]);
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
                std::swap(m_dialog_res[i], m_dialog_res[i + 1]);
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
                std::swap(m_dialog_res[i], m_dialog_res[i - 1]);
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
                PostMessageDx(WM_COMMAND, ID_COPY);
            }
            break;
        case 'D':
            if (GetAsyncKeyState(VK_CONTROL) < 0)
            {
                // Ctrl+D
                PostMessageDx(WM_COMMAND, ID_SHOWHIDEINDEX);
            }
            break;
        case 'V':
            if (GetAsyncKeyState(VK_CONTROL) < 0)
            {
                // Ctrl+V
                PostMessageDx(WM_COMMAND, ID_PASTE);
            }
            break;
        case 'X':
            if (GetAsyncKeyState(VK_CONTROL) < 0)
            {
                // Ctrl+X
                PostMessageDx(WM_COMMAND, ID_CUT);
            }
            break;
        default:
            return;
        }
    }

    void SelectAll(HWND hwnd)
    {
        MRadCtrl::DeselectSelection();
        HWND hwndNext = MRadDialog::GetFirstCtrl(hwnd);
        while (hwndNext)
        {
            MRadCtrl::Select(hwndNext);
            hwndNext = m_rad_dialog.GetNextCtrl(hwndNext);
        }
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        PopupMenuDx(hwnd, hwndContext, IDR_POPUPMENUS, 1, xPos, yPos);
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

        g_settings.nRadLeft = rc.left;
        g_settings.nRadTop = rc.top;
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        if (m_rad_dialog.m_bMovingSizing)
            return;

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

            DialogItem& item = m_dialog_res[pCtrl->m_nIndex];
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
