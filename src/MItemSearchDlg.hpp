// MItemSearchDlg --- RisohEditor "Item Search" dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Win32API resource editor
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

#ifndef MZC4_MITEMSEARCHDLG_HPP_
#define MZC4_MITEMSEARCHDLG_HPP_

#include "RisohEditor.hpp"
#include <set>

class MItemSearchDlg;

//////////////////////////////////////////////////////////////////////////////

class MItemSearchDlg : public MDialogBase
{
public:
    BOOL m_bIgnoreCases;
    BOOL m_bDownward;
    MString m_strText;
    HICON m_hIcon;
    HICON m_hIconSm;

    MItemSearchDlg() : MDialogBase(IDD_ITEMSEARCH),
                       m_bIgnoreCases(TRUE), m_bDownward(TRUE)
    {
        m_hIcon = LoadIconDx(4);
        m_hIconSm = LoadSmallIconDx(4);
    }

    virtual ~MItemSearchDlg()
    {
        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    typedef std::set<MItemSearchDlg *> dialogs_type;

    static dialogs_type& Dialogs()
    {
        static dialogs_type s_dialogs;
        return s_dialogs;
    }

    virtual void PostNcDestroy()
    {
        Dialogs().erase(this);
        MDialogBase::PostNcDestroy();
        delete this;
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        Dialogs().insert(this);

        if (m_bIgnoreCases)
            CheckDlgButton(hwnd, chx1, BST_UNCHECKED);
        else
            CheckDlgButton(hwnd, chx1, BST_CHECKED);

        if (m_bDownward)
            CheckRadioButton(hwnd, rad1, rad2, rad2);
        else
            CheckRadioButton(hwnd, rad1, rad2, rad1);

        SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        m_strText = GetDlgItemText(edt1);
        m_bIgnoreCases = IsDlgButtonChecked(hwnd, chx1) == BST_UNCHECKED;
        m_bDownward = IsDlgButtonChecked(hwnd, rad2) == BST_CHECKED;
        SendMessage(GetParent(hwnd), WM_COMMAND, CMDID_ITEMSEARCHBANG, (WPARAM)this);
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
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MITEMSEARCHDLG_HPP_
