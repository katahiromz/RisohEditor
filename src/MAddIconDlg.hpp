// MAddIconDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MADDICONDLG_HPP_
#define MZC4_MADDICONDLG_HPP_

#include "RisohEditor.hpp"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckNameComboBox(HWND hCmb2, ID_OR_STRING& Name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& Lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& File);

//////////////////////////////////////////////////////////////////////////////

class MAddIconDlg : public MDialogBase
{
public:
    ResEntries& m_Entries;
    LPCWSTR File;
    HICON   m_hIcon;

    MAddIconDlg(ResEntries& Entries)
        : MDialogBase(IDD_ADDICON), m_Entries(Entries), File(NULL), m_hIcon(NULL)
    {
    }

    ~MAddIconDlg()
    {
        DestroyIcon(m_hIcon);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemTextW(hwnd, edt1, File);
        if (m_hIcon)
            DestroyIcon(m_hIcon);
        m_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
        Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);

        DragAcceptFiles(hwnd, TRUE);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitLangComboBox(hCmb3, GetUserDefaultLangID());

        CenterWindowDx();
        return TRUE;
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

    void OnOK(HWND hwnd)
    {
        ID_OR_STRING Type = RT_GROUP_ICON;

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

        BOOL Overwrite = FALSE;
        INT iEntry = Res_Find(m_Entries, RT_GROUP_ICON, Name, Lang);
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

        if (!Res_AddGroupIcon(m_Entries, Name, Lang, File, Overwrite))
        {
            if (Overwrite)
                ErrorBoxDx(IDS_CANTREPLACEICO);
            else
                ErrorBoxDx(IDS_CANNOTADDICON);
            return;
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
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ICOFILTER));
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = _countof(szFile);
        ofn.lpstrTitle = LoadStringDx(IDS_ADDICON);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"ico";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, szFile);
            if (m_hIcon)
                DestroyIcon(m_hIcon);
            m_hIcon = ExtractIcon(GetModuleHandle(NULL), szFile, 0);
            Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);
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

        if (m_hIcon)
            DestroyIcon(m_hIcon);
        m_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
        Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MADDICONDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
