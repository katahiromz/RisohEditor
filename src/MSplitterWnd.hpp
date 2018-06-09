// MSplitterWnd.hpp --- MZC4 splitter window                    -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MSPLITTERWND_HPP_
#define MZC4_MSPLITTERWND_HPP_      9   /* Version 9 */

class MSplitterWnd;

//////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"
#include <vector>

// The styles of MSplitterWnd
#define SWS_HORZ            0
#define SWS_VERT            1
#define SWS_LEFTALIGN       0
#define SWS_TOPALIGN        0
#define SWS_RIGHTALIGN      2
#define SWS_BOTTOMALIGN     2

class MSplitterWnd : public MWindowBase
{
public:
    enum { m_cxyBorder = 4, m_cxyMin = 8 };
    enum { NOTIFY_CHANGED = 0x2934 };

    MSplitterWnd() : m_iDraggingBorder(-1), m_nPaneCount(0)
    {
        m_vecPanes.resize(1);
    }

    BOOL CreateDx(HWND hwndParent, INT nPaneCount = 2, 
                  DWORD dwStyle = WS_CHILD | WS_VISIBLE | SWS_HORZ | SWS_LEFTALIGN, 
                  DWORD dwExStyle = 0)
    {
        RECT rc;
        GetClientRect(hwndParent, &rc);

        if (!CreateWindowDx(hwndParent, NULL, dwStyle, dwExStyle, 
                            rc.left, rc.top, 
                            rc.right - rc.left, rc.bottom - rc.top))
        {
            return FALSE;
        }

        SetPaneCount(nPaneCount);
        PostMessageDx(WM_SIZE);
        return TRUE;
    }

    BOOL IsHorizontal() const
    {
        return !IsVertical();
    }
    BOOL IsVertical() const
    {
        return (GetStyleDx() & SWS_VERT) == SWS_VERT;
    }
    BOOL IsRightBottomAlign() const
    {
        return (GetStyleDx() & SWS_RIGHTALIGN) == SWS_RIGHTALIGN;
    }

    INT GetPaneCount() const
    {
        return m_nPaneCount;
    }
    VOID SetPaneCount(INT nCount)
    {
        m_vecPanes.resize(nCount + 1);
        m_nPaneCount = nCount;
        SplitEqually();
    }

    HWND GetPane(INT nIndex) const
    {
        assert(0 <= nIndex && nIndex < m_nPaneCount);
        return m_vecPanes[nIndex].hwndPane;
    }
    VOID SetPane(INT nIndex, HWND hwndPane)
    {
        if (m_nPaneCount == 0)
            return;

        assert(0 <= nIndex && nIndex < m_nPaneCount);
        m_vecPanes[nIndex].hwndPane = hwndPane;
    }

    INT GetPanePos(INT nIndex) const
    {
        assert(0 <= nIndex && nIndex <= m_nPaneCount);
        return m_vecPanes[nIndex].xyPos;
    }
    VOID SetPanePos(INT nIndex, INT nPos, BOOL bBounded = TRUE)
    {
        if (m_nPaneCount == 0)
            return;

        assert(0 <= nIndex && nIndex <= m_nPaneCount);
        if (nIndex == 0)
            return;

        if (bBounded)
        {
            if (nIndex < m_nPaneCount)
            {
                const PANEINFO& info = m_vecPanes[nIndex];
                const PANEINFO& next_info = m_vecPanes[nIndex + 1];
                if (next_info.xyPos < nPos + info.cxyMin)
                    nPos = next_info.xyPos - info.cxyMin;
            }

            const PANEINFO& prev_info = m_vecPanes[nIndex - 1];
            if (nPos < prev_info.xyPos + prev_info.cxyMin)
                nPos = prev_info.xyPos + prev_info.cxyMin;
        }

        m_vecPanes[nIndex].xyPos = nPos;
    }

    INT GetPaneExtent(INT nIndex) const
    {
        assert(0 <= nIndex && nIndex < m_nPaneCount);
        return m_vecPanes[nIndex + 1].xyPos - m_vecPanes[nIndex].xyPos;
    }

    VOID SetPaneExtent(INT nIndex, INT cxy, BOOL bUpdate = TRUE)
    {
        if (m_nPaneCount == 0)
            return;

        assert(0 <= nIndex && nIndex < m_nPaneCount);
        if (nIndex == m_nPaneCount - 1)
        {
            SetPanePos(nIndex, m_vecPanes[m_nPaneCount].xyPos - cxy);
        }
        else
        {
            SetPanePos(nIndex + 1, m_vecPanes[nIndex].xyPos + cxy);
        }
        UpdatePanes();
    }

    VOID SetPaneMinExtent(INT nIndex, INT cxyMin = MSplitterWnd::m_cxyMin)
    {
        if (m_nPaneCount == 0)
            return;

        assert(0 <= nIndex && nIndex < m_nPaneCount);
        m_vecPanes[nIndex].cxyMin = cxyMin;
    }

    INT GetTotalMinExtent() const
    {
        INT cxy = 0;
        for (INT i = 0; i < m_nPaneCount; ++i)
        {
            cxy += m_vecPanes[i].cxyMin;
        }
        return cxy;
    }

    VOID GetPaneRect(INT nIndex, RECT *prc) const
    {
        assert(0 <= nIndex && nIndex < m_nPaneCount);
        GetClientRect(m_hwnd, prc);
        if (IsVertical())
        {
            prc->top = m_vecPanes[nIndex].xyPos;
            prc->bottom = m_vecPanes[nIndex + 1].xyPos;
            if (nIndex < m_nPaneCount - 1)
                prc->bottom -= m_cxyBorder;
        }
        else
        {
            prc->left = m_vecPanes[nIndex].xyPos;
            prc->right = m_vecPanes[nIndex + 1].xyPos;
            if (nIndex < m_nPaneCount - 1)
                prc->right -= m_cxyBorder;
        }
    }

    INT HitTestBorder(POINT ptClient) const
    {
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        if (!::PtInRect(&rcClient, ptClient))
            return -1;

        INT xy = (IsVertical() ? ptClient.y : ptClient.x);
        for (INT i = 1; i < m_nPaneCount; ++i)
        {
            INT xyPos = m_vecPanes[i].xyPos;
            if (xyPos - m_cxyBorder <= xy && xy <= xyPos)
            {
                return i;
            }
        }
        return -1;
    }

    void SplitEqually()
    {
        if (m_nPaneCount == 0)
            return;

        RECT rc;
        GetClientRect(m_hwnd, &rc);

        INT cxy = (IsVertical() ? rc.bottom : rc.right);
        INT xy = 0, cxyPane = cxy / m_nPaneCount;
        for (INT i = 0; i < m_nPaneCount; ++i)
        {
            m_vecPanes[i].xyPos = xy;
            xy += cxyPane;
        }
        m_vecPanes[m_nPaneCount].xyPos = cxy;
        PostMessageDx(WM_SIZE);
    }

    void UpdatePanes()
    {
        RECT rc;
        HDWP hDWP = BeginDeferWindowPos(m_nPaneCount);
        for (INT i = 0; i < m_nPaneCount; ++i)
        {
            const PANEINFO *pInfo = &m_vecPanes[i];
            HWND hwndPane = pInfo->hwndPane;
            if (hwndPane == NULL)
                continue;

            GetPaneRect(i, &rc);
            hDWP = DeferWindowPos(hDWP, hwndPane, NULL, 
                rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        EndDeferWindowPos(hDWP);

        UINT nID = GetDlgCtrlID(m_hwnd);
        NMHDR notify = { 0 };
        notify.hwndFrom = m_hwnd;
        notify.idFrom = nID;
        notify.code = NOTIFY_CHANGED;
        FORWARD_WM_NOTIFY(GetParent(m_hwnd), nID, &notify, SendMessage);
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
        HANDLE_MSG(hwnd, WM_SETCURSOR, OnSetCursor);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
        HANDLE_MSG(hwnd, WM_SYSCOLORCHANGE, OnSysColorChange);
        case WM_CAPTURECHANGED:
            m_iDraggingBorder = -1;
            return 0;
        default:
            return DefaultProcDx();
        }
    }

    void OnSysColorChange(HWND hwnd)
    {
        for (size_t i = 0; i < m_vecPanes.size(); ++i)
        {
            SendMessage(m_vecPanes[i].hwndPane, WM_SYSCOLORCHANGE, 0, 0);
        }
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("MZC4 MSplitterWnd Class");
    }

    virtual VOID ModifyWndClassDx(WNDCLASSEX& wcx)
    {
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        FORWARD_WM_COMMAND(GetParent(hwnd), id, hwndCtl, codeNotify, PostMessage);
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
    {
        return FORWARD_WM_NOTIFY(GetParent(hwnd), idFrom, pnmhdr, SendMessage);
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        FORWARD_WM_CONTEXTMENU(GetParent(hwnd), hwndContext, xPos, yPos, SendMessage);
    }

    static HCURSOR& CursorNS()
    {
        static HCURSOR s_hcurNS = ::LoadCursor(NULL, IDC_SIZENS);
        return s_hcurNS;
    }
    static HCURSOR& CursorWE()
    {
        static HCURSOR s_hcurWE = ::LoadCursor(NULL, IDC_SIZEWE);
        return s_hcurWE;
    }

protected:
    struct PANEINFO
    {
        HWND    hwndPane;
        INT     xyPos;
        INT     cxyMin;

        PANEINFO()
        {
            hwndPane = NULL;
            xyPos = 0;
            cxyMin = m_cxyMin;
        }
    };
    INT                     m_iDraggingBorder;
    INT                     m_nPaneCount;
    std::vector<PANEINFO>   m_vecPanes;

    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        if (fDoubleClick)
            return;

        POINT pt = { x, y };
        INT iBorder = HitTestBorder(pt);
        if (iBorder < 0)
            return;

        SetCapture(hwnd);
        m_iDraggingBorder = iBorder;

        if (IsVertical())
            SetCursor(CursorNS());
        else
            SetCursor(CursorWE());
    }

    void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
    {
        if (m_iDraggingBorder == -1)
            return;

        SetPanePos(m_iDraggingBorder, (IsVertical() ? y : x) + m_cxyBorder / 2);
        UpdatePanes();

        m_iDraggingBorder = -1;
        ReleaseCapture();
    }

    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
        if (m_iDraggingBorder == -1)
            return;

        SetPanePos(m_iDraggingBorder, (IsVertical() ? y : x) + m_cxyBorder / 2);
        UpdatePanes();
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        if (m_nPaneCount == 0)
            return;

        RECT rc;
        GetClientRect(hwnd, &rc);
        INT cxy = (IsVertical() ? rc.bottom : rc.right);
        Resize(cxy);
    }

    void Resize(INT cxy)
    {
        if (IsRightBottomAlign())
        {
            INT dxy = cxy - m_vecPanes[m_nPaneCount].xyPos;
            for (INT i = 1; i < m_nPaneCount; ++i)
            {
                SetPanePos(i, m_vecPanes[i].xyPos + dxy, FALSE);
            }
        }

        SetPanePos(m_nPaneCount, cxy, FALSE);
        UpdatePanes();
    }

    BOOL OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);

        if (HitTestBorder(pt) == -1)
        {
            SetCursor(::LoadCursor(NULL, IDC_ARROW));
            return TRUE;
        }

        if (IsVertical())
            SetCursor(CursorNS());
        else
            SetCursor(CursorWE());
        return TRUE;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MSPLITTERWND_HPP_
