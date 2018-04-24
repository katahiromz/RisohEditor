// MAddCtrlDlg.hpp --- "Add Control" Dialog
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

#ifndef MZC4_MADDCTRLDLG_HPP_
#define MZC4_MADDCTRLDLG_HPP_

#include "RisohEditor.hpp"
#include "DialogRes.hpp"
#include "MString.hpp"
#include "resource.h"
#include "MToolBarCtrl.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "MCtrlDataDlg.hpp"
#include "MStringListDlg.hpp"
#include <oledlg.h>

class MAddCtrlDlg;

void ReplaceFullWithHalf(MStringW& strText);
BOOL IsThereWndClass(const WCHAR *pszName);

//////////////////////////////////////////////////////////////////////////////

void GetStyleSelect(HWND hLst, std::vector<BYTE>& sel);
void GetStyleSelect(std::vector<BYTE>& sel,
                  const ConstantsDB::TableType& table, DWORD dwValue);
DWORD AnalyseStyleDiff(DWORD dwValue, ConstantsDB::TableType& table,
    std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel);
void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table);
void InitClassComboBox(HWND hCmb, ConstantsDB& db, LPCTSTR pszClass);
void InitCaptionComboBox(HWND hCmb, RisohSettings& settings, LPCTSTR pszCaption);
void InitWndClassComboBox(HWND hCmb, ConstantsDB& db, LPCTSTR pszWndClass);
void InitCtrlIDComboBox(HWND hCmb, ConstantsDB& db);
void InitResNameComboBox(HWND hCmb, ConstantsDB& db, MIdOrString id, INT nIDTYPE_);

//////////////////////////////////////////////////////////////////////////////

class MAddCtrlDlg : public MDialogBase
{
public:
    DialogRes&      m_dialog_res;
    BOOL            m_bUpdating;
    DWORD           m_dwStyle;
    DWORD           m_dwExStyle;
    POINT           m_pt;
    ConstantsDB&    m_db;
    ConstantsDB::TableType  m_style_table;
    ConstantsDB::TableType  m_exstyle_table;
    std::vector<BYTE>       m_style_selection;
    std::vector<BYTE>       m_exstyle_selection;
    RisohSettings&          m_settings;
    MToolBarCtrl            m_hTB;
    HIMAGELIST              m_himlControls;
    std::vector<std::wstring> m_vecControls;
    MComboBoxAutoComplete m_cmb1;
    MComboBoxAutoComplete m_cmb2;
    MComboBoxAutoComplete m_cmb3;
    MComboBoxAutoComplete m_cmb4;
    MComboBoxAutoComplete m_cmb5;
    std::vector<BYTE> m_data;

    MAddCtrlDlg(DialogRes& dialog_res, ConstantsDB& db, POINT pt,
                RisohSettings& settings)
        : MDialogBase(IDD_ADDCTRL), m_dialog_res(dialog_res),
          m_db(db), m_bUpdating(FALSE), m_pt(pt), m_settings(settings)
    {
        m_himlControls = NULL;
    }

    ~MAddCtrlDlg()
    {
        if (m_himlControls)
        {
            ImageList_Destroy(m_himlControls);
            m_himlControls = NULL;
        }
    }

    void InitTables(LPCTSTR pszClass)
    {
        ConstantsDB::TableType table;

        m_style_table.clear();
        if (pszClass && pszClass[0])
        {
            table = m_db.GetTable(pszClass);
            if (table.size())
            {
                m_style_table.insert(m_style_table.end(),
                    table.begin(), table.end());
            }
        }
        table = m_db.GetTable(TEXT("STYLE"));
        if (table.size())
        {
            m_style_table.insert(m_style_table.end(),
                table.begin(), table.end());
        }
        m_style_selection.resize(m_style_table.size());

        m_exstyle_table.clear();
        table = m_db.GetTable(TEXT("EXSTYLE"));
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

    void InitToolBar()
    {
        std::vector<TBBUTTON> buttons;

        ConstantsDB::TableType table = m_db.GetTable(TEXT("CONTROLS.ICONS"));
        size_t count = table.size();
        INT nCount = INT(count);

        m_vecControls.clear();
        if (m_himlControls)
        {
            ImageList_Destroy(m_himlControls);
            m_himlControls = NULL;
        }
        m_himlControls = ImageList_LoadBitmap(
            GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CONTROLS),
            16, 0, RGB(255, 0, 255));
        m_hTB.SetImageList(m_himlControls);

        buttons.resize(nCount);
        for (INT i = 0; i < nCount; ++i)
        {
            buttons[i].iBitmap = i;
            buttons[i].idCommand = i + 1000;
            buttons[i].fsState = TBSTATE_ENABLED;
            buttons[i].fsStyle = TBSTYLE_BUTTON;
            buttons[i].iString = 0;
            m_vecControls.push_back(table[i].name);
        }
        m_hTB.AddButtons(nCount, &buttons[0]);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SubclassChildDx(m_hTB, ctl1);
        InitToolBar();

        SetDlgItemInt(hwnd, edt1, m_pt.x, FALSE);
        SetDlgItemInt(hwnd, edt2, m_pt.y, FALSE);

        INT cx = m_db.GetValue(TEXT("CONTROL.SIZE"), TEXT("WIDTH"));
        INT cy = m_db.GetValue(TEXT("CONTROL.SIZE"), TEXT("HEIGHT"));
        SetDlgItemInt(hwnd, edt3, cx, FALSE);
        SetDlgItemInt(hwnd, edt4, cy, FALSE);

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        InitClassComboBox(hCmb1, m_db, TEXT(""));
        SubclassChildDx(m_cmb1, cmb1);
        EnableWindow(GetDlgItem(hwnd, psh3), FALSE);

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        InitCaptionComboBox(hCmb2, m_settings, TEXT(""));
        SubclassChildDx(m_cmb2, cmb2);

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitCtrlIDComboBox(hCmb3, m_db);
        if (m_db.DoesUseIDC_STATIC())
            SetDlgItemText(hwnd, cmb3, TEXT("IDC_STATIC"));
        else
            SetDlgItemText(hwnd, cmb3, TEXT("-1"));
        SubclassChildDx(m_cmb3, cmb3);

        HWND hCmb4 = GetDlgItem(hwnd, cmb4);
        InitWndClassComboBox(hCmb4, m_db, TEXT(""));
        SubclassChildDx(m_cmb4, cmb4);

        HWND hCmb5 = GetDlgItem(hwnd, cmb5);
        InitResNameComboBox(hCmb5, m_db, WORD(0), IDTYPE_HELP);
        SetDlgItemInt(hwnd, cmb5, 0, FALSE);
        SubclassChildDx(m_cmb5, cmb5);

        InitTables(NULL);

        TCHAR szText[64];

        HWND hLst1 = GetDlgItem(hwnd, lst1);
        m_dwStyle = WS_VISIBLE | WS_CHILD;
        GetStyleSelect(m_style_selection, m_style_table, m_dwStyle);
        InitStyleListBox(hLst1, m_style_table);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        StringCchPrintf(szText, _countof(szText), TEXT("%08lX"), m_dwStyle);
        SetDlgItemText(hwnd, edt6, szText);
        SendDlgItemMessage(hwnd, edt6, EM_SETLIMITTEXT, 8, 0);
        m_bUpdating = FALSE;

        HWND hLst2 = GetDlgItem(hwnd, lst2);
        m_dwExStyle = 0;
        GetStyleSelect(m_exstyle_selection, m_exstyle_table, m_dwExStyle);
        InitStyleListBox(hLst2, m_exstyle_table);
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, m_dwExStyle);

        m_bUpdating = TRUE;
        StringCchPrintf(szText, _countof(szText), TEXT("%08lX"), m_dwExStyle);
        SetDlgItemText(hwnd, edt7, szText);
        SendDlgItemMessage(hwnd, edt7, EM_SETLIMITTEXT, 8, 0);
        m_bUpdating = FALSE;

        if (!m_dialog_res.IsExtended())
            EnableWindow(GetDlgItem(hwnd, psh1), FALSE);

        SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));
        SendDlgItemMessage(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));
        SendDlgItemMessage(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));
        SendDlgItemMessage(hwnd, scr4, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        MString strCaption = GetDlgItemText(cmb2);
        mstr_trim(strCaption);
        m_settings.AddCaption(strCaption.c_str());
        if (strCaption[0] == TEXT('"'))
        {
            mstr_unquote(strCaption);
        }

        INT x = GetDlgItemInt(hwnd, edt1, NULL, TRUE);
        INT y = GetDlgItemInt(hwnd, edt2, NULL, TRUE);
        INT cx = GetDlgItemInt(hwnd, edt3, NULL, TRUE);
        INT cy = GetDlgItemInt(hwnd, edt4, NULL, TRUE);
        MString strID = GetDlgItemText(cmb3);
        mstr_trim(strID);
        WORD id;
        if ((TEXT('0') <= strID[0] && strID[0] <= TEXT('9')) ||
            strID[0] == TEXT('-') || strID[0] == TEXT('+'))
        {
            id = (WORD)mstr_parse_int(strID.c_str());
        }
        else if (m_db.HasResID(strID))
        {
            id = (WORD)m_db.GetResIDValue(strID);
        }
        else if (m_db.HasCtrlID(strID))
        {
            id = (WORD)m_db.GetCtrlIDValue(strID);
        }
        else
        {
            HWND hCmb3 = GetDlgItem(hwnd, cmb3);
            ComboBox_SetEditSel(hCmb3, 0, -1);
            SetFocus(hCmb3);
            ErrorBoxDx(IDS_NOSUCHID);
            return;
        }

        MString strClass = GetDlgItemText(cmb4);
        mstr_trim(strClass);
        if (!IsThereWndClass(strClass.c_str()))
        {
            SetFocus(GetDlgItem(hwnd, cmb4));
            MsgBoxDx(LoadStringDx(IDS_ENTERCLASS), MB_ICONERROR);
            return;
        }

        MString strHelp = GetDlgItemText(cmb5);
        ReplaceFullWithHalf(strHelp);
        mstr_trim(strHelp);
        DWORD help;
        if (m_db.HasResID(strHelp))
        {
            help = m_db.GetResIDValue(strHelp);
        }
        else
        {
            help = mstr_parse_int(strHelp.c_str(), false);
        }

        MString strStyle = GetDlgItemText(edt6);
        ReplaceFullWithHalf(strStyle);
        mstr_trim(strStyle);
        DWORD style = mstr_parse_int(strStyle.c_str(), false, 16);

        MString strExStyle = GetDlgItemText(edt7);
        mstr_trim(strExStyle);
        DWORD exstyle = mstr_parse_int(strExStyle.c_str(), false, 16);

        DialogItem item;
        item.m_help_id = help;
        item.m_style = style;
        item.m_ex_style = exstyle;
        item.m_pt.x = x;
        item.m_pt.y = y;
        item.m_siz.cx = cx;
        item.m_siz.cy = cy;
        item.m_id = id;
        item.m_class = strClass.c_str();
        item.m_title = strCaption.c_str();
        item.m_extra = m_data;

        if (lstrcmpiW(strClass.c_str(), L"STATIC") == 0)
        {
            if ((style & SS_TYPEMASK) == SS_ICON ||
                (style & SS_TYPEMASK) == SS_BITMAP)
            {
                if (mchr_is_digit(strCaption[0]))
                {
                    LONG n = mstr_parse_int(strCaption.c_str());
                    item.m_title = WORD(n);
                }
            }
        }

        m_dialog_res.m_cItems++;
        m_dialog_res.m_items.push_back(item);

        for (size_t i = 0; i < m_dialog_res.m_dlginit.size(); ++i)
        {
            DlgInitEntry& entry = m_dialog_res.m_dlginit[i];
            if (entry.wCtrl == WORD(-1))
                entry.wCtrl = id;
        }

        for (size_t k = 0; k < m_dialog_res.size(); ++k)
        {
            DialogItem& item = m_dialog_res[k];
            if (item.m_id != id)
                continue;

            for (size_t i = 0; i < m_dialog_res.m_dlginit.size(); ++i)
            {
                DlgInitEntry& entry = m_dialog_res.m_dlginit[i];
                if (entry.wCtrl == id || entry.wCtrl == WORD(-1))
                {
                    entry.wCtrl = id;
                    entry.wMsg = CB_ADDSTRING;
                }
            }
        }

        EndDialog(IDOK);
    }

    void OnLst1(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        HWND hLst1 = GetDlgItem(hwnd, lst1);

        std::vector<BYTE> old_style_selection = m_style_selection;
        GetStyleSelect(hLst1, m_style_selection);

        m_dwStyle = AnalyseStyleDiff(m_dwStyle, m_style_table,
                                     old_style_selection, m_style_selection);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        StringCchPrintf(szText, _countof(szText), TEXT("%08lX"), m_dwStyle);
        SetDlgItemText(hwnd, edt6, szText);
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

    void UpdateClass(HWND hwnd, HWND hLst1, const MString& strClass)
    {
        DWORD dwType = m_db.GetValue(TEXT("CONTROL.CLASSES"), strClass);
        if (dwType == 0)
            return;

        MString strSuper;
        if (dwType >= 3)
        {
            ConstantsDB::TableType table;
            table = m_db.GetTable(strClass + TEXT(".SUPERCLASS"));
            if (table.size())
            {
                strSuper = table[0].name;
            }
        }

        if (strSuper.size())
            InitTables(strSuper.c_str());
        else
            InitTables(strClass.c_str());

        MString str = strClass + TEXT(".DEFAULT.STYLE");
        m_dwStyle = m_db.GetValue(str, TEXT("STYLE"));

        GetStyleSelect(m_style_selection, m_style_table, m_dwStyle);
        InitStyleListBox(hLst1, m_style_table);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        StringCchPrintf(szText, _countof(szText), TEXT("%08lX"), m_dwStyle);
        SetDlgItemText(hwnd, edt6, szText);
        m_bUpdating = FALSE;

        ListBox_SetTopIndex(hLst1, 0);

        INT cx, cy;
        cx = m_db.GetValue(strClass + TEXT(".SIZE"), TEXT("WIDTH"));
        cy = m_db.GetValue(strClass + TEXT(".SIZE"), TEXT("HEIGHT"));
        if (cx == 0 && cy == 0)
        {
            cx = m_db.GetValue(strSuper + TEXT(".SIZE"), TEXT("WIDTH"));
            cy = m_db.GetValue(strSuper + TEXT(".SIZE"), TEXT("HEIGHT"));
        }
        if (cx == 0 && cy == 0)
        {
            cx = m_db.GetValue(TEXT("CONTROL.SIZE"), TEXT("WIDTH"));
            cy = m_db.GetValue(TEXT("CONTROL.SIZE"), TEXT("HEIGHT"));
        }
        if (lstrcmpi(strClass.c_str(), TEXT("COMBOBOX")) == 0 ||
            lstrcmpi(strSuper.c_str(), TEXT("COMBOBOX")) == 0 ||
            lstrcmpi(strClass.c_str(), WC_COMBOBOXEX) == 0 ||
            lstrcmpi(strSuper.c_str(), WC_COMBOBOXEX) == 0)
        {
            cy = m_settings.nComboHeight;
        }
        SetDlgItemInt(hwnd, edt3, cx, FALSE);
        SetDlgItemInt(hwnd, edt4, cy, FALSE);

        if (strSuper.size())
            SetDlgItemText(hwnd, cmb4, strSuper.c_str());
        else
            SetDlgItemText(hwnd, cmb4, strClass.c_str());

        if (strClass == L"COMBOBOX" || strClass == L"LISTBOX" ||
            strClass == L"ComboBoxEx32")
        {
            EnableWindow(GetDlgItem(hwnd, psh3), TRUE);
        }
        else
        {
            EnableWindow(GetDlgItem(hwnd, psh3), FALSE);
        }
    }

    void OnPsh1(HWND hwnd)
    {
        MCtrlDataDlg dialog(m_data);
        dialog.DialogBoxDx(hwnd);
    }

    void OnPsh2(HWND hwnd)
    {
        OLEUIINSERTOBJECT insert_object;
        TCHAR szFile[MAX_PATH] = { 0 };

        ZeroMemory(&insert_object, sizeof(insert_object));
        insert_object.cbStruct = sizeof(insert_object);
        insert_object.dwFlags = IOF_DISABLEDISPLAYASICON | IOF_SELECTCREATENEW | IOF_DISABLELINK;
        insert_object.hWndOwner = hwnd;
        insert_object.lpszCaption = LoadStringDx(IDS_CHOOSE_OLE_CLSID);
        insert_object.iid = IID_IOleObject;
        insert_object.lpszFile = szFile;
        insert_object.cchFile = _countof(szFile);

        UINT uResult = OleUIInsertObject(&insert_object);
        if (uResult == OLEUI_OK)
        {
            LPOLESTR pszCLSID = NULL;
            if (S_OK == StringFromCLSID(insert_object.clsid, &pszCLSID))
            {
                if (GetDlgItemText(hwnd, cmb4).find(TEXT("AtlAxWin")) == 0)
                {
                    WCHAR szText[64];
                    StringCchCopyW(szText, _countof(szText), L"CLSID:");
                    StringCchCatW(szText, _countof(szText), pszCLSID);
                    SetDlgItemTextW(hwnd, cmb2, szText);
                }
                else
                {
                    SetDlgItemTextW(hwnd, cmb2, pszCLSID);
                }
                SetDlgItemTextW(hwnd, cmb4, m_settings.strAtlAxWin.c_str());
                CoTaskMemFree(pszCLSID);
            }
        }
    }

    void OnPsh3(HWND hwnd)
    {
        MStringListDlg dialog(m_dialog_res);
        dialog.DialogBoxDx(hwnd);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        HWND hCmb4 = GetDlgItem(hwnd, cmb4);
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
                UpdateClass(hwnd, hLst1, text);
            }
            else if (codeNotify == CBN_EDITCHANGE)
            {
                DWORD dwPos;
                m_cmb1.OnEditChange(dwPos);
                {
                    MString text = GetDlgItemText(hwnd, cmb1);
                    mstr_trim(text);
                    InitTables(text.c_str());
                    UpdateClass(hwnd, hLst1, text);
                }
                m_cmb1.SetEditSel(LOWORD(dwPos), -1);
            }
            break;
        case cmb2:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb2.OnEditChange();
            }
            break;
        case cmb3:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb3.OnEditChange();
            }
            break;
        case cmb4:
            if (codeNotify == CBN_SELCHANGE)
            {
                INT nIndex = ComboBox_GetCurSel(hCmb4);
                ComboBox_GetLBText(hCmb4, nIndex, szText);
                MString text = szText;
                mstr_trim(text);
                UpdateClass(hwnd, hLst1, text);
            }
            else if (codeNotify == CBN_EDITCHANGE)
            {
                DWORD dwPos;
                m_cmb4.OnEditChange(dwPos);
                {
                    MString text = GetDlgItemText(hwnd, cmb4);
                    mstr_trim(text);
                    InitTables(text.c_str());
                    UpdateClass(hwnd, hLst1, text);
                }
                m_cmb4.SetEditSel(LOWORD(dwPos), -1);
            }
            break;
        case cmb5:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb5.OnEditChange();
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
        case psh1:
            OnPsh1(hwnd);
            break;
        case psh2:
            OnPsh2(hwnd);
            break;
        case psh3:
            OnPsh3(hwnd);
            break;
        default:
            if (size_t(id - 1000) < m_vecControls.size())
            {
                MString text;
                UINT nID = UINT(id - 1000);
                if (nID == m_db.GetValue(TEXT("CONTROLS.OLE.CONTROL"), TEXT("INDEX")))
                {
                    // OLE controls
                    text = m_settings.strAtlAxWin;
                    SetDlgItemTextW(hwnd, cmb4, text.c_str());
                }
                else
                {
                    text = m_vecControls[nID];
                    SetDlgItemTextW(hwnd, cmb1, text.c_str());
                }
                mstr_trim(text);
                InitTables(text.c_str());
                UpdateClass(hwnd, hLst1, text);
            }
            break;
        }
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
    {
        if (pnmhdr->code == TTN_NEEDTEXT)
        {
            TOOLTIPTEXT *ttt = (TOOLTIPTEXT *)pnmhdr;
            UINT nID = UINT(pnmhdr->idFrom - 1000);
            if (size_t(nID) < m_vecControls.size())
            {
                MString text;
                if (nID == m_db.GetValue(TEXT("CONTROLS.OLE.CONTROL"), TEXT("INDEX")))
                {
                    // OLE controls
                    text = m_settings.strAtlAxWin;
                }
                else
                {
                    text = m_vecControls[nID];
                }
                StringCchCopyW(ttt->szText, _countof(ttt->szText), text.c_str());
            }
        }
        return 0;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MADDCTRLDLG_HPP_
