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

    CHAR szText[512];
    GetDlgItemTextA(s_hwndEga, cmb1, szText, 612);
    lstrcpynA(buf, szText, buflen);
    SetDlgItemTextA(s_hwndEga, cmb1, NULL);
    s_bEnter = FALSE;
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

static DWORD WINAPI EgaThreadFunc(LPVOID)
{
    EGA_interactive(true);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

class MEgaDlg : public MDialogBase
{
public:
    MEgaDlg() : MDialogBase(IDD_EGA)
    {
        EGA_init();
        EGA_set_input_fn(EGA_dialog_input);
        EGA_set_print_fn(EGA_dialog_print);
    }

    virtual ~MEgaDlg()
    {
        EGA_uninit();
        DeleteObject(m_hFont);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        s_hwndEga = hwnd;
        SubclassChildDx(m_cmb1, cmb1);
        SubclassChildDx(m_cmb2, cmb2);
        m_cmb1.AddString(L"help");

        LOGFONTW lf;
        ZeroMemory(&lf, sizeof(lf));
        lf.lfHeight = -12;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
        m_hFont = CreateFontIndirectW(&lf);
        SendDlgItemMessageW(hwnd, edt1, WM_SETFONT, (WPARAM)m_hFont, TRUE);

        HANDLE hThread = CreateThread(NULL, 0, EgaThreadFunc, NULL, 0, NULL);
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

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        default:
            return DefaultProcDx();
        }
    }

protected:
    HFONT m_hFont;
    MComboBox m_cmb1;
    MComboBox m_cmb2;
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MEGADLG_HPP_
