// MEditAccelDlg.hpp --- Dialogs for edit of Accelerator Table
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

#ifndef MZC4_MEDITACCELDLG_HPP_
#define MZC4_MEDITACCELDLG_HPP_

#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "MResizable.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "resource.h"

#include "AccelRes.hpp"

//////////////////////////////////////////////////////////////////////////////

void Cmb1_InitVirtualKeys(HWND hCmb1, ConstantsDB& db);
BOOL Cmb1_CheckKey(HWND hwnd, HWND hCmb1, BOOL bVirtKey, std::wstring& str);
void InitResNameComboBox(HWND hCmb, ConstantsDB& db, MIdOrString id, INT nIDTYPE_);
void InitResNameComboBox(HWND hCmb, ConstantsDB& db, MIdOrString id, INT nIDTYPE_1, INT nIDTYPE_2);
BOOL CheckCommand(ConstantsDB& db, MString& strCommand);
void ReplaceFullWithHalf(LPWSTR pszText);

std::wstring GetKeyID(ConstantsDB& db, UINT wId);

class MAddKeyDlg;
class MModifyKeyDlg;
class MEditAccelDlg;

//////////////////////////////////////////////////////////////////////////////

class MAddKeyDlg : public MDialogBase
{
public:
    ACCEL_ENTRY& m_entry;
    ConstantsDB& m_db;
    MComboBoxAutoComplete m_cmb1;
    MComboBoxAutoComplete m_cmb2;

    MAddKeyDlg(ACCEL_ENTRY& entry, ConstantsDB& db) :
        MDialogBase(IDD_ADDKEY), m_entry(entry), m_db(db)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        CheckDlgButton(hwnd, chx1, BST_CHECKED);

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        Cmb1_InitVirtualKeys(hCmb1, m_db);
        SubclassChildDx(m_cmb1, cmb1);

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        InitResNameComboBox(hCmb2, m_db, MIdOrString(L""), IDTYPE_COMMAND, IDTYPE_NEWCOMMAND);
        SubclassChildDx(m_cmb2, cmb2);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        GetWindowTextW(hCmb1, m_entry.sz0, _countof(m_entry.sz0));

        std::wstring str = m_entry.sz0;
        BOOL bVirtKey = IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED;
        if (!Cmb1_CheckKey(hwnd, hCmb1, bVirtKey, str))
        {
            ErrorBoxDx(IDS_INVALIDKEY);
            return;
        }
        lstrcpynW(m_entry.sz0, str.c_str(), _countof(m_entry.sz0));

        WORD wFlags = 0;
        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            wFlags |= FVIRTKEY;
        if (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED)
            wFlags |= FNOINVERT;
        if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
            wFlags |= FCONTROL;
        if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
            wFlags |= FSHIFT;
        if (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED)
            wFlags |= FALT;

        str = GetKeyFlags(wFlags);
        lstrcpynW(m_entry.sz1, str.c_str(), _countof(m_entry.sz1));

        GetDlgItemTextW(hwnd, cmb2, m_entry.sz2, _countof(m_entry.sz2));
        ReplaceFullWithHalf(m_entry.sz2);
        mstr_trim(m_entry.sz2);
        if (!CheckCommand(m_db, m_entry.sz2))
        {
            ErrorBoxDx(IDS_NOSUCHID);
            return;
        }

        EndDialog(IDOK);
    }

    void OnPsh1(HWND hwnd)
    {
        SendMessage(GetParent(GetParent(hwnd)), WM_COMMAND, ID_IDLIST, 0);
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
        case psh1:
            OnPsh1(hwnd);
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

//////////////////////////////////////////////////////////////////////////////

class MModifyKeyDlg : public MDialogBase
{
public:
    ACCEL_ENTRY& m_entry;
    ConstantsDB& m_db;
    MComboBoxAutoComplete m_cmb1;
    MComboBoxAutoComplete m_cmb2;

    MModifyKeyDlg(ACCEL_ENTRY& entry, ConstantsDB& db) :
        MDialogBase(IDD_MODIFYKEY), m_entry(entry), m_db(db)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        MIdOrString id(m_entry.sz2);
        InitResNameComboBox(hCmb2, m_db, id, IDTYPE_COMMAND, IDTYPE_NEWCOMMAND);
        SubclassChildDx(m_cmb2, cmb2);

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        Cmb1_InitVirtualKeys(hCmb1, m_db);
        SetDlgItemTextW(hwnd, cmb1, m_entry.sz0);
        SubclassChildDx(m_cmb1, cmb1);

        WORD wFlags;
        SetKeyFlags(wFlags, m_entry.sz1);
        if (wFlags & FVIRTKEY)
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
        if (wFlags & FNOINVERT)
            CheckDlgButton(hwnd, chx2, BST_CHECKED);
        if (wFlags & FCONTROL)
            CheckDlgButton(hwnd, chx3, BST_CHECKED);
        if (wFlags & FSHIFT)
            CheckDlgButton(hwnd, chx4, BST_CHECKED);
        if (wFlags & FALT)
            CheckDlgButton(hwnd, chx5, BST_CHECKED);

        if (wFlags & FVIRTKEY)
        {
            HWND hCmb1 = GetDlgItem(hwnd, cmb1);
            Cmb1_InitVirtualKeys(hCmb1, m_db);

            INT i = ComboBox_FindStringExact(hCmb1, -1, m_entry.sz0);
            if (i != CB_ERR)
            {
                ComboBox_SetCurSel(hCmb1, i);
            }
        }

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        GetWindowTextW(hCmb1, m_entry.sz0, _countof(m_entry.sz0));

        std::wstring str = m_entry.sz0;
        BOOL bVirtKey = IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED;
        if (!Cmb1_CheckKey(hwnd, hCmb1, bVirtKey, str))
        {
            ErrorBoxDx(IDS_INVALIDKEY);
            return;
        }
        lstrcpynW(m_entry.sz0, str.c_str(), _countof(m_entry.sz0));

        WORD wFlags = 0;
        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            wFlags |= FVIRTKEY;
        if (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED)
            wFlags |= FNOINVERT;
        if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
            wFlags |= FCONTROL;
        if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
            wFlags |= FSHIFT;
        if (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED)
            wFlags |= FALT;

        str = GetKeyFlags(wFlags);
        lstrcpynW(m_entry.sz1, str.c_str(), _countof(m_entry.sz1));

        GetDlgItemTextW(hwnd, cmb2, m_entry.sz2, _countof(m_entry.sz2));
        ReplaceFullWithHalf(m_entry.sz2);
        mstr_trim(m_entry.sz2);
        if (!CheckCommand(m_db, m_entry.sz2))
        {
            ErrorBoxDx(IDS_NOSUCHID);
            return;
        }

        EndDialog(IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case chx1:
            if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            {
                Cmb1_InitVirtualKeys(GetDlgItem(hwnd, cmb1), m_db);
            }
            else
            {
                SetDlgItemTextW(hwnd, cmb1, NULL);
            }
            break;
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
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

class MEditAccelDlg : public MDialogBase
{
public:
    AccelRes& m_accel_res;
    ConstantsDB& m_db;
    MResizable m_resizable;
    HICON m_hIcon;
    HICON m_hIconSm;
    HWND m_hLst1;

    MEditAccelDlg(AccelRes& accel_res, ConstantsDB& db)
        : MDialogBase(IDD_EDITACCEL), m_accel_res(accel_res), m_db(db)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);
        m_hLst1 = NULL;
    }

    ~MEditAccelDlg()
    {
        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    void OnUp(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem == 0)
            return;

        ACCEL_ENTRY ae0, ae1;
        ListView_GetItemText(m_hLst1, iItem - 1, 0, ae0.sz0, _countof(ae0.sz0));
        ListView_GetItemText(m_hLst1, iItem - 1, 1, ae0.sz1, _countof(ae0.sz1));
        ListView_GetItemText(m_hLst1, iItem - 1, 2, ae0.sz2, _countof(ae0.sz2));
        ListView_GetItemText(m_hLst1, iItem, 0, ae1.sz0, _countof(ae1.sz0));
        ListView_GetItemText(m_hLst1, iItem, 1, ae1.sz1, _countof(ae1.sz1));
        ListView_GetItemText(m_hLst1, iItem, 2, ae1.sz2, _countof(ae1.sz2));

        ListView_SetItemText(m_hLst1, iItem - 1, 0, ae1.sz0);
        ListView_SetItemText(m_hLst1, iItem - 1, 1, ae1.sz1);
        ListView_SetItemText(m_hLst1, iItem - 1, 2, ae1.sz2);
        ListView_SetItemText(m_hLst1, iItem, 0, ae0.sz0);
        ListView_SetItemText(m_hLst1, iItem, 1, ae0.sz1);
        ListView_SetItemText(m_hLst1, iItem, 2, ae0.sz2);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, iItem - 1, state, state);
    }

    void OnDown(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem + 1 == ListView_GetItemCount(m_hLst1))
            return;

        ACCEL_ENTRY ae0, ae1;
        ListView_GetItemText(m_hLst1, iItem, 0, ae0.sz0, _countof(ae0.sz0));
        ListView_GetItemText(m_hLst1, iItem, 1, ae0.sz1, _countof(ae0.sz1));
        ListView_GetItemText(m_hLst1, iItem, 2, ae0.sz2, _countof(ae0.sz2));
        ListView_GetItemText(m_hLst1, iItem + 1, 0, ae1.sz0, _countof(ae1.sz0));
        ListView_GetItemText(m_hLst1, iItem + 1, 1, ae1.sz1, _countof(ae1.sz1));
        ListView_GetItemText(m_hLst1, iItem + 1, 2, ae1.sz2, _countof(ae1.sz2));

        ListView_SetItemText(m_hLst1, iItem, 0, ae1.sz0);
        ListView_SetItemText(m_hLst1, iItem, 1, ae1.sz1);
        ListView_SetItemText(m_hLst1, iItem, 2, ae1.sz2);
        ListView_SetItemText(m_hLst1, iItem + 1, 0, ae0.sz0);
        ListView_SetItemText(m_hLst1, iItem + 1, 1, ae0.sz1);
        ListView_SetItemText(m_hLst1, iItem + 1, 2, ae0.sz2);

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
        ACCEL_ENTRY entry;

        MAddKeyDlg dialog(entry, m_db);
        if (IDOK != dialog.DialogBoxDx(hwnd))
        {
            return;
        }

        INT iItem = ListView_GetItemCount(m_hLst1);

        LV_ITEM item;

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = entry.sz0;
        ListView_InsertItem(m_hLst1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 1;
        item.pszText = entry.sz1;
        ListView_SetItem(m_hLst1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 2;
        item.pszText = entry.sz2;
        ListView_SetItem(m_hLst1, &item);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, iItem, state, state);
        ListView_EnsureVisible(m_hLst1, iItem, FALSE);
    }

    void OnModify(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
        {
            return;
        }

        ACCEL_ENTRY a_entry;
        ListView_GetItemText(m_hLst1, iItem, 0, a_entry.sz0, _countof(a_entry.sz0));
        ListView_GetItemText(m_hLst1, iItem, 1, a_entry.sz1, _countof(a_entry.sz1));
        ListView_GetItemText(m_hLst1, iItem, 2, a_entry.sz2, _countof(a_entry.sz2));

        MModifyKeyDlg dialog(a_entry, m_db);
        if (IDOK == dialog.DialogBoxDx(hwnd))
        {
            ListView_SetItemText(m_hLst1, iItem, 0, a_entry.sz0);
            ListView_SetItemText(m_hLst1, iItem, 1, a_entry.sz1);
            ListView_SetItemText(m_hLst1, iItem, 2, a_entry.sz2);
        }
    }

    void OnOK(HWND hwnd)
    {
        INT i, nCount = ListView_GetItemCount(m_hLst1);

        if (nCount == 0)
        {
            ErrorBoxDx(IDS_DATAISEMPTY);
            return;
        }

        m_accel_res.entries().clear();
        for (i = 0; i < nCount; ++i)
        {
            ACCEL_ENTRY a_entry;
            ListView_GetItemText(m_hLst1, i, 0, a_entry.sz0, _countof(a_entry.sz0));
            ListView_GetItemText(m_hLst1, i, 1, a_entry.sz1, _countof(a_entry.sz1));
            ListView_GetItemText(m_hLst1, i, 2, a_entry.sz2, _countof(a_entry.sz2));

            WORD wFlags;
            SetKeyFlags(wFlags, a_entry.sz1);

            AccelTableEntry entry;
            entry.fFlags = wFlags;
            if (wFlags & FVIRTKEY)
            {
                entry.wAscii = (WORD)m_db.GetValue(L"VIRTUALKEYS", a_entry.sz0);
            }
            else
            {
                std::wstring str, str2 = a_entry.sz0;
                LPCWSTR pch = str2.c_str();
                if (guts_quote(str, pch))
                {
                    entry.wAscii = str[0];
                }
                else
                {
                    entry.wAscii = (WORD)mstr_parse_int(a_entry.sz0);
                }
            }
            entry.wId = (WORD)m_db.GetResIDValue(a_entry.sz2);

            m_accel_res.entries().push_back(entry);
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

    void OnItemChanged(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        BOOL bSelected = (iItem != -1);
        EnableWindow(GetDlgItem(hwnd, psh2), bSelected);
        EnableWindow(GetDlgItem(hwnd, psh3), bSelected);
        EnableWindow(GetDlgItem(hwnd, psh4), bSelected);
        EnableWindow(GetDlgItem(hwnd, psh5), bSelected);
    }

    void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        BOOL bSelected = (iItem != -1);
        EnableMenuItem(hMenu, psh2, bSelected ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, psh3, bSelected ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, psh4, bSelected ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(hMenu, psh5, bSelected ? MF_ENABLED : MF_GRAYED);
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

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        m_hLst1 = GetDlgItem(hwnd, lst1);
        ListView_SetExtendedListViewStyle(m_hLst1, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 105;
        column.pszText = LoadStringDx(IDS_KEY);
        column.iSubItem = 0;
        ListView_InsertColumn(m_hLst1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 75;
        column.pszText = LoadStringDx(IDS_FLAGS);
        column.iSubItem = 1;
        ListView_InsertColumn(m_hLst1, 1, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 185;
        column.pszText = LoadStringDx(IDS_COMMANDID);
        column.iSubItem = 2;
        ListView_InsertColumn(m_hLst1, 2, &column);

        typedef AccelRes::entries_type entries_type;
        const entries_type& entries = m_accel_res.entries();

        INT i = 0;
        entries_type::const_iterator it, end = entries.end();
        for (it = entries.begin(); it != end; ++it, ++i)
        {
            std::wstring str;
            if (it->fFlags & FVIRTKEY)
            {
                str = m_db.GetName(L"VIRTUALKEYS", it->wAscii);
            }
            else
            {
                str += (WCHAR)it->wAscii;
                str = mstr_quote(str);
            }

            LV_ITEM item;
            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 0;
            item.pszText = &str[0];
            ListView_InsertItem(m_hLst1, &item);

            str = GetKeyFlags(it->fFlags);

            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 1;
            item.pszText = &str[0];
            ListView_SetItem(m_hLst1, &item);

            str = GetKeyID(m_db, it->wId);

            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 2;
            item.pszText = &str[0];
            ListView_SetItem(m_hLst1, &item);
        }

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, 0, state, state);
        SetFocus(m_hLst1);

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

        CenterWindowDx();
        return TRUE;
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        if (hwndContext == m_hLst1)
        {
            HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_POPUPMENUS));
            HMENU hSubMenu = GetSubMenu(hMenu, 7);

            if (xPos == 0xFFFF && yPos == 0xFFFF)
            {
                RECT rc;
                GetWindowRect(m_hLst1, &rc);
                xPos = rc.left;
                yPos = rc.top;
            }

            SetForegroundWindow(hwnd);
            TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                xPos, yPos, 0, hwnd, NULL);
            PostMessage(hwnd, WM_NULL, 0, 0);
            DestroyMenu(hMenu);
        }
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MEDITACCELDLG_HPP_
