// MPathsDlg.hpp --- Dialogs for Include Directories
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

#ifndef MZC4_MPATHSDLG_HPP_
#define MZC4_MPATHSDLG_HPP_

#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "MResizable.hpp"
#include "resource.h"

class MPathsDlg;

//////////////////////////////////////////////////////////////////////////////

class MPathsDlg : public MDialogBase
{
public:
    RisohSettings& m_settings;
    HWND m_hLst1;
    MResizable m_resizable;
    HICON m_hIcon;
    HICON m_hIconSm;
    std::vector<MString> m_list;

    MPathsDlg(RisohSettings& settings)
        : MDialogBase(IDD_PATHS), m_settings(settings)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);
    }

    ~MPathsDlg()
    {
        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    void OnDelete(HWND hwnd)
    {
        INT iItem = ListBox_GetCurSel(m_hLst1);
        if (iItem < 0)
            return;

        ListBox_DeleteString(m_hLst1, iItem);
    }

    void OnAdd(HWND hwnd)
    {
    }

    void OnModify(HWND hwnd)
    {
    }

    void OnOK(HWND hwnd)
    {
        TCHAR szText[MAX_PATH];

        m_list.clear();
        INT i, nCount = ListBox_GetCount(m_hLst1);
        for (i = 0; i < nCount; ++i)
        {
            ListBox_GetText(m_hLst1, i, szText);
            mstr_trim(szText);
            m_list.push_back(szText);
        }

        m_settings.includes = m_list;

        EndDialog(IDOK);
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        if (hwndContext == m_hLst1)
        {
            HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(2));
            HMENU hSubMenu = GetSubMenu(hMenu, 5);

            SetForegroundWindow(hwnd);
            TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                xPos, yPos, 0, hwnd, NULL);
            PostMessage(hwnd, WM_NULL, 0, 0);
            DestroyMenu(hMenu);
        }
    }

    void OnPsh6(HWND hwnd)
    {
        ListBox_ResetContent(m_hLst1);
        OnOK(hwnd);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case psh1:
        case CMDID_ADD:
            OnAdd(hwnd);
            break;
        case psh2:
        case CMDID_MODIFY:
            OnModify(hwnd);
            break;
        case psh3:
        case CMDID_DELETE:
            OnDelete(hwnd);
            break;
        case IDOK:
            if (codeNotify == 0 || codeNotify == BN_CLICKED)
                OnOK(hwnd);
            break;
        case IDCANCEL:
            if (codeNotify == 0 || codeNotify == BN_CLICKED)
                EndDialog(IDCANCEL);
            break;
        case psh6:
            if (codeNotify == 0 || codeNotify == BN_CLICKED)
                OnPsh6(hwnd);
            break;
        }
    }

    void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
    {
        INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem >= 0)
        {
            EnableMenuItem(hMenu, CMDID_MODIFY, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, CMDID_DELETE, MF_BYCOMMAND | MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, CMDID_MODIFY, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenu, CMDID_DELETE, MF_BYCOMMAND | MF_GRAYED);
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
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

        m_list.clear();

        DWORD dwNumIncludes = DWORD(m_settings.includes.size());
        for (DWORD i = 0; i < dwNumIncludes; ++i)
        {
            m_list.push_back(m_settings.includes[i]);
            ListBox_AddString(m_hLst1, m_settings.includes[i].c_str());
        }

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

        SetFocus(m_hLst1);

        CenterWindowDx();
        return TRUE;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MPATHSDLG_HPP_
