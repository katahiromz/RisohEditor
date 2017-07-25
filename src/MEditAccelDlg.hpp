// MEditAccelDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MEDITACCELDLG_HPP_
#define MZC4_MEDITACCELDLG_HPP_

#include "RisohEditor.hpp"
#include "ConstantsDB.hpp"
#include "AccelRes.hpp"

//////////////////////////////////////////////////////////////////////////////

void Cmb1_InitVirtualKeys(HWND hCmb1);
BOOL Cmb1_CheckKey(HWND hwnd, HWND hCmb1, BOOL bVirtKey, std::wstring& str);

std::wstring GetKeyID(UINT wId);

class MAddKeyDlg;
class MModifyKeyDlg;
class MEditAccelDlg;

//////////////////////////////////////////////////////////////////////////////

class MAddKeyDlg : public MDialogBase
{
public:
    ACCEL_ENTRY& m_entry;

    MAddKeyDlg(ACCEL_ENTRY& entry) : MDialogBase(IDD_ADDKEY), m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        CheckDlgButton(hwnd, chx1, BST_CHECKED);

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        Cmb1_InitVirtualKeys(hCmb1);

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ::GetWindowTextW(hCmb1, m_entry.sz0, _countof(m_entry.sz0));

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

        ::GetDlgItemTextW(hwnd, cmb2, m_entry.sz2, _countof(m_entry.sz2));

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

    MModifyKeyDlg(ACCEL_ENTRY& entry) : MDialogBase(IDD_MODIFYKEY), m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemTextW(hwnd, cmb1, m_entry.sz0);
        SetDlgItemTextW(hwnd, cmb2, m_entry.sz2);

        WORD Flags;
        SetKeyFlags(Flags, m_entry.sz1);
        if (Flags & FVIRTKEY)
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
        if (Flags & FNOINVERT)
            CheckDlgButton(hwnd, chx2, BST_CHECKED);
        if (Flags & FCONTROL)
            CheckDlgButton(hwnd, chx3, BST_CHECKED);
        if (Flags & FSHIFT)
            CheckDlgButton(hwnd, chx4, BST_CHECKED);
        if (Flags & FALT)
            CheckDlgButton(hwnd, chx5, BST_CHECKED);

        if (Flags & FVIRTKEY)
        {
            HWND hCmb1 = GetDlgItem(hwnd, cmb1);
            Cmb1_InitVirtualKeys(hCmb1);

            INT i = ComboBox_FindStringExact(hCmb1, -1, m_entry.sz0);
            if (i != CB_ERR)
            {
                ComboBox_SetCurSel(hCmb1, i);
            }
        }

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ::GetWindowTextW(hCmb1, m_entry.sz0, _countof(m_entry.sz0));

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

        ::GetDlgItemTextW(hwnd, cmb2, m_entry.sz2, _countof(m_entry.sz2));

        EndDialog(IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case chx1:
            if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            {
                Cmb1_InitVirtualKeys(GetDlgItem(hwnd, cmb1));
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

class MEditAccelDlg : public MDialogBase
{
public:
    AccelRes& m_accel_res;
    ConstantsDB& m_db;

    MEditAccelDlg(AccelRes& accel_res, ConstantsDB& db)
        : MDialogBase(IDD_EDITACCEL), m_accel_res(accel_res), m_db(db)
    {
    }

    void OnUp(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem == 0)
            return;

        ACCEL_ENTRY ae0, ae1;
        ListView_GetItemText(hCtl1, iItem - 1, 0, ae0.sz0, _countof(ae0.sz0));
        ListView_GetItemText(hCtl1, iItem - 1, 1, ae0.sz1, _countof(ae0.sz1));
        ListView_GetItemText(hCtl1, iItem - 1, 2, ae0.sz2, _countof(ae0.sz2));
        ListView_GetItemText(hCtl1, iItem, 0, ae1.sz0, _countof(ae1.sz0));
        ListView_GetItemText(hCtl1, iItem, 1, ae1.sz1, _countof(ae1.sz1));
        ListView_GetItemText(hCtl1, iItem, 2, ae1.sz2, _countof(ae1.sz2));

        ListView_SetItemText(hCtl1, iItem - 1, 0, ae1.sz0);
        ListView_SetItemText(hCtl1, iItem - 1, 1, ae1.sz1);
        ListView_SetItemText(hCtl1, iItem - 1, 2, ae1.sz2);
        ListView_SetItemText(hCtl1, iItem, 0, ae0.sz0);
        ListView_SetItemText(hCtl1, iItem, 1, ae0.sz1);
        ListView_SetItemText(hCtl1, iItem, 2, ae0.sz2);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem - 1, state, state);
    }

    void OnDown(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem + 1 == ListView_GetItemCount(hCtl1))
            return;

        ACCEL_ENTRY ae0, ae1;
        ListView_GetItemText(hCtl1, iItem, 0, ae0.sz0, _countof(ae0.sz0));
        ListView_GetItemText(hCtl1, iItem, 1, ae0.sz1, _countof(ae0.sz1));
        ListView_GetItemText(hCtl1, iItem, 2, ae0.sz2, _countof(ae0.sz2));
        ListView_GetItemText(hCtl1, iItem + 1, 0, ae1.sz0, _countof(ae1.sz0));
        ListView_GetItemText(hCtl1, iItem + 1, 1, ae1.sz1, _countof(ae1.sz1));
        ListView_GetItemText(hCtl1, iItem + 1, 2, ae1.sz2, _countof(ae1.sz2));

        ListView_SetItemText(hCtl1, iItem, 0, ae1.sz0);
        ListView_SetItemText(hCtl1, iItem, 1, ae1.sz1);
        ListView_SetItemText(hCtl1, iItem, 2, ae1.sz2);
        ListView_SetItemText(hCtl1, iItem + 1, 0, ae0.sz0);
        ListView_SetItemText(hCtl1, iItem + 1, 1, ae0.sz1);
        ListView_SetItemText(hCtl1, iItem + 1, 2, ae0.sz2);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem + 1, state, state);
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

    void OnAdd(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        ACCEL_ENTRY entry;

        MAddKeyDlg dialog(entry);
        if (IDOK != dialog.DialogBoxDx(hwnd))
        {
            return;
        }

        INT iItem = ListView_GetItemCount(hCtl1);

        LV_ITEM item;

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = entry.sz0;
        ListView_InsertItem(hCtl1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 1;
        item.pszText = entry.sz1;
        ListView_SetItem(hCtl1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 2;
        item.pszText = entry.sz2;
        ListView_SetItem(hCtl1, &item);

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, iItem, state, state);
    }

    void OnModify(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT iItem = ListView_GetNextItem(hCtl1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
        {
            return;
        }

        ACCEL_ENTRY a_entry;
        ListView_GetItemText(hCtl1, iItem, 0, a_entry.sz0, _countof(a_entry.sz0));
        ListView_GetItemText(hCtl1, iItem, 1, a_entry.sz1, _countof(a_entry.sz1));
        ListView_GetItemText(hCtl1, iItem, 2, a_entry.sz2, _countof(a_entry.sz2));

        MModifyKeyDlg dialog(a_entry);
        if (IDOK == dialog.DialogBoxDx(hwnd))
        {
            ListView_SetItemText(hCtl1, iItem, 0, a_entry.sz0);
            ListView_SetItemText(hCtl1, iItem, 1, a_entry.sz1);
            ListView_SetItemText(hCtl1, iItem, 2, a_entry.sz2);
        }
    }

    void OnOK(HWND hwnd)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);

        INT i, Count = ListView_GetItemCount(hCtl1);

        if (Count == 0)
        {
            ErrorBoxDx(IDS_DATAISEMPTY);
            return;
        }

        m_accel_res.Entries().clear();
        for (i = 0; i < Count; ++i)
        {
            ACCEL_ENTRY a_entry;
            ListView_GetItemText(hCtl1, i, 0, a_entry.sz0, _countof(a_entry.sz0));
            ListView_GetItemText(hCtl1, i, 1, a_entry.sz1, _countof(a_entry.sz1));
            ListView_GetItemText(hCtl1, i, 2, a_entry.sz2, _countof(a_entry.sz2));

            WORD Flags;
            SetKeyFlags(Flags, a_entry.sz1);

            AccelTableEntry entry;
            entry.fFlags = Flags;
            if (Flags & FVIRTKEY)
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
                    entry.wAscii = _wtoi(a_entry.sz0);
                }
            }
            entry.wId = _wtoi(a_entry.sz2);

            m_accel_res.Entries().push_back(entry);
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

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        HWND hCtl1 = GetDlgItem(hwnd, ctl1);
        ListView_SetExtendedListViewStyle(hCtl1, LVS_EX_FULLROWSELECT);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 105;
        column.pszText = LoadStringDx(IDS_KEY);
        column.iSubItem = 0;
        ListView_InsertColumn(hCtl1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 75;
        column.pszText = LoadStringDx(IDS_FLAGS);
        column.iSubItem = 1;
        ListView_InsertColumn(hCtl1, 1, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 185;
        column.pszText = LoadStringDx(IDS_COMMANDID);
        column.iSubItem = 2;
        ListView_InsertColumn(hCtl1, 2, &column);

        typedef AccelRes::entries_type entries_type;
        const entries_type& entries = m_accel_res.Entries();

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
                str = str_quote(str);
            }

            LV_ITEM item;
            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 0;
            item.pszText = &str[0];
            ListView_InsertItem(hCtl1, &item);

            str = GetKeyFlags(it->fFlags);

            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 1;
            item.pszText = &str[0];
            ListView_SetItem(hCtl1, &item);

            str = GetKeyID(it->wId);

            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 2;
            item.pszText = &str[0];
            ListView_SetItem(hCtl1, &item);
        }

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hCtl1, 0, state, state);
        SetFocus(hCtl1);

        return TRUE;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MEDITACCELDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
