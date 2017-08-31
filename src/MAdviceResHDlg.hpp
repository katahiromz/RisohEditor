// MAdviceResHDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MADVICERESHDLG_HPP_
#define MZC4_MADVICERESHDLG_HPP_

#include "RisohEditor.hpp"

//////////////////////////////////////////////////////////////////////////////

class MAdviceResHDlg : public MDialogBase
{
public:
    RisohSettings& m_settings;
    MString m_str;

    MAdviceResHDlg(RisohSettings& settings, const MString& str) :
        MDialogBase(IDD_ADVICERESH), m_settings(settings), m_str(str)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemText(hwnd, edt1, m_str.c_str());

        CenterWindowDx();

        SetFocus(GetDlgItem(hwnd, IDOK));
        return FALSE;
    }

    void OnOK(HWND hwnd)
    {
        EndDialog(IDOK);
    }

    void OnPsh1(HWND hwnd)
    {
        m_str.clear();
        m_settings.added_ids.clear();
        m_settings.removed_ids.clear();
        SetDlgItemText(hwnd, edt1, NULL);
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

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MADVICERESHDLG_HPP_
