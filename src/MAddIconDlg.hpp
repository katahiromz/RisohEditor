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

#ifndef MZC4_MADDICONDLG_HPP_
#define MZC4_MADDICONDLG_HPP_

#include "MWindowBase.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "resource.h"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckNameComboBox(HWND hCmb2, MIdOrString& name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& file);

//////////////////////////////////////////////////////////////////////////////

class MAddIconDlg : public MDialogBase
{
public:
    LPCWSTR m_file;
    HICON   m_hIcon;
    EntryBase m_entry_copy;
    MComboBoxAutoComplete m_cmb2;
    MComboBoxAutoComplete m_cmb3;

    MAddIconDlg()
        : MDialogBase(IDD_ADDICON), m_file(NULL), m_hIcon(NULL)
    {
    }

    ~MAddIconDlg()
    {
        DestroyIcon(m_hIcon);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemTextW(hwnd, edt1, m_file);
        if (m_hIcon)
            DestroyIcon(m_hIcon);
        m_hIcon = ExtractIcon(GetModuleHandle(NULL), m_file, 0);
        Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);

        DragAcceptFiles(hwnd, TRUE);

        // for Names
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        InitResNameComboBox(hCmb2, L"", IDTYPE_ICON);
        SetWindowText(hCmb2, L"");
        SubclassChildDx(m_cmb2, cmb2);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, GetUserDefaultLangID());
        SubclassChildDx(m_cmb3, cmb3);

        CenterWindowDx();
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
            return;

        bool bOverwrite = false;
        auto entry = g_res.find(ET_LANG, RT_GROUP_ICON, name, lang);
        if (entry)
        {
            INT id = MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL);
            switch (id)
            {
            case IDYES:
                bOverwrite = true;
                break;
            case IDNO:
            case IDCANCEL:
                return;
            }
        }

        if (!g_res.add_group_icon(name, lang, file, bOverwrite))
        {
            if (bOverwrite)
                ErrorBoxDx(IDS_CANTREPLACEICO);
            else
                ErrorBoxDx(IDS_CANNOTADDICON);
            return;
        }

        m_entry_copy = EntryBase(ET_LANG, type, name, lang);

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
            SetDlgItemTextW(hwnd, edt1, szFile);
            if (m_hIcon)
                DestroyIcon(m_hIcon);
            m_hIcon = ExtractIcon(GetModuleHandle(NULL), szFile, 0);
            Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);
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
        SetDlgItemTextW(hwnd, edt1, file);

        if (m_hIcon)
            DestroyIcon(m_hIcon);
        m_hIcon = ExtractIcon(GetModuleHandle(NULL), file, 0);
        Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MADDICONDLG_HPP_
