// MAddCtrlDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MADDCTRLDLG_HPP_
#define MZC4_MADDCTRLDLG_HPP_

#include "RisohEditor.hpp"
#include "DialogRes.hpp"
#include "resource.h"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckNameComboBox(HWND hCmb2, ID_OR_STRING& Name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& Lang);

//////////////////////////////////////////////////////////////////////////////

class MAddCtrlDlg : public MDialogBase
{
public:
    DialogRes&  m_dialog_res;

    MAddCtrlDlg(DialogRes& dialog_res)
        : MDialogBase(IDD_ADDCTRL), m_dialog_res(dialog_res)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        extern ConstantsDB g_ConstantsDB;

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);

        ConstantsDB::TableType table =
            g_ConstantsDB.GetTable(TEXT("CONTROL.CLASSES"));
        ConstantsDB::TableType::iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            ComboBox_AddString(hCmb1, it->name.c_str());
        }

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

#endif  // ndef MZC4_MADDCTRLDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
