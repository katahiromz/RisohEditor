// MEgaDlg.hpp --- Programming Language EGA dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#include "resource.h"
#include "MResizable.hpp"
#include "RisohSettings.hpp"
#include "../EGA/ega.hpp"

using namespace EGA;

class MEgaDlg;
static HWND s_hwndEga = NULL;
static BOOL s_bEnter = FALSE;
static LONG s_nEgaRunning = 0;
extern HWND g_hMainWnd;
extern MIdOrString g_RES_select_type;
extern MIdOrString g_RES_select_name;
extern WORD g_RES_select_lang;

static bool EGA_dialog_input(char *buf, size_t buflen)
{
    if (buf == NULL && buflen == 0)
    {
        SendMessageW(g_hMainWnd, WM_COMMAND, ID_EGAFINISH, 0);
        return true;
    }

    while (!s_bEnter || !::IsWindowVisible(s_hwndEga))
    {
        Sleep(100);
    }
    s_bEnter = FALSE;

    WCHAR szTextW[512];
    GetDlgItemTextW(s_hwndEga, edt2, szTextW, ARRAYSIZE(szTextW));

    char szTextA[512];
    WideCharToMultiByte(CP_UTF8, 0, szTextW, -1, szTextA, ARRAYSIZE(szTextA), NULL, NULL);

    StringCchCopyA(buf, buflen, szTextA);
    SetDlgItemTextA(s_hwndEga, edt2, NULL);

    if (lstrcmpA(szTextA, "exit") == 0 || lstrcmpA(szTextA, "exit;") == 0)
        PostMessageW(s_hwndEga, WM_COMMAND, IDCANCEL, 0);

    mstr_trim(szTextW);

    return true;
}

static void EGA_dialog_print(const char *fmt, va_list va)
{
    if (!IsWindow(s_hwndEga))
        return;

    std::string str;
    str.resize(512);
    while (StringCbVPrintfA(&str[0], str.size(), fmt, va) == STRSAFE_E_INSUFFICIENT_BUFFER)
    {
        str.resize(str.size() * 2);
    }
    str.resize(lstrlenA(str.c_str()));

    mstr_replace_all(str, "\n", "\r\n");

    MAnsiToWide wide(CP_UTF8, str.c_str());

    INT cch = GetWindowTextLengthW(GetDlgItem(s_hwndEga, edt1));
    SendDlgItemMessageW(s_hwndEga, edt1, EM_SETSEL, cch, cch);
    SendDlgItemMessageW(s_hwndEga, edt1, EM_REPLACESEL, FALSE, (LPARAM)wide.c_str());
    SendDlgItemMessageW(s_hwndEga, edt1, EM_SCROLLCARET, 0, 0);
}

static inline DWORD WINAPI EgaThreadFunc(LPVOID args)
{
    if (InterlockedIncrement(&s_nEgaRunning) == 1)
    {
        EGA_interactive(NULL, true);
        InterlockedDecrement(&s_nEgaRunning);
    }
    return 0;
}

void EGA_extension(void);

//////////////////////////////////////////////////////////////////////////////

class MEgaDlg : public MDialogBase
{
public:
    std::wstring m_filename;

    DECLARE_DYNAMIC(MEgaDlg)

    MEgaDlg() : MDialogBase(IDD_EGA)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);

        EGA_init();
        EGA_set_input_fn(EGA_dialog_input);
        EGA_set_print_fn(EGA_dialog_print);
        EGA_extension();
    }

    virtual ~MEgaDlg()
    {
        EGA_uninit();
        DeleteObject(m_hFont);

        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    void Run(LPCWSTR filename)
    {
        char szFileName[MAX_PATH];
        if (filename && filename[0])
        {
            WideCharToMultiByte(CP_ACP, 0, filename, -1, szFileName, _countof(szFileName), NULL, NULL);
            g_RES_select_type = (WORD)0;
            g_RES_select_name = (WORD)0;
            g_RES_select_lang = BAD_LANG;
            EGA_file_input(szFileName);
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        s_hwndEga = hwnd;
        m_resizable.OnParentCreate(hwnd);

        m_resizable.SetLayoutAnchor(grp1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(edt1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(stc1, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(edt2, mzcLA_BOTTOM_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(IDOK, mzcLA_BOTTOM_RIGHT);
        SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

        LOGFONTW lf;
        ZeroMemory(&lf, sizeof(lf));
        lf.lfHeight = -12;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
        m_hFont = CreateFontIndirectW(&lf);
        SendDlgItemMessageW(hwnd, edt1, WM_SETFONT, (WPARAM)m_hFont, TRUE);

        if (!s_nEgaRunning)
        {
            HANDLE hThread = ::CreateThread(NULL, 0, EgaThreadFunc, NULL, 0, NULL);
            ::CloseHandle(hThread);
        }

        if (g_settings.nEgaX != CW_USEDEFAULT && g_settings.nEgaWidth != CW_USEDEFAULT)
        {
            SetWindowPos(hwnd, NULL,
                g_settings.nEgaX, g_settings.nEgaY,
                g_settings.nEgaWidth, g_settings.nEgaHeight,
                SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
        }
        else if (g_settings.nEgaX != CW_USEDEFAULT)
        {
            SetWindowPos(hwnd, NULL,
                g_settings.nEgaX, g_settings.nEgaY,
                g_settings.nEgaWidth, g_settings.nEgaHeight,
                SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
        }
        else if (g_settings.nEgaWidth != CW_USEDEFAULT)
        {
            SetWindowPos(hwnd, NULL,
                g_settings.nEgaX, g_settings.nEgaY,
                g_settings.nEgaWidth, g_settings.nEgaHeight,
                SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
        }
        else
        {
            CenterWindowDx();
        }
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        g_RES_select_type = (WORD)0;
        g_RES_select_name = (WORD)0;
        g_RES_select_lang = BAD_LANG;
        s_bEnter = TRUE;
        ::SetFocus(::GetDlgItem(hwnd, edt2));
    }

    void OnPsh1(HWND hwnd)
    {
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            ::ShowWindow(hwnd, SW_HIDE);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        }
    }

    void OnShowWindow(HWND hwnd, BOOL fShow, UINT status)
    {
        if (fShow && !s_nEgaRunning)
        {
            HANDLE hThread = ::CreateThread(NULL, 0, EgaThreadFunc, NULL, 0, NULL);
            ::CloseHandle(hThread);
            ::SetFocus(::GetDlgItem(hwnd, edt2));
        }
    }

    HBRUSH OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
    {
        UINT id;
        switch (type)
        {
        case CTLCOLOR_EDIT:
        case CTLCOLOR_STATIC:
        case CTLCOLOR_BTN:
            id = GetDlgCtrlID(hwndChild);
            switch (id)
            {
            case edt1:
            case edt2:
                SetTextColor(hdc, RGB(0, 255, 0));
                SetBkColor(hdc, RGB(0, 0, 0));
                return GetStockBrush(BLACK_BRUSH);
            }
        }
        return (HBRUSH)(COLOR_3DFACE + 1);
    }

    void OnMove(HWND hwnd, int x, int y)
    {
        if (IsWindowVisible(hwnd) && !IsMinimized(hwnd) && !IsMaximized(hwnd))
        {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            g_settings.nEgaX = rc.left;
            g_settings.nEgaY = rc.top;
        }
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        m_resizable.OnSize();

        if (IsWindowVisible(hwnd) && !IsMinimized(hwnd) && !IsMaximized(hwnd))
        {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            g_settings.nEgaWidth = rc.right - rc.left;
            g_settings.nEgaHeight = rc.bottom - rc.top;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_CTLCOLOREDIT, OnCtlColor);
        HANDLE_MSG(hwnd, WM_CTLCOLORSTATIC, OnCtlColor);
        HANDLE_MSG(hwnd, WM_MOVE, OnMove);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_SHOWWINDOW, OnShowWindow);
        default:
            return DefaultProcDx();
        }
    }

protected:
    HFONT m_hFont;
    HICON m_hIcon;
    HICON m_hIconSm;
    MResizable m_resizable;
};
