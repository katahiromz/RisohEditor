// MBmpView
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MBMPVIEW_HPP_
#define MZC4_MBMPVIEW_HPP_

#include "RisohEditor.hpp"

//////////////////////////////////////////////////////////////////////////////

struct MBmpView : MWindowBase
{
    BITMAP      m_bm;
    HBITMAP     m_hBitmap;

    MBmpView()
    {
        ZeroMemory(&m_bm, sizeof(m_bm));
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("RisohEditor MBmpView Class");
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hIcon = NULL;
        wcx.hCursor = LoadCursor(NULL, IDC_CROSS);
        wcx.hbrBackground = GetStockBrush(LTGRAY_BRUSH);
        wcx.lpszMenuName = NULL;
    }

    operator HBITMAP() const
    {
        return m_hBitmap;
    }

    MBmpView& operator=(HBITMAP hbm)
    {
        DestroyBmp();
        m_hBitmap = hbm;
        return *this;
    }

    void DestroyBmp()
    {
        if (m_hBitmap)
        {
            DeleteObject(m_hBitmap);
            m_hBitmap = NULL;
        }
    }

    BOOL CreateDx(HWND hwndParent, INT CtrlID = 4)
    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
        DWORD dwExStyle = WS_EX_CLIENTEDGE;
        return CreateWindowDx(hwndParent, NULL, dwStyle, dwExStyle,
            0, 0, 0, 0, (HMENU)CtrlID);
    }

    void OnPaint(HWND hwnd)
    {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hwnd, &ps);
        if (hDC == NULL)
            return;

        HDC hMemDC = CreateCompatibleDC(NULL);
        {
            SelectObject(hMemDC, m_hBitmap);
            INT dx = GetScrollPos(hwnd, SB_HORZ);
            INT dy = GetScrollPos(hwnd, SB_VERT);
            BitBlt(hDC, -dx, -dy, m_bm.bmWidth, m_bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
        }
        DeleteDC(hMemDC);
        EndPaint(hwnd, &ps);
    }

    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, GetStockBrush(COLOR_BACKGROUND));
        return TRUE;
    }

    void OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);

        SCROLLINFO info;
        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);
        info.fMask = SIF_POS | SIF_PAGE | SIF_DISABLENOSCROLL;
        info.nPage = rc.right - rc.left;
        switch (code)
        {
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            info.nPos = pos;
            break;
        case SB_TOP:
            info.nPos = 0;
            break;
        case SB_BOTTOM:
            info.nPos = m_bm.bmHeight;
            break;
        case SB_ENDSCROLL:
            return;
        case SB_LINEDOWN:
            info.nPos = GetScrollPos(hwnd, SB_HORZ) + 10;
            break;
        case SB_LINEUP:
            info.nPos = GetScrollPos(hwnd, SB_HORZ) - 10;
            break;
        case SB_PAGEDOWN:
            info.nPos = GetScrollPos(hwnd, SB_HORZ) + info.nPage;
            break;
        case SB_PAGEUP:
            info.nPos = GetScrollPos(hwnd, SB_HORZ) - info.nPage;
            break;
        }
        SetScrollInfo(hwnd, SB_HORZ, &info, TRUE);
        InvalidateRect(hwnd, NULL, TRUE);
    }

    void OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);

        SCROLLINFO info;
        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);
        info.fMask = SIF_POS | SIF_PAGE | SIF_DISABLENOSCROLL;
        info.nPage = rc.bottom - rc.top;
        switch (code)
        {
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            info.nPos = pos;
            break;
        case SB_TOP:
            info.nPos = 0;
            break;
        case SB_BOTTOM:
            info.nPos = m_bm.bmHeight;
            break;
        case SB_ENDSCROLL:
            return;
        case SB_LINEDOWN:
            info.nPos = GetScrollPos(hwnd, SB_VERT) + 10;
            break;
        case SB_LINEUP:
            info.nPos = GetScrollPos(hwnd, SB_VERT) - 10;
            break;
        case SB_PAGEDOWN:
            info.nPos = GetScrollPos(hwnd, SB_VERT) + info.nPage;
            break;
        case SB_PAGEUP:
            info.nPos = GetScrollPos(hwnd, SB_VERT) - info.nPage;
            break;
        }
        SetScrollInfo(hwnd, SB_VERT, &info, TRUE);
        InvalidateRect(hwnd, NULL, TRUE);
    }

    void UpdateScrollInfo(HWND hwnd)
    {
        if (!GetObjectW(m_hBitmap, sizeof(m_bm), &m_bm))
            return;

        RECT rc;
        GetClientRect(hwnd, &rc);

        SCROLLINFO info;

        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);
        info.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
        info.nMin = 0;
        info.nMax = m_bm.bmWidth;
        info.nPage = rc.right - rc.left;
        info.nPos = 0;
        SetScrollInfo(hwnd, SB_HORZ, &info, TRUE);

        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);
        info.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
        info.nMin = 0;
        info.nMax = m_bm.bmHeight;
        info.nPage = rc.bottom - rc.top;
        info.nPos = 0;
        SetScrollInfo(hwnd, SB_VERT, &info, TRUE);

        InvalidateRect(hwnd, NULL, TRUE);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        if (id != 999)
            return;

        UpdateScrollInfo(hwnd);
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        UpdateScrollInfo(hwnd);
        FORWARD_WM_SIZE(hwnd, state, cx, cy, DefWindowProcW);
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
            HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
            HANDLE_MSG(hwnd, WM_HSCROLL, OnHScroll);
            HANDLE_MSG(hwnd, WM_VSCROLL, OnVScroll);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        default:
            return DefaultProcDx();
        }
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MBMPVIEW_HPP_

//////////////////////////////////////////////////////////////////////////////
