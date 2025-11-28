// MChooseLangDlg.hpp --- "Choose UI Language" Dialog
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
#include "MString.hpp"
#include "ConstantsDB.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "MLangAutoComplete.hpp"
#include "Common.hpp"

class MChooseLangDlg;

//////////////////////////////////////////////////////////////////////////////

class MChooseLangDlg : public MDialogBase
{
public:
    LANGID m_langid;
    MComboBoxAutoComplete m_cmb3;
    MLangAutoComplete *m_pAutoComplete;

    MChooseLangDlg()
        : MDialogBase(IDD_UILANG)
        , m_pAutoComplete(new MLangAutoComplete(TRUE))
    {
        m_cmb3.m_bAcceptSpace = TRUE;
        m_cmb3.m_bIgnoreCase = TRUE;
    }

    ~MChooseLangDlg()
    {
        if (m_pAutoComplete)
        {
            m_pAutoComplete->unbind();
            m_pAutoComplete->Release();
            m_pAutoComplete = NULL;
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, GetThreadUILanguage(), TRUE);
        SubclassChildDx(m_cmb3, cmb3);

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
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD lang;
        if (!CheckLangComboBox(hCmb3, lang, LANG_TYPE_1))
            return;

        m_langid = lang;
        EndDialog(IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case psh1:
            m_langid = GetUserDefaultLangID();
            EndDialog(IDOK);
            break;
        case IDCANCEL:
            EndDialog(IDCANCEL);
            break;
        case cmb3:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb3.OnEditChange();
            }
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};
