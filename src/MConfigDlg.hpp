// MConfigDlg.hpp --- "Configuration" Dialog
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

#ifndef MZC4_MCONFIGDLG_HPP_
#define MZC4_MCONFIGDLG_HPP_

#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "MMacrosDlg.hpp"
#include "MPathsDlg.hpp"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////////

class MConfigDlg : public MDialogBase
{
public:
    RisohSettings& m_settings;
    ConstantsDB& m_db;

    MConfigDlg(RisohSettings& settings, ConstantsDB& db)
        : MDialogBase(IDD_CONFIG), m_settings(settings), m_db(db)
    {
    }

    void Reload(HWND hwnd)
    {
        CheckDlgButton(hwnd, chx1, m_settings.bAlwaysControl ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx2, m_settings.bHideID ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx3, m_settings.bResumeWindowPos ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx4, m_settings.bAutoLoadNearbyResH ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx5, m_settings.bAutoShowIDList ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx6, m_settings.bShowDotsOnDialog ? BST_CHECKED : BST_UNCHECKED);
        SetDlgItemInt(hwnd, edt1, m_settings.nComboHeight, FALSE);
        CheckDlgButton(hwnd, chx7, m_settings.bUpdateResH ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx8, m_settings.bCompressByUPX ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx9, m_settings.bOldStyle ? BST_CHECKED : BST_UNCHECKED);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        Reload(hwnd);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        BOOL bTranslated = FALSE;
        INT nHeight = GetDlgItemInt(hwnd, edt1, &bTranslated, FALSE);
        if (!bTranslated)
        {
            HWND hEdt1 = GetDlgItem(hwnd, edt1);
            Edit_SetSel(hEdt1, 0, -1);
            SetFocus(hEdt1);
            ErrorBoxDx(IDS_ENTERINT);
            return;
        }
        m_settings.nComboHeight = nHeight;

        m_settings.bAlwaysControl = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
        m_settings.bHideID = (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
        m_settings.bResumeWindowPos = (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);
        m_settings.bAutoLoadNearbyResH = (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
        m_settings.bAutoShowIDList = (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED);
        m_settings.bShowDotsOnDialog = (IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED);
        m_settings.bUpdateResH = (IsDlgButtonChecked(hwnd, chx7) == BST_CHECKED);
        m_settings.bCompressByUPX = (IsDlgButtonChecked(hwnd, chx8) == BST_CHECKED);
        m_settings.bOldStyle = (IsDlgButtonChecked(hwnd, chx9) == BST_CHECKED);

        m_db.ShowMacroID(!m_settings.bHideID);

        EndDialog(IDOK);
    }

    void OnPsh1(HWND hwnd)
    {
        MMacrosDlg dialog(m_settings.macros, m_db);
        if (dialog.DialogBoxDx(hwnd) == psh6)
        {
            m_settings.ResetMacros();
        }
    }

    void OnPsh2(HWND hwnd)
    {
        MPathsDlg dialog(m_settings);
        dialog.DialogBoxDx(hwnd);
    }

    void OnPsh3(HWND hwnd)
    {
        SendMessage(GetParent(hwnd), WM_COMMAND, CMDID_SETDEFAULTS, 0);
        Reload(hwnd);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        case psh2:
            OnPsh2(hwnd);
            break;
        case psh3:
            OnPsh3(hwnd);
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            DO_MSG(WM_INITDIALOG, OnInitDialog);
            DO_MSG(WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MCONFIGDLG_HPP_
