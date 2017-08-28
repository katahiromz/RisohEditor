// MModifyAssoc.hpp
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MMODIFYASSOCDLG_HPP_
#define MZC4_MMODIFYASSOCDLG_HPP_

#include "MWindowBase.hpp"

//////////////////////////////////////////////////////////////////////////////

class MModifyAssoc : public MDialogBase
{
public:
    MModifyAssoc()
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
            EndDialog(IDOK);
            break;
        case IDCANCEL:
            EndDialog(IDCANCEL);
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
        default:
            return DefaultProcDx();
        }
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MMODIFYASSOCDLG_HPP_
