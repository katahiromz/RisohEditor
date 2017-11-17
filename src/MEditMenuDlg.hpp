// MEditMenuDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MEDITMENUDLG_HPP_
#define MZC4_MEDITMENUDLG_HPP_

#include "RisohEditor.hpp"
#include "MenuRes.hpp"

class MAddMItemDlg;
class MModifyMItemDlg;
class MEditMenuDlg;

BOOL CheckCommand(ConstantsDB& db, MString strCommand);
void InitCommandComboBox(HWND hCmb, ConstantsDB& db, MString strCommand);

//////////////////////////////////////////////////////////////////////////////

class MAddMItemDlg : public MDialogBase
{
public:
    MENU_ENTRY& m_entry;
    ConstantsDB& m_db;

    MAddMItemDlg(ConstantsDB& db, MENU_ENTRY& entry)
        : MDialogBase(IDD_ADDMITEM), m_entry(entry), m_db(db)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        InitCommandComboBox(GetDlgItem(hwnd, cmb2), m_db, L"");
        SetDlgItemInt(hwnd, edt1, 0, TRUE);
        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        ::GetDlgItemTextW(hwnd, cmb1, m_entry.Caption, _countof(m_entry.Caption));
        mstr_trim(m_entry.Caption);
        if (m_entry.Caption[0] == L'"')
        {
            mstr_unquote(m_entry.Caption);
        }

        ::GetDlgItemTextW(hwnd, cmb2, m_entry.CommandID, _countof(m_entry.CommandID));
        mstr_trim(m_entry.CommandID);
        if (!CheckCommand(m_db, m_entry.CommandID))
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
        if (m_entry.Caption[0] == 0 ||
            lstrcmpiW(m_entry.Caption, LoadStringDx(IDS_SEPARATOR)) == 0)
        {
            m_entry.Caption[0] = 0;
            dwType |= MFT_SEPARATOR;
            str = GetMenuTypeAndState(dwType, dwState);
        }
        lstrcpynW(m_entry.Flags, str.c_str(), _countof(m_entry.Flags));

        ::GetDlgItemTextW(hwnd, edt1, m_entry.HelpID, _countof(m_entry.HelpID));
        DWORD help = m_db.GetResIDValue(m_entry.HelpID);
        MString strHelp = m_db.GetNameOfResID(IDTYPE_HELP, help);
        lstrcpynW(m_entry.HelpID, strHelp.c_str(), _countof(m_entry.HelpID));

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

class MModifyMItemDlg : public MDialogBase
{
public:
    MENU_ENTRY& m_entry;
    ConstantsDB& m_db;

    MModifyMItemDlg(ConstantsDB& db, MENU_ENTRY& entry)
        : MDialogBase(IDD_MODIFYMITEM), m_entry(entry), m_db(db)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemTextW(hwnd, cmb1, mstr_quote(m_entry.Caption).c_str());

        InitCommandComboBox(GetDlgItem(hwnd, cmb2), m_db, m_entry.CommandID);
        SetDlgItemTextW(hwnd, edt1, m_entry.HelpID);

        DWORD dwType, dwState;
        dwType = dwState = 0;
        SetMenuTypeAndState(dwType, dwState, m_entry.Flags);

        if (lstrcmpiW(m_entry.Caption, LoadStringDx(IDS_SEPARATOR)) == 0 ||
            m_entry.Caption[0] == 0 || (dwType & MFT_SEPARATOR))
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
        ::GetDlgItemTextW(hwnd, cmb1, m_entry.Caption, _countof(m_entry.Caption));
        mstr_trim(m_entry.Caption);
        if (m_entry.Caption[0] == L'"')
        {
            mstr_unquote(m_entry.Caption);
        }

        ::GetDlgItemTextW(hwnd, cmb2, m_entry.CommandID, _countof(m_entry.CommandID));
        mstr_trim(m_entry.CommandID);
        if (!CheckCommand(m_db, m_entry.CommandID))
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

        if (lstrcmpiW(m_entry.Caption, LoadStringDx(IDS_SEPARATOR)) == 0 ||
            m_entry.Caption[0] == 0 || (dwType & MFT_SEPARATOR))
        {
            m_entry.Caption[0] = 0;
            dwType |= MFT_SEPARATOR;
        }

        std::wstring str = GetMenuTypeAndState(dwType, dwState);
        lstrcpynW(m_entry.Flags, str.c_str(), _countof(m_entry.Flags));

        ::GetDlgItemTextW(hwnd, edt1, m_entry.HelpID, _countof(m_entry.HelpID));
        DWORD help = m_db.GetResIDValue(m_entry.HelpID);
        MString strHelp = m_db.GetNameOfResID(IDTYPE_HELP, help);
        lstrcpynW(m_entry.HelpID, strHelp.c_str(), _countof(m_entry.HelpID));

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

class MEditMenuDlg : public MDialogBase
{
public:
    MenuRes& m_menu_res;
    ConstantsDB& m_db;

    MEditMenuDlg(ConstantsDB& db, MenuRes& menu_res)
        : MDialogBase(IDD_EDITMENU), m_menu_res(menu_res), m_db(db)
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
                str = mstr_repeat(LoadStringDx(IDS_INDENT), it->wDepth);
                if (it->text.empty() && it->menuId == 0)
                {
                    str += LoadStringDx(IDS_SEPARATOR);
                    it->dwType |= MFT_SEPARATOR;
                }
                else
                {
                    str += mstr_quote(it->text);
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

                str = m_db.GetNameOfResID(IDTYPE_COMMAND, it->menuId);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 2;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);

                str = m_db.GetNameOfResID(IDTYPE_HELP, it->dwHelpId);

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
                str = mstr_repeat(LoadStringDx(IDS_INDENT), it->wDepth);
                if (it->text.empty() && it->wMenuID == 0)
                {
                    str += LoadStringDx(IDS_SEPARATOR);
                }
                else
                {
                    str += mstr_quote(it->text);
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

                str = m_db.GetNameOfResID(IDTYPE_COMMAND, it->wMenuID);

                ZeroMemory(&item, sizeof(item));
                item.iItem = i;
                item.mask = LVIF_TEXT;
                item.iSubItem = 2;
                item.pszText = &str[0];
                ListView_SetItem(hCtl1, &item);

                str = TEXT("0");

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

        CenterWindowDx();
        return TRUE;
    }

    void OnAdd(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        MENU_ENTRY m_entry;
        ZeroMemory(&m_entry, sizeof(m_entry));
        MAddMItemDlg dialog(m_db, m_entry);
        INT nID = (INT)dialog.DialogBoxDx(hwnd);
        if (IDOK != nID)
        {
            return;
        }

        INT iItem = ListView_GetItemCount(hCtl1);

        MStringW str, strIndent = LoadStringDx(IDS_INDENT);
        str = mstr_quote(m_entry.Caption);
        if (str.empty() || wcsstr(m_entry.Flags, L"S ") != NULL)
            str = LoadStringDx(IDS_SEPARATOR);
        str = mstr_repeat(strIndent, m_entry.wDepth) + str;

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
        MStringW str = Caption, strIndent = LoadStringDx(IDS_INDENT);
        while (str.find(strIndent) == 0)
        {
            str = str.substr(strIndent.size());
            ++entry.wDepth;
        }
        mstr_trim(str);
        if (str[0] == L'"')
        {
            mstr_unquote(str);
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
        MStringW str, strIndent = LoadStringDx(IDS_INDENT);
        str = mstr_repeat(strIndent, entry.wDepth);

        if (entry.Caption[0] == 0 || wcsstr(entry.Flags, L"S ") != NULL)
            str += LoadStringDx(IDS_SEPARATOR);
        else
            str += mstr_quote(entry.Caption);

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
        
        MModifyMItemDlg dialog(m_db, m_entry);
        INT nID = (INT)dialog.DialogBoxDx(hwnd);
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
        INT depth_up = mstr_repeat_count(CaptionUp, strIndent);
        INT depth = mstr_repeat_count(Caption, strIndent);

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
                exitem.menuId = m_db.GetResIDValue(entry.CommandID);
                exitem.bResInfo = 0;
                exitem.text = entry.Caption;
                exitem.dwHelpId = m_db.GetResIDValue(entry.HelpID);
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
                item.wMenuID = (WORD)m_db.GetResIDValue(entry.CommandID);
                item.wDepth = entry.wDepth;
                item.text = entry.Caption;

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
