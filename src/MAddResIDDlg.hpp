// MAddResIDDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MADDRESIDDLG_HPP_
#define MZC4_MADDRESIDDLG_HPP_

#include "RisohEditor.hpp"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////////

class MAddResIDDlg : public MDialogBase
{
public:
    ConstantsDB& m_db;

    MAddResIDDlg(ConstantsDB& db) : MDialogBase(IDD_ADDRESID), m_db(db)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        EndDialog(IDOK);
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
        case edt1:
            if (codeNotify == EN_CHANGE)
            {
                ;
            }
            break;
        case edt2:
            if (codeNotify == EN_CHANGE)
            {
                ;
            }
            break;
        case edt3:
            if (codeNotify == EN_CHANGE)
            {
                ;
            }
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

#endif  // ndef MZC4_MADDRESIDDLG_HPP_
