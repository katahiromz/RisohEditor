// MMacrosDlg.hpp --- Dialogs for Predefined Macros
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

#ifndef MZC4_MMACROSDLG_HPP_
#define MZC4_MMACROSDLG_HPP_

#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "MResizable.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "resource.h"

struct MACRO_ENTRY;
class MAddMacroDlg;
class MMacrosDlg;

//////////////////////////////////////////////////////////////////////////////

struct MACRO_ENTRY
{
    TCHAR szKey[128];
    TCHAR szValue[256];
};

class MAddMacroDlg : public MDialogBase
{
public:
    MACRO_ENTRY& m_entry;
    ConstantsDB& m_db;
    MComboBoxAutoComplete m_cmb1;
    MComboBoxAutoComplete m_cmb2;

    MAddMacroDlg(MACRO_ENTRY& entry, ConstantsDB& db) :
        MDialogBase(IDD_ADDMACRO), m_entry(entry), m_db(db)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        CheckDlgButton(hwnd, chx1, BST_CHECKED);

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        SubclassChildDx(m_cmb1, cmb1);

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        SubclassChildDx(m_cmb2, cmb2);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);

        ::GetWindowText(hCmb1, m_entry.szKey, _countof(m_entry.szKey));
        ::GetWindowText(hCmb2, m_entry.szValue, _countof(m_entry.szValue));

        mstr_trim(m_entry.szKey);
        mstr_trim(m_entry.szValue);

        if (m_entry.szKey[0] == 0)
        {
            ComboBox_SetEditSel(hCmb1, 0, -1);
            SetFocus(hCmb1);
            ErrorBoxDx(IDS_EMPTYSTR);
            return;
        }

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

class MMacrosDlg : public MDialogBase
{
public:
    macro_map_type& m_map;
    ConstantsDB& m_db;
    MResizable m_resizable;
    HWND m_hLst1;
    HICON m_hIcon;
    HICON m_hIconSm;

    MMacrosDlg(macro_map_type& map, ConstantsDB& db)
        : MDialogBase(IDD_MACROS), m_map(map), m_db(db)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);
    }

    ~MMacrosDlg()
    {
        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    void OnUp(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem == 0)
            return;

        MACRO_ENTRY me0, me1;
        ListView_GetItemText(m_hLst1, iItem - 1, 0, me0.szKey, _countof(me0.szKey));
        ListView_GetItemText(m_hLst1, iItem - 1, 1, me0.szValue, _countof(me0.szValue));
        ListView_GetItemText(m_hLst1, iItem, 0, me1.szKey, _countof(me1.szKey));
        ListView_GetItemText(m_hLst1, iItem, 1, me1.szValue, _countof(me1.szValue));

        ListView_SetItemText(m_hLst1, iItem - 1, 0, me1.szKey);
        ListView_SetItemText(m_hLst1, iItem - 1, 1, me1.szValue);
        ListView_SetItemText(m_hLst1, iItem, 0, me0.szKey);
        ListView_SetItemText(m_hLst1, iItem, 1, me0.szValue);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, iItem - 1, state, state);
    }

    void OnDown(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem + 1 == ListView_GetItemCount(m_hLst1))
            return;

        MACRO_ENTRY me0, me1;
        ListView_GetItemText(m_hLst1, iItem, 0, me0.szKey, _countof(me0.szKey));
        ListView_GetItemText(m_hLst1, iItem, 1, me0.szValue, _countof(me0.szValue));
        ListView_GetItemText(m_hLst1, iItem + 1, 0, me1.szKey, _countof(me1.szKey));
        ListView_GetItemText(m_hLst1, iItem + 1, 1, me1.szValue, _countof(me1.szValue));

        ListView_SetItemText(m_hLst1, iItem, 0, me1.szKey);
        ListView_SetItemText(m_hLst1, iItem, 1, me1.szValue);
        ListView_SetItemText(m_hLst1, iItem + 1, 0, me0.szKey);
        ListView_SetItemText(m_hLst1, iItem + 1, 1, me0.szValue);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, iItem + 1, state, state);
    }

    void OnDelete(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem >= 0)
        {
            ListView_DeleteItem(m_hLst1, iItem);
        }
    }

    void OnAdd(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
        {
            return;
        }

        MACRO_ENTRY entry;
        MAddMacroDlg dialog(entry, m_db);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            INT iItem = ListView_GetItemCount(m_hLst1);
            LV_ITEM item;

            ZeroMemory(&item, sizeof(item));
            item.iItem = iItem;
            item.mask = LVIF_TEXT;
            item.iSubItem = 0;
            item.pszText = entry.szKey;
            ListView_InsertItem(m_hLst1, &item);

            ZeroMemory(&item, sizeof(item));
            item.iItem = iItem;
            item.mask = LVIF_TEXT;
            item.iSubItem = 1;
            item.pszText = entry.szValue;
            ListView_SetItem(m_hLst1, &item);

            UINT state = LVIS_SELECTED | LVIS_FOCUSED;
            ListView_SetItemState(m_hLst1, iItem, state, state);
        }
    }

    void OnModify(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
        {
            return;
        }

        MACRO_ENTRY entry;
        ListView_GetItemText(m_hLst1, iItem, 0, entry.szKey, _countof(entry.szKey));
        ListView_GetItemText(m_hLst1, iItem, 1, entry.szValue, _countof(entry.szValue));

        mstr_trim(entry.szKey);
        mstr_trim(entry.szValue);

        if (entry.szKey[0] == 0)
        {
            HWND hCmb1 = GetDlgItem(hwnd, cmb1);
            ComboBox_SetEditSel(hCmb1, 0, -1);
            SetFocus(hCmb1);
            ErrorBoxDx(IDS_EMPTYSTR);
            return;
        }

        LV_ITEM item;

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = entry.szKey;
        ListView_SetItem(m_hLst1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 1;
        item.pszText = entry.szValue;
        ListView_SetItem(m_hLst1, &item);
    }

    void OnOK(HWND hwnd)
    {
        INT i, nCount = ListView_GetItemCount(m_hLst1);

        if (nCount == 0)
        {
            ErrorBoxDx(IDS_DATAISEMPTY);
            return;
        }

        MACRO_ENTRY entry;

        m_map.clear();
        for (i = 0; i < nCount; ++i)
        {
            ListView_GetItemText(m_hLst1, i, 0, entry.szKey, _countof(entry.szKey));
            ListView_GetItemText(m_hLst1, i, 1, entry.szValue, _countof(entry.szValue));
            m_map[entry.szKey] = entry.szValue;
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
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(IDCANCEL);
            break;
        }
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
        }
        return DefaultProcDx();
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        m_resizable.OnSize();
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        m_hLst1 = GetDlgItem(hwnd, lst1);
        ListView_SetExtendedListViewStyle(m_hLst1, LVS_EX_FULLROWSELECT);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 200;
        column.pszText = LoadStringDx(IDS_MACRONAME);
        column.iSubItem = 0;
        ListView_InsertColumn(m_hLst1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 300;
        column.pszText = LoadStringDx(IDS_MACROVALUE);
        column.iSubItem = 1;
        ListView_InsertColumn(m_hLst1, 1, &column);

        LV_ITEM item;
        INT iItem = 0;

        macro_map_type::iterator it, end = m_map.end();
        for (it = m_map.begin(); it != end; ++it)
        {
            ZeroMemory(&item, sizeof(item));
            item.iItem = iItem;
            item.mask = LVIF_TEXT;
            item.iSubItem = 0;
            item.pszText = const_cast<LPTSTR>(it->first.c_str());
            ListView_InsertItem(m_hLst1, &item);

            ZeroMemory(&item, sizeof(item));
            item.iItem = iItem;
            item.mask = LVIF_TEXT;
            item.iSubItem = 1;
            item.pszText = const_cast<LPTSTR>(it->second.c_str());
            ListView_SetItem(m_hLst1, &item);

            ++iItem;
        }

        m_resizable.OnParentCreate(hwnd);

        m_resizable.SetLayoutAnchor(lst1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(psh1, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(psh2, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(psh3, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(psh4, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(psh5, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(IDOK, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(IDCANCEL, mzcLA_BOTTOM_RIGHT);

        SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, 0, state, state);

        SetFocus(m_hLst1);

        CenterWindowDx();
        return TRUE;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MMACROSDLG_HPP_
