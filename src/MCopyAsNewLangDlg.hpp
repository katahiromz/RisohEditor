// MCopyAsNewLangDlg
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

#ifndef MZC4_MCOPYASNEWLANGDLG_HPP_
#define MZC4_MCOPYASNEWLANGDLG_HPP_

//////////////////////////////////////////////////////////////////////////////

#include "RisohEditor.hpp"
#include "ConstantsDB.hpp"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& type);
BOOL CheckNameComboBox(ConstantsDB& db, HWND hCmb2, MIdOrString& name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& file);
void InitCommandComboBox(HWND hCmb, ConstantsDB& db, MString strCommand);

//////////////////////////////////////////////////////////////////////////////

class MCopyAsNewLangDlg : public MDialogBase
{
public:
    ResEntries& m_entries;
    ResEntry& m_Entry;
    ConstantsDB& m_db;
    WORD m_lang;

    MCopyAsNewLangDlg(ResEntries& entries, ResEntry& entry, ConstantsDB& db)
        : MDialogBase(IDD_COPYASNEWLANG), m_entries(entries), m_Entry(entry), m_db(db)
    {
        m_lang = 0xFFFF;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx(hwnd, uMsg, wParam, lParam);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // for Types
        INT k;
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& table = m_db.GetTable(L"RESOURCE");
        for (size_t i = 0; i < table.size(); ++i)
        {
            WCHAR sz[MAX_PATH];
            wsprintfW(sz, L"%s (%lu)", table[i].name.c_str(), table[i].value);
            k = ComboBox_AddString(hCmb1, sz);
            if (m_Entry.type == WORD(table[i].value))
            {
                ComboBox_SetCurSel(hCmb1, k);
            }
        }
        k = ComboBox_AddString(hCmb1, TEXT("WAVE"));
        if (m_Entry.type == TEXT("WAVE"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("PNG"));
        if (m_Entry.type == TEXT("PNG"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("GIF"));
        if (m_Entry.type == TEXT("GIF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("JPEG"));
        if (m_Entry.type == TEXT("JPEG"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("TIFF"));
        if (m_Entry.type == TEXT("TIFF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("AVI"));
        if (m_Entry.type == TEXT("AVI"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("EMF"));
        if (m_Entry.type == TEXT("EMF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("WMF"));
        if (m_Entry.type == TEXT("WMF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, m_Entry.lang);

        CenterWindowDx();
        return TRUE;
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

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD lang;
        if (!CheckLangComboBox(hCmb3, lang))
            return;

        m_lang = lang;

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
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MCOPYASNEWLANGDLG_HPP_
