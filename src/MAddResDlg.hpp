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
BOOL CheckNameComboBox(ConstantsDB& db, HWND hCmb2, MIdOrString& name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& file);
void ReplaceFullWithHalf(LPWSTR pszText);
MStringW GetRisohTemplate(ConstantsDB& db, const MIdOrString& type, WORD wLang);

//////////////////////////////////////////////////////////////////////////////

class MAddResDlg : public MDialogBase
{
public:
    ResEntries& m_entries;
    ConstantsDB& m_db;
    MIdOrString m_type;
    LPCTSTR m_file;
    MStringW m_strTemplate;
    ResEntry m_entry_copy;
    MComboBoxAutoComplete m_cmb1;
    MComboBoxAutoComplete m_cmb2;
    MComboBoxAutoComplete m_cmb3;

    MAddResDlg(ResEntries& entries, ConstantsDB& db)
        : MDialogBase(IDD_ADDRES), m_entries(entries), m_db(db),
          m_type(0xFFFF), m_file(NULL)
    {
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
            StringCchPrintfW(sz, _countof(sz), L"%s (%lu)", table[i].name.c_str(), table[i].value);
            k = ComboBox_AddString(hCmb1, sz);
            if (m_type == WORD(table[i].value))
            {
                ComboBox_SetCurSel(hCmb1, k);
            }
        }
        k = ComboBox_AddString(hCmb1, TEXT("WAVE"));
        if (m_type == TEXT("WAVE"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("PNG"));
        if (m_type == TEXT("PNG"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("IMAGE"));
        if (m_type == TEXT("IMAGE"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("GIF"));
        if (m_type == TEXT("GIF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("JPEG"));
        if (m_type == TEXT("JPEG"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("TIFF"));
        if (m_type == TEXT("TIFF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("AVI"));
        if (m_type == TEXT("AVI"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("EMF"));
        if (m_type == TEXT("EMF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("ENHMETAFILE"));
        if (m_type == TEXT("ENHMETAFILE"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("WMF"));
        if (m_type == TEXT("WMF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        k = ComboBox_AddString(hCmb1, TEXT("RISOHTEMPLATE"));
        if (m_type == TEXT("RISOHTEMPLATE"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }
        SubclassChildDx(m_cmb1, cmb1);
        SubclassChildDx(m_cmb2, cmb2);

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

        CenterWindowDx();

        if (m_type == 0xFFFF)
        {
            SetFocus(GetDlgItem(hwnd, cmb1));
        }
        else
        {
            SetFocus(GetDlgItem(hwnd, cmb2));
        }

        OnCmb1(hwnd);

        return FALSE;
    }

    void OnOK(HWND hwnd)
    {
        MIdOrString type;

        const ConstantsDB::TableType& table = m_db.GetTable(L"RESOURCE");
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

        if (type == RT_STRING || type == RT_MESSAGETABLE)
        {
            WCHAR sz[16];
            GetDlgItemTextW(hwnd, edt1, sz, _countof(sz));
            mstr_trim(sz);
            if (sz[0] == 0)
                SetDlgItemTextW(hwnd, cmb2, L"1");
        }

        if (type == RT_VERSION)
        {
            SetDlgItemTextW(hwnd, cmb2, L"1");
        }

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        MIdOrString name;
        if (!Res_HasNoName(type) && !CheckNameComboBox(m_db, hCmb2, name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD lang;
        if (!CheckLangComboBox(hCmb3, lang))
            return;

        std::wstring file;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Res_HasSample(type) && !Edt1_CheckFile(hEdt1, file))
            return;

        BOOL bOverwrite = FALSE;
        INT iEntry = Res_Find(m_entries, type, name, lang, FALSE);
        if (iEntry != -1)
        {
            if (file.empty() && Res_HasSample(type))
            {
                ErrorBoxDx(IDS_ALREADYEXISTS);
                return;
            }

            INT id = MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL);
            switch (id)
            {
            case IDYES:
                bOverwrite = TRUE;
                break;
            case IDNO:
            case IDCANCEL:
                return;
            }
        }

        BOOL bOK = FALSE;
        BOOL bAdded = FALSE;
        if (file.empty() && Res_HasSample(type))
        {
            bOK = TRUE;
            if (Res_HasNoName(type))
            {
                Res_DeleteNames(m_entries, type, lang);
            }

            MByteStreamEx stream;
            if (type == RT_ACCELERATOR || type == RT_DIALOG ||
                type == RT_MENU || type == RT_STRING ||
                type == RT_VERSION || type == RT_HTML ||
                type == RT_MANIFEST || type == RT_MESSAGETABLE ||
                type == RT_DLGINIT)
            {
                m_strTemplate = GetRisohTemplate(m_db, type, lang);
            }
            else if (type == L"RISOHTEMPLATE")
            {
                m_strTemplate = L" ";
            }
            else
            {
                bOK = FALSE;
            }

            if (type == RT_STRING || type == RT_MESSAGETABLE)
            {
                name = 1;
            }

            if (bOK)
            {
                Res_AddEntry(m_entries, type, name, lang, m_strTemplate, stream.data(), FALSE);
                ResEntry entry(type, name, lang, m_strTemplate);
                m_entry_copy = entry;
                bAdded = TRUE;
            }
        }

        MByteStreamEx bs;
        if (!bOK && !bs.LoadFromFile(file.c_str()))
        {
            if (bOverwrite)
                ErrorBoxDx(IDS_CANNOTREPLACE);
            else
                ErrorBoxDx(IDS_CANNOTADDRES);
            return;
        }
        if (!bAdded)
        {
            Res_AddEntry(m_entries, type, name, lang, L"", bs.data(), bOverwrite);
            ResEntry entry(type, name, lang);
            m_entry_copy = entry;
        }

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
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ALLFILES));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = _countof(szFile);
        ofn.lpstrTitle = LoadStringDx(IDS_ADDRES);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"bin";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, szFile);
        }
    }

    void OnPsh2(HWND hwnd)
    {
        SendMessage(GetParent(hwnd), WM_COMMAND, ID_IDLIST, 0);
    }

    void OnCmb1(HWND hwnd)
    {
        MIdOrString type;

        INT iSel = ComboBox_GetCurSel(m_cmb1);
        TCHAR szText[64];
        if (iSel == CB_ERR || ComboBox_GetLBText(m_cmb1, iSel, szText) == CB_ERR)
            szText[0] = 0;

        MString strIDType = szText;
        mstr_trim(strIDType);
        size_t k = strIDType.find(L" (");
        if (k != MString::npos)
        {
            strIDType = strIDType.substr(0, k);
        }

        WORD nRT_ = (WORD)m_db.GetValue(L"RESOURCE", strIDType);
        INT iType = m_db.IDTypeFromResType(nRT_);
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

        if (Res_HasSample(type))
        {
            SetDlgItemText(hwnd, stc2, LoadStringDx(IDS_OPTIONAL));
        }
        else
        {
            SetDlgItemText(hwnd, stc2, NULL);
        }

        if (type == RT_STRING || type == RT_MESSAGETABLE || type == RT_VERSION)
        {
            SetDlgItemText(hwnd, stc1, LoadStringDx(IDS_OPTIONAL));
        }
        else
        {
            SetDlgItemText(hwnd, stc1, NULL);
        }

        MString prefix = m_db.GetName(L"RESOURCE.ID.PREFIX", iType);
        if (prefix.empty())
            return;

        ComboBox_ResetContent(m_cmb2);
        if (type != RT_STRING && type != RT_MESSAGETABLE)
        {
            ConstantsDB::TableType table;
            table = m_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
            for (size_t i = 0; i < table.size(); ++i)
            {
                ComboBox_AddString(m_cmb2, table[i].name.c_str());
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
            EndDialog(IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        case psh2:
            OnPsh2(hwnd);
            break;
        case cmb1:
            if (codeNotify == CBN_SELCHANGE)
            {
                OnCmb1(hwnd);
            }
            else if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb1.OnEditChange();
                OnCmb1(hwnd);
            }
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
