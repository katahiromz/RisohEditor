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

#ifndef MZC4_MEGADLG_HPP_
#define MZC4_MEGADLG_HPP_

#include "RisohEditor.hpp"
#include "MComboBox.hpp"
#include "MResizable.hpp"
#include "resource.h"
#include "../EGA/ega.hpp"

using namespace EGA;

class MEgaDlg;
static HWND s_hwndEga = NULL;
static BOOL s_bEnter = FALSE;

static bool EGA_dialog_input(char *buf, size_t buflen)
{
    while (!s_bEnter && IsWindow(s_hwndEga))
    {
        Sleep(100);
    }
    if (!IsWindow(s_hwndEga))
        return false;

    WCHAR szTextW[260];
    GetDlgItemTextW(s_hwndEga, cmb1, szTextW, ARRAYSIZE(szTextW));
    SendDlgItemMessageW(s_hwndEga, cmb1, CB_ADDSTRING, 0, (LPARAM)szTextW);

    char szTextA[260];
    WideCharToMultiByte(CP_UTF8, 0, szTextW, -1, szTextA, ARRAYSIZE(szTextA), NULL, NULL);

    lstrcpynA(buf, szTextA, buflen);
    SetDlgItemTextA(s_hwndEga, cmb1, NULL);
    s_bEnter = FALSE;

    if (lstrcmpA(szTextA, "exit") == 0 || lstrcmpA(szTextA, "exit;") == 0)
        PostMessageW(s_hwndEga, WM_COMMAND, IDCANCEL, 0);

    return true;
}

static void EGA_dialog_print(const char *fmt, va_list va)
{
    if (!IsWindow(s_hwndEga))
        return;

    CHAR szText[512];
    StringCbVPrintfA(szText, sizeof(szText), fmt, va);
    std::string str = szText;
    mstr_replace_all(str, "\n", "\r\n");

    INT cch = GetWindowTextLengthA(GetDlgItem(s_hwndEga, edt1));
    SendDlgItemMessageA(s_hwndEga, edt1, EM_SETSEL, cch, cch);
    SendDlgItemMessageA(s_hwndEga, edt1, EM_REPLACESEL, FALSE, (LPARAM)str.c_str());
    SendDlgItemMessageA(s_hwndEga, edt1, EM_SCROLLCARET, 0, 0);
}

static DWORD WINAPI EgaThreadFunc(LPVOID args)
{
    LPCWSTR filename = (LPCWSTR)args;
    char szFileName[MAX_PATH];
    if (filename && filename[0])
    {
        WideCharToMultiByte(CP_ACP, 0, filename, -1, szFileName, MAX_PATH, NULL, NULL);
        EGA_interactive(szFileName, true);
    }
    else
    {
        EGA_interactive(NULL, true);
    }
    return 0;
}

void EGA_extension(void);

//////////////////////////////////////////////////////////////////////////////

class MEgaDlg : public MDialogBase
{
public:
    MEgaDlg(LPCWSTR filename = NULL) : MDialogBase(IDD_EGA)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);
        if (filename)
            m_filename = filename;

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

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        m_resizable.OnParentCreate(hwnd);

        m_resizable.SetLayoutAnchor(grp1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(edt1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(stc1, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(cmb1, mzcLA_BOTTOM_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(IDOK, mzcLA_BOTTOM_RIGHT);

        s_hwndEga = hwnd;
        SubclassChildDx(m_cmb1, cmb1);

        SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

        m_cmb1.AddString(L"help");
        m_cmb1.AddString(L"help print");
        m_cmb1.AddString(L"print(+(1, 2));");

        LOGFONTW lf;
        ZeroMemory(&lf, sizeof(lf));
        lf.lfHeight = -12;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
        m_hFont = CreateFontIndirectW(&lf);
        SendDlgItemMessageW(hwnd, edt1, WM_SETFONT, (WPARAM)m_hFont, TRUE);

        HANDLE hThread = CreateThread(NULL, 0, EgaThreadFunc,
                                      (LPWSTR)m_filename.c_str(), 0, NULL);
        CloseHandle(hThread);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        s_bEnter = TRUE;
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
            s_hwndEga = NULL;
            s_bEnter = FALSE;
            EndDialog(IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
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
            if (GetParent(hwndChild) == m_cmb1)
            {
                id = cmb1;
            }
            switch (id)
            {
            case cmb1:
            case edt1:
                SetTextColor(hdc, RGB(0, 255, 0));
                SetBkColor(hdc, RGB(0, 0, 0));
                return GetStockBrush(BLACK_BRUSH);
            }
        }
        return (HBRUSH)(COLOR_3DFACE + 1);
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        m_resizable.OnSize();
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
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        default:
            return DefaultProcDx();
        }
    }

protected:
    HFONT m_hFont;
    HICON m_hIcon;
    HICON m_hIconSm;
    std::wstring m_filename;
    MComboBox m_cmb1;
    MResizable m_resizable;
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MEGADLG_HPP_
