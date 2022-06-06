// MEditToolbarDlg.hpp --- Dialogs for edit of TOOLBAR resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
#include "ConstantsDB.hpp"
#include "MComboBoxAutoComplete.hpp"

#include "ToolbarRes.hpp"

class MEditToolbarDlg;

void InitCtrlIDComboBox(HWND hCmb);
BOOL CheckCommand(MString strCommand);
void InitResNameComboBox(HWND hCmb, MIdOrString id, IDTYPE_ nIDTYPE_);
void InitResNameComboBox(HWND hCmb, MIdOrString id, INT nIDTYPE_1, INT nIDTYPE_2);
void ReplaceFullWithHalf(LPWSTR pszText);

//////////////////////////////////////////////////////////////////////////////

class MAddTBBtnDlg : public MDialogBase
{
public:
    std::wstring m_str;
    MComboBoxAutoComplete m_cmb1;

    MAddTBBtnDlg() : MDialogBase(IDD_ADDTBBTN)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        InitCtrlIDComboBox(GetDlgItem(hwnd, cmb1));
        SubclassChildDx(m_cmb1, cmb1);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        WCHAR szText[MAX_PATH];
        GetDlgItemTextW(hwnd, cmb1, szText, _countof(szText));
        ReplaceFullWithHalf(szText);
        mstr_trim(szText);

        if (szText[0] && !CheckCommand(szText))
        {
            ErrorBoxDx(IDS_NOSUCHID);
            return;
        }

        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            szText[0] = 0;

        m_str = szText;
        EndDialog(IDOK);
    }

    void OnChx1(HWND hwnd)
    {
        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
        {
            SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, -1, 0);
            SetDlgItemTextW(hwnd, cmb1, NULL);
        }
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
        case chx1:
            OnChx1(hwnd);
            break;
        case cmb1:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb1.OnEditChange();
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

class MEditToolbarDlg : public MDialogBase
{
public:
    ToolbarRes& m_toolbar_res;
    HICON m_hIcon;
    HICON m_hIconSm;
    HWND m_hLst1;

    MEditToolbarDlg(ToolbarRes& toolbar_res)
        : MDialogBase(IDD_TOOLBARRES), m_toolbar_res(toolbar_res)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);
        m_hLst1 = NULL;
    }

    ~MEditToolbarDlg()
    {
        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        m_hLst1 = GetDlgItem(hwnd, lst1);

        for (size_t i = 0; i < m_toolbar_res.size(); ++i)
        {
            DWORD id = m_toolbar_res[i];
            if (id != 0)
            {
                std::wstring str = g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, id, true);
                SendMessageW(m_hLst1, LB_ADDSTRING, 0, (LPARAM)str.c_str());
            }
            else
            {
                SendMessageW(m_hLst1, LB_ADDSTRING, 0, (LPARAM)L"---");
            }
        }

        SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

        CenterWindowDx();
        return TRUE;
    }

    void OnAdd(HWND hwnd)
    {
        MAddTBBtnDlg dialog;
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            auto& str = dialog.m_str;
            if (str.empty() || str[0] == L'-')
                str = L"---";
            SendMessageW(m_hLst1, LB_INSERTSTRING, -1, (LPARAM)str.c_str());
        }
    }

    void OnModify(HWND hwnd)
    {
        INT iItem = (INT)SendMessageW(m_hLst1, LB_GETCURSEL, 0, 0);
        if (iItem >= 0)
        {
            SendMessageW(m_hLst1, LB_DELETESTRING, iItem, 0);
            SendMessageW(m_hLst1, LB_INSERTSTRING, iItem, (LPARAM)L"TEST2");
            SendMessageW(m_hLst1, LB_SETCURSEL, iItem, 0);
        }
    }

    void OnDelete(HWND hwnd)
    {
        INT iItem = (INT)SendMessageW(m_hLst1, LB_GETCURSEL, 0, 0);
        if (iItem >= 0)
        {
            SendMessageW(m_hLst1, LB_DELETESTRING, iItem, 0);
        }
    }

    void OnUp(HWND hwnd)
    {
        INT iItem = (INT)SendMessageW(m_hLst1, LB_GETCURSEL, 0, 0);
        if (iItem > 0)
        {
            WCHAR sz1[MAX_PATH], sz2[MAX_PATH];
            sz1[0] = sz2[0] = 0;
            SendMessageW(m_hLst1, LB_GETTEXT, iItem - 1, (LPARAM)sz1);
            SendMessageW(m_hLst1, LB_GETTEXT, iItem, (LPARAM)sz2);
            SendMessageW(m_hLst1, LB_DELETESTRING, iItem - 1, 0);
            SendMessageW(m_hLst1, LB_DELETESTRING, iItem - 1, 0);
            SendMessageW(m_hLst1, LB_INSERTSTRING, iItem - 1, (LPARAM)sz1);
            SendMessageW(m_hLst1, LB_INSERTSTRING, iItem - 1, (LPARAM)sz2);
            SendMessageW(m_hLst1, LB_SETCURSEL, iItem - 1, 0);
        }
    }

    void OnDown(HWND hwnd)
    {
        INT iItem = (INT)SendMessageW(m_hLst1, LB_GETCURSEL, 0, 0);
        INT cItems = (INT)SendMessageW(m_hLst1, LB_GETCOUNT, 0, 0);
        if (iItem + 1 < cItems)
        {
            WCHAR sz1[MAX_PATH], sz2[MAX_PATH];
            sz1[0] = sz2[0] = 0;
            SendMessageW(m_hLst1, LB_GETTEXT, iItem, (LPARAM)sz1);
            SendMessageW(m_hLst1, LB_GETTEXT, iItem + 1, (LPARAM)sz2);
            SendMessageW(m_hLst1, LB_DELETESTRING, iItem, 0);
            SendMessageW(m_hLst1, LB_DELETESTRING, iItem, 0);
            SendMessageW(m_hLst1, LB_INSERTSTRING, iItem, (LPARAM)sz1);
            SendMessageW(m_hLst1, LB_INSERTSTRING, iItem, (LPARAM)sz2);
            SendMessageW(m_hLst1, LB_SETCURSEL, iItem + 1, 0);
        }
    }

    void OnOK(HWND hwnd)
    {
        EndDialog(IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case psh1:
            OnAdd(hwnd);
            break;
        case psh2:
            OnModify(hwnd);
            break;
        case psh3:
            OnDelete(hwnd);
            break;
        case psh4:
            OnUp(hwnd);
            break;
        case psh5:
            OnDown(hwnd);
            break;
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
