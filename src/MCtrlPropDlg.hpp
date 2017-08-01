// MCtrlPropDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MCTRLPROPDLG_HPP_
#define MZC4_MCTRLPROPDLG_HPP_

#include "RisohEditor.hpp"
#include "DialogRes.hpp"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckNameComboBox(HWND hCmb2, ID_OR_STRING& Name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& Lang);

//////////////////////////////////////////////////////////////////////////////

class MCtrlPropDlg : public MDialogBase
{
public:
    DialogRes&  m_dialog_res;
    INT         m_nCtrlIndex;

    MCtrlPropDlg(DialogRes& dialog_res, INT nCtrlIndex)
        : MDialogBase(IDD_CTRLPROP), m_dialog_res(dialog_res),
          m_nCtrlIndex(nCtrlIndex)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
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

#endif  // ndef MZC4_MCTRLPROPDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
