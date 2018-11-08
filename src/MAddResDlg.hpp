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

#ifndef MZC4_MADDRESDLG_HPP_
#define MZC4_MADDRESDLG_HPP_

#include "MWindowBase.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "resource.h"
#include "DlgInit.h"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& type);
BOOL CheckNameComboBox(HWND hCmb2, MIdOrString& name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& file);
void ReplaceFullWithHalf(LPWSTR pszText);
MStringW GetRisohTemplate(const MIdOrString& type, WORD wLang);

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

    MAddResDlg() : MDialogBase(IDD_ADDRES), m_type(0xFFFF), m_file(NULL)
    {
        m_cmb3.m_bAcceptSpace = TRUE;
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // accept file dropping
        DragAcceptFiles(hwnd, TRUE);

        // for Types
        INT k;
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);

        auto table = g_db.GetTable(L"RESOURCE");
        for (auto& table_entry : table)
        {
            WCHAR sz[MAX_PATH];
            StringCchPrintfW(sz, _countof(sz), L"%s (%lu)",
                             table_entry.name.c_str(), table_entry.value);
            k = ComboBox_AddString(hCmb1, sz);
            if (m_type == WORD(table_entry.value))
            {
                ComboBox_SetCurSel(hCmb1, k);
            }
        }
        table = g_db.GetTable(L"RESOURCE.STRING.TYPE");
        for (auto& table_entry : table)
        {
            k = ComboBox_AddString(hCmb1, table_entry.name.c_str());
            if (m_type == table_entry.name.c_str())
            {
                ComboBox_SetCurSel(hCmb1, k);
            }
        }

        // enable input complete
        SubclassChildDx(m_cmb1, cmb1);
        SubclassChildDx(m_cmb2, cmb2);

        // set 1 to the name if it's a RT_VERSION
        if (m_type == RT_VERSION)
        {
            SetDlgItemInt(hwnd, cmb2, 1, FALSE);
        }

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, GetUserDefaultLangID());
        SubclassChildDx(m_cmb3, cmb3);

        // for file
        if (m_file)
        {
            SetDlgItemTextW(hwnd, edt1, m_file);
        }

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

        return FALSE;
    }

    bool HasSample(const MIdOrString& type, WORD wLang) const
    {
        return !GetRisohTemplate(type, wLang).empty();
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
            WCHAR sz[16];
            GetDlgItemTextW(hwnd, edt1, sz, _countof(sz));
            mstr_trim(sz);

            // clear the name if sz is empty
            if (sz[0] == 0)
                SetDlgItemTextW(hwnd, cmb2, NULL);
        }
        if (type == RT_MESSAGETABLE)
        {
            // edt1 --> sz (trimmed)
            WCHAR sz[16];
            GetDlgItemTextW(hwnd, edt1, sz, _countof(sz));
            mstr_trim(sz);

            // clear the name if sz is empty
            if (sz[0] == 0)
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
        if (!HasSample(type, lang) && !Edt1_CheckFile(hEdt1, file))
            return;     // failure

        // find the language entry by type, name, lang
        if (auto entry = g_res.find(ET_LANG, type, name, lang))
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
        if (file.empty() && HasSample(type, lang))
        {
            bTemplateToAdd = true;     // assume OK

            if (Res_HasNoName(type))
            {
                // if this type has no name, clear the related entries
                g_res.search_and_delete(ET_NAME, type, (WORD)0, lang);
            }

            if (HasSample(type, lang))
            {
                // if the type has sample, then store the template text
                m_strText = GetRisohTemplate(m_type, lang);
            }
            else if (type == L"RISOHTEMPLATE")
            {
                // if the type is RISOHTEMPLATE, then store a blank text
                m_strText = L" ";
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
            // set the file path
            SetDlgItemTextW(hwnd, edt1, szFile);
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
        INT iSel = ComboBox_GetCurSel(m_cmb1);
        TCHAR szText[64];
        if (iSel == CB_ERR || ComboBox_GetLBText(m_cmb1, iSel, szText) == CB_ERR)
            szText[0] = 0;

        // szText --> strIDType (trimmed)
        MString strIDType = szText;
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
        else if (iType != IDTYPE_UNKNOWN)
        {
            type = nRT_;
        }
        else
        {
            type.m_str = strIDType;
        }

        if (HasSample(type, m_lang))
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
        }
        else
        {
            // otherwise the name is non-optional
            SetDlgItemText(hwnd, stc1, NULL);
        }

        // iType (IDTYPE_*) --> prefix
        MString prefix = g_db.GetName(L"RESOURCE.ID.PREFIX", iType);
        if (prefix.empty())
            return;

        // prefix --> m_cmb2
        ComboBox_ResetContent(m_cmb2);
        if (type != RT_STRING && type != RT_MESSAGETABLE)
        {
            auto table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
            for (auto& table_entry : table)
            {
                ComboBox_AddString(m_cmb2, table_entry.name.c_str());
            }
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
        SetDlgItemTextW(hwnd, edt1, file);
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

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MADDRESDLG_HPP_
