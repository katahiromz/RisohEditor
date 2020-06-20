// MEditMenuDlg.hpp --- Dialogs for edit of Menus
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
#include "ConstantsDB.hpp"
#include "MComboBoxAutoComplete.hpp"

#include "MenuRes.hpp"

class MAddMItemDlg;
class MModifyMItemDlg;
class MEditMenuDlg;

void InitCtrlIDComboBox(HWND hCmb);
BOOL CheckCommand(MString strCommand);
void InitResNameComboBox(HWND hCmb, MIdOrString id, IDTYPE_ nIDTYPE_);
void InitResNameComboBox(HWND hCmb, MIdOrString id, INT nIDTYPE_1, INT nIDTYPE_2);
void ReplaceFullWithHalf(LPWSTR pszText);

//////////////////////////////////////////////////////////////////////////////

class MAddMItemDlg : public MDialogBase
{
public:
    MENU_ENTRY& m_entry;
    MComboBoxAutoComplete m_cmb2;
    MComboBoxAutoComplete m_cmb3;

    MAddMItemDlg(MENU_ENTRY& entry)
        : MDialogBase(IDD_ADDMITEM), m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        InitCtrlIDComboBox(GetDlgItem(hwnd, cmb2));
        SubclassChildDx(m_cmb2, cmb2);
        SetDlgItemText(hwnd, cmb2, L"0");

        InitResNameComboBox(GetDlgItem(hwnd, cmb3), MIdOrString(L""), IDTYPE_HELP);
        SetDlgItemInt(hwnd, cmb3, 0, TRUE);
        SubclassChildDx(m_cmb3, cmb3);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        GetDlgItemTextW(hwnd, cmb1, m_entry.szCaption, _countof(m_entry.szCaption));
        if (m_entry.szCaption[0] == L'"')
        {
            mstr_unquote(m_entry.szCaption);
        }

        GetDlgItemTextW(hwnd, cmb2, m_entry.szCommandID, _countof(m_entry.szCommandID));
        ReplaceFullWithHalf(m_entry.szCommandID);
        mstr_trim(m_entry.szCommandID);
        if (!CheckCommand(m_entry.szCommandID))
        {
            ErrorBoxDx(IDS_NOSUCHID);
            return;
        }

        DWORD dwType = 0, dwState = 0;
        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            dwState |= MFS_GRAYED;
        if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
            dwType |= MFT_BITMAP;
        if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
            dwType |= MFT_OWNERDRAW;
        if (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED)
            dwState |= MFS_CHECKED;
        if (IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED)
            dwType |= MFT_SEPARATOR;
        if (IsDlgButtonChecked(hwnd, chx7) == BST_CHECKED)
            dwType |= MFT_MENUBARBREAK;
        if (IsDlgButtonChecked(hwnd, chx8) == BST_CHECKED)
            dwType |= MFT_MENUBREAK;
        if (IsDlgButtonChecked(hwnd, chx9) == BST_CHECKED)
            dwState |= MFS_DEFAULT;
        if (IsDlgButtonChecked(hwnd, chx10) == BST_CHECKED)
            dwState |= MFS_HILITE;
        if (IsDlgButtonChecked(hwnd, chx11) == BST_CHECKED)
            dwType |= MFT_RADIOCHECK;
        if (IsDlgButtonChecked(hwnd, chx12) == BST_CHECKED)
            dwType |= MFT_RIGHTORDER;
        if (IsDlgButtonChecked(hwnd, chx13) == BST_CHECKED)
            dwType |= MFT_RIGHTJUSTIFY;

        std::wstring str = GetMenuTypeAndState(dwType, dwState);
        if (m_entry.szCaption[0] == 0 ||
            lstrcmpiW(m_entry.szCaption, LoadStringDx(IDS_SEPARATOR)) == 0)
        {
            m_entry.szCaption[0] = 0;
            dwType |= MFT_SEPARATOR;
            str = GetMenuTypeAndState(dwType, dwState);
        }
        lstrcpynW(m_entry.szFlags, str.c_str(), _countof(m_entry.szFlags));

        GetDlgItemTextW(hwnd, cmb3, m_entry.szHelpID, _countof(m_entry.szHelpID));
        ReplaceFullWithHalf(m_entry.szHelpID);
        mstr_trim(m_entry.szHelpID);

        DWORD help = g_db.GetResIDValue(m_entry.szHelpID);
        MString strHelp = g_db.GetNameOfResID(IDTYPE_HELP, help);
        ReplaceFullWithHalf(strHelp);
        lstrcpynW(m_entry.szHelpID, strHelp.c_str(), _countof(m_entry.szHelpID));

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
        case chx6:
            if (IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED)
            {
                SetDlgItemTextW(hwnd, cmb1, NULL);
                SetDlgItemInt(hwnd, cmb2, 0, FALSE);
                SetDlgItemInt(hwnd, edt1, 0, FALSE);
            }
            break;
        case psh1:
            OnPsh1(hwnd);
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
        }
    }

    void OnPsh1(HWND hwnd)
    {
        SendMessage(GetParent(GetParent(hwnd)), WM_COMMAND, ID_IDLIST, 0);
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

//////////////////////////////////////////////////////////////////////////////

class MModifyMItemDlg : public MDialogBase
{
public:
    MENU_ENTRY& m_entry;
    MComboBoxAutoComplete m_cmb2;
    MComboBoxAutoComplete m_cmb3;

    MModifyMItemDlg(MENU_ENTRY& entry)
        : MDialogBase(IDD_MODIFYMITEM), m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemTextW(hwnd, cmb1, mstr_quote(m_entry.szCaption).c_str());

        InitCtrlIDComboBox(GetDlgItem(hwnd, cmb2));
        SetDlgItemText(hwnd, cmb2, m_entry.szCommandID);
        SubclassChildDx(m_cmb2, cmb2);

        MIdOrString help_id(m_entry.szHelpID);
        InitResNameComboBox(GetDlgItem(hwnd, cmb3), help_id, IDTYPE_HELP);
        SubclassChildDx(m_cmb3, cmb3);

        DWORD dwType, dwState;
        dwType = dwState = 0;
        SetMenuTypeAndState(dwType, dwState, m_entry.szFlags);

        if (lstrcmpiW(m_entry.szCaption, LoadStringDx(IDS_SEPARATOR)) == 0 ||
            m_entry.szCaption[0] == 0 || (dwType & MFT_SEPARATOR))
        {
            dwType |= MFT_SEPARATOR;
            SetDlgItemTextW(hwnd, cmb1, NULL);
        }

        if ((dwState & MFS_GRAYED) == MFS_GRAYED)
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
        if ((dwType & MFT_BITMAP) == MFT_BITMAP)
            CheckDlgButton(hwnd, chx3, BST_CHECKED);
        if ((dwType & MFT_OWNERDRAW) == MFT_OWNERDRAW)
            CheckDlgButton(hwnd, chx4, BST_CHECKED);
        if ((dwState & MFS_CHECKED) == MFS_CHECKED)
            CheckDlgButton(hwnd, chx5, BST_CHECKED);
        if ((dwType & MFT_SEPARATOR) == MFT_SEPARATOR)
            CheckDlgButton(hwnd, chx6, BST_CHECKED);
        if ((dwType & MFT_MENUBARBREAK) == MFT_MENUBARBREAK)
            CheckDlgButton(hwnd, chx7, BST_CHECKED);
        if ((dwType & MFT_MENUBREAK) == MFT_MENUBREAK)
            CheckDlgButton(hwnd, chx8, BST_CHECKED);
        if ((dwState & MFS_DEFAULT) == MFS_DEFAULT)
            CheckDlgButton(hwnd, chx9, BST_CHECKED);
        if ((dwState & MFS_HILITE) == MFS_HILITE)
            CheckDlgButton(hwnd, chx10, BST_CHECKED);
        if ((dwType & MFT_RADIOCHECK) == MFT_RADIOCHECK)
            CheckDlgButton(hwnd, chx11, BST_CHECKED);
        if ((dwType & MFT_RIGHTORDER) == MFT_RIGHTORDER)
            CheckDlgButton(hwnd, chx12, BST_CHECKED);
        if ((dwType & MFT_RIGHTJUSTIFY) == MFT_RIGHTJUSTIFY)
            CheckDlgButton(hwnd, chx13, BST_CHECKED);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        GetDlgItemTextW(hwnd, cmb1, m_entry.szCaption, _countof(m_entry.szCaption));
        if (m_entry.szCaption[0] == L'"')
        {
            mstr_unquote(m_entry.szCaption);
        }

        GetDlgItemTextW(hwnd, cmb2, m_entry.szCommandID, _countof(m_entry.szCommandID));
        ReplaceFullWithHalf(m_entry.szCommandID);
        mstr_trim(m_entry.szCommandID);
        if (!CheckCommand(m_entry.szCommandID))
        {
            ErrorBoxDx(IDS_NOSUCHID);
            return;
        }

        DWORD dwType = 0, dwState = 0;
        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            dwState |= MFS_GRAYED;
        if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
            dwType |= MFT_BITMAP;
        if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
            dwType |= MFT_OWNERDRAW;
        if (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED)
            dwState |= MFS_CHECKED;
        if (IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED)
            dwType |= MFT_SEPARATOR;
        if (IsDlgButtonChecked(hwnd, chx7) == BST_CHECKED)
            dwType |= MFT_MENUBARBREAK;
        if (IsDlgButtonChecked(hwnd, chx8) == BST_CHECKED)
            dwType |= MFT_MENUBREAK;
        if (IsDlgButtonChecked(hwnd, chx9) == BST_CHECKED)
            dwState |= MFS_DEFAULT;
        if (IsDlgButtonChecked(hwnd, chx10) == BST_CHECKED)
            dwState |= MFS_HILITE;
        if (IsDlgButtonChecked(hwnd, chx11) == BST_CHECKED)
            dwType |= MFT_RADIOCHECK;
        if (IsDlgButtonChecked(hwnd, chx12) == BST_CHECKED)
            dwType |= MFT_RIGHTORDER;
        if (IsDlgButtonChecked(hwnd, chx13) == BST_CHECKED)
            dwType |= MFT_RIGHTJUSTIFY;

        if (lstrcmpiW(m_entry.szCaption, LoadStringDx(IDS_SEPARATOR)) == 0 ||
            m_entry.szCaption[0] == 0 || (dwType & MFT_SEPARATOR))
        {
            m_entry.szCaption[0] = 0;
            dwType |= MFT_SEPARATOR;
        }

        std::wstring str = GetMenuTypeAndState(dwType, dwState);
        lstrcpynW(m_entry.szFlags, str.c_str(), _countof(m_entry.szFlags));

        GetDlgItemTextW(hwnd, cmb3, m_entry.szHelpID, _countof(m_entry.szHelpID));
        DWORD help = g_db.GetResIDValue(m_entry.szHelpID);
        MString strHelp = g_db.GetNameOfResID(IDTYPE_HELP, help);
        lstrcpynW(m_entry.szHelpID, strHelp.c_str(), _countof(m_entry.szHelpID));

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
        case chx6:
            if (IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED)
            {
                SetDlgItemTextW(hwnd, cmb1, NULL);
                SetDlgItemInt(hwnd, cmb2, 0, FALSE);
                SetDlgItemInt(hwnd, cmb3, 0, FALSE);
            }
            break;
        case psh1:
            OnPsh1(hwnd);
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
        }
    }

    void OnPsh1(HWND hwnd)
    {
        SendMessage(GetParent(GetParent(hwnd)), WM_COMMAND, ID_IDLIST, 0);
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

//////////////////////////////////////////////////////////////////////////////

class MEditMenuDlg : public MDialogBase
{
public:
    MenuRes& m_menu_res;
    MResizable m_resizable;
    HICON m_hIcon;
    HICON m_hIconSm;
    HWND m_hLst1;

    MEditMenuDlg(MenuRes& menu_res)
        : MDialogBase(IDD_EDITMENU), m_menu_res(menu_res)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);
        m_hLst1 = NULL;
    }

    ~MEditMenuDlg()
    {
        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        if (m_menu_res.IsExtended())
            CheckDlgButton(hwnd, chx1, BST_CHECKED);

        m_hLst1 = GetDlgItem(hwnd, lst1);
        ListView_SetExtendedListViewStyle(m_hLst1, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 225;
        column.pszText = LoadStringDx(IDS_CAPTION);
        column.iSubItem = 0;
        ListView_InsertColumn(m_hLst1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 95;
        column.pszText = LoadStringDx(IDS_FLAGS);
        column.iSubItem = 1;
        ListView_InsertColumn(m_hLst1, 1, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 150;
        column.pszText = LoadStringDx(IDS_COMMANDID);
        column.iSubItem = 2;
        ListView_InsertColumn(m_hLst1, 2, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 180;
        column.pszText = LoadStringDx(IDS_HELPID);
        column.iSubItem = 3;
        ListView_InsertColumn(m_hLst1, 3, &column);

        INT i = 0;
        std::wstring str;
        LV_ITEM item;
        if (m_menu_res.IsExtended())
        {
            typedef MenuRes::ExMenuItemsType exitems_type;
            exitems_type& exitems = m_menu_res.exitems();
            for (auto& element : exitems)
            {
                str = mstr_repeat(LoadStringDx(IDS_INDENT), element.wDepth);
                if (element.text.empty() && element.menuId == 0)
                {
                    str += LoadStringDx(IDS_SEPARATOR);
                    element.dwType |= MFT_SEPARATOR;
                }
                else
                {
                    str += mstr_quote(element.text);
                }

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 0;
                item.pszText = &str[0];
                ListView_InsertItem(m_hLst1, &item);

                str = GetMenuTypeAndState(element.dwType, element.dwState);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 1;
                item.pszText = &str[0];
                ListView_SetItem(m_hLst1, &item);

                str = g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, element.menuId, true);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 2;
                item.pszText = &str[0];
                ListView_SetItem(m_hLst1, &item);

                str = g_db.GetNameOfResID(IDTYPE_HELP, element.dwHelpId);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 3;
                item.pszText = &str[0];
                ListView_SetItem(m_hLst1, &item);

                ++i;
            }
        }
        else
        {
            typedef MenuRes::MenuItemsType items_type;
            items_type& items = m_menu_res.items();
            for (auto& element : items)
            {
                str = mstr_repeat(LoadStringDx(IDS_INDENT), element.wDepth);
                if (element.text.empty() && element.wMenuID == 0)
                {
                    str += LoadStringDx(IDS_SEPARATOR);
                }
                else
                {
                    str += mstr_quote(element.text);
                }

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 0;
                item.pszText = &str[0];
                ListView_InsertItem(m_hLst1, &item);

                str = GetMenuFlags(element.fItemFlags);
                if (element.text.empty() && element.wMenuID == 0)
                    str += L"S ";

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 1;
                item.pszText = &str[0];
                ListView_SetItem(m_hLst1, &item);

                str = g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, element.wMenuID, true);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 2;
                item.pszText = &str[0];
                ListView_SetItem(m_hLst1, &item);

                str = TEXT("0");

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 3;
                item.pszText = &str[0];
                ListView_SetItem(m_hLst1, &item);

                ++i;
            }
        }

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, 0, state, state);
        SetFocus(m_hLst1);

        m_resizable.OnParentCreate(hwnd);

        m_resizable.SetLayoutAnchor(lst1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(psh1, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(psh2, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(psh3, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(chx1, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(psh4, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(psh5, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(psh6, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(psh7, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(IDOK, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(IDCANCEL, mzcLA_BOTTOM_RIGHT);

        SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

        CenterWindowDx();
        return TRUE;
    }

    void OnAdd(HWND hwnd)
    {
        MENU_ENTRY m_entry;
        ZeroMemory(&m_entry, sizeof(m_entry));
        MAddMItemDlg dialog(m_entry);
        INT nID = (INT)dialog.DialogBoxDx(hwnd);
        if (IDOK != nID)
        {
            return;
        }

        INT iItem = ListView_GetItemCount(m_hLst1);

        MStringW str, strIndent = LoadStringDx(IDS_INDENT);
        str = mstr_quote(m_entry.szCaption);
        if (str.empty() || wcsstr(m_entry.szFlags, L"S ") != NULL)
            str = LoadStringDx(IDS_SEPARATOR);
        str = mstr_repeat(strIndent, m_entry.wDepth) + str;

        LV_ITEM item;

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = &str[0];
        ListView_InsertItem(m_hLst1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 1;
        item.pszText = m_entry.szFlags;
        ListView_SetItem(m_hLst1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 2;
        item.pszText = m_entry.szCommandID;
        ListView_SetItem(m_hLst1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 3;
        item.pszText = m_entry.szHelpID;
        ListView_SetItem(m_hLst1, &item);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, iItem, state, state);
        ListView_EnsureVisible(m_hLst1, iItem, FALSE);
    }

    BOOL GetEntry(HWND hwnd, MENU_ENTRY& entry, INT iItem)
    {
        WCHAR szCaption[256];
        ListView_GetItemText(m_hLst1, iItem, 0, szCaption, _countof(szCaption));

        entry.wDepth = 0;
        MStringW str = szCaption, strIndent = LoadStringDx(IDS_INDENT);
        while (str.find(strIndent) == 0)
        {
            str = str.substr(strIndent.size());
            ++entry.wDepth;
        }
        if (str[0] == L'"')
        {
            mstr_unquote(str);
        }
        if (str.empty() || str == LoadStringDx(IDS_SEPARATOR))
        {
            str.clear();
        }

        lstrcpynW(entry.szCaption, str.c_str(), _countof(entry.szCaption));

        ListView_GetItemText(m_hLst1, iItem, 1, entry.szFlags, _countof(entry.szFlags));
        ListView_GetItemText(m_hLst1, iItem, 2, entry.szCommandID, _countof(entry.szCommandID));
        ListView_GetItemText(m_hLst1, iItem, 3, entry.szHelpID, _countof(entry.szHelpID));
        return TRUE;
    }

    BOOL SetEntry(HWND hwnd, MENU_ENTRY& entry, INT iItem)
    {
        MStringW str, strIndent = LoadStringDx(IDS_INDENT);
        str = mstr_repeat(strIndent, entry.wDepth);

        if (entry.szCaption[0] == 0 || wcsstr(entry.szFlags, L"S ") != NULL)
            str += LoadStringDx(IDS_SEPARATOR);
        else
            str += mstr_quote(entry.szCaption);

        ListView_SetItemText(m_hLst1, iItem, 0, &str[0]);
        ListView_SetItemText(m_hLst1, iItem, 1, entry.szFlags);
        ListView_SetItemText(m_hLst1, iItem, 2, entry.szCommandID);
        ListView_SetItemText(m_hLst1, iItem, 3, entry.szHelpID);
        return TRUE;
    }

    void OnModify(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
        {
            return;
        }

        MENU_ENTRY m_entry;
        GetEntry(hwnd, m_entry, iItem);
        
        MModifyMItemDlg dialog(m_entry);
        INT nID = (INT)dialog.DialogBoxDx(hwnd);
        if (IDOK == nID)
        {
            SetEntry(hwnd, m_entry, iItem);
        }
    }

    void OnDelete(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem >= 0)
        {
            ListView_DeleteItem(m_hLst1, iItem);
        }
    }

    void OnUp(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem <= 0)
            return;

        MENU_ENTRY entry0, entry1;

        GetEntry(hwnd, entry0, iItem - 1);
        GetEntry(hwnd, entry1, iItem);

        SetEntry(hwnd, entry1, iItem - 1);
        SetEntry(hwnd, entry0, iItem);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, iItem - 1, state, state);
    }

    void OnDown(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
            return;

        INT nCount = ListView_GetItemCount(m_hLst1);
        if (iItem + 1 >= nCount)
            return;

        MENU_ENTRY entry0, entry1;

        GetEntry(hwnd, entry0, iItem);
        GetEntry(hwnd, entry1, iItem + 1);

        SetEntry(hwnd, entry1, iItem);
        SetEntry(hwnd, entry0, iItem + 1);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, iItem + 1, state, state);
    }

    void OnLeft(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
            return;

        WCHAR szCaption[128];
        ListView_GetItemText(m_hLst1, iItem, 0, szCaption, _countof(szCaption));

        std::wstring strIndent = LoadStringDx(IDS_INDENT);

        std::wstring str = szCaption;
        if (str.find(strIndent) == 0)
        {
            str = str.substr(strIndent.size());
        }

        ListView_SetItemText(m_hLst1, iItem, 0, &str[0]);
    }

    void OnRight(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
            return;

        if (iItem == 0)
            return;

        WCHAR CaptionUp[128];
        ListView_GetItemText(m_hLst1, iItem - 1, 0, CaptionUp, _countof(CaptionUp));
        WCHAR szCaption[128];
        ListView_GetItemText(m_hLst1, iItem, 0, szCaption, _countof(szCaption));

        MStringW strIndent = LoadStringDx(IDS_INDENT);
        size_t depth_up = mstr_repeat_count(CaptionUp, strIndent);
        size_t depth = mstr_repeat_count(szCaption, strIndent);

        if (depth_up < depth)
            return;

        std::wstring str = strIndent + szCaption;
        ListView_SetItemText(m_hLst1, iItem, 0, &str[0]);
    }

    void OnOK(HWND hwnd)
    {
        MENU_ENTRY entry;
        INT iItem, nCount = ListView_GetItemCount(m_hLst1);

        if (nCount == 0)
        {
            ErrorBoxDx(IDS_DATAISEMPTY);
            return;
        }

        BOOL Extended = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
        if (Extended)
        {
            m_menu_res.header().wVersion = 1;
            m_menu_res.header().wOffset = 4;
            m_menu_res.header().dwHelpId = 0;
            m_menu_res.exitems().clear();
            for (iItem = 0; iItem < nCount; ++iItem)
            {
                GetEntry(hwnd, entry, iItem);

                MenuRes::ExMenuItem exitem;

                SetMenuTypeAndState(exitem.dwType, exitem.dwState, entry.szFlags);
                exitem.menuId = g_db.GetResIDValue(entry.szCommandID);
                exitem.bResInfo = 0;
                exitem.text = entry.szCaption;
                exitem.dwHelpId = g_db.GetResIDValue(entry.szHelpID);
                exitem.wDepth = entry.wDepth;

                m_menu_res.exitems().push_back(exitem);
            }
        }
        else
        {
            m_menu_res.header().wVersion = 0;
            m_menu_res.header().wOffset = 4;
            m_menu_res.header().dwHelpId = 0;
            m_menu_res.items().clear();
            for (iItem = 0; iItem < nCount; ++iItem)
            {
                GetEntry(hwnd, entry, iItem);

                MenuRes::MenuItem item;

                SetMenuFlags(item.fItemFlags, entry.szFlags);
                item.wMenuID = (WORD)g_db.GetResIDValue(entry.szCommandID);
                item.wDepth = entry.wDepth;
                item.text = entry.szCaption;

                m_menu_res.items().push_back(item);
            }
        }

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
        case psh6:
            OnLeft(hwnd);
            break;
        case psh7:
            OnRight(hwnd);
            break;
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(IDCANCEL);
            break;
        }
    }

    void OnItemChanged(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        BOOL bSelected = (iItem != -1);
        EnableWindow(GetDlgItem(hwnd, psh2), bSelected);
        EnableWindow(GetDlgItem(hwnd, psh3), bSelected);
        EnableWindow(GetDlgItem(hwnd, psh4), bSelected);
        EnableWindow(GetDlgItem(hwnd, psh5), bSelected);
        EnableWindow(GetDlgItem(hwnd, psh6), bSelected);
        EnableWindow(GetDlgItem(hwnd, psh7), bSelected);
    }

    void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        BOOL bSelected = (iItem != -1);
        EnableMenuItem(hMenu, psh2, bSelected ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, psh3, bSelected ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, psh4, bSelected ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, psh5, bSelected ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, psh6, bSelected ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, psh7, bSelected ? MF_ENABLED : MF_GRAYED);
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
    {
        if (idFrom == lst1)
        {
            if (pnmhdr->code == LVN_KEYDOWN)
            {
                LV_KEYDOWN *KeyDown = (LV_KEYDOWN *)pnmhdr;
                if (KeyDown->wVKey == VK_DELETE)
                {
                    OnDelete(hwnd);
                    return 0;
                }
            }
            if (pnmhdr->code == NM_DBLCLK)
            {
                OnModify(hwnd);
                return 0;
            }
            if (pnmhdr->code == LVN_GETINFOTIP)
            {
                NMLVGETINFOTIP *pGetInfoTip = (NMLVGETINFOTIP *)pnmhdr;
                INT iItem = pGetInfoTip->iItem;
                INT iSubItem = pGetInfoTip->iSubItem;
                TCHAR szText[128];
                ListView_GetItemText(m_hLst1, iItem, iSubItem, szText, _countof(szText));
                StringCchCopy(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, szText);
            }
            if (pnmhdr->code == LVN_ITEMCHANGED)
            {
                //NM_LISTVIEW *pListView = (NM_LISTVIEW *)pnmhdr;
                OnItemChanged(hwnd);
            }
        }
        return 0;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
            HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
            HANDLE_MSG(hwnd, WM_INITMENUPOPUP, OnInitMenuPopup);
        }
        return DefaultProcDx();
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        m_resizable.OnSize();
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        if (hwndContext == m_hLst1)
        {
            PopupMenuDx(hwnd, m_hLst1, IDR_POPUPMENUS, 6, xPos, yPos);
        }
    }
};
