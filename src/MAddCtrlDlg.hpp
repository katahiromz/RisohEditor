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
    BOOL m_bUpdating;
    DWORD m_dwStyle;
    DWORD m_dwExStyle;

    MAddCtrlDlg(DialogRes& dialog_res)
        : MDialogBase(IDD_ADDCTRL), m_dialog_res(dialog_res), m_bUpdating(FALSE)
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
                    if (it->value == 0)
                        continue;

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
                if (it->value == 0)
                    continue;

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
                if (it->value == 0)
                    continue;

                ListBox_AddString(hLst2, it->name.c_str());
            }
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        extern ConstantsDB g_ConstantsDB;

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);

        ConstantsDB::TableType table;

        {
            table = g_ConstantsDB.GetTable(TEXT("CONTROL.CLASSES"));
            ConstantsDB::TableType::iterator it, end = table.end();
            for (it = table.begin(); it != end; ++it)
            {
                ComboBox_AddString(hCmb1, it->name.c_str());
            }
        }

        HWND hLst1 = GetDlgItem(hwnd, lst1);
        InitStyleListBox(hLst1, TEXT(""));

        HWND hLst2 = GetDlgItem(hwnd, lst2);
        InitExStyleListBox(hLst2);

        TCHAR szText[64];
        m_dwStyle = WS_VISIBLE | WS_CHILD;
        wsprintf(szText, TEXT("%08lX"), m_dwStyle);
        SetDlgItemText(hwnd, edt6, szText);

        m_dwExStyle = 0;
        wsprintf(szText, TEXT("%08lX"), m_dwExStyle);
        SetDlgItemText(hwnd, edt7, szText);

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        EndDialog(IDOK);
    }

    void OnLst1(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        extern ConstantsDB g_ConstantsDB;

        TCHAR szText[64];
        MString strClass = GetDlgItemText(hwnd, cmb1);
        mstr_trim(strClass);

        INT sels[64];
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        INT nCount = ListBox_GetSelItems(hLst1, _countof(sels), sels);
        DWORD dwStyle = 0;
        for (INT i = 0; i < nCount; ++i)
        {
            ListBox_GetText(hLst1, sels[i], szText);

            DWORD dwValue = g_ConstantsDB.GetValue(strClass, szText);
            if (dwValue == 0)
                dwValue = g_ConstantsDB.GetValue(TEXT("STYLE"), szText);
            dwStyle |= dwValue;
        }

        m_dwStyle = dwStyle;
        wsprintf(szText, TEXT("%08lX"), m_dwStyle);
        m_bUpdating = TRUE;
        SetDlgItemText(hwnd, edt6, szText);
        m_bUpdating = FALSE;
    }

    void OnLst2(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        extern ConstantsDB g_ConstantsDB;

        TCHAR szText[64];
        INT sels[64];
        HWND hLst2 = GetDlgItem(hwnd, lst2);
        INT nCount = ListBox_GetSelItems(hLst2, _countof(sels), sels);
        DWORD dwExStyle = 0;
        for (INT i = 0; i < nCount; ++i)
        {
            ListBox_GetText(hLst2, sels[i], szText);

            DWORD dwValue = g_ConstantsDB.GetValue(TEXT("EXSTYLE"), szText);
            dwExStyle |= dwValue;
        }

        m_dwExStyle = dwExStyle;
        wsprintf(szText, TEXT("%08lX"), m_dwExStyle);
        m_bUpdating = TRUE;
        SetDlgItemText(hwnd, edt7, szText);
        m_bUpdating = FALSE;
    }

    void OnEdt6(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        extern ConstantsDB g_ConstantsDB;

        MString strClass = GetDlgItemText(hwnd, cmb1);
        mstr_trim(strClass);

        MString text = GetDlgItemText(hwnd, edt6);
        mstr_trim(text);
        DWORD dwStyle = _tcstoul(text.c_str(), NULL, 16);

        TCHAR szText[64];
        INT sels[64];
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        INT nCount = ListBox_GetSelItems(hLst1, _countof(sels), sels);
        for (INT i = 0; i < nCount; ++i)
        {
            ListBox_GetText(hLst1, sels[i], szText);

            DWORD dwValue = g_ConstantsDB.GetValue(strClass, szText);
            if (dwValue == 0)
                dwValue = g_ConstantsDB.GetValue(TEXT("STYLE"), szText);
            if (dwValue == 0)
                continue;

            m_bUpdating = TRUE;
            if ((dwStyle & dwValue) == dwValue)
            {
                ListBox_SetSel(hLst1, TRUE, i);
            }
            else
            {
                ListBox_SetSel(hLst1, FALSE, i);
            }
            m_bUpdating = FALSE;
        }
    }

    void OnEdt7(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        extern ConstantsDB g_ConstantsDB;

        MString text = GetDlgItemText(hwnd, edt6);
        mstr_trim(text);
        DWORD dwExStyle = _tcstoul(text.c_str(), NULL, 16);

        TCHAR szText[64];
        INT sels[64];
        HWND hLst2 = GetDlgItem(hwnd, lst2);
        INT nCount = ListBox_GetSelItems(hLst2, _countof(sels), sels);
        for (INT i = 0; i < nCount; ++i)
        {
            ListBox_GetText(hLst2, sels[i], szText);

            DWORD dwValue = g_ConstantsDB.GetValue(TEXT("EXSTYLE"), szText);
            if (dwValue == 0)
                continue;

            m_bUpdating = TRUE;
            if ((dwExStyle & dwValue) == dwValue)
            {
                ListBox_SetSel(hLst2, TRUE, i);
            }
            else
            {
                ListBox_SetSel(hLst2, FALSE, i);
            }
            m_bUpdating = FALSE;
        }
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
        case lst1:
            if (codeNotify == LBN_SELCHANGE)
            {
                OnLst1(hwnd);
            }
            break;
        case lst2:
            if (codeNotify == LBN_SELCHANGE)
            {
                OnLst2(hwnd);
            }
            break;
        case edt6:
            if (codeNotify == EN_CHANGE)
            {
                OnEdt6(hwnd);
            }
            break;
        case edt7:
            if (codeNotify == EN_CHANGE)
            {
                OnEdt7(hwnd);
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
