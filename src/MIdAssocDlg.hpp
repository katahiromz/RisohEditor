// MIdAssocDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MIDASSOCDLG_HPP_
#define MZC4_MIDASSOCDLG_HPP_

#include "MWindowBase.hpp"
#include "MModifyAssocDlg.hpp"

//////////////////////////////////////////////////////////////////////////////

class MIdAssocDlg : public MDialogBase
{
public:
    typedef std::map<MString, MString> map_type;
    map_type& m_map;
    HWND m_hLst1;

    MIdAssocDlg(map_type& map) : MDialogBase(IDD_IDASSOC), m_map(map)
    {
    }

    void Lst1_Init(HWND hLst1)
    {
        ListView_DeleteAllItems(hLst1);

        LV_ITEM item;

        INT iItem = 0;
        map_type::iterator it, end = m_map.end();
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

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, 0, state, state);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        m_hLst1 = GetDlgItem(hwnd, lst1);
        ListView_SetExtendedListViewStyle(m_hLst1, LVS_EX_FULLROWSELECT);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 155;
        column.pszText = LoadStringDx(IDS_IDTYPE);
        column.iSubItem = 0;
        ListView_InsertColumn(m_hLst1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 150;
        column.pszText = LoadStringDx(IDS_IDPREFIX);
        column.iSubItem = 1;
        ListView_InsertColumn(m_hLst1, 1, &column);

        Lst1_Init(m_hLst1);

        return TRUE;
    }

    void OnPsh1(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem == -1)
            return;

        TCHAR szText[64];
        MString str1, str2;

        ListView_GetItemText(m_hLst1, iItem, 0, szText, _countof(szText));
        str1 = szText;

        ListView_GetItemText(m_hLst1, iItem, 1, szText, _countof(szText));
        str2 = szText;

        MModifyAssocDlg dialog(str1, str2);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            m_map[str1] = str2;
            ListView_SetItemText(m_hLst1, iItem, 1, const_cast<LPTSTR>(str2.c_str()));
        }
    }

    void OnOK(HWND hwnd)
    {
        TCHAR szText[64];
        MString str1, str2;

        INT iItem, nCount = ListView_GetItemCount(m_hLst1);
        for (iItem = 0; iItem < nCount; ++iItem)
        {
            ListView_GetItemText(m_hLst1, iItem, 0, szText, _countof(szText));
            str1 = szText;

            ListView_GetItemText(m_hLst1, iItem, 1, szText, _countof(szText));
            str2 = szText;

            m_map[str1] = str2;
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
        case psh1:
            OnPsh1(hwnd);
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
        default:
            return DefaultProcDx();
        }
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MIDASSOCDLG_HPP_
