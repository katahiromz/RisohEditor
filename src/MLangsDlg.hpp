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
#include "MResizable.hpp"
#include "RisohSettings.hpp"

void InitLangListView(HWND hLst1, LPCTSTR pszText);
MString GetLanguageStatement(WORD langid, BOOL bOldStyle);

//////////////////////////////////////////////////////////////////////////////

class MLangsDlg : public MDialogBase
{
public:
    HWND m_hLst1;
    MResizable m_resizable;
    HICON m_hIcon;
    HICON m_hIconSm;

    MLangsDlg() : MDialogBase(IDD_LANGS)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);
    }

    ~MLangsDlg()
    {
        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    void Lst1_Init(HWND hLst1)
    {
        InitLangListView(hLst1, NULL);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        m_resizable.OnParentCreate(hwnd);

        m_resizable.SetLayoutAnchor(lst1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(stc1, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(cmb1, mzcLA_BOTTOM_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(IDOK, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(edt1, mzcLA_BOTTOM_LEFT, mzcLA_BOTTOM_RIGHT);

        SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

        m_hLst1 = GetDlgItem(hwnd, lst1);
        ListView_SetExtendedListViewStyle(m_hLst1, LVS_EX_FULLROWSELECT);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 180;
        column.pszText = LoadStringDx(IDS_LANGUAGE);
        column.iSubItem = 0;
        ListView_InsertColumn(m_hLst1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_RIGHT;
        column.cx = 110;
        column.pszText = LoadStringDx(IDS_INTVALUE);
        column.iSubItem = 1;
        ListView_InsertColumn(m_hLst1, 1, &column);

        Lst1_Init(m_hLst1);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        OnCopy(hwnd);
        EndDialog(IDOK);
    }

    void OnCmb1(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        MString strText = GetWindowText(hCmb1);

        InitLangListView(m_hLst1, strText.c_str());
    }

    void OnLst1(HWND hwnd)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
            return;

        TCHAR szText[32];
        ListView_GetItemText(m_hLst1, iItem, 1, szText, _countof(szText));

        LANGID langid = _wtoi(szText);
        MString strStatement = GetLanguageStatement(langid, TRUE);

        SetDlgItemText(hwnd, edt1, strStatement.c_str());
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
        case cmb1:
            if (codeNotify == CBN_EDITCHANGE)
            {
                OnCmb1(hwnd);
            }
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
            switch (pnmhdr->code)
            {
            case NM_DBLCLK:
                OnCopy(hwnd);
                return 1;
            case LVN_ITEMCHANGED:
                if (NM_LISTVIEW *pListView = (NM_LISTVIEW *)pnmhdr)
                {
                    if ((pListView->uChanged & LVIF_STATE) &&
                        (pListView->uNewState & LVIS_SELECTED))
                    {
                        OnLst1(hwnd);
                    }
                }
                break;
            }
        }
        return 0;
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        if (hwndContext == m_hLst1)
        {
            PopupMenuDx(hwnd, m_hLst1, IDR_POPUPMENUS, 5, xPos, yPos);
        }
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        m_resizable.OnSize();
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
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        default:
            return DefaultProcDx();
        }
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MLANGSDLG_HPP_
