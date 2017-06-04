// TestMenuDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_TESTMENUDLG_HPP_
#define MZC4_TESTMENUDLG_HPP_

#include "WindowBase.hpp"

//////////////////////////////////////////////////////////////////////////////

struct TestMenuDlg : DialogBase
{
    HMENU m_hMenu;

    TestMenuDlg(HMENU hMenu) : m_hMenu(hMenu)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetMenu(hwnd, m_hMenu);
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK: case IDCANCEL:
            EndDialog(hwnd, id);
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

#endif  // ndef MZC4_TESTMENUDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
