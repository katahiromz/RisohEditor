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

#pragma once

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
#include <unordered_set>     // for std::unordered_set
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
#define MYWM_UPDATEDLGRES       (WM_USER + 110)     // update the dialog res
#define MYWM_REDRAW             (WM_USER + 111)     // redraw MRadDialog

#define GRID_SIZE   5   // grid size

//////////////////////////////////////////////////////////////////////////////
// MRadCtrl --- the RADical controls
// NOTE: An MRadCtrl is a dialog control, subclassed by MRadDialog.

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
    typedef std::unordered_set<HWND> set_type;
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
    static std::unordered_set<INT> GetTargetIndeces()
    {
        set_type targets = MRadCtrl::GetTargets();

        std::unordered_set<INT> indeces;
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
        SendMessage(GetParent(*it), MYWM_DELETESEL, 0, 0);
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
            // go to bottom! (for better hittest & drawing)
            SetWindowPosDx(hwnd, NULL, NULL, HWND_BOTTOM);

            // NOTE: The index top control is drawed on background. The index bottom control is drawed on foreground.
        }

        // add the handle to the targets
        GetTargets().insert(hwnd);

        // set the handle to the last selection
        GetLastSel() = hwnd;
    }

    static void SelectByIndex(INT nIndex)
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

        PostMessage(GetParent(hwnd), MYWM_REDRAW, 0, 0);
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
                if (auto band = pCtrl->GetRubberBand())
                {
                    band->FitToTarget();
                }
            }
        }

        PostMessage(GetParent(hwnd), MYWM_REDRAW, 0, 0);
    }

    // range selection
    struct RANGE_SELECT
    {
        RECT rc;
        BOOL bCtrlDown;
    };

    // callback function for DoRangeSelect
    static BOOL CALLBACK
    RangeSelectProc(HWND hwnd, LPARAM lParam)
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
    static void DoRangeSelect(HWND hwndParent, const RECT *prc, BOOL bCtrlDown)
    {
        RANGE_SELECT rs;
        rs.rc = *prc;
        rs.bCtrlDown = bCtrlDown;
        ::EnumChildWindows(hwndParent, RangeSelectProc, (LPARAM)&rs);
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
            HANDLE_MESSAGE(hwnd, MYWM_REDRAW, OnRedraw);
            case WM_MOVING: case WM_SIZING:
                return 0;
        }
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    // MRadCtrl MYWM_REDRAW
    LRESULT OnRedraw(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        PostMessage(GetParent(hwnd), MYWM_REDRAW, 0, 0);
        return 0;
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
        // eat
    }

    // MRadCtrl WM_LBUTTONDOWN/WM_LBUTTONDBLCLK
    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        // eat
    }

    // MRadCtrl WM_LBUTTONUP
    void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
    {
        // eat
    }

    // MRadCtrl WM_MOUSEMOVE
    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
        // eat
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
        if (!IsGroupBox(hwnd) && IsWindow(hwnd))
        {
            // go to bottom (for better hittest & drawing)
            SetWindowPosDx(hwnd, NULL, NULL, HWND_BOTTOM);

            // NOTE: The index top control is drawed on background. The index bottom control is drawed on foreground.
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

    void DoTest()
    {
        WCHAR szText[256];
        MStringW str = GetWindowTextW();
        StringCchPrintfW(szText, _countof(szText),
            L"MRadCtrl:%p, m_hwnd:%p, m_dwMagic:0x%08X, m_bTopCtrl:%d, m_nIndex:%d, "
            L"str:%s, IndexToCtrlMap()[m_nIndex]: %p",
            this, m_hwnd, m_dwMagic, m_bTopCtrl, m_nIndex,
            str.c_str(), IndexToCtrlMap()[m_nIndex]
        );

        MessageBoxW(NULL, szText, L"MRadCtrl::DoTest()", MB_ICONINFORMATION);
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
    BOOL            m_bMovingSizing;        // the lock of moving and/or resizing
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

    // the target types to get
    enum TARGET_TYPE
    {
        TARGET_NEXT,        // get the next target
        TARGET_PREV,        // get the previous target
        TARGET_FIRST,       // get the first target
        TARGET_LAST         // get the last target
    };

    // the structure to get the target control
    struct GET_TARGET
    {
        TARGET_TYPE target;
        HWND hwndTarget;
        INT m_nIndex;
        INT m_nCurrentIndex;
    };

    // EnumChildWindows' callback function to get the target
    static BOOL CALLBACK GetTargetProc(HWND hwnd, LPARAM lParam)
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

    HWND GetNextCtrl(HWND hwndCtrl) const
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

    HWND GetPrevCtrl(HWND hwndCtrl) const
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

    // the dialog procedure of MRadDialog
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
            HANDLE_MESSAGE(hwnd, MYWM_REDRAW, OnRedraw);
        }
        return 0;
    }

    // MRadDialog WM_RBUTTONDOWN
    void OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
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
        SendMessage(GetParent(hwnd), MYWM_SELCHANGE, 0, 0);
    }

    // MRadDialog MYWM_REDRAW
    LRESULT OnRedraw(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    // MRadDialog MYWM_SELCHANGE
    LRESULT OnSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        // notify MYWM_SELCHANGE to the parent
        SendMessage(GetParent(hwnd), MYWM_SELCHANGE, wParam, lParam);
        return 0;
    }

    // get the normalized rectangle from two points
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

    // draw the dragging rectangle
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

    // MRadDialog WM_LBUTTONDOWN/WM_LBUTTONDBLCLK
    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        // update the clicked position
        m_ptClicked.x = x;
        m_ptClicked.y = y;

        // if not [Shift] nor [Ctrl] pressed
        if (::GetKeyState(VK_SHIFT) >= 0 && ::GetKeyState(VK_CONTROL) >= 0)
        {
            // deselect the controls
            MRadCtrl::DeselectSelection();
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
        SendMessage(GetParent(hwnd), MYWM_SELCHANGE, 0, 0);
    }

    // MRadDialog WM_MOUSEMOVE
    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
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
    void OnCaptureChanged(HWND hwnd)
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
    void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
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
        SendMessage(GetParent(hwnd), MYWM_SELCHANGE, 0, 0);
    }

    // MRadDialog WM_ERASEBKGND
    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
    {
        // create the background brush if necessary
        if (m_hbrBack == NULL)
            ReCreateBackBrush();

        // get the client rectangle
        RECT rc;
        GetClientRect(hwnd, &rc);

        // fill the rectangle by the brush
        FillRect(hdc, &rc, m_hbrBack);

        return TRUE;    // processed
    }

    // the window procedure of MRadDialog
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

    // MRadDialog WM_SIZE
    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        // moving or resizing?
        if (m_bMovingSizing)
            return;     // ignore

        // send MYWM_DLGSIZE to the parent
        SendMessage(GetParent(hwnd), MYWM_DLGSIZE, 0, 0);
    }

    // MRadDialog MYWM_DELETESEL
    LRESULT OnDeleteSel(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        // send MYWM_DELETESEL to the parent
        SendMessage(GetParent(hwnd), MYWM_DELETESEL, wParam, lParam);
        return 0;
    }

    // MRadDialog MYWM_CTRLMOVE
    LRESULT OnCtrlMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        // send MYWM_CTRLMOVE to the parent
        SendMessage(GetParent(hwnd), MYWM_CTRLMOVE, wParam, lParam);
        return 0;
    }

    // MRadDialog MYWM_CTRLSIZE
    LRESULT OnCtrlSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        // send MYWM_CTRLSIZE to the parent
        SendMessage(GetParent(hwnd), MYWM_CTRLSIZE, wParam, lParam);
        return 0;
    }

    // MRadDialog WM_NCLBUTTONDOWN/WM_NCLBUTTONDBLCLK
    void OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
    {
        // eat
    }

    // MRadDialog WM_NCLBUTTONUP
    void OnNCLButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
    {
        // eat
    }

    // MRadDialog WM_NCRBUTTONDOWN/WM_NCRBUTTONDBLCLK
    void OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
    {
        if (fDoubleClick)
            return;

        // update the clicked position
        m_ptClicked.x = x;
        m_ptClicked.y = y;
        // into screen coordinates
        ScreenToClient(hwnd, &m_ptClicked);

        // send WM_CONTEXTMENU to the parent
        FORWARD_WM_CONTEXTMENU(GetParent(hwnd), hwnd, x, y, SendMessage);
    }

    // MRadDialog WM_NCRBUTTONUP
    void OnNCRButtonUp(HWND hwnd, int x, int y, UINT codeHitTest)
    {
        // eat
    }

    // MRadDialog WM_NCMOUSEMOVE
    void OnNCMouseMove(HWND hwnd, int x, int y, UINT codeHitTest)
    {
        // eat
    }

    // MRadDialog WM_KEYDOWN/WM_KEYUP
    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        if (fDown)
        {
            // send it to the parent
            FORWARD_WM_KEYDOWN(GetParent(hwnd), vk, cRepeat, flags, SendMessage);
        }
    }

    // NOTE: We have to do subclassing all the children controls and their descendants
    //       to modify the hittesting.

    // do subclassing a control and its descendants
    void DoSubclass(HWND hCtrl, INT nIndex)
    {
        // make it an MRadCtrl
        MRadCtrl *pCtrl = new MRadCtrl();
        pCtrl->SubclassDx(hCtrl);
        pCtrl->m_bTopCtrl = (nIndex != -1);
        pCtrl->m_nIndex = nIndex;

        if (nIndex != -1)   // a top control?
        {
            // update the index-to-control mapping
            MRadCtrl::IndexToCtrlMap()[nIndex] = hCtrl;
        }

        pCtrl->PostSubclass();

#ifndef NDEBUG
        MString text = GetWindowText(hCtrl);
        DebugPrintDx(TEXT("MRadCtrl::DoSubclass: %p, %d, '%s'\n"), hCtrl, nIndex, text.c_str());
#endif

        // do subclassing its children
        DoSubclassChildren(hCtrl);
    }

    // do subclassing the children
    void DoSubclassChildren(HWND hwnd, BOOL bTop = FALSE)
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
    BOOL ReCreateBackBrush()
    {
        // delete the previous
        if (m_hbrBack)
        {
            DeleteObject(m_hbrBack);
            m_hbrBack = NULL;
        }

        // 3D face collor
        COLORREF rgb = GetSysColor(COLOR_3DFACE);

        // calculate another color
        DWORD dwTotal = GetRValue(rgb) + GetGValue(rgb) + GetBValue(rgb);
        rgb = (dwTotal < 255) ? RGB(255, 255, 255) : RGB(0, 0, 0);

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
    void OnSysColorChange(HWND hwnd)
    {
        // recreate the back brush
        ReCreateBackBrush();

        // redraw
        InvalidateRect(hwnd, NULL, TRUE);
    }

    // MRadDialog WM_INITDIALOG
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
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

        // do subclassing the children of this dialog
        DoSubclassChildren(hwnd, TRUE);

        // if indeces are visible
        if (m_index_visible)
            ShowHideLabels(TRUE);   // show the labels

        // do subclassing this dialog
        SubclassDx(hwnd);

        return FALSE;
    }

    // show/hide the labels
    void ShowHideLabels(BOOL bShow = TRUE)
    {
        m_index_visible = bShow;
        if (bShow)
            m_labels.ReCreate(m_hwnd, MRadCtrl::IndexToCtrlMap());
        else
            m_labels.Destroy();
    }
};

//////////////////////////////////////////////////////////////////////////////
// MRadWindow --- the RADical window
// NOTE: An MRadWindow contains an MRadDialog.

class MRadWindow : public MWindowBase
{
public:
    INT             m_xDialogBaseUnit;      // the X dialog base unit
    INT             m_yDialogBaseUnit;      // the Y dialog base unit
    MRadDialog      m_rad_dialog;           // the MRadDialog instance
    DialogRes       m_dialog_res;           // the dialog resource
    HICON           m_hIcon;                // the icon
    HICON           m_hIconSm;              // the small icon
    MTitleToBitmap  m_title_to_bitmap;      // a title-to-bitmap mapping
    MTitleToIcon    m_title_to_icon;        // a title-to-icon mapping
    DialogItemClipboard m_clipboard;        // a clipboard manager

    // constructor
    MRadWindow() : m_xDialogBaseUnit(0), m_yDialogBaseUnit(0),
          m_hIcon(NULL), m_hIconSm(NULL), m_clipboard(m_dialog_res)
    {
    }

    // create the mappings
    void create_maps(WORD lang)
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
    void clear_maps()
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

    // destructor
    ~MRadWindow()
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

    // create an MRadWindow window
    BOOL CreateDx(HWND hwndParent)
    {
        // lock the moving/resizing
        BOOL bMovingSizing = m_rad_dialog.m_bMovingSizing;
        m_rad_dialog.m_bMovingSizing = TRUE;

        // create the window
        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
        if (CreateWindowDx(hwndParent, MAKEINTRESOURCE(IDS_RADWINDOW), style))
        {
            // show it
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
    void ClientToDialog(POINT *ppt)
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        ppt->x = (ppt->x * 4) / m_xDialogBaseUnit;
        ppt->y = (ppt->y * 8) / m_yDialogBaseUnit;
    }

    // convert the coordinates
    void ClientToDialog(SIZE *psiz)
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        psiz->cx = (psiz->cx * 4) / m_xDialogBaseUnit;
        psiz->cy = (psiz->cy * 8) / m_yDialogBaseUnit;
    }

    // convert the coordinates
    void ClientToDialog(RECT *prc)
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        prc->left = (prc->left * 4) / m_xDialogBaseUnit;
        prc->right = (prc->right * 4) / m_xDialogBaseUnit;
        prc->top = (prc->top * 8) / m_yDialogBaseUnit;
        prc->bottom = (prc->bottom * 8) / m_yDialogBaseUnit;
    }

    // convert the coordinates
    void DialogToClient(POINT *ppt)
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        ppt->x = (ppt->x * m_xDialogBaseUnit + 2) / 4;
        ppt->y = (ppt->y * m_yDialogBaseUnit + 4) / 8;
    }

    // convert the coordinates
    void DialogToClient(SIZE *psiz)
    {
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        psiz->cx = (psiz->cx * m_xDialogBaseUnit + 2) / 4;
        psiz->cy = (psiz->cy * m_yDialogBaseUnit + 4) / 8;
    }

    // convert the coordinates
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

    // adjust MRadWindow's position and size to MRadDialog's client area
    void FitToRadDialog()
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
    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("katahiromz's MRadWindow Class");
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        // no class icon
        wcx.hIcon = NULL;
        wcx.hIconSm = NULL;

        // dark gray background
        wcx.hbrBackground = GetStockBrush(DKGRAY_BRUSH);
    }

    // recreate the MRadDialog window
    BOOL ReCreateRadDialog(HWND hwnd, INT nSelectStartIndex = -1)
    {
        assert(IsWindow(hwnd));

        // lock moving/resizing
        BOOL bMovingSizingOld = m_rad_dialog.m_bMovingSizing;
        m_rad_dialog.m_bMovingSizing = TRUE;

        // destroy MRadDialog window if any
        if (m_rad_dialog)
        {
            DestroyWindow(m_rad_dialog);
        }

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

        // unlock
        m_rad_dialog.m_bMovingSizing = bMovingSizingOld;

        // show the dialog
        ShowWindow(m_rad_dialog, SW_SHOWNOACTIVATE);
        UpdateWindow(m_rad_dialog);

        // make it foreground
        SetForegroundWindow(hwnd);

        // update the mappings
        update_maps();

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

        // notify MYWM_SELCHANGE to the parent
        SendMessage(GetParent(hwnd), MYWM_SELCHANGE, 0, 0);

        return TRUE;
    }

    // update the mappings
    void update_maps()
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

                        // resize
                        SetWindowPosDx(hCtrl, NULL, &siz);
                    }
                }
            }
        }
    }

    // MRadWindow WM_CREATE
    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        // create the icons
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);

        // set the icons
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

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
    void OnDestroy(HWND hwnd)
    {
        // post ID_DESTROYRAD to the owner
        HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
        PostMessage(hwndOwner, WM_COMMAND, ID_DESTROYRAD, 0);

        // notify selection change to the owner
        MRadCtrl::GetTargetIndeces().clear();
        SendMessage(hwndOwner, MYWM_SELCHANGE, 0, 0);

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

    // MRadWindow WM_SYSCOLORCHANGE
    void OnSysColorChange(HWND hwnd)
    {
        m_rad_dialog.SendMessageDx(WM_SYSCOLORCHANGE);
    }

    // MRadWindow MYWM_GETUNITS
    LRESULT OnGetUnits(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        // store the dialog base units
        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);
        return 0;
    }

    // MRadWindow WM_ACTIVATE
    void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
    {
        if (state == WA_ACTIVE || state == WA_CLICKACTIVE)
        {
            // check whether compilation requires or not
            HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
            if (!SendMessage(hwndOwner, MYWM_COMPILECHECK, (WPARAM)hwnd, 0))
            {
                return;
            }

            // reopen MRadWindow if necessary
            SendMessage(hwndOwner, MYWM_REOPENRAD, 0, 0);
        }

        // default processing
        FORWARD_WM_ACTIVATE(hwnd, state, hwndActDeact, fMinimized, DefWindowProc);
    }

    // MRadWindow MYWM_SELCHANGE
    LRESULT OnSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        // get the owner window
        HWND hwndOwner = GetWindow(hwnd, GW_OWNER);

        // is there the last selection?
        HWND hwndSel = MRadCtrl::GetLastSel();
        if (hwndSel == NULL)    // no
        {
            // report the selection change to the owner window
            SendMessage(hwndOwner, MYWM_SELCHANGE, 0, 0);
        
            // clear the status
            SendMessage(hwndOwner, MYWM_CLEARSTATUS, 0, 0);

            return 0;
        }

        // get the MRadCtrl pointer
        auto pCtrl = MRadCtrl::GetRadCtrl(hwndSel);
        if (pCtrl == NULL)
        {
            // report the selection change to the owner window
            SendMessage(hwndOwner, MYWM_SELCHANGE, 0, 0);

            // clear the status
            SendMessage(hwndOwner, MYWM_CLEARSTATUS, 0, 0);
            return 0;
        }

        // check the index
        if (size_t(pCtrl->m_nIndex) < m_dialog_res.m_items.size())
        {
            // report the position and size of the index
            DialogItem& item = m_dialog_res[pCtrl->m_nIndex];
            SendMessage(hwndOwner, MYWM_MOVESIZEREPORT, 
                MAKEWPARAM(item.m_pt.x, item.m_pt.y),
                MAKELPARAM(item.m_siz.cx, item.m_siz.cy));
        }

        // report the selection change to the owner window
        SendMessage(hwndOwner, MYWM_SELCHANGE, 0, 0);
        return 0;
    }

    // MRadWindow WM_INITMENUPOPUP
    void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
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

    // MRadWindow MYWM_DELETESEL
    LRESULT OnDeleteSel(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        // delete the selected dialog items from m_dialog_res
        auto indeces = MRadCtrl::GetTargetIndeces();
        for (size_t i = m_dialog_res.size(); i > 0;)
        {
            --i;
            if (indeces.find(i) != indeces.end())
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
    LRESULT OnCtrlMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        HWND hwndCtrl = (HWND)wParam;

        auto pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl);
        if (pCtrl == NULL)
            return 0;   // invalid

        // check the index
        if (pCtrl->m_nIndex < 0 || m_dialog_res.m_cItems <= pCtrl->m_nIndex)
            return 0;   // invalid

        DebugPrintDx("OnCtrlMove: %d\n", pCtrl->m_nIndex);

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
        SendMessage(hwndOwner, MYWM_MOVESIZEREPORT,
            MAKEWPARAM(item.m_pt.x, item.m_pt.y),
            MAKELPARAM(item.m_siz.cx, item.m_siz.cy));

        // redraw
        PostMessage(m_rad_dialog, MYWM_REDRAW, 0, 0);

        return 0;
    }

    // MRadWindow MYWM_CTRLSIZE
    LRESULT OnCtrlSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        HWND hwndCtrl = (HWND)wParam;
        auto pCtrl = MRadCtrl::GetRadCtrl(hwndCtrl);
        if (pCtrl == NULL)
            return 0;   // invalid

        // check the index
        if (pCtrl->m_nIndex < 0 || m_dialog_res.m_cItems <= pCtrl->m_nIndex)
            return 0;   // invalid

        DebugPrintDx("OnCtrlSize: %d\n", pCtrl->m_nIndex);

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
        TCHAR szClass[64];
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
        SendMessage(hwndOwner, MYWM_MOVESIZEREPORT,
            MAKEWPARAM(item.m_pt.x, item.m_pt.y),
            MAKELPARAM(item.m_siz.cx, item.m_siz.cy));

        // redraw
        PostMessage(m_rad_dialog, MYWM_REDRAW, 0, 0);

        return 0;
    }

    // update the resource
    void UpdateRes()
    {
        // notify the update of dialog resource to the owner window
        HWND hwndOwner = ::GetWindow(m_hwnd, GW_OWNER);
        SendMessage(hwndOwner, MYWM_UPDATEDLGRES, 0, 0);

        // redraw the labels
        m_rad_dialog.ShowHideLabels(m_rad_dialog.m_index_visible);
    }

    // MRadWindow MYWM_DLGSIZE
    LRESULT OnDlgSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
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
        SendMessage(hwndOwner, MYWM_MOVESIZEREPORT,
            MAKEWPARAM(rc.left, rc.top),
            MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));

        return 0;
    }

    // called from MMainWnd WM_COMMAND ID_ADDCTRL
    void OnAddCtrl(HWND hwnd)
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
    void OnCtrlProp(HWND hwnd)
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
    void OnDlgProp(HWND hwnd)
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
    void OnRefresh(HWND hwnd)
    {
        // recreate MRadDialog
        ReCreateRadDialog(hwnd);

        // update the resource
        UpdateRes();
    }

    // show/hide the indeces
    void OnShowHideIndex(HWND hwnd)
    {
        m_rad_dialog.m_index_visible = !m_rad_dialog.m_index_visible;
        m_rad_dialog.ShowHideLabels(m_rad_dialog.m_index_visible);
    }

    // get the selected dialog items
    BOOL GetSelectedItems(DialogItems& items)
    {
        auto indeces = MRadCtrl::GetTargetIndeces();
        auto end = indeces.end();
        for (auto it = indeces.begin(); it != end; ++it)
        {
            DialogItem& item = m_dialog_res[*it];
            items.push_back(item);
        }
        return !items.empty();
    }

    // MRadWindow WM_COMMAND
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

                for (size_t i = 0; i < items.size(); ++i)
                {
                    m_dialog_res.m_cItems++;
                    m_dialog_res.m_items.push_back(items[i]);
                }

                OnRefresh(hwnd);
            }
            return;
        }

        // notify WM_COMMAND to the owner window
        HWND hwndOwner = ::GetWindow(m_hwnd, GW_OWNER);
        FORWARD_WM_COMMAND(hwndOwner, id, hwndCtl, codeNotify, SendMessage);
    }

    // called from MMainWnd WM_COMMAND ID_TOPALIGN
    void OnTopAlign(HWND hwnd)
    {
        auto set = MRadCtrl::GetTargets();
        if (set.size() < 2)
            return;     

        RECT rc;

        // the highest Y coordinate --> nUp
        INT nUp = INT_MAX;
        auto end = set.end();
        for (auto it = set.begin(); it != end; ++it)
        {
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);
            if (rc.top < nUp)
                nUp = rc.top;
        }

        // move the selected controls to the highest Y coordinate
        for (auto it = set.begin(); it != end; ++it)
        {
            // get the coordinates of the control
            auto pCtrl = MRadCtrl::GetRadCtrl(*it);
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);

            // move it
            pCtrl->m_bMoving = TRUE;
            SetWindowPos(*it, NULL, rc.left, nUp, 0, 0,
                SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            pCtrl->m_bMoving = FALSE;
        }
    }

    // called from MMainWnd WM_COMMAND ID_BOTTOMALIGN
    void OnBottomAlign(HWND hwnd)
    {
        auto set = MRadCtrl::GetTargets();
        if (set.size() < 2)
            return;

        RECT rc;

        // the lowest Y coordinate --> nDown
        INT nDown = INT_MIN;
        auto end = set.end();
        for (auto it = set.begin(); it != end; ++it)
        {
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);
            if (nDown < rc.bottom)
                nDown = rc.bottom;
        }

        // move the selected controls to the lowest Y coordinate
        for (auto it = set.begin(); it != end; ++it)
        {
            // get the coordinates of the control
            MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(*it);
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);

            // the height
            INT cy = rc.bottom - rc.top;

            // move it
            pCtrl->m_bMoving = TRUE;
            SetWindowPos(*it, NULL, rc.left, nDown - cy, 0, 0,
                         SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            pCtrl->m_bMoving = FALSE;
        }
    }

    // called from MMainWnd WM_COMMAND ID_LEFTALIGN
    void OnLeftAlign(HWND hwnd)
    {
        auto set = MRadCtrl::GetTargets();
        if (set.size() < 2)
            return;

        RECT rc;

        // the leftest X coordinate --> nLeft
        INT nLeft = INT_MAX;
        auto end = set.end();
        for (auto it = set.begin(); it != end; ++it)
        {
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);
            if (rc.left < nLeft)
                nLeft = rc.left;
        }

        // move the selected controls to the leftest coordinate
        for (auto it = set.begin(); it != end; ++it)
        {
            // get the coordinates of the control
            MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(*it);
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);

            // move it
            pCtrl->m_bMoving = TRUE;
            SetWindowPos(*it, NULL, nLeft, rc.top, 0, 0,
                SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            pCtrl->m_bMoving = FALSE;
        }
    }

    // called from MMainWnd WM_COMMAND ID_RIGHTALIGN
    void OnRightAlign(HWND hwnd)
    {
        MRadCtrl::set_type set = MRadCtrl::GetTargets();
        if (set.size() < 2)
            return;

        RECT rc;

        // the rightest X coordinate --> nRight
        INT nRight = INT_MIN;
        auto end = set.end();
        for (auto it = set.begin(); it != end; ++it)
        {
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);
            if (nRight < rc.right)
                nRight = rc.right;
        }

        for (auto it = set.begin(); it != end; ++it)
        {
            // get the coordinates of the control
            MRadCtrl *pCtrl = MRadCtrl::GetRadCtrl(*it);
            GetWindowRect(*it, &rc);
            MapWindowRect(NULL, m_rad_dialog, &rc);

            // the width
            INT cx = rc.right - rc.left;

            // move it
            pCtrl->m_bMoving = TRUE;
            SetWindowPos(*it, NULL, nRight - cx, rc.top, 0, 0,
                SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
            pCtrl->m_bMoving = FALSE;
        }
    }

    // able to make it top index?
    BOOL CanIndexTop() const
    {
        auto indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty())
            return FALSE;   // no

        INT iUnselected = -1;
        for (UINT i = 0; i < m_dialog_res.m_cItems; ++i)
        {
            if (indeces.find(i) != indeces.end())
            {
                if (iUnselected != -1)
                    return TRUE;    // yes
            }
            else
            {
                iUnselected = i;
            }
        }

        return FALSE;   // no
    }

    // make it top index
    void IndexTop(HWND hwnd)
    {
        auto indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty())
            return;

        // move the dialog items
        DialogItems items1, items2;
        for (UINT i = 0; i < m_dialog_res.m_cItems; ++i)
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

        // refresh
        OnRefresh(hwnd);
    }

    // able to make it bottom index?
    BOOL CanIndexBottom() const
    {
        auto indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty())
            return FALSE;

        // find two items to swap
        INT iUnselected = -1;
        for (INT i = m_dialog_res.m_cItems - 1; i >= 0; --i)
        {
            if (indeces.find(i) != indeces.end())
            {
                if (iUnselected != -1)
                    return TRUE;
            }
            else
            {
                iUnselected = i;
            }
        }

        return FALSE;
    }

    // make it bottom index
    void IndexBottom(HWND hwnd)
    {
        auto indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty())
            return;

        // move the dialog items
        DialogItems items1, items2;
        for (UINT i = 0; i < m_dialog_res.m_cItems; ++i)
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

        // swap
        m_dialog_res.m_items = items1;
        m_dialog_res.m_items.insert(m_dialog_res.m_items.end(), items2.begin(), items2.end());

        // refresh
        OnRefresh(hwnd);
    }

    // able to decrement the control index?
    BOOL CanIndexMinus() const
    {
        auto indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty() || indeces.count(0) > 0)
            return FALSE;

        return TRUE;
    }

    // decrement the control index
    void IndexMinus(HWND hwnd)
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
    BOOL CanIndexPlus() const
    {
        auto indeces = MRadCtrl::GetTargetIndeces();
        if (indeces.empty() || indeces.count(m_dialog_res.m_cItems - 1) > 0)
            return FALSE;

        return TRUE;
    }

    // increment the control index
    void IndexPlus(HWND hwnd)
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
    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
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
                SendMessageDx(WM_COMMAND, ID_SHOWHIDEINDEX);
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
            pCtrl->DoTest();
            break;

        default:
            return;
        }
    }

    // select all the RADical controls
    void SelectAll(HWND hwnd)
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
    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        // show the popup menu
        PopupMenuDx(hwnd, hwndContext, IDR_POPUPMENUS, 1, xPos, yPos);
    }

    // get the dialog base units
    BOOL GetBaseUnits(INT& xDialogBaseUnit, INT& yDialogBaseUnit)
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
    void OnMove(HWND hwnd, int x, int y)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);

        // remember the position
        g_settings.nRadLeft = rc.left;
        g_settings.nRadTop = rc.top;
    }

    // MRadWindow WM_SIZE
    void OnSize(HWND hwnd, UINT state, int cx, int cy)
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
            DebugPrintDx("PTSIZ: %d, %d, %d, %d\n", pt.x, pt.y, siz.cx, siz.cy);
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
            SendMessage(hwndOwner, MYWM_MOVESIZEREPORT,
                MAKEWPARAM(item.m_pt.x, item.m_pt.y),
                MAKELPARAM(item.m_siz.cx, item.m_siz.cy));
        }

        // update the resource
        UpdateRes();
    }
};
