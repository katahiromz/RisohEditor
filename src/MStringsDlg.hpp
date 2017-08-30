// MStringsDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MSTRINGSDLG_HPP_
#define MZC4_MSTRINGSDLG_HPP_

#include "RisohEditor.hpp"
#include "StringRes.hpp"

class MAddStrDlg;
class MModifyStrDlg;
class MStringsDlg;

void InitStringComboBox(HWND hCmb, ConstantsDB& db, MString strString);
BOOL StrDlg_GetEntry(HWND hwnd, STRING_ENTRY& entry, ConstantsDB& db);
void StrDlg_SetEntry(HWND hwnd, STRING_ENTRY& entry, ConstantsDB& db);

//////////////////////////////////////////////////////////////////////////////

class MAddStrDlg : public MDialogBase
{
public:
    STRING_ENTRY& m_entry;
    ConstantsDB& m_db;
    ResEntries& m_entries;

    MAddStrDlg(ConstantsDB& db, STRING_ENTRY& entry, ResEntries& entries)
        : MDialogBase(IDD_ADDSTR), m_entry(entry), m_db(db), m_entries(entries)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        CenterWindowDx();
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            if (!StrDlg_GetEntry(hwnd, m_entry, m_db))
            {
                ErrorBoxDx(IDS_NOSUCHID);
                return;
            }
            EndDialog(IDOK);
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

class MModifyStrDlg : public MDialogBase
{
public:
    STRING_ENTRY& m_entry;
    ConstantsDB& m_db;

    MModifyStrDlg(ConstantsDB& db, STRING_ENTRY& entry)
        : MDialogBase(IDD_MODIFYSTR), m_entry(entry), m_db(db)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        InitStringComboBox(hCmb1, m_db, L"");

        StrDlg_SetEntry(hwnd, m_entry, m_db);
        CenterWindowDx();
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            if (!StrDlg_GetEntry(hwnd, m_entry, m_db))
            {
                ErrorBoxDx(IDS_NOSUCHID);
                return;
            }
            EndDialog(IDOK);
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

//////////////////////////////////////////////////////////////////////////////

class MStringsDlg : public MDialogBase
{
public:
    StringRes& m_str_res;
    ConstantsDB& m_db;
    ResEntries& m_entries;

    MStringsDlg(ConstantsDB& db, StringRes& str_res, ResEntries& entries)
        : MDialogBase(IDD_STRINGS), m_str_res(str_res), m_db(db),
          m_entries(entries)
    {
    }

    void InitCtl1(HWND hCtl1)
    {
        ListView_DeleteAllItems(hCtl1);

        typedef StringRes::map_type map_type;
        const map_type& map = m_str_res.map();

        INT i = 0;
        map_type::const_iterator it, end = map.end();
        for (it = map.begin(); it != end; ++it)
        {
            if (it->second.empty())
                continue;

            MStringW str = m_db.GetNameOfResID(IDTYPE_STRING, it->first);

            LV_ITEM item;
            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 0;
            item.pszText = &str[0];
            ListView_InsertItem(hCtl1, &item);

            str = mstr_quote(it->second);

            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 1;
            item.pszText = &str[0];
            ListView_SetItem(hCtl1, &item);

            ++i;
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);
        ListView_SetExtendedListViewStyle(hCtl1, LVS_EX_FULLROWSELECT);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 140;
        column.pszText = LoadStringDx(IDS_STRINGID);
        column.iSubItem = 0;
        ListView_InsertColumn(hCtl1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 500;
        column.pszText = LoadStringDx(IDS_STRINGVALUE);
        column.iSubItem = 1;
        ListView_InsertColumn(hCtl1, 1, &column);

        InitCtl1(hCtl1);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, 0, state, state);
        SetFocus(hCtl1);

        CenterWindowDx();
        return TRUE;
    }

    void OnAdd(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        STRING_ENTRY s_entry;
        ZeroMemory(&s_entry, sizeof(s_entry));
        MAddStrDlg dialog(m_db, s_entry, m_entries);
        INT nID = dialog.DialogBoxDx(hwnd);
        if (IDOK != nID)
        {
            return;
        }

        INT iItem;

        LV_FINDINFO find;
        TCHAR sz[128];
        ZeroMemory(&find, sizeof(find));
        find.flags = LVFI_STRING;
        find.psz = sz;
        iItem = ListView_FindItem(hCtl1, -1, &find);
        if (iItem != -1)
        {
            ListView_DeleteItem(hCtl1, iItem);
        }

        LV_ITEM item;
        iItem = ListView_GetItemCount(hCtl1);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = s_entry.StringID;
        ListView_InsertItem(hCtl1, &item);

        std::wstring str = s_entry.StringValue;
        str = mstr_quote(str);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 1;
        item.pszText = &str[0];
        ListView_SetItem(hCtl1, &item);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem, state, state);
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

    void GetEntry(HWND hwnd, HWND hCtl1, INT iItem, STRING_ENTRY& entry)
    {
        ListView_GetItemText(hCtl1, iItem, 0, entry.StringID, _countof(entry.StringID));
        mstr_trim(entry.StringID);

        ListView_GetItemText(hCtl1, iItem, 1, entry.StringValue, _countof(entry.StringValue));
        mstr_trim(entry.StringValue);
        if (entry.StringValue[0] == L'"')
        {
            mstr_unquote(entry.StringValue);
        }
    }

    void OnModify(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
        {
            return;
        }

        STRING_ENTRY s_entry;
        GetEntry(hwnd, hCtl1, iItem, s_entry);

        MModifyStrDlg dialog(m_db, s_entry);
        INT nID = dialog.DialogBoxDx(hwnd);
        if (IDOK == nID)
        {
            ListView_SetItemText(hCtl1, iItem, 0, s_entry.StringID);

            std::wstring str = mstr_quote(s_entry.StringValue);
            ListView_SetItemText(hCtl1, iItem, 1, &str[0]);
        }
    }

    void OnOK(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem, Count = ListView_GetItemCount(hCtl1);
        if (Count == 0)
        {
            ErrorBoxDx(IDS_DATAISEMPTY);
            return;
        }

        m_str_res.map().clear();

        STRING_ENTRY s_entry;
        for (iItem = 0; iItem < Count; ++iItem)
        {
            GetEntry(hwnd, hCtl1, iItem, s_entry);

            WORD wID = (WORD)wcstol(s_entry.StringID, NULL, 0);
            std::wstring str = s_entry.StringValue;

            m_str_res.map().insert(std::make_pair(wID, str));
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

#endif  // ndef MZC4_MSTRINGSDLG_HPP_
