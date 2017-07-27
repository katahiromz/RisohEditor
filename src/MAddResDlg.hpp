// MAddResDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MADDRESDLG_HPP_
#define MZC4_MADDRESDLG_HPP_

#include "RisohEditor.hpp"
#include "ConstantsDB.hpp"
#include "Samples.hpp"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckTypeComboBox(HWND hCmb1, ID_OR_STRING& Type);
BOOL CheckNameComboBox(HWND hCmb2, ID_OR_STRING& Name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& Lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& File);

//////////////////////////////////////////////////////////////////////////////

class MAddResDlg : public MDialogBase
{
public:
    ResEntries& m_Entries;
    ConstantsDB& m_db;
    HWND m_hTreeView;

    MAddResDlg(ResEntries& Entries, ConstantsDB& db, HWND hTreeView)
        : MDialogBase(IDD_ADDRES), m_Entries(Entries), m_db(db),
          m_hTreeView(hTreeView)
    {
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
            ComboBox_AddString(hCmb1, sz);
        }

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, GetUserDefaultLangID());

        return TRUE;
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
        if (!Res_HasNoName(Type) && !CheckNameComboBox(hCmb2, Name))
            return;

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        WORD Lang;
        if (!CheckLangComboBox(hCmb3, Lang))
            return;

        std::wstring File;
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        if (!Res_HasSample(Type) && !Edt1_CheckFile(hEdt1, File))
            return;

        BOOL Overwrite = FALSE;
        INT iEntry = Res_Find(m_Entries, Type, Name, Lang);
        if (iEntry != -1)
        {
            if (File.empty() && Res_HasSample(Type))
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

        BOOL bOK = TRUE;
        if (File.empty() && Res_HasSample(Type))
        {
            if (Res_HasNoName(Type))
            {
                Res_DeleteNames(m_Entries, Type, Lang);
            }

            MByteStreamEx stream;
            if (Type == RT_ACCELERATOR)
            {
                DWORD Size;
                const BYTE *pb = GetAccelSample(Size);
                stream.assign(pb, Size);
            }
            else if (Type == RT_DIALOG)
            {
                DWORD Size;
                const BYTE *pb = GetDialogSample(Size);
                stream.assign(pb, Size);
            }
            else if (Type == RT_MENU)
            {
                DWORD Size;
                const BYTE *pb = GetMenuSample(Size);
                stream.assign(pb, Size);
            }
            else if (Type == RT_STRING)
            {
                DWORD Size;
                const BYTE *pb = GetStringSample(Size);
                stream.assign(pb, Size);
                Name = 1;
            }
            else if (Type == RT_VERSION)
            {
                DWORD Size;
                const BYTE *pb = GetVersionSample(Size);
                stream.assign(pb, Size);
            }
            else if (Type == RT_HTML)
            {
                DWORD Size;
                const BYTE *pb = GetHtmlSample(Size);
                stream.assign(pb, Size);
            }
            else if (Type == RT_MANIFEST)
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
                Res_AddEntry(m_Entries, Type, Name, Lang, stream.data(), FALSE);

                TV_RefreshInfo(m_hTreeView, m_Entries, FALSE, FALSE);

                ResEntry entry(Type, Name, Lang);
                TV_SelectEntry(m_hTreeView, m_Entries, entry);
            }
        }

        if (!bOK)
        {
            if (Overwrite)
            {
                if (!DoReplaceBin(hwnd, m_Entries, Type, Name, Lang, File))
                {
                    ErrorBoxDx(IDS_CANNOTREPLACE);
                    return;
                }
            }
            else
            {
                if (!DoAddBin(hwnd, m_Entries, Type, Name, Lang, File))
                {
                    ErrorBoxDx(IDS_CANNOTADDRES);
                    return;
                }
            }
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
        WCHAR File[MAX_PATH];
        DragQueryFileW(hdrop, 0, File, _countof(File));
        SetDlgItemTextW(hwnd, edt1, File);
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

//////////////////////////////////////////////////////////////////////////////
