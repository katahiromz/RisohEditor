// MDlgPropDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MDLGPROPDLG_HPP_
#define MZC4_MDLGPROPDLG_HPP_

#include "RisohEditor.hpp"
#include "DialogRes.hpp"
#include "resource.h"

void InitLangComboBox(HWND hCmb3, LANGID langid);
BOOL CheckNameComboBox(HWND hCmb2, ID_OR_STRING& Name);
BOOL CheckLangComboBox(HWND hCmb3, WORD& Lang);

//////////////////////////////////////////////////////////////////////////////

class MDlgPropDlg : public MDialogBase
{
public:
    DialogRes&  m_dialog_res;

    MDlgPropDlg(DialogRes& dialog_res) :
        MDialogBase(IDD_DLGPROP), m_dialog_res(dialog_res)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        ::SetDlgItemTextW(hwnd, cmb1, m_dialog_res.m_Title.c_str_or_empty());
        ::SendDlgItemMessage(hwnd, cmb1, CB_LIMITTEXT, 64, 0);

        if (m_dialog_res.IsExtended())
        {
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        }

        ::SetDlgItemInt(hwnd, edt1, m_dialog_res.m_pt.x, TRUE);
        ::SetDlgItemInt(hwnd, edt2, m_dialog_res.m_pt.y, TRUE);
        ::SetDlgItemInt(hwnd, edt3, m_dialog_res.m_siz.cx, TRUE);
        ::SetDlgItemInt(hwnd, edt4, m_dialog_res.m_siz.cy, TRUE);
        ::SendDlgItemMessage(hwnd, edt1, EM_SETLIMITTEXT, 12, 0);
        ::SendDlgItemMessage(hwnd, edt2, EM_SETLIMITTEXT, 12, 0);
        ::SendDlgItemMessage(hwnd, edt3, EM_SETLIMITTEXT, 12, 0);
        ::SendDlgItemMessage(hwnd, edt4, EM_SETLIMITTEXT, 12, 0);

        ::SetDlgItemTextW(hwnd, cmb2, m_dialog_res.m_Class.c_str_or_empty());
        ::SendDlgItemMessage(hwnd, cmb2, CB_LIMITTEXT, 64, 0);

        ::SetDlgItemInt(hwnd, cmb3, m_dialog_res.m_HelpID, FALSE);
        ::SendDlgItemMessage(hwnd, cmb2, CB_LIMITTEXT, 12, 0);

        ::SetDlgItemTextW(hwnd, cmb4, m_dialog_res.m_TypeFace.c_str_or_empty());
        ::SendDlgItemMessage(hwnd, cmb4, CB_LIMITTEXT, LF_FULLFACESIZE - 1, 0);

        ::SetDlgItemInt(hwnd, edt5, m_dialog_res.m_PointSize, TRUE);
        ::SendDlgItemMessage(hwnd, edt5, EM_SETLIMITTEXT, 12, 0);

        HWND hCmb5 = GetDlgItem(hwnd, cmb5);
        InitLangComboBox(hCmb5, m_dialog_res.m_LangID);

        ::SetDlgItemTextW(hwnd, cmb6, m_dialog_res.m_Menu.c_str_or_empty());
        ::SendDlgItemMessage(hwnd, cmb6, CB_LIMITTEXT, 64, 0);

        WCHAR Buf[32];

        wsprintfW(Buf, L"%08X", m_dialog_res.m_Style);
        ::SetDlgItemTextW(hwnd, edt6, Buf);
        ::SendDlgItemMessage(hwnd, edt6, EM_SETLIMITTEXT, 8, 0);
        wsprintfW(Buf, L"%08X", m_dialog_res.m_ExStyle);
        ::SetDlgItemTextW(hwnd, edt7, Buf);
        ::SendDlgItemMessage(hwnd, edt7, EM_SETLIMITTEXT, 8, 0);

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        HWND hCmb5 = GetDlgItem(hwnd, cmb5);
        WORD Lang;
        if (!CheckLangComboBox(hCmb5, Lang))
            return;

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

#endif  // ndef MZC4_MDLGPROPDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
