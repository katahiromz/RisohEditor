// MReplaceCursorDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MREPLACECURSORDLG_HPP_
#define MZC4_MREPLACECURSORDLG_HPP_

#include "RisohEditor.hpp"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckNameComboBox(ConstantsDB& db, HWND hCmb2, MIdOrString& Name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& Lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& File);
void InitCommandComboBox(HWND hCmb, ConstantsDB& db, MString strCommand);

//////////////////////////////////////////////////////////////////////////////

class MReplaceCursorDlg : public MDialogBase
{
protected:
    HCURSOR   m_hCursor;
public:
    ResEntries& m_Entries;
    ResEntry& m_Entry;
    ConstantsDB& m_db;

    MReplaceCursorDlg(ConstantsDB& db, ResEntries& Entries, ResEntry& Entry) :
        MDialogBase(IDD_REPLACECUR), m_Entries(Entries), m_Entry(Entry),
        m_db(db)
    {
        m_hCursor = NULL;
    }

    ~MReplaceCursorDlg()
    {
        DestroyCursor(m_hCursor);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        DragAcceptFiles(hwnd, TRUE);

        // for Name
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        InitCommandComboBox(hCmb2, m_db, m_Entry.name.str());
        ::EnableWindow(hCmb2, FALSE);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, m_Entry.lang);
        ::EnableWindow(hCmb3, FALSE);

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        MIdOrString Type = m_Entry.type;

        MIdOrString Name;
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (!CheckNameComboBox(m_db, hCmb2, Name))
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

        if (bAni)
        {
            MByteStream bs;
            if (!bs.LoadFromFile(File.c_str()) ||
                !Res_AddEntry(m_Entries, RT_ANICURSOR, Name, Lang, bs.data(), TRUE))
            {
                ErrorBoxDx(IDS_CANTREPLACECUR);
                return;
            }
        }
        else
        {
            if (!Res_AddGroupCursor(m_Entries, Name, Lang, File, TRUE))
            {
                ErrorBoxDx(IDS_CANTREPLACECUR);
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
        ofn.lpstrTitle = LoadStringDx(IDS_REPLACECUR);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        if (m_Entry.type == RT_ANICURSOR)
        {
            ofn.nFilterIndex = 2;
            ofn.lpstrDefExt = L"ani";
        }
        else
        {
            ofn.nFilterIndex = 1;
            ofn.lpstrDefExt = L"cur";
        }
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
        DragFinish(hdrop);
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

#endif  // ndef MZC4_MREPLACECURSORDLG_HPP_
