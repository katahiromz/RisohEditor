// MReplaceIconDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MREPLACEICONDLG_HPP_
#define MZC4_MREPLACEICONDLG_HPP_

#include "RisohEditor.hpp"

void Cmb3_InsertLangItemsAndSelectLang(HWND hCmb3, LANGID langid);
BOOL Cmb2_CheckName(HWND hCmb2, ID_OR_STRING& Name);
BOOL Cmb3_CheckLang(HWND hCmb3, WORD& Lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& File);

//////////////////////////////////////////////////////////////////////////////

class MReplaceIconDlg : public MDialogBase
{
protected:
    HICON   m_hIcon;
public:
    ResEntries& m_Entries;
    ResEntry& m_Entry;

    MReplaceIconDlg(ResEntries& Entries, ResEntry& Entry)
        : MDialogBase(IDD_REPLACEICON), m_Entries(Entries), m_Entry(Entry)
    {
        m_hIcon = NULL;
    }

    ~MReplaceIconDlg()
    {
        DestroyIcon(m_hIcon);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        DragAcceptFiles(hwnd, TRUE);

        // for Name
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        if (m_Entry.name.is_str())
        {
            SetWindowTextW(hCmb2, m_Entry.name.m_Str.c_str());
        }
        else
        {
            SetDlgItemInt(hwnd, cmb2, m_Entry.name.m_ID, FALSE);
        }
        EnableWindow(hCmb2, FALSE);

        // for Langs
        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        Cmb3_InsertLangItemsAndSelectLang(hCmb3, GetUserDefaultLangID());

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        ID_OR_STRING Type = RT_GROUP_ICON;

        ID_OR_STRING Name;
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
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

        if (!DoReplaceIcon(hwnd, m_Entries, Name, Lang, File))
        {
            ErrorBoxDx(IDS_CANTREPLACEICO);
            return;
        }

        EndDialog(hwnd, IDOK);
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
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ICOFILTER));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx2(IDS_REPLACEICO);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"ico";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, File);
            if (m_hIcon)
                DestroyIcon(m_hIcon);
            m_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
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

        if (m_hIcon)
            DestroyIcon(m_hIcon);
        m_hIcon = ExtractIcon(GetModuleHandle(NULL), File, 0);
        Static_SetIcon(GetDlgItem(hwnd, ico1), m_hIcon);
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

#endif  // ndef MZC4_MREPLACEICONDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
