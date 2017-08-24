// MAddCursorDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MADDCURSORDLG_HPP_
#define MZC4_MADDCURSORDLG_HPP_

#include "RisohEditor.hpp"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckNameComboBox(HWND hCmb2, ID_OR_STRING& Name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& Lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& File);

//////////////////////////////////////////////////////////////////////////////

class MAddCursorDlg : public MDialogBase
{
public:
    ResEntries& m_Entries;
    LPCWSTR   m_File;
    HCURSOR   m_hCursor;

    MAddCursorDlg(ResEntries& Entries)
        : MDialogBase(IDD_ADDCURSOR), m_Entries(Entries), m_File(NULL)
    {
        m_hCursor = NULL;
    }

    ~MAddCursorDlg()
    {
        DestroyCursor(m_hCursor);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemTextW(hwnd, edt1, m_File);
        if (m_hCursor)
            DestroyCursor(m_hCursor);
        m_hCursor = LoadCursorFromFile(m_File);
        SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(m_hCursor));

        DragAcceptFiles(hwnd, TRUE);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, GetUserDefaultLangID());

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        ID_OR_STRING Type = RT_GROUP_CURSOR;

        ID_OR_STRING Name;
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
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

        BOOL bAni = FALSE;
        size_t ich = File.find(L'.');
        if (ich != std::wstring::npos && lstrcmpiW(&File[ich], L".ani") == 0)
            bAni = TRUE;

        BOOL Overwrite = FALSE;
        INT iEntry = Res_Find(m_Entries, (bAni ? RT_ANICURSOR : RT_GROUP_CURSOR), Name, Lang);
        if (iEntry != -1)
        {
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

        if (bAni)
        {
            MByteStream bs;
            if (!bs.LoadFromFile(File.c_str()) ||
                !Res_AddEntry(m_Entries, RT_ANICURSOR, Name, Lang, bs.data(), Overwrite))
            {
                if (Overwrite)
                    ErrorBoxDx(IDS_CANTREPLACECUR);
                else
                    ErrorBoxDx(IDS_CANNOTADDCUR);
                return;
            }
        }
        else
        {
            if (!Res_AddGroupCursor(m_Entries, Name, Lang, File, Overwrite))
            {
                if (Overwrite)
                    ErrorBoxDx(IDS_CANTREPLACECUR);
                else
                    ErrorBoxDx(IDS_CANNOTADDCUR);
                return;
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
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_CURFILTER));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = _countof(szFile);
        ofn.lpstrTitle = LoadStringDx(IDS_ADDCUR);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"cur";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, szFile);
            if (m_hCursor)
                DestroyCursor(m_hCursor);
            m_hCursor = LoadCursorFromFile(szFile);
            SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(m_hCursor));
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
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR File[MAX_PATH];
        DragQueryFileW(hdrop, 0, File, _countof(File));
        SetDlgItemTextW(hwnd, edt1, File);

        if (m_hCursor)
            DestroyCursor(m_hCursor);
        m_hCursor = LoadCursorFromFile(File);
        SendDlgItemMessage(hwnd, ico1, STM_SETIMAGE, IMAGE_CURSOR, LPARAM(m_hCursor));
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
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MADDCURSORDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
