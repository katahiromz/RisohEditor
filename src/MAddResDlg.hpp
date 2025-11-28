// MAddResDlg.hpp --- "Add Resource" Dialog
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
#include "DlgInit.h"
#include "MLangAutoComplete.hpp"
#include "Common.hpp"

//////////////////////////////////////////////////////////////////////////////

class MAddResDlg : public MDialogBase
{
public:
    MIdOrString m_type;
    MIdOrString m_name;
    WORD m_lang;
    MStringW m_strTemplate;
    LPCTSTR m_file;
    MStringW m_strText;
    MComboBoxAutoComplete m_cmb1;
    MComboBoxAutoComplete m_cmb2;
    MComboBoxAutoComplete m_cmb3;
    MLangAutoComplete *m_pAutoComplete;

    MAddResDlg()
        : MDialogBase(IDD_ADDRES)
        , m_type(0xFFFF)
        , m_file(NULL)
        , m_pAutoComplete(new MLangAutoComplete())
    {
        m_cmb3.m_bAcceptSpace = TRUE;
        m_cmb3.m_bIgnoreCase = TRUE;
    }

    ~MAddResDlg()
    {
        m_pAutoComplete->unbind();
        m_pAutoComplete->Release();
        m_pAutoComplete = NULL;
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // accept file dropping
        DragAcceptFiles(hwnd, TRUE);

        // for Types
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        InitResTypeComboBox(hCmb1, m_type);

        // enable input complete
        SubclassChildDx(m_cmb1, cmb1);
        SubclassChildDx(m_cmb2, cmb2);

        InitComboBoxPlaceholder(m_cmb2, IDS_INTEGERORIDENTIFIER);

        // set 1 to the name if it's a RT_VERSION
        if (m_type == RT_VERSION)
        {
            SetDlgItemInt(hwnd, cmb2, 1, FALSE);
        }

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, GetDefaultResLanguage());
        SubclassChildDx(m_cmb3, cmb3);

        // for file
        if (m_file)
        {
            SetDlgItemTextW(hwnd, edt1, m_file);
            DoFile(hwnd, m_file);
        }
        FileSystemAutoComplete(GetDlgItem(hwnd, edt1));

        // do centering the dialog
        CenterWindowDx();

        // move focus to help the user input
        if (m_type == 0xFFFF)
        {
            SetFocus(GetDlgItem(hwnd, cmb1));
        }
        else
        {
            SetFocus(GetDlgItem(hwnd, cmb2));
        }

        // select the type
        OnCmb1(hwnd);

        // auto complete
        COMBOBOXINFO info = { sizeof(info) };
        GetComboBoxInfo(m_cmb3, &info);
        HWND hwndEdit = info.hwndItem;
        m_pAutoComplete->bind(hwndEdit);

        return FALSE;
    }

    bool HasSample(const MIdOrString& type, const MIdOrString& name, WORD wLang) const
    {
        return !GetRisohTemplate(type, name, wLang).empty();
    }

    void OnOK(HWND hwnd)
    {
        MIdOrString type;

        // cmb1 --> (iType, type)
        auto table = g_db.GetTable(L"RESOURCE");
        INT iType = ComboBox_GetCurSel(m_cmb1);
        if (iType != CB_ERR && iType < INT(table.size()))
        {
            type = WORD(table[iType].value);
        }
        else
        {
            if (!CheckTypeComboBox(m_cmb1, type))
            {
                return;
            }
        }

        if (type == RT_STRING)
        {
            // edt1 --> sz (trimmed)
            MStringW sz = GetDlgItemText(edt1);
            mstr_trim(sz);

            // clear the name if sz is empty
            if (sz.empty())
                SetDlgItemTextW(hwnd, cmb2, NULL);
        }
        if (type == RT_MESSAGETABLE)
        {
            // edt1 --> sz (trimmed)
            MStringW sz = GetDlgItemText(edt1);
            mstr_trim(sz);

            // clear the name if sz is empty
            if (sz.empty())
                SetDlgItemTextW(hwnd, cmb2, NULL);
        }

        // if RT_VERSION, the name is one
        if (type == RT_VERSION)
        {
            SetDlgItemTextW(hwnd, cmb2, L"1");
        }

        // check the name combobox cmb2
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        MIdOrString name;
        if (!Res_HasNoName(type) && !CheckNameComboBox(hCmb2, name))
            return;     // failure

        // check the language combobox cmb3
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD lang;
        if (!CheckLangComboBox(hCmb3, lang))
            return;     // failure

        // get the file path from edt1
        std::wstring file;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);

        // if there is no sample for the type, check if the file path exists
        if (!Edt1_CheckFile(hEdt1, file) && !HasSample(type, name, lang))
        {
            Edit_SetSel(hEdt1, 0, -1);  // select all
            SetFocus(hEdt1);    // set focus
            ErrorBoxDx(IDS_FILENOTFOUND);
            return;     // failure
        }

        // find the language entry by type, name, lang
        if (g_res.find(ET_LANG, type, name, lang))
        {
            // query overwriting
            INT id = MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL);
            switch (id)
            {
            case IDYES:
                // delete the overlapped entries
                g_res.search_and_delete(ET_LANG, type, name, lang);
                break;

            case IDNO:
            case IDCANCEL:
                return;     // cancelled
            }
        }

        bool bTemplateToAdd = false;
        bool bAdded = false;

        // if there is sample and no file was specified, then
        if (file.empty() && HasSample(type, name, lang))
        {
            bTemplateToAdd = true;     // assume OK

            if (Res_HasNoName(type))
            {
                // if this type has no name, clear the related entries
                g_res.search_and_delete(ET_NAME, type, BAD_NAME, lang);
            }

            if (HasSample(type, name, lang))
            {
                // if the type has sample, then store the template text
                m_strText = GetRisohTemplate(type, name, lang);
            }
            else
            {
                // otherwise it's not OK
                bTemplateToAdd = false;
            }

            // set one to the name if it's RT_STRING or RT_MESSAGETABLE
            if (type == RT_STRING || type == RT_MESSAGETABLE)
            {
                name = 1;   // it will be fixed later
            }

            // TEXTINCLUDE should be neutral
            if (type == L"TEXTINCLUDE")
            {
                lang = 0;
            }

            if (bTemplateToAdd)    // it's OK
            {
                // add an empty entry (data will be set later)
                g_res.add_lang_entry(type, name, lang);
                bAdded = true;

                // store the results
                m_type = type;
                m_name = name;
                m_lang = lang;
                m_strTemplate = m_strText;  // the template text
            }
        }

        // try to load the file if not OK
        MByteStreamEx bs;
        if (!bTemplateToAdd && !bs.LoadFromFile(file.c_str()))
        {
            // error
            ErrorBoxDx(IDS_CANNOTADDRES);
            return;
        }

        // if not added yet, then
        if (!bAdded)
        {
            // add the data from the file
            g_res.add_lang_entry(type, name, lang, bs.data());

            // store the results
            m_type = type;
            m_name = name;
            m_lang = lang;
            m_strTemplate.clear();  // no template text
        }

        // finish the dialog
        EndDialog(IDOK);
    }

    void OnPsh1(HWND hwnd)  // "browse"
    {
        // get the text (trimmed)
        MStringW strFile = GetDlgItemText(edt1);
        mstr_trim(strFile);

        // strFile --> szFile
        WCHAR szFile[MAX_PATH];
        StringCchCopyW(szFile, _countof(szFile), strFile.c_str());

        // initialize OPENFILENAME structure
        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ALLFILES));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = _countof(szFile);
        ofn.lpstrTitle = LoadStringDx(IDS_ADDRES);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
                    OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"bin";   // the default extension
        if (GetOpenFileNameW(&ofn)) // "OK" button was pressed
        {
            DoFile(hwnd, szFile);
        }
    }

    void DoFile(HWND hwnd, LPCWSTR szFile)
    {
        // set the file path
        SetDlgItemTextW(hwnd, edt1, szFile);

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
        // show the ID list window
        SendMessage(GetParent(hwnd), WM_COMMAND, ID_IDLIST, 0);
    }

    void OnCmb1(HWND hwnd)
    {
        // get the text of combobox cmb1
        MString strText;
        INT iItem = ComboBox_GetCurSel(m_cmb1);
        if (iItem == CB_ERR)
        {
            strText = GetComboBoxText(m_cmb1);
        }
        else
        {
            strText = GetComboBoxLBText(m_cmb1, iItem);
        }

        // strText --> strIDType (trimmed)
        MString strIDType = strText;
        mstr_trim(strIDType);

        // cut off the text from " (" to end
        size_t k = strIDType.find(L" (");
        if (k != MString::npos)
        {
            strIDType = strIDType.substr(0, k);
        }

        // the resource type (RT_*) --> (type, iType)
        MIdOrString type;
        WORD nRT_ = (WORD)g_db.GetValue(L"RESOURCE", strIDType);
        INT iType = g_db.IDTypeFromResType(nRT_);
        if (nRT_ != 0)
        {
            type = nRT_;
            if (iType == IDTYPE_UNKNOWN)
            {
                iType = IDTYPE_RESOURCE;
            }
        }
        else if (iType != IDTYPE_UNKNOWN && nRT_ != 0)
        {
            type = nRT_;
        }
        else
        {
            type.m_str = std::move(strIDType);
        }

        if (HasSample(type, m_name, m_lang))
        {
            // if there is a sample for this type, the file path is optional
            SetDlgItemText(hwnd, stc2, LoadStringDx(IDS_OPTIONAL));
        }
        else
        {
            // the file path is non-optional
            SetDlgItemText(hwnd, stc2, NULL);
        }

        if (type == RT_STRING || type == RT_MESSAGETABLE || type == RT_VERSION)
        {
            // the name is optional if RT_STRING, RT_MESSAGETABLE or RT_VERSION
            SetDlgItemText(hwnd, stc1, LoadStringDx(IDS_OPTIONAL));
            InitComboBoxPlaceholder(m_cmb2, IDS_NOTEXT);
        }
        else
        {
            // otherwise the name is non-optional
            SetDlgItemText(hwnd, stc1, NULL);
            InitComboBoxPlaceholder(m_cmb2, IDS_INTEGERORIDENTIFIER);
        }

        // iType (IDTYPE_*) --> prefix
        MString prefix = g_db.GetName(L"RESOURCE.ID.PREFIX", iType);
        if (prefix.empty())
            return;

        // prefix --> m_cmb2
        MString strCmb2Text = GetComboBoxText(m_cmb2);
        ComboBox_ResetContent(m_cmb2);
        if (type != RT_STRING && type != RT_MESSAGETABLE)
        {
            auto table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
            for (auto& table_entry : table)
            {
                ComboBox_AddString(m_cmb2, table_entry.name.c_str());
            }
        }
        ComboBox_SetText(m_cmb2, strCmb2Text.c_str());
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;

        case IDCANCEL:
            // cancel the dialog
            EndDialog(IDCANCEL);
            break;

        case psh1:      // browse
            OnPsh1(hwnd);
            break;

        case psh2:      // show the resource ID list
            OnPsh2(hwnd);
            break;

        case cmb1:
            if (codeNotify == CBN_SELCHANGE)
            {
                // selection of cmb1 was changed
                OnCmb1(hwnd);
            }
            else if (codeNotify == CBN_EDITCHANGE)
            {
                // the text of cmb1 was changed
                m_cmb1.OnEditChange();  // input completion
                OnCmb1(hwnd);
            }
            break;

        case cmb2:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb2.OnEditChange();  // input completion
            }
            break;

        case cmb3:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb3.OnEditChange();  // input completion
            }
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        // file(s) has dropped
        WCHAR file[MAX_PATH];
        DragQueryFileW(hdrop, 0, file, _countof(file));
        DoFile(hwnd, file);
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
};
