// MDialogFontSubstDlg --- "Replacing Dialog Fonts" Dialog
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

#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "Common.hpp"

//////////////////////////////////////////////////////////////////////////////

class MDialogFontSubstDlg : public MDialogBase
{
public:
    MComboBoxAutoComplete m_cmb1;
    MComboBoxAutoComplete m_cmb2;
    MComboBoxAutoComplete m_cmb3;
    MComboBoxAutoComplete m_cmb4;
    MComboBoxAutoComplete m_cmb5;
    MComboBoxAutoComplete m_cmb6;

    MDialogFontSubstDlg() : MDialogBase(IDD_REPLACEFONTS)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        InitFontComboBox(GetDlgItem(hwnd, cmb1));
        InitFontComboBox(GetDlgItem(hwnd, cmb2));
        InitFontComboBox(GetDlgItem(hwnd, cmb3));
        InitFontComboBox(GetDlgItem(hwnd, cmb4));
        InitFontComboBox(GetDlgItem(hwnd, cmb5));
        InitFontComboBox(GetDlgItem(hwnd, cmb6));

        SetDlgItemTextW(hwnd, cmb1, g_settings.strFontReplaceFrom1.c_str());
        SetDlgItemTextW(hwnd, cmb2, g_settings.strFontReplaceTo1.c_str());
        SetDlgItemTextW(hwnd, cmb3, g_settings.strFontReplaceFrom2.c_str());
        SetDlgItemTextW(hwnd, cmb4, g_settings.strFontReplaceTo2.c_str());
        SetDlgItemTextW(hwnd, cmb5, g_settings.strFontReplaceFrom3.c_str());
        SetDlgItemTextW(hwnd, cmb6, g_settings.strFontReplaceTo3.c_str());

        SubclassChildDx(m_cmb1, cmb1);
        SubclassChildDx(m_cmb2, cmb2);
        SubclassChildDx(m_cmb3, cmb3);
        SubclassChildDx(m_cmb4, cmb4);
        SubclassChildDx(m_cmb5, cmb5);
        SubclassChildDx(m_cmb6, cmb6);

        CenterWindowDx();
        return TRUE;
    }

    BOOL OnOK(HWND hwnd)
    {
        g_settings.strFontReplaceFrom1 = GetDlgItemText(cmb1);
        g_settings.strFontReplaceTo1 = GetDlgItemText(cmb2);
        g_settings.strFontReplaceFrom2 = GetDlgItemText(cmb3);
        g_settings.strFontReplaceTo2 = GetDlgItemText(cmb4);
        g_settings.strFontReplaceFrom3 = GetDlgItemText(cmb5);
        g_settings.strFontReplaceTo3 = GetDlgItemText(cmb6);
        return TRUE;
    }

    void OnReset(HWND hwnd)
    {
        SetDlgItemTextW(hwnd, cmb1, L"MS Shell Dlg");
        SetDlgItemTextW(hwnd, cmb2, L"MS Shell Dlg");
        SetDlgItemTextW(hwnd, cmb3, L"MS Shell Dlg 2");
        SetDlgItemTextW(hwnd, cmb4, L"MS Shell Dlg 2");
        SetDlgItemTextW(hwnd, cmb5, L"");
        SetDlgItemTextW(hwnd, cmb6, L"");
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            if (OnOK(hwnd))
            {
                EndDialog(hwnd, id);
            }
            break;

        case IDCANCEL:
            EndDialog(hwnd, id);
            break;

        case psh1:
            OnReset(hwnd);
            break;

        case cmb1:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb1.OnEditChange();
            }
            break;

        case cmb2:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb2.OnEditChange();
            }
            break;

        case cmb3:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb3.OnEditChange();
            }
            break;

        case cmb4:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb4.OnEditChange();
            }
            break;

        case cmb5:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb5.OnEditChange();
            }
            break;

        case cmb6:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb6.OnEditChange();
            }
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
        }
        return DefaultProcDx();
    }
};
