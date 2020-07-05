// MDfmSettingsDlg.hpp --- "Delphi DFM settings" Dialog
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

#include "resource.h"
#include "MWindowBase.hpp"
#include "RisohSettings.hpp"

//////////////////////////////////////////////////////////////////////////////

class MDfmSettingsDlg : public MDialogBase
{
public:
    INT m_nCodePage;
    BOOL m_bComments;

    MDfmSettingsDlg() : MDialogBase(IDD_DFMSETTINGS)
    {
        m_nCodePage = g_settings.nDfmCodePage;
        m_bComments = g_settings.bDfmRawTextComments;
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_AddString(hCmb1, TEXT("0"));
        ComboBox_AddString(hCmb1, TEXT("1252 (Latin 1)"));
        ComboBox_AddString(hCmb1, TEXT("1250 (Latin 2)"));
        ComboBox_AddString(hCmb1, TEXT("1251 (Cyrillic)"));
        ComboBox_AddString(hCmb1, TEXT("1253 (Greek)"));
        ComboBox_AddString(hCmb1, TEXT("1254 (Turkish)"));
        ComboBox_AddString(hCmb1, TEXT("1255 (Hebrew)"));
        ComboBox_AddString(hCmb1, TEXT("1256 (Arabic)"));
        ComboBox_AddString(hCmb1, TEXT("1257 (Baltic)"));
        ComboBox_AddString(hCmb1, TEXT("874 (Thai)"));
        ComboBox_AddString(hCmb1, TEXT("932 (Japanese)"));
        ComboBox_AddString(hCmb1, TEXT("936 (Simplified Chinese)"));
        ComboBox_AddString(hCmb1, TEXT("949 (Korean)"));
        ComboBox_AddString(hCmb1, TEXT("950 (Traditional Chinese)"));
        ComboBox_AddString(hCmb1, TEXT("65001 (UTF-8)"));

        TCHAR szText[32];
        StringCbPrintf(szText, sizeof(szText), TEXT("%u"), m_nCodePage);
        ComboBox_SetText(hCmb1, szText);

        if (m_bComments)
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
        else
            CheckDlgButton(hwnd, chx1, BST_UNCHECKED);

        CenterWindowDx();

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);

        TCHAR szText[32];
        INT iItem = ComboBox_GetCurSel(hCmb1);
        if (iItem == CB_ERR)
        {
            ComboBox_GetText(hCmb1, szText, 32);
        }
        else
        {
            ComboBox_GetLBText(hCmb1, iItem, szText);
        }
        m_nCodePage = _tcstoul(szText, NULL, 0);

        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            m_bComments = TRUE;
        else
            m_bComments = FALSE;

        EndDialog(IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(IDCANCEL);
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
