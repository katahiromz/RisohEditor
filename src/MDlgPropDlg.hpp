// MDlgPropDlg.hpp --- "Properties for Dialog" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, 
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MDLGPROPDLG_HPP_
#define MZC4_MDLGPROPDLG_HPP_

#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "resource.h"
#include "DialogRes.hpp"

class MDlgPropDlg;

//////////////////////////////////////////////////////////////////////////////

void InitFontComboBox(HWND hCmb);
void GetStyleSelect(HWND hLst, std::vector<BYTE>& sel);
void GetStyleSelect(std::vector<BYTE>& sel, 
                    const ConstantsDB::TableType& table, DWORD dwValue);
DWORD AnalyseStyleDiff(DWORD dwValue, ConstantsDB::TableType& table, 
    std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel);
void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table);
void InitCharSetComboBox(HWND hCmb, BYTE CharSet);
BYTE GetCharSetFromComboBox(HWND hCmb);
void InitResNameComboBox(HWND hCmb, MIdOrString id, IDTYPE_ nIDTYPE_);
void InitCaptionComboBox(HWND hCmb, LPCTSTR pszCaption);
void ReplaceFullWithHalf(MStringW& strText);

//////////////////////////////////////////////////////////////////////////////

class MDlgPropDlg : public MDialogBase
{
public:
    DialogRes&      m_dialog_res;
    BOOL            m_bUpdating;
    DWORD           m_dwStyle;
    DWORD           m_dwExStyle;
    ConstantsDB::TableType  m_style_table;
    ConstantsDB::TableType  m_exstyle_table;
    std::vector<BYTE>       m_style_selection;
    std::vector<BYTE>       m_exstyle_selection;
    MComboBoxAutoComplete m_cmb1;
    MComboBoxAutoComplete m_cmb3;
    MComboBoxAutoComplete m_cmb6;

    MDlgPropDlg(DialogRes& dialog_res) :
        MDialogBase(IDD_DLGPROP), m_dialog_res(dialog_res), m_bUpdating(FALSE)
    {
        m_cmb1.m_bAcceptSpace = TRUE;
    }

    void InitTables(LPCTSTR pszClass)
    {
        ConstantsDB::TableType table;

        m_style_table.clear();
        if (pszClass && pszClass[0])
        {
            table = g_db.GetTable(pszClass);
            for (auto& item : table)
            {
                if (item.name.find(L'|') != std::wstring::npos)
                    continue;
                m_style_table.push_back(item);
            }
        }
        table = g_db.GetTable(TEXT("PARENT.STYLE"));
        for (auto& item : table)
        {
            if (item.name.find(L'|') != std::wstring::npos)
                continue;
            m_style_table.push_back(item);
        }
        m_style_selection.resize(m_style_table.size());

        m_exstyle_table.clear();
        table = g_db.GetTable(TEXT("EXSTYLE"));
        if (table.size())
        {
            m_exstyle_table.insert(m_exstyle_table.end(), 
                table.begin(), table.end());
        }
        m_exstyle_selection.resize(m_exstyle_table.size());
    }

    void InitMenuComboBox(HWND hCmb)
    {
        ComboBox_ResetContent(hCmb);

        auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
        MStringW prefix = table[IDTYPE_MENU].name;

        table = g_db.GetTable(L"RESOURCE.ID");
        for (auto& table_entry : table)
        {
            if (table_entry.name.find(prefix) == 0)
            {
                ComboBox_AddString(hCmb, table_entry.name.c_str());
            }
        }
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
        MString strCaption = m_dialog_res.m_title.c_str();
        if (strCaption.size())
        {
            strCaption = mstr_quote(strCaption);
        }

        SubclassChildDx(m_cmb1, cmb1);
        InitCaptionComboBox(m_cmb1, strCaption.c_str());
        SendDlgItemMessage(hwnd, cmb1, CB_LIMITTEXT, 64, 0);

        if (m_dialog_res.IsExtended())
        {
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
            EnableWindow(GetDlgItem(hwnd, cmb3), TRUE);
            EnableWindow(GetDlgItem(hwnd, cmb6), TRUE);
            EnableWindow(GetDlgItem(hwnd, chx2), TRUE);
            EnableWindow(GetDlgItem(hwnd, chx3), TRUE);
            EnableWindow(GetDlgItem(hwnd, cmb5), TRUE);
        }
        else
        {
            CheckDlgButton(hwnd, chx1, BST_UNCHECKED);
            EnableWindow(GetDlgItem(hwnd, cmb3), FALSE);
            EnableWindow(GetDlgItem(hwnd, cmb6), FALSE);
            EnableWindow(GetDlgItem(hwnd, chx2), FALSE);
            EnableWindow(GetDlgItem(hwnd, chx3), FALSE);
            EnableWindow(GetDlgItem(hwnd, cmb5), FALSE);
        }

        HWND hCmb4 = GetDlgItem(hwnd, cmb4);
        InitFontComboBox(hCmb4);

        HWND hCmb5 = GetDlgItem(hwnd, cmb5);
        InitCharSetComboBox(hCmb5, m_dialog_res.m_charset);

        HWND hCmb6 = GetDlgItem(hwnd, cmb6);
        InitMenuComboBox(hCmb6);

        if (m_dialog_res.m_weight >= FW_BOLD)
            CheckDlgButton(hwnd, chx2, BST_CHECKED);
        if (m_dialog_res.m_italic)
            CheckDlgButton(hwnd, chx3, BST_CHECKED);

        SetDlgItemInt(hwnd, edt1, m_dialog_res.m_pt.x, TRUE);
        SetDlgItemInt(hwnd, edt2, m_dialog_res.m_pt.y, TRUE);
        SetDlgItemInt(hwnd, edt3, m_dialog_res.m_siz.cx, TRUE);
        SetDlgItemInt(hwnd, edt4, m_dialog_res.m_siz.cy, TRUE);
        SendDlgItemMessage(hwnd, edt1, EM_SETLIMITTEXT, 12, 0);
        SendDlgItemMessage(hwnd, edt2, EM_SETLIMITTEXT, 12, 0);
        SendDlgItemMessage(hwnd, edt3, EM_SETLIMITTEXT, 12, 0);
        SendDlgItemMessage(hwnd, edt4, EM_SETLIMITTEXT, 12, 0);

        SetDlgItemTextW(hwnd, cmb2, m_dialog_res.m_class.c_str_or_empty());
        SendDlgItemMessage(hwnd, cmb2, CB_LIMITTEXT, 64, 0);

        MStringW strHelp = g_db.GetNameOfResID(IDTYPE_HELP, m_dialog_res.m_help_id);
        SetDlgItemText(hwnd, cmb3, strHelp.c_str());
        InitResNameComboBox(GetDlgItem(hwnd, cmb3), L"", IDTYPE_HELP);
        SubclassChildDx(m_cmb3, cmb3);
        SendDlgItemMessage(hwnd, cmb3, CB_LIMITTEXT, 64, 0);

        SetDlgItemTextW(hwnd, cmb4, m_dialog_res.type_face().c_str_or_empty());
        SendDlgItemMessage(hwnd, cmb4, CB_LIMITTEXT, LF_FULLFACESIZE - 1, 0);

        SetDlgItemInt(hwnd, edt5, m_dialog_res.m_point_size, TRUE);
        SendDlgItemMessage(hwnd, edt5, EM_SETLIMITTEXT, 12, 0);

        MString strMenu;
        if (m_dialog_res.m_menu.empty())
            ;
        else if (m_dialog_res.m_menu.is_int())
            strMenu = g_db.GetNameOfResID(IDTYPE_MENU, m_dialog_res.m_menu.m_id);
        else
            strMenu = m_dialog_res.m_menu.str();
        SetDlgItemTextW(hwnd, cmb6, strMenu.c_str());
        SendDlgItemMessage(hwnd, cmb6, CB_LIMITTEXT, 64, 0);
        SubclassChildDx(m_cmb6, cmb6);

        InitTables(TEXT("DIALOG"));

        WCHAR Buf[32];

        m_dwStyle = m_dialog_res.m_style;
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        GetStyleSelect(m_style_selection, m_style_table, m_dwStyle);
        InitStyleListBox(hLst1, m_style_table);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        StringCchPrintfW(Buf, _countof(Buf), L"%08X", m_dwStyle);
        SetDlgItemTextW(hwnd, edt6, Buf);
        SendDlgItemMessage(hwnd, edt6, EM_SETLIMITTEXT, 8, 0);
        m_bUpdating = FALSE;

        m_dwExStyle = m_dialog_res.m_ex_style;
        HWND hLst2 = GetDlgItem(hwnd, lst2);
        GetStyleSelect(m_exstyle_selection, m_exstyle_table, m_dwExStyle);
        InitStyleListBox(hLst2, m_exstyle_table);
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, m_dwExStyle);

        m_bUpdating = TRUE;
        StringCchPrintfW(Buf, _countof(Buf), L"%08X", m_dwExStyle);
        SetDlgItemTextW(hwnd, edt7, Buf);
        SendDlgItemMessage(hwnd, edt7, EM_SETLIMITTEXT, 8, 0);
        m_bUpdating = FALSE;

        SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));
        SendDlgItemMessage(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));
        SendDlgItemMessage(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));
        SendDlgItemMessage(hwnd, scr4, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));

        CenterWindowDx();
        return TRUE;
    }

    INT PointSizeFromStockFont(MString& strFont, INT nObject)
    {
        HFONT hFont = HFONT(GetStockObject(nObject));

        LOGFONT lf;
        GetObject(hFont, sizeof(lf), &lf);

        strFont = lf.lfFaceName;

        HDC hDC = CreateCompatibleDC(NULL);
        SelectObject(hDC, hFont);

        // lf.lfHeight --> nFontSize
        INT nFontSize;
        if (lf.lfHeight < 0)
        {
            lf.lfHeight = -lf.lfHeight;
        }
        else
        {
            TEXTMETRIC tm;
            GetTextMetrics(hDC, &tm);
            lf.lfHeight -= tm.tmInternalLeading;
        }
        nFontSize = MulDiv(lf.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));

        DeleteDC(hDC);

        return nFontSize;
    }

    void OnOK(HWND hwnd)
    {
        BOOL bExtended = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
        BOOL bBold = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
        BOOL bItalic = (::IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);

        MString strCaption = GetDlgItemText(cmb1);
        g_settings.AddCaption(strCaption.c_str());
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
        if (strClass[0] == TEXT('"'))
        {
            mstr_unquote(strClass);
        }

        MString strHelp = GetDlgItemText(cmb3);
        ReplaceFullWithHalf(strHelp);
        mstr_trim(strHelp);
        DWORD help;
        if (g_db.HasResID(strHelp))
        {
            help = g_db.GetResIDValue(strHelp);
        }
        else
        {
            help = mstr_parse_int(strHelp.c_str());
        }

        INT nFontSize = GetDlgItemInt(hwnd, edt5, NULL, TRUE);

        MString strMenu = GetDlgItemText(cmb6);
        mstr_trim(strMenu);
        MIdOrString menu(strMenu.c_str());
        if (menu.is_str() && g_db.HasResID(menu.c_str()))
        {
            menu = (WORD)g_db.GetResIDValue(menu.c_str());
        }

        MString strStyle = GetDlgItemText(edt6);
        ReplaceFullWithHalf(strStyle);
        mstr_trim(strStyle);
        DWORD style = mstr_parse_int(strStyle.c_str(), false, 16);

        MString strExStyle = GetDlgItemText(edt7);
        mstr_trim(strExStyle);
        DWORD exstyle = mstr_parse_int(strExStyle.c_str(), false, 16);

        MString strFont = GetDlgItemText(cmb4);
        mstr_trim(strFont);

        if (strFont.empty())
            style &= ~DS_SETFONT;

        if ((style & DS_SHELLFONT) == DS_FIXEDSYS)
        {
            strFont.clear();
            nFontSize = 0;
        }

        if (bExtended)
        {
            m_dialog_res.m_version = 1;
            m_dialog_res.m_signature = 0xFFFF;
        }
        else
        {
            m_dialog_res.m_version = 0;
            m_dialog_res.m_signature = 0;
        }

        m_dialog_res.m_help_id = help;
        m_dialog_res.m_style = style;
        m_dialog_res.m_ex_style = exstyle;
        m_dialog_res.m_pt.x = x;
        m_dialog_res.m_pt.y = y;
        m_dialog_res.m_siz.cx = cx;
        m_dialog_res.m_siz.cy = cy;
        m_dialog_res.m_menu = menu;
        m_dialog_res.m_class = strClass.c_str();
        m_dialog_res.m_title = strCaption.c_str();
        m_dialog_res.m_point_size = (short)nFontSize;
        m_dialog_res.m_weight = FW_NORMAL;
        m_dialog_res.m_italic = FALSE;
        m_dialog_res.m_charset = DEFAULT_CHARSET;
        m_dialog_res.type_face(strFont.c_str());

        m_dialog_res.m_weight = (bBold ? FW_BOLD : FW_DONTCARE);
        m_dialog_res.m_italic = (bItalic ? TRUE : FALSE);

        HWND hCmb5 = GetDlgItem(hwnd, cmb5);
        m_dialog_res.m_charset = GetCharSetFromComboBox(hCmb5);

        EndDialog(IDOK);
    }

    void OnLst1(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        HWND hLst1 = GetDlgItem(hwnd, lst1);

        std::vector<BYTE> old_style_selection = m_style_selection;
        GetStyleSelect(hLst1, m_style_selection);

        DWORD dwOldStyle = m_dwStyle;
        m_dwStyle = AnalyseStyleDiff(m_dwStyle, m_style_table, 
                                     old_style_selection, m_style_selection);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        StringCchPrintf(szText, _countof(szText), TEXT("%08lX"), m_dwStyle);
        SetDlgItemText(hwnd, edt6, szText);
        if ((dwOldStyle & DS_SETFONT) && !(m_dwStyle & DS_SETFONT))
        {
            SetDlgItemTextW(hwnd, cmb4, NULL);
            SetDlgItemInt(hwnd, edt5, 0, FALSE);
        }
        if (!(dwOldStyle & DS_SETFONT) && (m_dwStyle & DS_SETFONT))
        {
            if ((m_dwStyle & DS_SHELLFONT) == DS_SHELLFONT)
            {
                SetDlgItemTextW(hwnd, cmb4, L"MS Shell Dlg");
            }
            else
            {
                HDC hDC = CreateCompatibleDC(NULL);
                HFONT hFont = HFONT(GetStockObject(DEFAULT_GUI_FONT));
                SelectObject(hDC, hFont);

                LOGFONTW lf;
                GetObjectW(hFont, sizeof(lf), &lf);
                if (lf.lfHeight < 0)
                {
                    lf.lfHeight = -lf.lfHeight;
                }
                else
                {
                    TEXTMETRIC tm;
                    GetTextMetrics(hDC, &tm);
                    lf.lfHeight -= tm.tmInternalLeading;
                }
                INT nFontSize = MulDiv(lf.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
                DeleteDC(hDC);

                SetDlgItemTextW(hwnd, cmb4, lf.lfFaceName);
                SetDlgItemInt(hwnd, edt5, nFontSize, FALSE);
            }
        }
        m_bUpdating = FALSE;
    }

    void OnLst2(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        HWND hLst2 = GetDlgItem(hwnd, lst2);

        std::vector<BYTE> old_exstyle_selection = m_exstyle_selection;
        GetStyleSelect(hLst2, m_exstyle_selection);

        m_dwExStyle = AnalyseStyleDiff(m_dwExStyle, m_exstyle_table, 
                                       old_exstyle_selection, m_exstyle_selection);
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, m_dwExStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        StringCchPrintf(szText, _countof(szText), TEXT("%08lX"), m_dwExStyle);
        SetDlgItemText(hwnd, edt7, szText);
        m_bUpdating = FALSE;
    }

    void OnEdt6(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        MString text = GetDlgItemText(hwnd, edt6);
        mstr_trim(text);
        DWORD dwStyle = mstr_parse_int(text.c_str(), false, 16);

        std::vector<BYTE> old_style_selection = m_style_selection;
        GetStyleSelect(m_style_selection, m_style_table, dwStyle);

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
        DWORD dwExStyle = mstr_parse_int(text.c_str(), false, 16);

        std::vector<BYTE> old_exstyle_selection = m_exstyle_selection;
        GetStyleSelect(m_exstyle_selection, m_exstyle_table, dwExStyle);

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
        case chx1:
            if (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            {
                EnableWindow(GetDlgItem(hwnd, cmb3), TRUE);
                EnableWindow(GetDlgItem(hwnd, cmb6), TRUE);
                EnableWindow(GetDlgItem(hwnd, chx2), TRUE);
                EnableWindow(GetDlgItem(hwnd, chx3), TRUE);
                EnableWindow(GetDlgItem(hwnd, cmb5), TRUE);
            }
            else
            {
                EnableWindow(GetDlgItem(hwnd, cmb3), FALSE);
                EnableWindow(GetDlgItem(hwnd, cmb6), FALSE);
                EnableWindow(GetDlgItem(hwnd, chx2), FALSE);
                EnableWindow(GetDlgItem(hwnd, chx3), FALSE);
                EnableWindow(GetDlgItem(hwnd, cmb5), FALSE);
            }
            break;
        case cmb3:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb3.OnEditChange();
            }
            break;
        case cmb6:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb6.OnEditChange();
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
