// MAddCtrlDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MADDCTRLDLG_HPP_
#define MZC4_MADDCTRLDLG_HPP_

#include "RisohEditor.hpp"
#include "DialogRes.hpp"
#include "MString.hpp"
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

    void InitStyleListBox(HWND hLst1, LPCTSTR pszClass)
    {
        extern ConstantsDB g_ConstantsDB;

        ListBox_ResetContent(hLst1);

        ConstantsDB::TableType table;
        if (pszClass[0])
        {
            table = g_ConstantsDB.GetTable(pszClass);
            if (table.size())
            {
                ConstantsDB::TableType::iterator it, end = table.end();
                for (it = table.begin(); it != end; ++it)
                {
                    ListBox_AddString(hLst1, it->name.c_str());
                }
            }
        }
        table = g_ConstantsDB.GetTable(TEXT("STYLE"));
        if (table.size())
        {
            ConstantsDB::TableType::iterator it, end = table.end();
            for (it = table.begin(); it != end; ++it)
            {
                ListBox_AddString(hLst1, it->name.c_str());
            }
        }
    }

    void InitExStyleListBox(HWND hLst2)
    {
        extern ConstantsDB g_ConstantsDB;

        ListBox_ResetContent(hLst2);

        ConstantsDB::TableType table;
        table = g_ConstantsDB.GetTable(TEXT("EXSTYLE"));
        if (table.size())
        {
            ConstantsDB::TableType::iterator it, end = table.end();
            for (it = table.begin(); it != end; ++it)
            {
                ListBox_AddString(hLst2, it->name.c_str());
            }
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        extern ConstantsDB g_ConstantsDB;

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);

        ConstantsDB::TableType table;

        table = g_ConstantsDB.GetTable(TEXT("CONTROL.CLASSES"));
        ConstantsDB::TableType::iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            ComboBox_AddString(hCmb1, it->name.c_str());
        }

        HWND hLst1 = GetDlgItem(hwnd, lst1);
        InitStyleListBox(hLst1, TEXT(""));

        HWND hLst2 = GetDlgItem(hwnd, lst2);
        InitExStyleListBox(hLst2);

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        EndDialog(IDOK);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        TCHAR szText[64];
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(IDCANCEL);
            break;
        case cmb1:
            if (codeNotify == CBN_SELCHANGE)
            {
                INT nIndex = ComboBox_GetCurSel(hCmb1);
                ComboBox_GetLBText(hCmb1, nIndex, szText);
                MString text = szText;
                mstr_trim(text);
                InitStyleListBox(hLst1, text.c_str());
            }
            else if (codeNotify == CBN_EDITCHANGE)
            {
                MString text = GetDlgItemText(hwnd, cmb1);
                mstr_trim(text);
                InitStyleListBox(hLst1, text.c_str());
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

#endif  // ndef MZC4_MADDCTRLDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
