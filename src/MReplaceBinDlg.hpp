// MReplaceBinDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MREPLACEBINDLG_HPP_
#define MZC4_MREPLACEBINDLG_HPP_

//////////////////////////////////////////////////////////////////////////////

#include "RisohEditor.hpp"
#include "ConstantsDB.hpp"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckTypeComboBox(HWND hCmb1, ID_OR_STRING& Type);
BOOL CheckNameComboBox(HWND hCmb2, ID_OR_STRING& Name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& Lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& File);

//////////////////////////////////////////////////////////////////////////////

class MReplaceBinDlg : public MDialogBase
{
public:
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
        INT k;
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        const ConstantsDB::TableType& Table = m_db.GetTable(L"RESOURCE");
        for (size_t i = 0; i < Table.size(); ++i)
        {
            WCHAR sz[MAX_PATH];
            wsprintfW(sz, L"%s (%lu)", Table[i].name.c_str(), Table[i].value);
            k = ComboBox_AddString(hCmb1, sz);
            if (m_Entry.type == WORD(Table[i].value))
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

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (m_Entry.name.is_str())
        {
            ::SetWindowTextW(hCmb2, m_Entry.name.m_Str.c_str());
        }
        else
        {
            ::SetDlgItemInt(hwnd, cmb2, m_Entry.name.m_ID, FALSE);
        }
        ::EnableWindow(hCmb2, FALSE);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, m_Entry.lang);
        ::EnableWindow(hCmb3, FALSE);

        CenterWindowDx();
        return TRUE;
    }

    void OnPsh1(HWND hwnd)
    {
        MStringW strFile = GetDlgItemText(edt1);
        mstr_trim(strFile);

        WCHAR szFile[MAX_PATH];
        lstrcpyn(szFile, strFile.c_str(), _countof(szFile));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ALLFILES));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = _countof(szFile);
        ofn.lpstrTitle = LoadStringDx(IDS_REPLACERES);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"bin";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, szFile);
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
            if (!CheckTypeComboBox(hCmb1, Type))
                return;
        }

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        ID_OR_STRING Name;
        if (!CheckNameComboBox(hCmb2, Name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD Lang;
        if (!CheckLangComboBox(hCmb3, Lang))
            return;

        std::wstring File;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Edt1_CheckFile(hEdt1, File))
            return;

        MByteStreamEx bs;
        if (!bs.LoadFromFile(File.c_str()))
        {
            ErrorBoxDx(IDS_CANNOTREPLACE);
            return;
        }

        Res_AddEntry(m_Entries, Type, Name, Lang, bs.data(), TRUE);

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
