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
        MString str1 = GetDlgItemText(hwnd, edt1);
        mstr_trim(str1);
        if (str1.empty())
        {
            HWND hEdt1 = GetDlgItem(hwnd, edt1);
            Edit_SetSel(hEdt1, 0, -1);
            SetFocus(hEdt1);
            ErrorBoxDx(IDS_ENTERTEXT);
            return;
        }
        m_str1 = str1;

        MString str2 = GetDlgItemText(hwnd, edt1);
        mstr_trim(str2);
        if (str2.empty())
        {
            HWND hEdt1 = GetDlgItem(hwnd, edt1);
            Edit_SetSel(hEdt1, 0, -1);
            SetFocus(hEdt1);
            ErrorBoxDx(IDS_ENTERID);
            return;
        }

        MString str3 = GetDlgItemText(hwnd, edt3);
        mstr_trim(str3);
        if (str3.empty())
        {
            HWND hEdt3 = GetDlgItem(hwnd, edt3);
            Edit_SetSel(hEdt3, 0, -1);
            SetFocus(hEdt3);
            ErrorBoxDx(IDS_ENTERTEXT);
            return;
        }
        m_str2 = str3;

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
                MString text = GetDlgItemText(hwnd, edt1);

                ConstantsDB::TableType table;
                table = m_db.GetTable(L"RESOURCE.ID.PREFIX");

                INT i = 0;
                ConstantsDB::TableType::iterator it, end = table.end();
                for (it = table.begin(); it != end; ++it)
                {
                    if (text.find(it->name) == 0)
                    {
                        text = m_db.GetName(L"RESOURCE.ID.TYPE", i);
                        SetDlgItemText(hwnd, edt2, text.c_str());
                        i = -1;
                        break;
                    }
                    ++i;
                }
                if (i != -1)
                {
                    SetDlgItemText(hwnd, edt2, NULL);
                }
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
