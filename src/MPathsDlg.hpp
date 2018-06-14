// MPathsDlg.hpp --- Dialogs for Paths
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
    HWND m_hLst1;
    MResizable m_resizable;
    HICON m_hIcon;
    HICON m_hIconSm;
    std::vector<MString> m_list;

    MPathsDlg() : MDialogBase(IDD_PATHS)
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
        OnSelChange(hwnd);
    }

    void OnAdd(HWND hwnd)
    {
        TCHAR szPath[MAX_PATH];
        BROWSEINFO bi;

        ZeroMemory(&bi, sizeof(bi));
        bi.hwndOwner = hwnd;
        bi.lpszTitle = LoadStringDx(IDS_ADDINCLUDE);
        bi.ulFlags = BIF_RETURNONLYFSDIRS;
        if (LPITEMIDLIST pidl = SHBrowseForFolder(&bi))
        {
            SHGetPathFromIDList(pidl, szPath);
            INT iItem = ListBox_AddString(m_hLst1, szPath);
            CoTaskMemFree(pidl);

            if (iItem != LB_ERR)
            {
                ListBox_SetCurSel(m_hLst1, iItem);
                OnSelChange(hwnd);
            }
        }
    }

    static INT CALLBACK
    BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
    {
        if (uMsg == BFFM_INITIALIZED)
        {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpData);
        }
        return 0;
    }

    BOOL SetItemText(HWND hLst1, INT iItem, LPCTSTR psz)
    {
        ListBox_DeleteString(hLst1, iItem);
        ListBox_InsertString(hLst1, iItem, psz);
        return TRUE;
    }

    void OnModify(HWND hwnd)
    {
        TCHAR szPath[MAX_PATH];
        BROWSEINFO bi;

        INT iItem = ListBox_GetCurSel(m_hLst1);
        if (iItem == LB_ERR)
            return;

        ListBox_GetText(m_hLst1, iItem, szPath);

        ZeroMemory(&bi, sizeof(bi));
        bi.hwndOwner = hwnd;
        bi.pidlRoot = NULL;
        bi.lpszTitle = LoadStringDx(IDS_EDITINCLUDE);
        bi.ulFlags = BIF_RETURNONLYFSDIRS;
        bi.lpfn = BrowseCallbackProc;
        bi.lParam = (LPARAM)szPath;
        if (LPITEMIDLIST pidl = SHBrowseForFolder(&bi))
        {
            SHGetPathFromIDList(pidl, szPath);
            SetItemText(m_hLst1, iItem, szPath);
            CoTaskMemFree(pidl);
        }
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

        GetDlgItemText(hwnd, cmb1, szText, _countof(szText));
        MString strWindResExe = szText;
        mstr_trim(strWindResExe);

        GetDlgItemText(hwnd, cmb2, szText, _countof(szText));
        MString strCppExe = szText;
        mstr_trim(strCppExe);

        if (strWindResExe.size() &&
            GetFileAttributes(strWindResExe.c_str()) == 0xFFFFFFFF)
        {
            HWND hCmb1 = GetDlgItem(hwnd, cmb1);
            ComboBox_SetEditSel(hCmb1, 0, -1);
            SetFocus(hCmb1);
            ErrorBoxDx(IDS_INVALIDPATH);
            return;
        }

        if (strCppExe.size() &&
            GetFileAttributes(strCppExe.c_str()) == 0xFFFFFFFF)
        {
            HWND hCmb2 = GetDlgItem(hwnd, cmb2);
            ComboBox_SetEditSel(hCmb2, 0, -1);
            SetFocus(hCmb2);
            ErrorBoxDx(IDS_INVALIDPATH);
            return;
        }

        g_settings.includes = m_list;
        g_settings.strWindResExe = strWindResExe;
        g_settings.strCppExe = strCppExe;

        EndDialog(IDOK);
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        if (hwndContext == m_hLst1)
        {
            PopupMenuDx(hwnd, m_hLst1, IDR_POPUPMENUS, 7, xPos, yPos);
        }
    }

    void OnPsh4(HWND hwnd)
    {
        INT iItem = ListBox_GetCurSel(m_hLst1);
        if (iItem == LB_ERR)
            return;

        TCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];
        ListBox_GetText(m_hLst1, iItem - 1, szPath1);
        ListBox_GetText(m_hLst1, iItem, szPath2);

        SetItemText(m_hLst1, iItem - 1, szPath2);
        SetItemText(m_hLst1, iItem, szPath1);

        ListBox_SetCurSel(m_hLst1, iItem - 1);
        OnSelChange(hwnd);
    }

    void OnPsh5(HWND hwnd)
    {
        INT iItem = ListBox_GetCurSel(m_hLst1);
        if (iItem < 0)
            return;

        INT nCount = ListBox_GetCount(m_hLst1);
        if (iItem + 1 >= nCount)
            return;

        TCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];
        ListBox_GetText(m_hLst1, iItem, szPath1);
        ListBox_GetText(m_hLst1, iItem + 1, szPath2);

        SetItemText(m_hLst1, iItem, szPath2);
        SetItemText(m_hLst1, iItem + 1, szPath1);

        ListBox_SetCurSel(m_hLst1, iItem + 1);
        OnSelChange(hwnd);
    }

    void OnPsh6(HWND hwnd)
    {
        ListBox_ResetContent(m_hLst1);
        OnSelChange(hwnd);
    }

    void OnPsh7(HWND hwnd)
    {
        WCHAR file[MAX_PATH];
        GetDlgItemText(hwnd, cmb1, file, _countof(file));
        mstr_trim(file);

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_WINDRESEXE));
        ofn.lpstrFile = file;
        ofn.nMaxFile = _countof(file);
        ofn.lpstrTitle = LoadStringDx(IDS_LOADWCLIB);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"exe";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemText(hwnd, cmb1, file);
            OnSelChange(hwnd);
        }
    }

    void OnPsh8(HWND hwnd)
    {
        WCHAR file[MAX_PATH];
        GetDlgItemText(hwnd, cmb2, file, _countof(file));
        mstr_trim(file);

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_CPPEXE));
        ofn.lpstrFile = file;
        ofn.nMaxFile = _countof(file);
        ofn.lpstrTitle = LoadStringDx(IDS_LOADWCLIB);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"exe";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemText(hwnd, cmb2, file);
            OnSelChange(hwnd);
        }
    }

    void OnPsh9(HWND hwnd)
    {
        ListBox_ResetContent(m_hLst1);
        SetDlgItemText(hwnd, cmb1, NULL);
        SetDlgItemText(hwnd, cmb2, NULL);
        OnSelChange(hwnd);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case psh1:
        case ID_ADD:
            OnAdd(hwnd);
            break;
        case psh2:
        case ID_MODIFY:
            OnModify(hwnd);
            break;
        case psh3:
        case ID_DELETE:
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
        case psh4:
            if (codeNotify == 0 || codeNotify == BN_CLICKED)
                OnPsh4(hwnd);
            break;
        case psh5:
            if (codeNotify == 0 || codeNotify == BN_CLICKED)
                OnPsh5(hwnd);
            break;
        case psh6:
            if (codeNotify == 0 || codeNotify == BN_CLICKED)
                OnPsh6(hwnd);
            break;
        case psh7:
            if (codeNotify == 0 || codeNotify == BN_CLICKED)
                OnPsh7(hwnd);
            break;
        case psh8:
            if (codeNotify == 0 || codeNotify == BN_CLICKED)
                OnPsh8(hwnd);
            break;
        case psh9:
            if (codeNotify == 0 || codeNotify == BN_CLICKED)
                OnPsh9(hwnd);
            break;
        case lst1:
            if (codeNotify == LBN_SELCHANGE)
            {
                OnSelChange(hwnd);
            }
            break;
        }
    }

    void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
    {
        INT iItem = ListBox_GetCurSel(m_hLst1);
        if (iItem >= 0)
        {
            EnableMenuItem(hMenu, psh2, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, psh3, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, psh4, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, psh5, MF_BYCOMMAND | MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, psh2, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenu, psh3, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenu, psh4, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenu, psh5, MF_BYCOMMAND | MF_GRAYED);
        }
    }

    void OnSelChange(HWND hwnd)
    {
        INT iItem = ListBox_GetCurSel(m_hLst1);
        BOOL bSelected = (iItem != -1);
        EnableWindow(GetDlgItem(hwnd, psh2), bSelected);
        EnableWindow(GetDlgItem(hwnd, psh3), bSelected);
        EnableWindow(GetDlgItem(hwnd, psh4), bSelected);
        EnableWindow(GetDlgItem(hwnd, psh5), bSelected);
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
        }
        return 0;
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        m_resizable.OnSize();
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        m_hLst1 = GetDlgItem(hwnd, lst1);

        m_list.clear();

        DWORD dwNumIncludes = DWORD(g_settings.includes.size());
        for (DWORD i = 0; i < dwNumIncludes; ++i)
        {
            m_list.push_back(g_settings.includes[i]);
            ListBox_AddString(m_hLst1, g_settings.includes[i].c_str());
        }

        SetDlgItemText(hwnd, cmb1, g_settings.strWindResExe.c_str());
        SetDlgItemText(hwnd, cmb2, g_settings.strCppExe.c_str());

        m_resizable.OnParentCreate(hwnd);

        m_resizable.SetLayoutAnchor(lst1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(psh1, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(psh2, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(psh3, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(psh4, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(psh5, mzcLA_TOP_RIGHT);
        m_resizable.SetLayoutAnchor(psh6, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(stc1, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(stc2, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(cmb1, mzcLA_BOTTOM_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(cmb2, mzcLA_BOTTOM_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(psh7, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(psh8, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(IDOK, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(IDCANCEL, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(psh9, mzcLA_BOTTOM_LEFT);

        SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

        SetFocus(m_hLst1);

        OnSelChange(hwnd);

        CenterWindowDx();
        return TRUE;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MPATHSDLG_HPP_
