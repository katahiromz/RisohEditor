// MCopyToMultiLangDlg.hpp --- "Copy to multiple languages" Dialog
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

void InitLangComboBox(HWND hCmb3, LANGID langid);
WORD LangFromText(LPWSTR pszLang);
MStringW TextFromLang(WORD lang);

//////////////////////////////////////////////////////////////////////////////

class MCopyToMultiLangDlg : public MDialogBase
{
public:
    EntryBase *m_entry;
    MIdOrString m_type;
    MIdOrString m_name;
    WORD m_lang;
    MComboBoxAutoComplete m_cmb3;
    std::vector<LANGID> m_selection;
    MLangAutoComplete *m_pAutoComplete;

    MCopyToMultiLangDlg(EntryBase* entry)
        : MDialogBase(IDD_COPYTOMULTILANG), m_entry(entry),
          m_type(entry->m_type), m_name(entry->m_name), m_lang(entry->m_lang),
          m_pAutoComplete(new MLangAutoComplete())
    {
        m_cmb3.m_bAcceptSpace = TRUE;
        m_pAutoComplete->AddRef();
    }

    ~MCopyToMultiLangDlg()
    {
        m_pAutoComplete->unbind();
        m_pAutoComplete->Release();
        m_pAutoComplete = NULL;
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
        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, BAD_LANG);
        SubclassChildDx(m_cmb3, cmb3);

        EntrySet found;
        g_res.search(found, ET_LANG, m_type, m_name);

        HWND hLst1 = GetDlgItem(hwnd, lst1);
        for (auto e : found)
        {
            if (m_lang != e->m_lang)
            {
                auto strLang = TextFromLang(e->m_lang);
                INT index = ListBox_AddString(hLst1, strLang.c_str());
                if (index != LB_ERR)
                    ListBox_SelItemRange(hLst1, TRUE, index, index);
            }
        }

        // auto complete
        COMBOBOXINFO info = { sizeof(info) };
        GetComboBoxInfo(m_cmb3, &info);
        HWND hwndEdit = info.hwndItem;
        m_pAutoComplete->bind(hwndEdit);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        HWND hLst1 = GetDlgItem(hwnd, lst1);

        m_selection.clear();

        std::vector<INT> indexes;
        INT nCount = ListBox_GetSelCount(hLst1);
        if (nCount == 0)
        {
            EndDialog(hwnd, IDOK);
            return;
        }
        indexes.resize(nCount);

        ListBox_GetSelItems(hLst1, nCount, &indexes[0]);

        WCHAR szText[MAX_PATH];
        for (auto& index : indexes)
        {
            ListBox_GetText(hLst1, index, szText);
            WORD wLang = LangFromText(szText);
            m_selection.push_back(wLang);
        }

        BOOL bOverwrite = FALSE;
        for (auto lang : m_selection)
        {
            if (!bOverwrite && g_res.find(ET_LANG, m_type, m_name, lang))
            {
                if (MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL) != IDYES)
                {
                    return;
                }
                bOverwrite = TRUE;
            }
        }

        EndDialog(IDOK);
    }

    void OnAddItem(HWND hwnd)
    {
        WCHAR szText[MAX_PATH];
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        INT iItem = ComboBox_GetCurSel(hCmb3);
        if (iItem == CB_ERR)
            GetDlgItemTextW(hwnd, cmb3, szText, _countof(szText));
        else
            ComboBox_GetLBText(hCmb3, iItem, szText);

        std::wstring str = szText;
        mstr_trim(str);
        if (str.empty())
        {
            MsgBoxDx(IDS_INVALIDLANG, MB_ICONERROR);
            return;
        }

        StringCchCopyW(szText, _countof(szText), str.c_str());
        WORD wLang = LangFromText(szText);
        if (wLang != BAD_LANG)
        {
            HWND hLst1 = GetDlgItem(hwnd, lst1);
            str = TextFromLang(wLang);
            INT iItem = ListBox_FindStringExact(hLst1, -1, str.c_str());
            if (iItem == LB_ERR)
            {
                iItem = ListBox_AddString(hLst1, str.c_str());
            }
            ListBox_SelItemRange(hLst1, TRUE, iItem, iItem);
            SetDlgItemTextW(hwnd, cmb3, NULL);
        }
        else
        {
            MsgBoxDx(IDS_INVALIDLANG, MB_ICONERROR);
        }
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
            OnAddItem(hwnd);
            break;
        case cmb3:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb3.OnEditChange();
            }
            break;
        }
    }
};
