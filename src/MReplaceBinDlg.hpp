// MReplaceBinDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MREPLACEBINDLG_HPP_
#define MZC4_MREPLACEBINDLG_HPP_

//////////////////////////////////////////////////////////////////////////////

#include "RisohEditor.hpp"
#include "ConstantsDB.hpp"

void Cmb3_InsertLangItemsAndSelectLang(HWND hCmb3, LANGID langid);
BOOL Cmb1_CheckType(HWND hCmb1, ID_OR_STRING& Type);
BOOL Cmb2_CheckName(HWND hCmb2, ID_OR_STRING& Name);
BOOL Cmb3_CheckLang(HWND hCmb3, WORD& Lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& File);

//////////////////////////////////////////////////////////////////////////////

struct MReplaceBinDlg : MDialogBase
{
    ResEntries& m_Entries;
    ResEntry& m_Entry;
    ConstantsDB& m_db;

    MReplaceBinDlg(ResEntries& Entries, ResEntry& Entry, ConstantsDB& db)
        : MDialogBase(IDD_REPLACERES), m_Entries(Entries), m_Entry(Entry), m_db(db)
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
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& Table = m_db.GetTable(L"RESOURCE");
        for (size_t i = 0; i < Table.size(); ++i)
        {
            WCHAR sz[MAX_PATH];
            wsprintfW(sz, L"%s (%lu)", Table[i].name.c_str(), Table[i].value);
            INT k = ComboBox_AddString(hCmb1, sz);
            if (m_Entry.type == WORD(Table[i].value))
            {
                ComboBox_SetCurSel(hCmb1, k);
            }
        }

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (m_Entry.name.is_str())
        {
            SetWindowTextW(hCmb2, m_Entry.name.m_Str.c_str());
        }
        else
        {
            SetDlgItemInt(hwnd, cmb2, m_Entry.name.m_ID, FALSE);
        }

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        Cmb3_InsertLangItemsAndSelectLang(hCmb3, m_Entry.lang);

        return TRUE;
    }

    void OnPsh1(HWND hwnd)
    {
        WCHAR File[MAX_PATH];
        GetDlgItemText(hwnd, edt1, File, _countof(File));

        std::wstring strFile = File;
        str_trim(strFile);
        lstrcpynW(File, strFile.c_str(), _countof(File));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ALLFILES));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx2(IDS_REPLACERES);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"bin";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, File);
        }
    }

    void OnOK(HWND hwnd)
    {
        ID_OR_STRING Type;
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& Table = m_db.GetTable(L"RESOURCE");
        INT iType = ComboBox_GetCurSel(hCmb1);
        if (iType != CB_ERR && iType < INT(Table.size()))
        {
            Type = WORD(Table[iType].value);
        }
        else
        {
            if (!Cmb1_CheckType(hCmb1, Type))
                return;
        }

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        ID_OR_STRING Name;
        if (!Cmb2_CheckName(hCmb2, Name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD Lang;
        if (!Cmb3_CheckLang(hCmb3, Lang))
            return;

        std::wstring File;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Edt1_CheckFile(hEdt1, File))
            return;

        if (!DoReplaceBin(hwnd, m_Entries, Type, Name, Lang, File))
        {
            ErrorBoxDx(IDS_CANNOTREPLACE);
            return;
        }

        EndDialog(hwnd, IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR File[MAX_PATH];
        DragQueryFileW(hdrop, 0, File, _countof(File));
        SetDlgItemTextW(hwnd, edt1, File);
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MREPLACEBINDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
