// MModifyStrDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MMODIFYSTRDLG_HPP_
#define MZC4_MMODIFYSTRDLG_HPP_

#include "RisohEditor.hpp"
#include "StringRes.hpp"

//////////////////////////////////////////////////////////////////////////////

struct MModifyStrDlg : MDialogBase
{
    STRING_ENTRY& m_entry;
    MModifyStrDlg(STRING_ENTRY& entry) : MDialogBase(IDD_MODIFYSTR), m_entry(entry)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        StrDlg_SetEntry(hwnd, m_entry);
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
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MMODIFYSTRDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
