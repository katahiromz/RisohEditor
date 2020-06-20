// MDlgInitDlg.hpp --- Dialogs for edit of DLGINIT
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

#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "MResizable.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "MString.hpp"
#include "DlgInitRes.hpp"
#include "resource.h"
#include "DlgInit.h"

class MAddDlgInitDlg;
class MModifyDlgInitDlg;
class MDlgInitDlg;

//////////////////////////////////////////////////////////////////////////////

class MAddDlgInitDlg : public MDialogBase
{
public:
    DLGINIT_ENTRY& m_entry;
    MComboBoxAutoComplete m_cmb1;
    MComboBoxAutoComplete m_cmb2;

    MAddDlgInitDlg(DLGINIT_ENTRY& entry) :
        MDialogBase(IDD_ADDDLGINIT), m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        InitCtrlIDComboBox(hCmb1);
        SubclassChildDx(m_cmb1, cmb1);

        SubclassChildDx(m_cmb2, cmb2);

        ExecuteDlgInitDx(hwnd, GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ADDDLGINIT));

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        GetWindowTextW(m_cmb1, m_entry.sz0, _countof(m_entry.sz0));
        GetWindowTextW(m_cmb2, m_entry.sz1, _countof(m_entry.sz1));
        GetDlgItemTextW(hwnd, edt1, m_entry.sz2, _countof(m_entry.sz2));
        ReplaceFullWithHalf(m_entry.sz0);
        ReplaceFullWithHalf(m_entry.sz1);
        mstr_trim(m_entry.sz0);
        mstr_trim(m_entry.sz1);
        mstr_trim(m_entry.sz2);
        if (m_entry.sz2[0] == L'"')
            mstr_unquote(m_entry.sz2);

        if (!g_db.HasResID(m_entry.sz0))
        {
            BOOL bTranslated = FALSE;
            INT nValue = GetDlgItemInt(hwnd, cmb1, &bTranslated, TRUE);
            if (!bTranslated)
            {
                if (mchr_is_digit(m_entry.sz0[0]) || 
                    m_entry.sz0[0] == L'-' || m_entry.sz0[0] == L'+')
                {
                    nValue = mstr_parse_int(m_entry.sz0);
                    SetDlgItemInt(hwnd, cmb1, nValue, TRUE);
                }
                else
                {
                    m_cmb1.SetEditSel(0, -1);
                    SetFocus(m_cmb1);
                    ErrorBoxDx(IDS_NOSUCHID);
                    return;
                }
            }
            SetDlgItemInt(hwnd, cmb1, nValue, TRUE);
            auto text = GetDlgItemText(hwnd, cmb1);
            StringCchCopy(m_entry.sz0, _countof(m_entry.sz0), text.c_str());
        }

        if (lstrcmpW(m_entry.sz1, L"LB_ADDSTRING") != 0 &&
            lstrcmpW(m_entry.sz1, L"CB_ADDSTRING") != 0 &&
            lstrcmpW(m_entry.sz1, L"CBEM_INSERTITEM") != 0)
        {
            BOOL bTranslated = FALSE;
            INT nValue = GetDlgItemInt(hwnd, cmb2, &bTranslated, TRUE);
            if (!bTranslated)
            {
                if (mchr_is_digit(m_entry.sz1[0]) || 
                    m_entry.sz1[0] == L'-' || m_entry.sz1[0] == L'+')
                {
                    nValue = mstr_parse_int(m_entry.sz1);
                }
                else
                {
                    m_cmb2.SetEditSel(0, -1);
                    SetFocus(m_cmb2);
                    ErrorBoxDx(IDS_DATAISINVALID);
                    return;
                }
            }
            auto text = mstr_hex_word(nValue);
            SetDlgItemText(hwnd, cmb2, text.c_str());
            StringCchCopy(m_entry.sz1, _countof(m_entry.sz1), text.c_str());
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

//////////////////////////////////////////////////////////////////////////////

class MModifyDlgInitDlg : public MDialogBase
{
public:
    DLGINIT_ENTRY& m_entry;
    MComboBoxAutoComplete m_cmb1;
    MComboBoxAutoComplete m_cmb2;

    MModifyDlgInitDlg(DLGINIT_ENTRY& entry) :
        MDialogBase(IDD_MODIFYDLGINIT), m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        InitCtrlIDComboBox(hCmb1);
        SetDlgItemTextW(hwnd, cmb1, m_entry.sz0);
        SubclassChildDx(m_cmb1, cmb1);

        SubclassChildDx(m_cmb2, cmb2);
        SetDlgItemTextW(hwnd, cmb2, m_entry.sz1);

        SetDlgItemTextW(hwnd, edt1, m_entry.sz2);

        ExecuteDlgInitDx(hwnd, GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MODIFYDLGINIT));

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        GetWindowTextW(m_cmb1, m_entry.sz0, _countof(m_entry.sz0));
        GetWindowTextW(m_cmb2, m_entry.sz1, _countof(m_entry.sz1));
        GetDlgItemTextW(hwnd, edt1, m_entry.sz2, _countof(m_entry.sz2));
        ReplaceFullWithHalf(m_entry.sz0);
        ReplaceFullWithHalf(m_entry.sz1);
        mstr_trim(m_entry.sz0);
        mstr_trim(m_entry.sz1);
        mstr_trim(m_entry.sz2);
        if (m_entry.sz2[0] == L'"')
            mstr_unquote(m_entry.sz2);

        if (!g_db.HasResID(m_entry.sz0))
        {
            BOOL bTranslated = FALSE;
            INT nValue = GetDlgItemInt(hwnd, cmb1, &bTranslated, TRUE);
            if (!bTranslated)
            {
                if (mchr_is_digit(m_entry.sz0[0]) || 
                    m_entry.sz0[0] == L'-' || m_entry.sz0[0] == L'+')
                {
                    nValue = mstr_parse_int(m_entry.sz0);
                }
                else
                {
                    m_cmb1.SetEditSel(0, -1);
                    SetFocus(m_cmb1);
                    ErrorBoxDx(IDS_NOSUCHID);
                    return;
                }
            }
            SetDlgItemInt(hwnd, cmb1, nValue, TRUE);
            auto text = GetDlgItemText(hwnd, cmb1);
            StringCchCopy(m_entry.sz0, _countof(m_entry.sz0), text.c_str());
        }

        if (lstrcmpW(m_entry.sz1, L"LB_ADDSTRING") != 0 &&
            lstrcmpW(m_entry.sz1, L"CB_ADDSTRING") != 0 &&
            lstrcmpW(m_entry.sz1, L"CBEM_INSERTITEM") != 0)
        {
            BOOL bTranslated = FALSE;
            INT nValue = GetDlgItemInt(hwnd, cmb2, &bTranslated, TRUE);
            if (!bTranslated)
            {
                if (mchr_is_digit(m_entry.sz1[0]) ||
                    m_entry.sz1[0] == L'-' || m_entry.sz1[0] == L'+')
                {
                    nValue = mstr_parse_int(m_entry.sz1);
                }
                else
                {
                    m_cmb2.SetEditSel(0, -1);
                    SetFocus(m_cmb2);
                    ErrorBoxDx(IDS_DATAISINVALID);
                    return;
                }
            }
            auto text = mstr_hex_word(nValue);
            SetDlgItemText(hwnd, cmb2, text.c_str());
            StringCchCopy(m_entry.sz1, _countof(m_entry.sz1), text.c_str());
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

//////////////////////////////////////////////////////////////////////////////

class MDlgInitDlg : public MDialogBase
{
public:
    DlgInitRes& m_dlginit_res;
    MResizable m_resizable;
    HICON m_hIcon;
    HICON m_hIconSm;
    HWND m_hLst1;

    MDlgInitDlg(DlgInitRes& dlginit_res)
        : MDialogBase(IDD_DLGINITEDIT), m_dlginit_res(dlginit_res)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);
        m_hLst1 = NULL;
    }

    ~MDlgInitDlg()
    {
        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    void OnUp(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem == 0)
            return;

        DLGINIT_ENTRY die0, die1;
        ListView_GetItemText(m_hLst1, iItem - 1, 0, die0.sz0, _countof(die0.sz0));
        ListView_GetItemText(m_hLst1, iItem - 1, 1, die0.sz1, _countof(die0.sz1));
        ListView_GetItemText(m_hLst1, iItem - 1, 2, die0.sz2, _countof(die0.sz2));
        ListView_GetItemText(m_hLst1, iItem, 0, die1.sz0, _countof(die1.sz0));
        ListView_GetItemText(m_hLst1, iItem, 1, die1.sz1, _countof(die1.sz1));
        ListView_GetItemText(m_hLst1, iItem, 2, die1.sz2, _countof(die1.sz2));

        ListView_SetItemText(m_hLst1, iItem - 1, 0, die1.sz0);
        ListView_SetItemText(m_hLst1, iItem - 1, 1, die1.sz1);
        ListView_SetItemText(m_hLst1, iItem - 1, 2, die1.sz2);
        ListView_SetItemText(m_hLst1, iItem, 0, die0.sz0);
        ListView_SetItemText(m_hLst1, iItem, 1, die0.sz1);
        ListView_SetItemText(m_hLst1, iItem, 2, die0.sz2);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, iItem - 1, state, state);
    }

    void OnDown(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem + 1 == ListView_GetItemCount(m_hLst1))
            return;

        DLGINIT_ENTRY die0, die1;
        ListView_GetItemText(m_hLst1, iItem, 0, die0.sz0, _countof(die0.sz0));
        ListView_GetItemText(m_hLst1, iItem, 1, die0.sz1, _countof(die0.sz1));
        ListView_GetItemText(m_hLst1, iItem, 2, die0.sz2, _countof(die0.sz2));
        ListView_GetItemText(m_hLst1, iItem + 1, 0, die1.sz0, _countof(die1.sz0));
        ListView_GetItemText(m_hLst1, iItem + 1, 1, die1.sz1, _countof(die1.sz1));
        ListView_GetItemText(m_hLst1, iItem + 1, 2, die1.sz2, _countof(die1.sz2));

        ListView_SetItemText(m_hLst1, iItem, 0, die1.sz0);
        ListView_SetItemText(m_hLst1, iItem, 1, die1.sz1);
        ListView_SetItemText(m_hLst1, iItem, 2, die1.sz2);
        ListView_SetItemText(m_hLst1, iItem + 1, 0, die0.sz0);
        ListView_SetItemText(m_hLst1, iItem + 1, 1, die0.sz1);
        ListView_SetItemText(m_hLst1, iItem + 1, 2, die0.sz2);

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

    void OnDeleteAll(HWND hwnd)
    {
        ListView_DeleteAllItems(m_hLst1);
    }

    void OnAdd(HWND hwnd)
    {
        DLGINIT_ENTRY entry;

        MAddDlgInitDlg dialog(entry);
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

        MString str2 = mstr_quote(entry.sz2);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 2;
        item.pszText = &str2[0];
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

        DLGINIT_ENTRY die;
        ListView_GetItemText(m_hLst1, iItem, 0, die.sz0, _countof(die.sz0));
        ListView_GetItemText(m_hLst1, iItem, 1, die.sz1, _countof(die.sz1));
        ListView_GetItemText(m_hLst1, iItem, 2, die.sz2, _countof(die.sz2));
        if (die.sz2[0] == L'"')
            mstr_unquote(die.sz2);

        MModifyDlgInitDlg dialog(die);
        if (IDOK == dialog.DialogBoxDx(hwnd))
        {
            MString str2 = mstr_quote(die.sz2);
            ListView_SetItemText(m_hLst1, iItem, 0, die.sz0);
            ListView_SetItemText(m_hLst1, iItem, 1, die.sz1);
            ListView_SetItemText(m_hLst1, iItem, 2, &str2[0]);
        }
    }

    void OnOK(HWND hwnd)
    {
        INT i, nCount = ListView_GetItemCount(m_hLst1);

        m_dlginit_res.entries().clear();
        for (i = 0; i < nCount; ++i)
        {
            DLGINIT_ENTRY die;
            ListView_GetItemText(m_hLst1, i, 0, die.sz0, _countof(die.sz0));
            ListView_GetItemText(m_hLst1, i, 1, die.sz1, _countof(die.sz1));
            ListView_GetItemText(m_hLst1, i, 2, die.sz2, _countof(die.sz2));
            if (die.sz2[0] == L'"')
                mstr_unquote(die.sz2);

            DlgInitEntry entry;
            entry.wCtrl = WORD(g_db.GetResIDValue(die.sz0));

            if (lstrcmpiW(die.sz1, L"LB_ADDSTRING") == 0)
            {
                entry.wMsg = LB_ADDSTRING;
            }
            else if (lstrcmpiW(die.sz1, L"CB_ADDSTRING") == 0)
            {
                entry.wMsg = CB_ADDSTRING;
            }
            else if (lstrcmpiW(die.sz1, L"CBEM_INSERTITEM") == 0)
            {
                entry.wMsg = CBEM_INSERTITEM;
            }
            else
            {
                entry.wMsg = mstr_parse_int(die.sz1);
            }

            entry.strText = MTextToAnsi(CP_ACP, die.sz2).c_str();

            m_dlginit_res.entries().push_back(entry);
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
            OnDeleteAll(hwnd);
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
        column.cx = 130;
        column.pszText = LoadStringDx(IDS_CONTROL);
        column.iSubItem = 0;
        ListView_InsertColumn(m_hLst1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 125;
        column.pszText = LoadStringDx(IDS_MESSAGE);
        column.iSubItem = 1;
        ListView_InsertColumn(m_hLst1, 1, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 200;
        column.pszText = LoadStringDx(IDS_STRING);
        column.iSubItem = 2;
        ListView_InsertColumn(m_hLst1, 2, &column);

        typedef DlgInitRes::entries_type entries_type;
        const entries_type& entries = m_dlginit_res.entries();

        INT i = 0;
        for (auto& entry : entries)
        {
            MString str;

            str = g_db.GetCtrlOrCmdName(entry.wCtrl);

            LV_ITEM item;
            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 0;
            item.pszText = &str[0];
            ListView_InsertItem(m_hLst1, &item);

            switch (entry.wMsg)
            {
            case WIN16_LB_ADDSTRING:
            case LB_ADDSTRING:
                str = L"LB_ADDSTRING";
                break;
            case WIN16_CB_ADDSTRING:
            case CB_ADDSTRING:
                str = L"CB_ADDSTRING";
                break;
            case AFX_CB_ADDSTRING:
            case CBEM_INSERTITEM:
                str = L"CBEM_INSERTITEM";
                break;
            default:
                str = mstr_hex_word(entry.wMsg);
                break;
            }

            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 1;
            item.pszText = &str[0];
            ListView_SetItem(m_hLst1, &item);

            str = MAnsiToText(CP_ACP, entry.strText).c_str();
            str = mstr_quote(str);

            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 2;
            item.pszText = &str[0];
            ListView_SetItem(m_hLst1, &item);

            ++i;
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
        m_resizable.SetLayoutAnchor(psh6, mzcLA_BOTTOM_LEFT);
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
            PopupMenuDx(hwnd, m_hLst1, IDR_POPUPMENUS, 7, xPos, yPos);
        }
    }
};
