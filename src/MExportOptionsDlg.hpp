// MExportOptionsDlg.hpp --- "Configuration" Dialog
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

#ifndef MZC4_MEXPORTOPTIONSDLG_HPP_
#define MZC4_MEXPORTOPTIONSDLG_HPP_

#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////////

class MExportOptionsDlg : public MDialogBase
{
public:
    RisohSettings& m_settings;

    MExportOptionsDlg(RisohSettings& settings)
        : MDialogBase(IDD_EXP_OPTIONS), m_settings(settings)
    {
    }

    void Reload(HWND hwnd)
    {
        CheckDlgButton(hwnd, chx1, m_settings.bSepFilesByLang ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx2, m_settings.bStoreToResFolder ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, chx3, m_settings.bSelectableByMacro ? BST_CHECKED : BST_UNCHECKED);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        Reload(hwnd);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        m_settings.bSepFilesByLang = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
        m_settings.bStoreToResFolder = (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
        m_settings.bSelectableByMacro = (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);

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
            EndDialog(hwnd, IDCANCEL);
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

#endif  // ndef MZC4_MEXPORTOPTIONSDLG_HPP_
