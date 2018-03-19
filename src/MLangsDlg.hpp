// MLangsDlg.hpp --- "Languages" Dialog
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

#ifndef MZC4_MLANGSDLG_HPP_
#define MZC4_MLANGSDLG_HPP_

#include "MWindowBase.hpp"
#include "MModifyAssocDlg.hpp"
#include "RisohSettings.hpp"

void InitLangListView(HWND hLst1);

//////////////////////////////////////////////////////////////////////////////

class MLangsDlg : public MDialogBase
{
public:
    HWND m_hLst1;

    MLangsDlg() : MDialogBase(IDD_LANGS)
    {
    }

    void Lst1_Init(HWND hLst1)
    {
        InitLangListView(hLst1);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        m_hLst1 = GetDlgItem(hwnd, lst1);
        ListView_SetExtendedListViewStyle(m_hLst1, LVS_EX_FULLROWSELECT);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 250;
        column.pszText = LoadStringDx(IDS_LANGUAGE);
        column.iSubItem = 0;
        ListView_InsertColumn(m_hLst1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 180;
        column.pszText = LoadStringDx(IDS_INTVALUE);
        column.iSubItem = 1;
        ListView_InsertColumn(m_hLst1, 1, &column);

        Lst1_Init(m_hLst1);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
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
        case ID_COPY:
            OnCopy(hwnd);
            break;
        }
    }

    void OnCopy(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
            return;

        TCHAR szText[32];
        ListView_GetItemText(m_hLst1, iItem, 1, szText, _countof(szText));

        INT cch = lstrlen(szText) + 1;
        DWORD cb = sizeof(WCHAR) * cch;

        if (HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, cb))
        {
            if (LPVOID pv = GlobalLock(hGlobal))
            {
                CopyMemory(pv, szText, cb);
                GlobalUnlock(hGlobal);

                if (OpenClipboard(m_hLst1))
                {
                    EmptyClipboard();
                    SetClipboardData(CF_UNICODETEXT, hGlobal);
                    CloseClipboard();
                    return;
                }
            }
            GlobalFree(hGlobal);
        }
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
    {
        if (pnmhdr->idFrom == lst1)
        {
            if (pnmhdr->code == NM_DBLCLK)
            {
                OnCopy(hwnd);
                return 1;
            }
        }
        return 0;
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        if (hwndContext == m_hLst1)
        {
            HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_POPUPMENUS));
            HMENU hSubMenu = GetSubMenu(hMenu, 5);

            SetForegroundWindow(hwnd);
            TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                xPos, yPos, 0, hwnd, NULL);
            PostMessage(hwnd, WM_NULL, 0, 0);
            DestroyMenu(hMenu);
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

#endif  // ndef MZC4_MLANGSDLG_HPP_
