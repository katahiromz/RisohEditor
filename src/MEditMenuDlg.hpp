// MEditMenuDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MEDITMENUDLG_HPP_
#define MZC4_MEDITMENUDLG_HPP_

#include "MAddMItemDlg.hpp"
#include "MModifyMItemDlg.hpp"

//////////////////////////////////////////////////////////////////////////////

struct MEditMenuDlg : MDialogBase
{
    MenuRes& m_menu_res;

    MEditMenuDlg(MenuRes& menu_res) : MDialogBase(IDD_EDITMENU), m_menu_res(menu_res)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        if (m_menu_res.IsExtended())
            CheckDlgButton(hwnd, chx1, BST_CHECKED);

        HWND hCtl1 = GetDlgItem(hwnd, ctl1);
        ListView_SetExtendedListViewStyle(hCtl1, LVS_EX_FULLROWSELECT);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 225;
        column.pszText = LoadStringDx(IDS_CAPTION);
        column.iSubItem = 0;
        ListView_InsertColumn(hCtl1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 95;
        column.pszText = LoadStringDx(IDS_FLAGS);
        column.iSubItem = 1;
        ListView_InsertColumn(hCtl1, 1, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 150;
        column.pszText = LoadStringDx(IDS_COMMANDID);
        column.iSubItem = 2;
        ListView_InsertColumn(hCtl1, 2, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 70;
        column.pszText = LoadStringDx(IDS_HELPID);
        column.iSubItem = 3;
        ListView_InsertColumn(hCtl1, 3, &column);

        INT i = 0;
        std::wstring str;
        LV_ITEM item;
        if (m_menu_res.IsExtended())
        {
            typedef MenuRes::ExMenuItemsType exitems_type;
            exitems_type& exitems = m_menu_res.exitems();
            exitems_type::iterator it, end = exitems.end();
            for (it = exitems.begin(); it != end; ++it, ++i)
            {
                str = str_repeat(LoadStringDx(IDS_INDENT), it->wDepth);
                if (it->text.empty() && it->menuId == 0)
                {
                    str += LoadStringDx(IDS_SEPARATOR);
                    it->dwType |= MFT_SEPARATOR;
                }
                else
                {
                    str += str_quote(it->text);
                }

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 0;
                item.pszText = &str[0];
                ListView_InsertItem(hCtl1, &item);

                str = GetMenuTypeAndState(it->dwType, it->dwState);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 1;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);

                str = str_dec(it->menuId);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 2;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);

                str = str_dec(it->dwHelpId);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 3;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);
            }
        }
        else
        {
            typedef MenuRes::MenuItemsType items_type;
            items_type& items = m_menu_res.items();
            items_type::iterator it, end = items.end();
            for (it = items.begin(); it != end; ++it, ++i)
            {
                str = str_repeat(LoadStringDx(IDS_INDENT), it->wDepth);
                if (it->text.empty() && it->wMenuID == 0)
                {
                    str += LoadStringDx(IDS_SEPARATOR);
                }
                else
                {
                    str += str_quote(it->text);
                }

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 0;
                item.pszText = &str[0];
                ListView_InsertItem(hCtl1, &item);

                str = GetMenuFlags(it->fItemFlags);
                if (it->text.empty() && it->wMenuID == 0)
                    str += L"S ";

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 1;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);

                str = str_dec(it->wMenuID);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 2;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);

                str = str_dec(0);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 3;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);
            }
        }

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, 0, state, state);
        SetFocus(hCtl1);

        return TRUE;
    }

    void OnAdd(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        MENU_ENTRY m_entry;
        ZeroMemory(&m_entry, sizeof(m_entry));
        MAddMItemDlg dialog(m_entry);
        INT nID = dialog.DialogBoxDx(hwnd);
        if (IDOK != nID)
        {
            return;
        }

        INT iItem = ListView_GetItemCount(hCtl1);

        std::wstring str, strIndent = LoadStringDx(IDS_INDENT);
        str = m_entry.Caption;
        if (str.empty() || wcsstr(m_entry.Flags, L"S ") != NULL)
            str = LoadStringDx(IDS_SEPARATOR);
        str = str_repeat(strIndent, m_entry.wDepth) + str;

        LV_ITEM item;

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = &str[0];
        ListView_InsertItem(hCtl1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 1;
        item.pszText = m_entry.Flags;
        ListView_SetItem(hCtl1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 2;
        item.pszText = m_entry.CommandID;
        ListView_SetItem(hCtl1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 3;
        item.pszText = m_entry.HelpID;
        ListView_SetItem(hCtl1, &item);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem, state, state);
    }

    BOOL GetEntry(HWND hwnd, HWND hCtl1, MENU_ENTRY& entry, INT iItem)
    {
        WCHAR Caption[256];
        ListView_GetItemText(hCtl1, iItem, 0, Caption, _countof(Caption));

        entry.wDepth = 0;
        std::wstring str = Caption, strIndent = LoadStringDx(IDS_INDENT);
        while (str.find(strIndent) == 0)
        {
            str = str.substr(strIndent.size());
            ++entry.wDepth;
        }
        str_trim(str);
        if (str[0] == L'"')
        {
            str_unquote(str);
        }
        if (str.empty() || str == LoadStringDx(IDS_SEPARATOR))
        {
            str.clear();
        }

        lstrcpynW(entry.Caption, str.c_str(), _countof(entry.Caption));

        ListView_GetItemText(hCtl1, iItem, 1, entry.Flags, _countof(entry.Flags));
        ListView_GetItemText(hCtl1, iItem, 2, entry.CommandID, _countof(entry.CommandID));
        ListView_GetItemText(hCtl1, iItem, 3, entry.HelpID, _countof(entry.HelpID));
        return TRUE;
    }

    BOOL SetEntry(HWND hwnd, HWND hCtl1, MENU_ENTRY& entry, INT iItem)
    {
        std::wstring str, strIndent = LoadStringDx(IDS_INDENT);
        str = str_repeat(strIndent, entry.wDepth);

        if (entry.Caption[0] == 0 || wcsstr(entry.Flags, L"S ") != NULL)
            str += LoadStringDx(IDS_SEPARATOR);
        else
            str += str_quote(entry.Caption);

        ListView_SetItemText(hCtl1, iItem, 0, &str[0]);
        ListView_SetItemText(hCtl1, iItem, 1, entry.Flags);
        ListView_SetItemText(hCtl1, iItem, 2, entry.CommandID);
        ListView_SetItemText(hCtl1, iItem, 3, entry.HelpID);
        return TRUE;
    }

    void OnModify(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
        {
            return;
        }

        MENU_ENTRY m_entry;
        GetEntry(hwnd, hCtl1, m_entry, iItem);
        
        MModifyMItemDlg dialog(m_entry);
        INT nID = dialog.DialogBoxDx(hwnd);
        if (IDOK == nID)
        {
            SetEntry(hwnd, hCtl1, m_entry, iItem);
        }
    }

    void OnDelete(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem >= 0)
        {
            ListView_DeleteItem(hCtl1, iItem);
        }
    }

    void OnUp(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem <= 0)
            return;

        MENU_ENTRY entry0, entry1;

        GetEntry(hwnd, hCtl1, entry0, iItem - 1);
        GetEntry(hwnd, hCtl1, entry1, iItem);

        SetEntry(hwnd, hCtl1, entry1, iItem - 1);
        SetEntry(hwnd, hCtl1, entry0, iItem);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem - 1, state, state);
    }

    void OnDown(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
            return;

        INT Count = ListView_GetItemCount(hCtl1);
        if (iItem + 1 >= Count)
            return;

        MENU_ENTRY entry0, entry1;

        GetEntry(hwnd, hCtl1, entry0, iItem);
        GetEntry(hwnd, hCtl1, entry1, iItem + 1);

        SetEntry(hwnd, hCtl1, entry1, iItem);
        SetEntry(hwnd, hCtl1, entry0, iItem + 1);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem + 1, state, state);
    }

    void OnLeft(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
            return;

        WCHAR Caption[128];
        ListView_GetItemText(hCtl1, iItem, 0, Caption, _countof(Caption));

        std::wstring strIndent = LoadStringDx(IDS_INDENT);

        std::wstring str = Caption;
        if (str.find(strIndent) == 0)
        {
            str = str.substr(strIndent.size());
        }

        ListView_SetItemText(hCtl1, iItem, 0, &str[0]);
    }

    void OnRight(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
            return;

        if (iItem == 0)
            return;

        WCHAR CaptionUp[128];
        ListView_GetItemText(hCtl1, iItem - 1, 0, CaptionUp, _countof(CaptionUp));
        WCHAR Caption[128];
        ListView_GetItemText(hCtl1, iItem, 0, Caption, _countof(Caption));

        std::wstring strIndent = LoadStringDx(IDS_INDENT);
        INT depth_up = str_repeat_count(CaptionUp, strIndent);
        INT depth = str_repeat_count(Caption, strIndent);

        if (depth_up < depth)
            return;

        std::wstring str = strIndent + Caption;
        ListView_SetItemText(hCtl1, iItem, 0, &str[0]);
    }

    void OnOK(HWND hwnd)
    {
        MENU_ENTRY entry;
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);
        INT iItem, Count = ListView_GetItemCount(hCtl1);

        if (Count == 0)
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
            for (iItem = 0; iItem < Count; ++iItem)
            {
                GetEntry(hwnd, hCtl1, entry, iItem);

                MenuRes::ExMenuItem exitem;

                SetMenuTypeAndState(exitem.dwType, exitem.dwState, entry.Flags);
                exitem.menuId = _wtoi(entry.CommandID);
                exitem.bResInfo = 0;
                exitem.text = entry.Caption;
                exitem.dwHelpId = _wtoi(entry.HelpID);
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
            for (iItem = 0; iItem < Count; ++iItem)
            {
                GetEntry(hwnd, hCtl1, entry, iItem);

                MenuRes::MenuItem item;

                SetMenuFlags(item.fItemFlags, entry.Flags);
                item.wMenuID = _wtoi(entry.CommandID);
                item.wDepth = entry.wDepth;
                item.text = entry.Caption;

                m_menu_res.items().push_back(item);
            }
        }

        EndDialog(hwnd, IDOK);
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
            EndDialog(hwnd, IDCANCEL);
            break;
        }
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
    {
        if (idFrom == ctl1)
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
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MEDITMENUDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
