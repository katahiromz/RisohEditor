// MReplaceBitmapDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MREPLACEBITMAPDLG_HPP_
#define MZC4_MREPLACEBITMAPDLG_HPP_

#include "RisohEditor.hpp"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckNameComboBox(HWND hCmb2, ID_OR_STRING& Name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& Lang);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& File);

//////////////////////////////////////////////////////////////////////////////

class MReplaceBitmapDlg : public MDialogBase
{
public:
    ResEntries& m_Entries;
    ResEntry& m_Entry;

    MReplaceBitmapDlg(ResEntries& Entries, ResEntry& Entry)
        : MDialogBase(IDD_REPLACEBMP), m_Entries(Entries), m_Entry(Entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        DragAcceptFiles(hwnd, TRUE);

        // for Name
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
        InitLangComboBox(hCmb3, GetUserDefaultLangID());

        return TRUE;
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

        if (!DoReplaceBitmap(hwnd, m_Entries, Name, Lang, File))
        {
            ErrorBoxDx(IDS_CANTREPLACEBMP);
            return;
        }

        EndDialog(IDOK);
    }

    void OnPsh1(HWND hwnd)
    {
        WCHAR File[MAX_PATH];
        ::GetDlgItemText(hwnd, edt1, File, _countof(File));

        std::wstring strFile = File;
        str_trim(strFile);
        lstrcpynW(File, strFile.c_str(), _countof(File));

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_BMPFILTER));
        ofn.lpstrFile = File;
        ofn.nMaxFile = _countof(File);
        ofn.lpstrTitle = LoadStringDx(IDS_REPLACEBMP);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = L"bmp";
        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hwnd, edt1, File);
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

#endif  // ndef MZC4_MREPLACEBITMAPDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
