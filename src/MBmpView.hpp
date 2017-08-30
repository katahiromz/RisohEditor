// MBmpView
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MBMPVIEW_HPP_
#define MZC4_MBMPVIEW_HPP_

#include "RisohEditor.hpp"

class MBmpView;

//////////////////////////////////////////////////////////////////////////////

class MBmpView : public MWindowBase
{
public:
    BITMAP      m_bm;
    HBITMAP     m_hBitmap;
    HICON       m_hIcon;
    HWND        m_hStatic;
    HWND        m_hPlayButton;

    MBmpView()
    {
        ZeroMemory(&m_bm, sizeof(m_bm));
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        DWORD style = WS_CHILD | SS_ICON | SS_REALSIZEIMAGE;
        m_hStatic = CreateWindowEx(0, TEXT("STATIC"), NULL,
            style, 0, 0, 32, 32, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);
        if (m_hStatic == NULL)
            return FALSE;
        style = WS_CHILD | BS_PUSHBUTTON | BS_CENTER | BS_ICON;
        m_hPlayButton = CreateWindowEx(0, TEXT("BUTTON"), TEXT("Play"),
            style, 0, 0, 64, 65, hwnd, (HMENU)2, GetModuleHandle(NULL), NULL);
        if (m_hPlayButton == NULL)
            return FALSE;
        HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(102));
        SendMessage(m_hPlayButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
        SetWindowFont(m_hPlayButton, GetStockFont(DEFAULT_GUI_FONT), TRUE);
        return TRUE;
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

    MBmpView& operator=(HBITMAP hbm)
    {
        DestroyView();
        m_hBitmap = hbm;
        ShowWindow(m_hStatic, SW_HIDE);
        ShowWindow(m_hPlayButton, SW_HIDE);
        return *this;
    }

    void SetIcon(HICON hIcon, BOOL bIcon)
    {
        DestroyView();
        m_hIcon = hIcon;
        SendMessage(m_hStatic, STM_SETIMAGE, (bIcon ? IMAGE_ICON : IMAGE_CURSOR), (LPARAM)hIcon);
        ShowWindow(m_hStatic, SW_SHOWNOACTIVATE);
        ShowWindow(m_hPlayButton, SW_HIDE);
    }

    void SetPlay()
    {
        DestroyView();
        ShowWindow(m_hStatic, SW_HIDE);
        ShowWindow(m_hPlayButton, SW_SHOWNOACTIVATE);
    }

    void DestroyView()
    {
        if (m_hBitmap)
        {
            DeleteObject(m_hBitmap);
            m_hBitmap = NULL;
        }
        if (m_hIcon)
        {
            DestroyIcon(m_hIcon);
            m_hIcon = NULL;
        }
    }

    BOOL CreateDx(HWND hwndParent, INT CtrlID = 4, BOOL bVisible = FALSE)
    {
        DWORD dwStyle = WS_CHILD | WS_HSCROLL | WS_VSCROLL;
        if (bVisible)
            dwStyle |= WS_VISIBLE;
        DWORD dwExStyle = WS_EX_CLIENTEDGE;
        return CreateAsChildDx(hwndParent, NULL, dwStyle, dwExStyle, CtrlID);
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
        switch (id)
        {
        case 999:
            UpdateScrollInfo(hwnd);
            break;
        case 1:
            break;
        case 2:
            if (codeNotify == BN_CLICKED)
            {
                PostMessage(GetParent(hwnd), WM_COMMAND, CMDID_PLAY, 0);
            }
            break;
        }
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
            HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
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
