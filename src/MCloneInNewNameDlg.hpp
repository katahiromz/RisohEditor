// MCloneInNewNameDlg.hpp --- "Clone In New Name" Dialog
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

#ifndef MZC4_MCLONEINNEWNAMEDLG_HPP_
#define MZC4_MCLONEINNEWNAMEDLG_HPP_

//////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "resource.h"

BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& type);
BOOL CheckNameComboBox(ConstantsDB& db, HWND hCmb2, MIdOrString& name);
void InitResNameComboBox(HWND hCmb, ConstantsDB& db, MIdOrString id, INT nIDTYPE_);

//////////////////////////////////////////////////////////////////////////////

class MCloneInNewNameDlg : public MDialogBase
{
public:
    ResEntries& m_entries;
    ResEntry& m_entry;
    ConstantsDB& m_db;
    MIdOrString m_name;
    MComboBoxAutoComplete m_cmb2;

    MCloneInNewNameDlg(ResEntries& entries, ResEntry& entry, ConstantsDB& db)
        : MDialogBase(IDD_CLONEINNEWNAME), m_entries(entries), m_entry(entry), m_db(db)
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
        k = ComboBox_AddString(hCmb1, TEXT("IMAGE"));
        if (m_entry.type == TEXT("IMAGE"))
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
        k = ComboBox_AddString(hCmb1, TEXT("ENHMETAFILE"));
        if (m_entry.type == TEXT("ENHMETAFILE"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("WMF"));
        if (m_entry.type == TEXT("WMF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }

        // for Names
        INT nIDTYPE_ = m_db.IDTypeFromRes(m_entry.type);
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        InitResNameComboBox(hCmb2, m_db, m_entry.name, nIDTYPE_);
        SubclassChildDx(m_cmb2, cmb2);

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

        // for Names
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        MIdOrString name;
        if (!CheckNameComboBox(m_db, hCmb2, name))
            return;

        if (Res_Find(m_entries, m_entry.type, name, m_entry.lang, FALSE) != -1)
        {
            if (MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL) != IDYES)
            {
                return;
            }
        }

        m_name = name;

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
        case cmb2:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb2.OnEditChange();
            }
            break;
        }
    }

    void OnPsh1(HWND hwnd)
    {
        SendMessage(GetParent(hwnd), WM_COMMAND, CMDID_IDLIST, 0);
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MCLONEINNEWNAMEDLG_HPP_
