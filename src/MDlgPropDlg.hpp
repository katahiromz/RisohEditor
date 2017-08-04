// MDlgPropDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MDLGPROPDLG_HPP_
#define MZC4_MDLGPROPDLG_HPP_

#include "RisohEditor.hpp"
#include "DialogRes.hpp"
#include "resource.h"

void InitFontComboBox(HWND hCmb);

void GetSelection(HWND hLst, std::vector<BYTE>& sel);
void GetSelection(std::vector<BYTE>& sel,
                  const ConstantsDB::TableType& table, DWORD dwValue);
DWORD AnalyseDifference(DWORD dwValue, ConstantsDB::TableType& table,
    std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel);
void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table);
void InitCharSetComboBox(HWND hCmb, BYTE CharSet);
BYTE GetCharSetFromComboBox(HWND hCmb);

//////////////////////////////////////////////////////////////////////////////

class MDlgPropDlg : public MDialogBase
{
public:
    DialogRes&  m_dialog_res;
    BOOL        m_bUpdating;
    DWORD       m_dwStyle;
    DWORD       m_dwExStyle;
    ConstantsDB::TableType  m_style_table;
    ConstantsDB::TableType  m_exstyle_table;
    std::vector<BYTE>       m_style_selection;
    std::vector<BYTE>       m_exstyle_selection;

    MDlgPropDlg(DialogRes& dialog_res) :
        MDialogBase(IDD_DLGPROP), m_dialog_res(dialog_res), m_bUpdating(FALSE)
    {
    }

    void InitTables(LPCTSTR pszClass)
    {
        extern ConstantsDB g_ConstantsDB;

        ConstantsDB::TableType table;

        m_style_table.clear();
        if (pszClass && pszClass[0])
        {
            table = g_ConstantsDB.GetTable(pszClass);
            if (table.size())
            {
                m_style_table.insert(m_style_table.end(),
                    table.begin(), table.end());
            }
        }
        table = g_ConstantsDB.GetTable(TEXT("STYLE"));
        if (table.size())
        {
            m_style_table.insert(m_style_table.end(),
                table.begin(), table.end());
        }
        m_style_selection.resize(m_style_table.size());

        m_exstyle_table.clear();
        table = g_ConstantsDB.GetTable(TEXT("EXSTYLE"));
        if (table.size())
        {
            m_exstyle_table.insert(m_exstyle_table.end(),
                table.begin(), table.end());
        }
        m_exstyle_selection.resize(m_exstyle_table.size());
    }

    void ApplySelection(HWND hLst, std::vector<BYTE>& sel)
    {
        m_bUpdating = TRUE;
        INT iTop = ListBox_GetTopIndex(hLst);
        for (size_t i = 0; i < sel.size(); ++i)
        {
            ListBox_SetSel(hLst, sel[i], (DWORD)i);
        }
        ListBox_SetTopIndex(hLst, iTop);
        m_bUpdating = FALSE;
    }

    void ApplySelection(HWND hLst, ConstantsDB::TableType& table,
                        std::vector<BYTE>& sel, DWORD dwValue)
    {
        m_bUpdating = TRUE;
        INT iTop = ListBox_GetTopIndex(hLst);
        for (size_t i = 0; i < table.size(); ++i)
        {
            sel[i] = ((dwValue & table[i].mask) == table[i].value);
            ListBox_SetSel(hLst, sel[i], (DWORD)i);
        }
        ListBox_SetTopIndex(hLst, iTop);
        m_bUpdating = FALSE;
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        MString strCaption = m_dialog_res.m_Title.c_str();
        if (strCaption.size())
        {
            strCaption = mstr_quote(strCaption);
        }

        ::SetDlgItemTextW(hwnd, cmb1, strCaption.c_str());
        ::SendDlgItemMessage(hwnd, cmb1, CB_LIMITTEXT, 64, 0);

        if (m_dialog_res.IsExtended())
        {
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        }

        HWND hCmb4 = GetDlgItem(hwnd, cmb4);
        InitFontComboBox(hCmb4);

        HWND hCmb5 = GetDlgItem(hwnd, cmb5);
        InitCharSetComboBox(hCmb5, m_dialog_res.m_CharSet);

        if (m_dialog_res.m_Weight >= FW_BOLD)
            CheckDlgButton(hwnd, chx2, BST_CHECKED);
        if (m_dialog_res.m_Italic)
            CheckDlgButton(hwnd, chx3, BST_CHECKED);

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

        ::SetDlgItemTextW(hwnd, cmb6, m_dialog_res.m_Menu.c_str_or_empty());
        ::SendDlgItemMessage(hwnd, cmb6, CB_LIMITTEXT, 64, 0);

        InitTables(TEXT("DIALOG"));

        WCHAR Buf[32];

        m_dwStyle = m_dialog_res.m_Style;
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        GetSelection(m_style_selection, m_style_table, m_dwStyle);
        InitStyleListBox(hLst1, m_style_table);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        wsprintfW(Buf, L"%08X", m_dwStyle);
        ::SetDlgItemTextW(hwnd, edt6, Buf);
        ::SendDlgItemMessage(hwnd, edt6, EM_SETLIMITTEXT, 8, 0);
        m_bUpdating = FALSE;

        m_dwExStyle = m_dialog_res.m_ExStyle;
        HWND hLst2 = GetDlgItem(hwnd, lst2);
        GetSelection(m_exstyle_selection, m_exstyle_table, m_dwExStyle);
        InitStyleListBox(hLst2, m_exstyle_table);
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, m_dwExStyle);

        m_bUpdating = TRUE;
        wsprintfW(Buf, L"%08X", m_dwExStyle);
        ::SetDlgItemTextW(hwnd, edt7, Buf);
        ::SendDlgItemMessage(hwnd, edt7, EM_SETLIMITTEXT, 8, 0);
        m_bUpdating = FALSE;

        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        BOOL bExtended = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
        BOOL bBold = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
        BOOL bItalic = (::IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);

        MString strCaption = GetDlgItemText(cmb1);
        mstr_trim(strCaption);
        if (strCaption[0] == TEXT('"'))
        {
            mstr_unquote(strCaption);
        }

        INT x = ::GetDlgItemInt(hwnd, edt1, NULL, TRUE);
        INT y = ::GetDlgItemInt(hwnd, edt2, NULL, TRUE);
        INT cx = ::GetDlgItemInt(hwnd, edt3, NULL, TRUE);
        INT cy = ::GetDlgItemInt(hwnd, edt4, NULL, TRUE);

        MString strClass = GetDlgItemText(cmb2);
        mstr_trim(strClass);

        MString strHelp = GetDlgItemText(cmb3);
        mstr_trim(strHelp);
        DWORD help = _tcstoul(strHelp.c_str(), NULL, 0);

        INT nFontSize = GetDlgItemInt(hwnd, edt5, NULL, TRUE);

        MString strMenu = GetDlgItemText(cmb6);
        mstr_trim(strMenu);

        MString strStyle = GetDlgItemText(edt6);
        mstr_trim(strStyle);
        DWORD style = _tcstoul(strStyle.c_str(), NULL, 16);

        MString strExStyle = GetDlgItemText(edt7);
        mstr_trim(strExStyle);
        DWORD exstyle = _tcstoul(strExStyle.c_str(), NULL, 16);

        MString strFont = GetDlgItemText(cmb4);
        mstr_trim(strFont);
        if (strFont.empty())
            style &= ~DS_SETFONT;
        else
            style |= DS_SETFONT;

        if (bExtended)
        {
            m_dialog_res.m_Version = 1;
            m_dialog_res.m_Signature = 0xFFFF;
        }
        else
        {
            m_dialog_res.m_Version = 0;
            m_dialog_res.m_Signature = 0;
        }

        m_dialog_res.m_HelpID = help;
        m_dialog_res.m_Style = style;
        m_dialog_res.m_ExStyle = exstyle;
        m_dialog_res.m_pt.x = x;
        m_dialog_res.m_pt.y = y;
        m_dialog_res.m_siz.cx = cx;
        m_dialog_res.m_siz.cy = cy;
        m_dialog_res.m_Menu = strMenu.c_str();
        m_dialog_res.m_Class = strClass.c_str();
        m_dialog_res.m_Title = strCaption.c_str();
        m_dialog_res.m_PointSize = (short)nFontSize;
        m_dialog_res.m_Weight = FW_NORMAL;
        m_dialog_res.m_Italic = FALSE;
        m_dialog_res.m_CharSet = DEFAULT_CHARSET;
        m_dialog_res.m_TypeFace = strFont.c_str();

        m_dialog_res.m_Weight = (bBold ? FW_BOLD : FW_DONTCARE);
        m_dialog_res.m_Italic = (bItalic ? TRUE : FALSE);

        HWND hCmb5 = GetDlgItem(hwnd, cmb5);
        m_dialog_res.m_CharSet = GetCharSetFromComboBox(hCmb5);

        EndDialog(IDOK);
    }

    void OnLst1(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        HWND hLst1 = GetDlgItem(hwnd, lst1);

        std::vector<BYTE> old_style_selection = m_style_selection;
        GetSelection(hLst1, m_style_selection);

        m_dwStyle = AnalyseDifference(m_dwStyle, m_style_table,
                                      old_style_selection, m_style_selection);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        wsprintf(szText, TEXT("%08lX"), m_dwStyle);
        SetDlgItemText(hwnd, edt6, szText);
        m_bUpdating = FALSE;
    }

    void OnLst2(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        HWND hLst2 = GetDlgItem(hwnd, lst2);

        std::vector<BYTE> old_exstyle_selection = m_exstyle_selection;
        GetSelection(hLst2, m_exstyle_selection);

        m_dwExStyle = AnalyseDifference(m_dwExStyle, m_exstyle_table,
                                        old_exstyle_selection, m_exstyle_selection);
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, m_dwExStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        wsprintf(szText, TEXT("%08lX"), m_dwExStyle);
        SetDlgItemText(hwnd, edt7, szText);
        m_bUpdating = FALSE;
    }

    void OnEdt6(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        MString text = GetDlgItemText(hwnd, edt6);
        mstr_trim(text);
        DWORD dwStyle = _tcstoul(text.c_str(), NULL, 16);

        std::vector<BYTE> old_style_selection = m_style_selection;
        GetSelection(m_style_selection, m_style_table, dwStyle);

        HWND hLst1 = GetDlgItem(hwnd, lst1);
        m_dwStyle = dwStyle;
        ApplySelection(hLst1, m_style_table, m_style_selection, dwStyle);
    }

    void OnEdt7(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        MString text = GetDlgItemText(hwnd, edt7);
        mstr_trim(text);
        DWORD dwExStyle = _tcstoul(text.c_str(), NULL, 16);

        std::vector<BYTE> old_exstyle_selection = m_exstyle_selection;
        GetSelection(m_exstyle_selection, m_exstyle_table, dwExStyle);

        HWND hLst2 = GetDlgItem(hwnd, lst2);
        m_dwExStyle = dwExStyle;
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, dwExStyle);
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

#endif  // ndef MZC4_MDLGPROPDLG_HPP_

//////////////////////////////////////////////////////////////////////////////
