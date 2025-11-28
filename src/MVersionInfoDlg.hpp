// MVersionInfoDlg.hpp --- version information dialog
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
#include "DialogRes.hpp"
#include "MString.hpp"
#include "MToolBarCtrl.hpp"
#include "MHyperLinkCtrl.hpp"

class MVersionInfoDlg;

//////////////////////////////////////////////////////////////////////////////

class MVersionInfoDlg : public MDialogBase
{
public:
    MVersionInfoDlg() : MDialogBase(IDD_VERSIONINFO)
    {
        m_visited = FALSE;
        m_old_tick = 0;
    }

    virtual ~MVersionInfoDlg()
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemText(hwnd, stc1, LoadStringDx(IDS_VERSIONINFO));
        SetDlgItemText(hwnd, edt1, LoadStringDx(IDS_TRANSLATORS));
        SubclassChildDx(m_hyperlink, stc2);
        CenterWindowDx();
        return TRUE;
    }

    // STN_CLICKED
    void OnStc2(HWND hwnd)
    {
        // STN_CLICKED is generated twice (WM_LBUTTONDOWN and WM_LBUTTONUP)
        bool double_generated = m_visited && (GetTickCount() - m_old_tick < 500);
        m_old_tick = GetTickCount();
        m_visited = TRUE;
        if (double_generated)
            return;

        MString url = GetDlgItemText(stc2);
        ShellExecute(hwnd, NULL, url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            EndDialog(IDOK);
            break;
        case IDCANCEL:
            EndDialog(IDCANCEL);
            break;
        case stc2:
            if (codeNotify == STN_CLICKED)
                OnStc2(hwnd);
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
    BOOL m_visited;
    DWORD m_old_tick;
    MHyperLinkCtrl m_hyperlink;
};
