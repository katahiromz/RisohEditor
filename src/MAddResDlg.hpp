// MAddResDlg
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

#ifndef MZC4_MADDRESDLG_HPP_
#define MZC4_MADDRESDLG_HPP_

#include "RisohEditor.hpp"
#include "ConstantsDB.hpp"
#include "Samples.hpp"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& type);
BOOL CheckNameComboBox(ConstantsDB& db, HWND hCmb2, MIdOrString& name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& file);

//////////////////////////////////////////////////////////////////////////////

class MAddResDlg : public MDialogBase
{
public:
    ResEntries& m_entries;
    ConstantsDB& m_db;
    MIdOrString m_type;
    LPCTSTR m_file;
    ResEntry m_entry;

    MAddResDlg(ResEntries& Entries, ConstantsDB& db)
        : MDialogBase(IDD_ADDRES), m_entries(Entries), m_db(db),
          m_type(0xFFFF), m_file(NULL)
    {
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
            if (m_type == WORD(Table[i].value))
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
        k = ComboBox_AddString(hCmb1, TEXT("WMF"));
        if (m_type == TEXT("WMF"))
        {
            ComboBox_SetCurSel(hCmb1, k);
        }

        if (m_type == RT_VERSION)
        {
            SetDlgItemInt(hwnd, cmb2, 1, FALSE);
        }

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, GetUserDefaultLangID());

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
        return FALSE;
    }

    void OnOK(HWND hwnd)
    {
        MIdOrString type;
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& Table = m_db.GetTable(L"RESOURCE");
        INT iType = ComboBox_GetCurSel(hCmb1);
        if (iType != CB_ERR && iType < INT(Table.size()))
        {
            type = WORD(Table[iType].value);
        }
        else
        {
            if (!CheckTypeComboBox(hCmb1, type))
            {
                return;
            }
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

        BOOL Overwrite = FALSE;
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
                Overwrite = TRUE;
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
            if (type == RT_ACCELERATOR)
            {
                DWORD Size;
                const BYTE *pb = GetAccelSample(Size);
                stream.assign(pb, Size);
            }
            else if (type == RT_DIALOG)
            {
                DWORD Size;
                const BYTE *pb = GetDialogSample(Size);
                stream.assign(pb, Size);
            }
            else if (type == RT_MENU)
            {
                DWORD Size;
                const BYTE *pb = GetMenuSample(Size);
                stream.assign(pb, Size);
            }
            else if (type == RT_STRING)
            {
                DWORD Size;
                const BYTE *pb = GetStringSample(Size);
                stream.assign(pb, Size);
                name = 1;
            }
            else if (type == RT_VERSION)
            {
                DWORD Size;
                const BYTE *pb = GetVersionSample(Size);
                stream.assign(pb, Size);
            }
            else if (type == RT_HTML)
            {
                DWORD Size;
                const BYTE *pb = GetHtmlSample(Size);
                stream.assign(pb, Size);
            }
            else if (type == RT_MANIFEST)
            {
                DWORD Size;
                const BYTE *pb = GetManifestSample(Size);
                stream.assign(pb, Size);
            }
            else
            {
                bOK = FALSE;
            }

            if (bOK)
            {
                Res_AddEntry(m_entries, type, name, lang, stream.data(), FALSE);
                ResEntry entry(type, name, lang);
                m_entry = entry;
                bAdded = TRUE;
            }
        }

        MByteStreamEx bs;
        if (!bOK && !bs.LoadFromFile(file.c_str()))
        {
            if (Overwrite)
                ErrorBoxDx(IDS_CANNOTREPLACE);
            else
                ErrorBoxDx(IDS_CANNOTADDRES);
            return;
        }
        if (!bAdded)
        {
            Res_AddEntry(m_entries, type, name, lang, bs.data(), Overwrite);
            ResEntry entry(type, name, lang);
            m_entry = entry;
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
        case cmb1:
            if (codeNotify == CBN_SELCHANGE)
            {
                
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
