// MAddIconDlg.hpp --- "Add Icon" Dialog
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

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "MLangAutoComplete.hpp"
#include "Common.hpp"

//////////////////////////////////////////////////////////////////////////////

class MAddIconDlg : public MDialogBase
{
public:
    LPCWSTR m_file;
    HICON   m_hIcon;
    MIdOrString m_type;
    MIdOrString m_name;
    WORD m_lang;
    MComboBoxAutoComplete m_cmb2;
    MComboBoxAutoComplete m_cmb3;
    MLangAutoComplete *m_pAutoComplete;

    MAddIconDlg()
        : MDialogBase(IDD_ADDICON)
        , m_file(NULL)
        , m_hIcon(NULL)
        , m_pAutoComplete(new MLangAutoComplete())
    {
        m_cmb3.m_bAcceptSpace = TRUE;
        m_cmb3.m_bIgnoreCase = TRUE;
    }

    ~MAddIconDlg()
    {
        m_pAutoComplete->unbind();
        m_pAutoComplete->Release();
        m_pAutoComplete = NULL;
        DestroyIcon(m_hIcon);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        ::DragAcceptFiles(hwnd, TRUE);

        // for Names
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        InitResNameComboBox(hCmb2, L"", IDTYPE_ICON);
        SetWindowText(hCmb2, L"");
        SubclassChildDx(m_cmb2, cmb2);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, GetDefaultResLanguage());
        SubclassChildDx(m_cmb3, cmb3);

        // auto complete
        COMBOBOXINFO info = { sizeof(info) };
        GetComboBoxInfo(m_cmb3, &info);
        HWND hwndEdit = info.hwndItem;
        m_pAutoComplete->bind(hwndEdit);

        CenterWindowDx();

        FileSystemAutoComplete(GetDlgItem(hwnd, edt1));

        if (m_file)
        {
            DoFile(hwnd, m_file);
            SetFocus(hCmb2);
            return FALSE;
        }

        return TRUE;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }

    void OnOK(HWND hwnd)
    {
        MIdOrString type = RT_GROUP_ICON;

        MIdOrString name;
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (!CheckNameComboBox(hCmb2, name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD lang;
        if (!CheckLangComboBox(hCmb3, lang))
            return;

        std::wstring file;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Edt1_CheckFile(hEdt1, file))
        {
            Edit_SetSel(hEdt1, 0, -1);  // select all
            SetFocus(hEdt1);    // set focus
            ErrorBoxDx(IDS_FILENOTFOUND);
            return;
        }

        if (auto entry = g_res.find(ET_LANG, RT_GROUP_ICON, name, lang))
        {
            INT id = MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL);
            switch (id)
            {
            case IDYES:
                g_res.delete_entry(entry);
                break;
            case IDNO:
            case IDCANCEL:
                return;
            }
        }

        if (!g_res.add_group_icon(name, lang, file))
        {
            ErrorBoxDx(IDS_CANNOTADDICON);
            return;
        }

        m_type = type;
        m_name = name;
        m_lang = lang;

        EndDialog(IDOK);
    }

    void OnPsh1(HWND hwnd)
    {
        MStringW strFile = GetDlgItemText(edt1);
        mstr_trim(strFile);

        WCHAR szFile[MAX_PATH];
        lstrcpynW(szFile, strFile.c_str(), _countof(szFile));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ICOFILTER));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = _countof(szFile);
        ofn.lpstrTitle = LoadStringDx(IDS_ADDICON);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"ico";
        if (GetOpenFileNameW(&ofn))
        {
            DoFile(hwnd, szFile);
        }
    }

    void DoFile(HWND hwnd, LPCWSTR szFile)
    {
        SetDlgItemTextW(hwnd, edt1, szFile);
        if (m_hIcon)
            DestroyIcon(m_hIcon);
        m_hIcon = ExtractIcon(GetModuleHandle(NULL), szFile, 0);
        Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);

        // If name was empty, use file title
        MString strText = GetComboBoxText(GetDlgItem(hwnd, cmb2));
        if (strText.empty())
        {
            WCHAR szText[MAX_PATH];
            StringCchCopyW(szText, _countof(szText), szFile);
            PathRemoveExtensionW(szText);
            ComboBox_SetText(GetDlgItem(hwnd, cmb2), PathFindFileNameW(szText));
        }
    }

    void OnPsh2(HWND hwnd)
    {
        SendMessage(GetParent(hwnd), WM_COMMAND, ID_IDLIST, 0);
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
        case cmb2:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb2.OnEditChange();
            }
            break;
        case cmb3:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb3.OnEditChange();
            }
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR file[MAX_PATH];
        DragQueryFileW(hdrop, 0, file, _countof(file));
        DoFile(hwnd, file);
    }
};
