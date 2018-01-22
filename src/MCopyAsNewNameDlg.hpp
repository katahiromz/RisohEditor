// MCopyAsNewNameDlg
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

#ifndef MZC4_MCOPYASNEWNAMEDLG_HPP_
#define MZC4_MCOPYASNEWNAMEDLG_HPP_

//////////////////////////////////////////////////////////////////////////////

#include "RisohEditor.hpp"
#include "ConstantsDB.hpp"
#include "id_string.hpp"

BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& Type);
BOOL CheckNameComboBox(ConstantsDB& db, HWND hCmb2, MIdOrString& Name);
void InitCommandComboBox(HWND hCmb, ConstantsDB& db, MString strCommand);

//////////////////////////////////////////////////////////////////////////////

class MCopyAsNewNameDlg : public MDialogBase
{
public:
    ResEntries& m_Entries;
    ResEntry& m_entry;
    ConstantsDB& m_db;
    MIdOrString m_name;

    MCopyAsNewNameDlg(ResEntries& Entries, ResEntry& entry, ConstantsDB& db)
        : MDialogBase(IDD_COPYASNEWNAME), m_Entries(Entries), m_entry(entry), m_db(db)
    {
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
        DragAcceptFiles(hwnd, TRUE);

        // for Types
        INT k;
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& Table = m_db.GetTable(L"RESOURCE");
        for (size_t i = 0; i < Table.size(); ++i)
        {
            WCHAR sz[MAX_PATH];
            wsprintfW(sz, L"%s (%lu)", Table[i].name.c_str(), Table[i].value);
            k = ComboBox_AddString(hCmb1, sz);
            if (m_entry.type == WORD(Table[i].value))
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
        InitCommandComboBox(hCmb2, m_db, m_entry.name.str());

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        MIdOrString Type;
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& Table = m_db.GetTable(L"RESOURCE");
        INT iType = ComboBox_GetCurSel(hCmb1);
        if (iType != CB_ERR && iType < INT(Table.size()))
        {
            Type = WORD(Table[iType].value);
        }
        else
        {
            if (!CheckTypeComboBox(hCmb1, Type))
                return;
        }

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        MIdOrString Name;
        if (!CheckNameComboBox(m_db, hCmb2, Name))
            return;

        m_name = Name;

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

#endif  // ndef MZC4_MCOPYASNEWNAMEDLG_HPP_
