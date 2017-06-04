// MAddStrDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MADDSTRDLG_HPP_
#define MZC4_MADDSTRDLG_HPP_

#include "RisohEditor.hpp"

//////////////////////////////////////////////////////////////////////////////

struct MAddStrDlg : MDialogBase
{
    STRING_ENTRY& m_entry;

    MAddStrDlg(STRING_ENTRY& entry) : MDialogBase(IDD_ADDSTR), m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            StrDlg_GetEntry(hwnd, m_entry);
            EndDialog(hwnd, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
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
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MADDSTRDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
