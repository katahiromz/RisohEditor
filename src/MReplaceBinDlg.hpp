// MReplaceBinDlg
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Win32API resource editor
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

#ifndef MZC4_MREPLACEBINDLG_HPP_
#define MZC4_MREPLACEBINDLG_HPP_

//////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "resource.h"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& type);
BOOL CheckNameComboBox(ConstantsDB& db, HWND hCmb2, MIdOrString& name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& file);
void InitResNameComboBox(HWND hCmb, ConstantsDB& db, MIdOrString id, INT nIDTYPE_);

//////////////////////////////////////////////////////////////////////////////

class MReplaceBinDlg : public MDialogBase
{
public:
    ResEntries& m_entries;
    ResEntry& m_entry;
    ConstantsDB& m_db;
    ResEntry m_entry_copy;

    MReplaceBinDlg(ResEntries& entries, ResEntry& entry, ConstantsDB& db)
        : MDialogBase(IDD_REPLACERES), m_entries(entries), m_entry(entry), m_db(db)
    {
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
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        DragAcceptFiles(hwnd, TRUE);

        // for Types
        INT k;
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& table = m_db.GetTable(L"RESOURCE");
        for (size_t i = 0; i < table.size(); ++i)
        {
            WCHAR sz[MAX_PATH];
            wsprintfW(sz, L"%s (%lu)", table[i].name.c_str(), table[i].value);
            k = ComboBox_AddString(hCmb1, sz);
            if (m_entry.type == WORD(table[i].value))
            {
                ComboBox_SetCurSel(hCmb1, k);
            }
        }
        k = ComboBox_AddString(hCmb1, TEXT("WAVE"));
        if (m_entry.type == TEXT("WAVE"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("PNG"));
        if (m_entry.type == TEXT("PNG"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("GIF"));
        if (m_entry.type == TEXT("GIF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("JPEG"));
        if (m_entry.type == TEXT("JPEG"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("TIFF"));
        if (m_entry.type == TEXT("TIFF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("AVI"));
        if (m_entry.type == TEXT("AVI"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("EMF"));
        if (m_entry.type == TEXT("EMF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("WMF"));
        if (m_entry.type == TEXT("WMF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }

        // for Names
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        InitResNameComboBox(hCmb2, m_db, m_entry.name, IDTYPE_RESOURCE);
        ::EnableWindow(hCmb2, FALSE);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, m_entry.lang);
        ::EnableWindow(hCmb3, FALSE);

        CenterWindowDx();
        return TRUE;
    }

    void OnPsh1(HWND hwnd)
    {
        MStringW strFile = GetDlgItemText(edt1);
        mstr_trim(strFile);

        WCHAR szFile[MAX_PATH];
        lstrcpyn(szFile, strFile.c_str(), _countof(szFile));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ALLFILES));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = _countof(szFile);
        ofn.lpstrTitle = LoadStringDx(IDS_REPLACERES);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"bin";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, szFile);
        }
    }

    void OnOK(HWND hwnd)
    {
        MIdOrString type;
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& table = m_db.GetTable(L"RESOURCE");
        INT iType = ComboBox_GetCurSel(hCmb1);
        if (iType != CB_ERR && iType < INT(table.size()))
        {
            type = WORD(table[iType].value);
        }
        else
        {
            if (!CheckTypeComboBox(hCmb1, type))
                return;
        }

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        MIdOrString name;
        if (!CheckNameComboBox(m_db, hCmb2, name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD lang;
        if (!CheckLangComboBox(hCmb3, lang))
            return;

        std::wstring file;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Edt1_CheckFile(hEdt1, file))
            return;

        MByteStreamEx bs;
        if (!bs.LoadFromFile(file.c_str()))
        {
            ErrorBoxDx(IDS_CANNOTREPLACE);
            return;
        }

        Res_AddEntry(m_entries, type, name, lang, bs.data(), TRUE);

        ResEntry entry(type, name, lang);
        m_entry_copy = entry;

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

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR file[MAX_PATH];
        DragQueryFileW(hdrop, 0, file, _countof(file));
        SetDlgItemTextW(hwnd, edt1, file);
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MREPLACEBINDLG_HPP_
