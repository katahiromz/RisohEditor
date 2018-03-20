// MIdAssocDlg.hpp --- "ID Association" Dialog
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

#ifndef MZC4_MIDASSOCDLG_HPP_
#define MZC4_MIDASSOCDLG_HPP_

#include "MWindowBase.hpp"
#include "MModifyAssocDlg.hpp"
#include "RisohSettings.hpp"

//////////////////////////////////////////////////////////////////////////////

class MIdAssocDlg : public MDialogBase
{
public:
    typedef std::map<MString, MString> map_type;
    RisohSettings& m_settings;
    HWND m_hLst1;

    MIdAssocDlg(RisohSettings& settings)
        : MDialogBase(IDD_IDASSOC), m_settings(settings)
    {
    }

    void Lst1_Init(HWND hLst1)
    {
        ListView_DeleteAllItems(hLst1);

        LV_ITEM item;

        INT iItem = 0;
        map_type::iterator it, end = m_settings.assoc_map.end();
        for (it = m_settings.assoc_map.begin(); it != end; ++it)
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

        CenterWindowDx();
        return TRUE;
    }

    void OnPsh2(HWND hwnd)
    {
        m_settings.ResetAssoc();
        Lst1_Init(m_hLst1);
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

            m_settings.assoc_map[str1] = str2;
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
        case psh2:
            OnPsh2(hwnd);
            break;
        case ID_MODIFYASSOC:
            OnPsh1(hwnd);
            break;
        }
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
    {
        if (pnmhdr->idFrom == lst1)
        {
            if (pnmhdr->code == NM_DBLCLK)
            {
                OnPsh1(hwnd);
                return 1;
            }
        }
        return 0;
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        if (hwndContext == m_hLst1)
        {
            PopupMenuDx(hwnd, m_hLst1, IDR_POPUPMENUS, 2, xPos, yPos);
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
        HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
        default:
            return DefaultProcDx();
        }
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MIDASSOCDLG_HPP_
