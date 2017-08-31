// MModifyResIDDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MMODIFYRESIDDLG_HPP_
#define MZC4_MMODIFYRESIDDLG_HPP_

#include "RisohEditor.hpp"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////////

class MModifyResIDDlg : public MDialogBase
{
public:
    ConstantsDB& m_db;
    MString m_str1;
    MString m_str2;

    MModifyResIDDlg(ConstantsDB& db, MString str1, MString str2)
        : MDialogBase(IDD_MODIFYRESID), m_db(db), m_str1(str1), m_str2(str2)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemText(hwnd, edt1, m_str1.c_str());
        SetDlgItemText(hwnd, edt3, m_str2.c_str());

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

#endif  // ndef MZC4_MMODIFYRESIDDLG_HPP_
