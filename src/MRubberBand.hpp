// MRubberBand.hpp -- Rubber Band for Win32
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

#ifndef MZC4_RUBBER_BAND_HPP_
#define MZC4_RUBBER_BAND_HPP_       5   // Version 5

class MRubberBand;

#include "MWindowBase.hpp"

//////////////////////////////////////////////////////////////////////////////

#ifndef MYWM_SELCHANGE
    #define MYWM_SELCHANGE      (WM_USER + 102)
#endif

class MRubberBand : public MWindowBase
{
public:
    DWORD m_dwMagic;                // for sanity check
    HRGN m_hRgn;                    // the window region
    HWND m_hwndTarget;              // the target
    enum { m_nGripSize = 3 };       // the grip size

    // constructor
    MRubberBand() : m_dwMagic(0x20110311), m_hRgn(NULL), m_hwndTarget(NULL)
    {
    }

    // get the associated rubber band from a window
    static MRubberBand* GetRubberBand(HWND hwnd)
    {
        auto base = GetUserData(hwnd);
        if (base)
        {
            auto pBand = static_cast<MRubberBand *>(base);
            if (pBand->m_dwMagic == 0x20110311)
                return pBand;
        }
        return NULL;
    }

    // create
    BOOL CreateDx(HWND hwndParent, HWND hwndTarget, BOOL bVisible = FALSE, 
                  INT x = 0, INT y = 0, INT cx = 0, INT cy = 0)
    {
        m_hwndTarget = hwndTarget;

        // create as a child
        DWORD style = (bVisible ? WS_VISIBLE : 0) | WS_CHILD | WS_THICKFRAME;
        DWORD exstyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
        BOOL bOK = CreateAsChildDx(hwndParent, NULL, style, exstyle, -1, 
                                   x, y, cx, cy);
        if (bOK)
        {
            // fit to the target
            FitToTarget();

            // send MYWM_SELCHANGE to the parent
            PostMessage(hwndParent, MYWM_SELCHANGE, 0, 0);
        }

        return bOK;
    }

    // get the target
    HWND GetTarget() const
    {
        return m_hwndTarget;
    }

    // called after WM_NCDESTROY
    virtual void PostNcDestroy()
    {
        MWindowBase::PostNcDestroy();
        delete this;
    }

    // the window procedure of MRubberBand
    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_MOVE, OnMove);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
            HANDLE_MSG(hwnd, WM_NCCALCSIZE, OnNCCalcSize);
            HANDLE_MSG(hwnd, WM_NCPAINT, OnNCPaint);
            HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
            HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
            HANDLE_MSG(hwnd, WM_NCHITTEST, OnNCHitTest);
            HANDLE_MSG(hwnd, WM_SETCURSOR, OnSetCursor);
        default:
            return DefaultProcDx(hwnd, uMsg, wParam, lParam);
        }
    }

    // get the ideal client rectangle
    void GetIdealClientRect(LPRECT prc)
    {
        GetClientRect(m_hwnd, prc);
        InflateRect(prc, -2 * m_nGripSize, -2 * m_nGripSize);
    }

    // adjust the target to this rubber band
    void FitToBand()
    {
        if (m_hwndTarget == NULL)
            return;     // no target

        // get the ideal rectangle in the parent coordinates
        RECT rc;
        GetIdealClientRect(&rc);
        MapWindowRect(m_hwnd, GetParent(m_hwndTarget), &rc);

        // move it
        SetWindowPos(m_hwndTarget, NULL, 
            rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS);

        // redraw
        InvalidateRect(m_hwnd, NULL, TRUE);
    }

    // adjust the rubber band's posision and size for the target
    void FitToTarget()
    {
        if (m_hwndTarget == NULL)
            return;     // not target

        // get the target rectangle in the parent coordinates
        RECT rc;
        GetWindowRect(m_hwndTarget, &rc);
        MapWindowRect(NULL, GetParent(m_hwnd), &rc);

        // expand the rectangle by the m_nGripSize
        InflateRect(&rc, 2 * m_nGripSize, 2 * m_nGripSize);

        // move it
        SetWindowPos(m_hwnd, NULL, 
            rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS);

        // redraw
        InvalidateRect(m_hwnd, NULL, TRUE);
    }

    // MRubberBand WM_DESTROY
    void OnDestroy(HWND hwnd)
    {
        // notity MYWM_SELCHANGE to the parent
        PostMessage(GetParent(hwnd), MYWM_SELCHANGE, 0, 0);

        // delete the region
        DeleteObject(m_hRgn);
        m_hRgn = NULL;

        // reset m_hwndTarget
        m_hwndTarget = NULL;
    }

    // get the rectangle
    void GetRect(HWND hwnd, LPRECT prc)
    {
        GetWindowRect(hwnd, prc);

        // NOTE: The coordinates of the window region are relative to the upper left corner of the window rectangle.
        OffsetRect(prc, -prc->left, -prc->top);
        assert(prc->left == 0 && prc->top == 0);
    }

    // set the region
    void SetRgn(HRGN hRgn, BOOL bClient = TRUE)
    {
        DeleteObject(m_hRgn);
        SetWindowRgn(m_hwnd, hRgn, TRUE);
        m_hRgn = hRgn;
    }

    // MRubberBand WM_NCPAINT
    void OnNCPaint(HWND hwnd, HRGN hrgn)
    {
        // get the rectangle
        RECT rc;
        GetRect(hwnd, &rc);

        // get the window DC
        if (HDC hDC = GetWindowDC(hwnd))
        {
            HPEN hPenOld = SelectPen(hDC, GetStockPen(BLACK_PEN));
            HBRUSH hbrOld = SelectBrush(hDC, GetStockBrush(WHITE_BRUSH));
            {
                // draw
                GetRgnOrDrawOrHitTest(hwnd, hDC, NULL);
            }
            SelectBrush(hDC, hbrOld);
            SelectPen(hDC, hPenOld);

            // release the window DC
            ReleaseDC(hwnd, hDC);
        }
    }

    // MRubberBand WM_PAINT
    void OnPaint(HWND hwnd)
    {
        // no client drawing
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
    }

    // MRubberBand WM_NCHITTEST
    UINT OnNCHitTest(HWND hwnd, int x, int y)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        x -= rc.left;
        y -= rc.top;

        // do hittest
        POINT pt = { x, y };
        HRGN hRgn = GetRgnOrDrawOrHitTest(hwnd, NULL, &pt);
        return (UINT)(UINT_PTR)hRgn;
    }

    // MRubberBand WM_SETCURSOR
    BOOL OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
    {
        switch (codeHitTest)
        {
        case HTTOPLEFT:         SetCursor(LoadCursor(NULL, IDC_SIZENWSE)); break;
        case HTLEFT:            SetCursor(LoadCursor(NULL, IDC_SIZEWE)); break;
        case HTBOTTOMLEFT:      SetCursor(LoadCursor(NULL, IDC_SIZENESW)); break;
        case HTTOP:             SetCursor(LoadCursor(NULL, IDC_SIZENS)); break;
        case HTBOTTOM:          SetCursor(LoadCursor(NULL, IDC_SIZENS)); break;
        case HTTOPRIGHT:        SetCursor(LoadCursor(NULL, IDC_SIZENESW)); break;
        case HTRIGHT:           SetCursor(LoadCursor(NULL, IDC_SIZEWE)); break;
        case HTBOTTOMRIGHT:     SetCursor(LoadCursor(NULL, IDC_SIZENWSE)); break;
        default:                SetCursor(LoadCursor(NULL, IDC_ARROW)); break;
        }
        return TRUE;
    }

    // MRubberBand WM_MOVE
    void OnMove(HWND hwnd, int x, int y)
    {
        HRGN hRgn;

        if (m_hwnd && m_hwndTarget)
        {
            // adjust the target to the band
            FitToBand();
        }

        // create and set the region
        hRgn = GetRgnOrDrawOrHitTest(hwnd);
        SetRgn(hRgn, TRUE);
    }

    // MRubberBand WM_SIZE
    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        // adjust the target to the band
        if (m_hwndTarget)
        {
            FitToBand();
        }

        // create and set the region
        HRGN hRgn = GetRgnOrDrawOrHitTest(hwnd);
        SetRgn(hRgn);

        // redraw
        InvalidateRect(hwnd, NULL, TRUE);
    }

    // get region, draw, or do hittest
    HRGN GetRgnOrDrawOrHitTest(HWND hwnd, HDC hDC = NULL, LPPOINT ppt = NULL)
    {
        RECT rc;
        GetRect(hwnd, &rc);

        // the size
        INT cx = rc.right - rc.left;
        INT cy = rc.bottom - rc.top;

        INT ax[] = { m_nGripSize, cx / 2, cx - m_nGripSize };
        INT ay[] = { m_nGripSize, cy / 2, cy - m_nGripSize };
        INT ahits[] =
        {
            HTTOPLEFT,    HTTOP,    HTTOPRIGHT, 
            HTLEFT,                 HTRIGHT, 
            HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT
        };

        // if hDC == NULL && ppt == NULL, then create the region
        HRGN hRgn = NULL;
        if (hDC == NULL && ppt == NULL)
            hRgn = CreateRectRgn(0, 0, 0, 0);

        for (INT k = 0, n = 0; k < 3; ++k)
        {
            for (INT i = 0; i < 3; ++i)
            {
                if (i == 1 && k == 1)
                    continue;

                if (hDC)
                {
                    // draw if hDC is non-null
                    Rectangle(hDC, 
                        ax[i] - m_nGripSize, ay[k] - m_nGripSize, 
                        ax[i] + m_nGripSize, ay[k] + m_nGripSize);
                }
                else if (ppt)
                {
                    // do hittest if ppt is non-null
                    RECT rect;
                    SetRect(&rect, 
                        ax[i] - m_nGripSize, ay[k] - m_nGripSize, 
                        ax[i] + m_nGripSize, ay[k] + m_nGripSize);
                    if (PtInRect(&rect, *ppt))
                    {
                        return (HRGN)(INT_PTR)ahits[n];
                    }
                }
                else
                {
                    // otherwise update the region
                    HRGN hRgn2 = CreateRectRgn(
                        ax[i] - m_nGripSize, ay[k] - m_nGripSize, 
                        ax[i] + m_nGripSize, ay[k] + m_nGripSize);
                    UnionRgn(hRgn, hRgn, hRgn2);
                    DeleteObject(hRgn2);
                }
                ++n;
            }
        }

        // return the hittest value if ppt is non-null
        if (ppt)
        {
            return (HRGN)(INT_PTR)HTCAPTION;
        }

        return hRgn;
    }

    // the window class name of MRubberBand
    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("katahiromz's Rubber Band Class");
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        // no class icon
        wcx.hIcon = NULL;
        wcx.hIconSm = NULL;

        // no background brush
        wcx.hbrBackground = GetStockBrush(NULL_BRUSH);
    }

    // MRubberBand WM_NCCALCSIZE
    UINT OnNCCalcSize(HWND hwnd, BOOL fCalcValidRects, NCCALCSIZE_PARAMS *lpcsp)
    {
        // no non-client area
        return WVR_VALIDRECTS | WVR_REDRAW;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_RUBBER_BAND_HPP_
