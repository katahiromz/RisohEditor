// MCtrlPropDlg.hpp --- "Properties for Control" Dialog
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

#pragma once

#include "resource.h"
#include "MToolBarCtrl.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "MCtrlDataDlg.hpp"
#include "MStringListDlg.hpp"
#include "DialogRes.hpp"
#include "MString.hpp"
#include "Common.hpp"

#include <unordered_set>     // for std::unordered_set
#include <oledlg.h>

//////////////////////////////////////////////////////////////////////////////

class MCtrlPropDlg : public MDialogBase
{
public:
    enum Flags
    {
        F_NONE = 0,
        F_HELP = 0x0001,
        F_STYLE = 0x0002,
        F_EXSTYLE = 0x0004,
        F_X = 0x0008,
        F_Y = 0x0010,
        F_CX = 0x0020,
        F_CY = 0x0040,
        F_ID = 0x0080,
        F_CLASS = 0x0100,
        F_TITLE = 0x0200,
        F_EXTRA = 0x0400,
        F_SLIST = 0x0800,
        F_ALL = 0x0FFF
    };
    DialogRes&          m_dialog_res;
    BOOL                m_bUpdating;
    std::unordered_set<INT>       m_indeces;
    DWORD               m_flags;
    DWORD               m_dwStyle;
    DWORD               m_dwExStyle;
    DialogItem          m_item;
    ConstantsDB::TableType  m_style_table;
    ConstantsDB::TableType  m_exstyle_table;
    std::vector<BYTE>       m_style_selection;
    std::vector<BYTE>       m_exstyle_selection;
    MToolBarCtrl            m_hTB;
    HIMAGELIST              m_himlControls;
    std::vector<std::wstring> m_vecControls;
    MComboBoxAutoComplete m_cmb1;
    MComboBoxAutoComplete m_cmb2;
    MComboBoxAutoComplete m_cmb3;
    MComboBoxAutoComplete m_cmb4;
    MComboBoxAutoComplete m_cmb5;

    MCtrlPropDlg(DialogRes& dialog_res, const std::unordered_set<INT>& indeces)
        : MDialogBase(IDD_CTRLPROP), m_dialog_res(dialog_res),
          m_bUpdating(FALSE), m_indeces(indeces)
    {
        m_himlControls = NULL;
        m_cmb2.m_bAcceptSpace = TRUE;
        m_cmb4.m_bAcceptSpace = TRUE;
    }

    ~MCtrlPropDlg()
    {
        if (m_himlControls)
        {
            ImageList_Destroy(m_himlControls);
            m_himlControls = NULL;
        }
    }

    void GetInfo()
    {
        if (m_indeces.empty())
            return;

        m_flags = F_ALL;
        auto end = m_indeces.end();
        auto it = m_indeces.begin();
        {
            DialogItem& item = m_dialog_res[*it];
            m_item = item;
        }
        for (++it; it != end; ++it)
        {
            DialogItem& item = m_dialog_res[*it];
            if (m_item.m_help_id != item.m_help_id)
                m_flags &= ~F_HELP;
            if (m_item.m_style != item.m_style)
                m_flags &= ~F_STYLE;
            if (m_item.m_ex_style != item.m_ex_style)
                m_flags &= ~F_EXSTYLE;
            if (m_item.m_pt.x != item.m_pt.x)
                m_flags &= ~F_X;
            if (m_item.m_pt.y != item.m_pt.y)
                m_flags &= ~F_Y;
            if (m_item.m_siz.cx != item.m_siz.cx)
                m_flags &= ~F_CX;
            if (m_item.m_siz.cy != item.m_siz.cy)
                m_flags &= ~F_CY;
            if (m_item.m_id != item.m_id)
                m_flags &= ~F_ID;
            if (m_item.m_class != item.m_class)
                m_flags &= ~F_CLASS;
            if (m_item.m_title != item.m_title)
                m_flags &= ~F_TITLE;
        }

        if (m_flags & F_CLASS)
        {
            if (m_item.m_class.is_int())
            {
                std::wstring cls;
                if (IDToPredefClass(m_item.m_class.m_id, cls))
                    m_item.m_class = cls.c_str();
            }
        }
    }

    DWORD GetItemAndFlags(DialogItem& item)
    {
        DWORD flags = m_flags;

        MString strCaption = GetDlgItemText(cmb2);
        if (!strCaption.empty())
            flags |= F_TITLE;
        g_settings.AddCaption(strCaption.c_str());
        if (strCaption[0] == TEXT('"'))
            mstr_unquote(strCaption);
        item.m_title = strCaption.c_str();

        MString strX = GetDlgItemText(edt1);
        mstr_trim(strX);
        if (!strX.empty())
            flags |= F_X;
        item.m_pt.x = mstr_parse_int(strX.c_str());

        MString strY = GetDlgItemText(edt2);
        mstr_trim(strY);
        if (!strY.empty())
            flags |= F_Y;
        item.m_pt.y = mstr_parse_int(strY.c_str());

        MString strCX = GetDlgItemText(edt3);
        mstr_trim(strCX);
        if (!strCX.empty())
            flags |= F_CX;
        item.m_siz.cx = mstr_parse_int(strCX.c_str());

        MString strCY = GetDlgItemText(edt4);
        mstr_trim(strCY);
        if (!strCY.empty())
            flags |= F_CY;
        item.m_siz.cy = mstr_parse_int(strCY.c_str());

        MString strID = GetDlgItemText(cmb3);
        mstr_trim(strID);
        UINT id = 0;
        if ((TEXT('0') <= strID[0] && strID[0] <= TEXT('9')) ||
            strID[0] == TEXT('-') || strID[0] == TEXT('+'))
        {
            id = mstr_parse_int(strID.c_str());
            flags |= F_ID;
        }
        else if (g_db.HasCtrlID(strID))
        {
            id = g_db.GetCtrlIDValue(strID);
            flags |= F_ID;
        }
        else if (g_db.HasResID(strID))
        {
            id = g_db.GetResIDValue(strID);
            flags |= F_ID;
        }
        else if (!strID.empty() && m_indeces.size() <= 1)
        {
            HWND hCmb3 = GetDlgItem(m_hwnd, cmb3);
            ComboBox_SetEditSel(hCmb3, 0, -1);
            SetFocus(hCmb3);
            ErrorBoxDx(IDS_NOSUCHID);
            return 0xFFFFFFFF;
        }
        if (flags & F_ID)
            item.m_id = (WORD)id;
        else
            item.m_id = 0;

        MString strClass = GetDlgItemText(cmb4);
        mstr_trim(strClass);
        if (strClass[0] == TEXT('"'))
        {
            mstr_unquote(strClass);
        }
        if (!strClass.empty())
            flags |= F_CLASS;
        item.m_class = strClass.c_str();

        MString strHelp = GetDlgItemText(cmb5);
        ReplaceFullWithHalf(strHelp);
        mstr_trim(strHelp);
        if (!strHelp.empty())
            flags |= F_HELP;
        if (g_db.HasResID(strHelp))
        {
            item.m_help_id = g_db.GetResIDValue(strHelp);
        }
        else
        {
            item.m_help_id = mstr_parse_int(strHelp.c_str());
        }

        MString strStyle = GetDlgItemText(edt6);
        ReplaceFullWithHalf(strStyle);
        mstr_trim(strStyle);
        if (!strStyle.empty())
            flags |= F_STYLE;
        item.m_style = mstr_parse_int(strStyle.c_str(), false, 16);

        MString strExStyle = GetDlgItemText(edt7);
        mstr_trim(strExStyle);
        if (!strExStyle.empty())
            flags |= F_EXSTYLE;
        item.m_ex_style = mstr_parse_int(strExStyle.c_str(), false, 16);

        flags |= F_EXTRA;

        if (!item.m_str_list.empty())
            flags |= F_SLIST;

        return flags;
    }

    BOOL SetInfo(DWORD flags)
    {
        if (m_indeces.empty())
            return TRUE;

        for (auto& index : m_indeces)
        {
            DialogItem& item = m_dialog_res[index];
            if ((m_flags & F_HELP) || (flags & F_HELP))
                item.m_help_id = m_item.m_help_id;
            if ((m_flags & F_STYLE) || (flags & F_STYLE))
                item.m_style = m_item.m_style;
            if ((m_flags & F_EXSTYLE) || (flags & F_EXSTYLE))
                item.m_ex_style = m_item.m_ex_style;
            if ((m_flags & F_X) || (flags & F_X))
                item.m_pt.x = m_item.m_pt.x;
            if ((m_flags & F_Y) || (flags & F_Y))
                item.m_pt.y = m_item.m_pt.y;
            if ((m_flags & F_CX) || (flags & F_CX))
                item.m_siz.cx = m_item.m_siz.cx;
            if ((m_flags & F_CY) || (flags & F_CY))
                item.m_siz.cy = m_item.m_siz.cy;
            if ((m_flags & F_ID) || (flags & F_ID))
                item.m_id = m_item.m_id;
            if ((m_flags & F_CLASS) || (flags & F_CLASS))
            {
                item.m_class = m_item.m_class;
                if (!IsThereWndClass(item.m_class.c_str()))
                {
                    HWND hCmb4 = GetDlgItem(m_hwnd, cmb4);
                    ComboBox_SetEditSel(hCmb4, 0, -1);
                    SetFocus(hCmb4);
                    ErrorBoxDx(IDS_ENTERCLASS);
                    return FALSE;
                }
            }
            if ((m_flags & F_TITLE) || (flags & F_TITLE))
            {
                item.m_title = m_item.m_title;
            }
            if ((m_flags & F_EXTRA) || (flags & F_EXTRA))
            {
                item.m_extra = m_item.m_extra;
            }
            if ((m_flags & F_SLIST) || (flags & F_SLIST))
            {
                if (m_item.IsStdComboBox() || m_item.IsListBox() || m_item.IsExtComboBox())
                {
                    item.m_str_list = m_item.m_str_list;
                }
                else
                {
                    item.m_str_list.clear();
                }
            }
            if (lstrcmpiW(item.m_class.c_str(), L"STATIC") == 0)
            {
                DWORD style = item.m_style;
                if ((style & SS_TYPEMASK) == SS_ICON ||
                    (style & SS_TYPEMASK) == SS_BITMAP)
                {
                    if (mchr_is_digit(item.m_title.str()[0]))
                    {
                        LONG n = mstr_parse_int(item.m_title.c_str());
                        item.m_title = WORD(n);
                    }
                }
            }
        }

        return TRUE;
    }

    void InitTables(LPCTSTR pszClass)
    {
        m_style_table.clear();
        if (pszClass && pszClass[0])
        {
            auto table = g_db.GetTable(pszClass);
            if (table.size())
            {
                m_style_table.insert(m_style_table.end(),
                    table.begin(), table.end());
            }
        }
        auto table = g_db.GetTable(TEXT("STYLE"));
        if (table.size())
        {
            m_style_table.insert(m_style_table.end(),
                table.begin(), table.end());
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

        ConstantsDB::TableType table = g_db.GetTable(TEXT("CONTROLS.ICONS"));
        size_t count = table.size();
        UINT nCount = UINT(count);

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
        for (UINT i = 0; i < nCount; ++i)
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

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        InitClassComboBox(hCmb1, TEXT(""));
        SubclassChildDx(m_cmb1, cmb1);

        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        InitCaptionComboBox(hCmb2, TEXT(""));
        SubclassChildDx(m_cmb2, cmb2);

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitCtrlIDComboBox(hCmb3);
        SubclassChildDx(m_cmb3, cmb3);

        HWND hCmb5 = GetDlgItem(hwnd, cmb5);
        InitResNameComboBox(hCmb5, BAD_NAME, IDTYPE_HELP);
        SubclassChildDx(m_cmb5, cmb5);

        GetInfo();

        if (m_flags & F_CLASS)
        {
            InitTables(m_item.m_class.c_str());
        }
        else
        {
            InitTables(NULL);
        }

        HWND hCmb4 = GetDlgItem(hwnd, cmb4);
        InitWndClassComboBox(hCmb4, m_item.m_class.c_str());
        SubclassChildDx(m_cmb4, cmb4);

        TCHAR szText[64];

        HWND hLst1 = GetDlgItem(hwnd, lst1);
        if (m_flags & F_STYLE)
            m_dwStyle = m_item.m_style;
        else
            m_dwStyle = 0;
        GetStyleSelect(m_style_selection, m_style_table, m_dwStyle);
        InitStyleListBox(hLst1, m_style_table);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        if (m_flags & F_STYLE)
        {
            m_bUpdating = TRUE;
            StringCchPrintf(szText, _countof(szText), TEXT("%08lX"), m_dwStyle);
            SetDlgItemText(hwnd, edt6, szText);
            SendDlgItemMessage(hwnd, edt6, EM_SETLIMITTEXT, 8, 0);
            m_bUpdating = FALSE;
        }

        HWND hLst2 = GetDlgItem(hwnd, lst2);
        if (m_flags & F_EXSTYLE)
            m_dwExStyle = m_item.m_ex_style;
        else
            m_dwExStyle = 0;
        GetStyleSelect(m_exstyle_selection, m_exstyle_table, m_dwExStyle);
        InitStyleListBox(hLst2, m_exstyle_table);
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, m_dwExStyle);

        if (m_flags & F_EXSTYLE)
        {
            m_bUpdating = TRUE;
            StringCchPrintf(szText, _countof(szText), TEXT("%08lX"), m_dwExStyle);
            SetDlgItemText(hwnd, edt7, szText);
            SendDlgItemMessage(hwnd, edt7, EM_SETLIMITTEXT, 8, 0);
            m_bUpdating = FALSE;
        }

        if (m_flags & F_HELP)
        {
            MStringW name = g_db.GetNameOfResID(IDTYPE_HELP, m_item.m_help_id);
            SetDlgItemTextW(hwnd, cmb5, name.c_str());
        }
        if (m_flags & F_X)
        {
            SetDlgItemInt(hwnd, edt1, m_item.m_pt.x, TRUE);
        }
        if (m_flags & F_Y)
        {
            SetDlgItemInt(hwnd, edt2, m_item.m_pt.y, TRUE);
        }
        if (m_flags & F_CX)
        {
            SetDlgItemInt(hwnd, edt3, m_item.m_siz.cx, TRUE);
        }
        if (m_flags & F_CY)
        {
            SetDlgItemInt(hwnd, edt4, m_item.m_siz.cy, TRUE);
        }
        if (m_flags & F_ID)
        {
            MStringW name = g_db.GetNameOfResID(IDTYPE_CONTROL, m_item.m_id);
            SetDlgItemTextW(hwnd, cmb3, name.c_str());
        }
        if (m_flags & F_CLASS)
        {
            SetDlgItemText(hwnd, cmb4, m_item.m_class.c_str());
        }
        if (m_flags & F_TITLE)
        {
            MString strCaption;
            if (!m_item.m_title.empty())
            {
                if (m_item.m_title.is_int())
                    strCaption = m_item.m_title.c_str();
                else
                    strCaption = m_item.m_title.quoted_wstr();
            }
            SetDlgItemText(hwnd, cmb2, strCaption.c_str());
        }

        if (!m_dialog_res.IsExtended())
            EnableWindow(GetDlgItem(hwnd, psh1), FALSE);

        SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));
        SendDlgItemMessage(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));
        SendDlgItemMessage(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));
        SendDlgItemMessage(hwnd, scr4, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));

        if (!(m_flags & F_X))
        {
            SetDlgItemTextW(hwnd, edt1, NULL);
        }
        if (!(m_flags & F_Y))
        {
            SetDlgItemTextW(hwnd, edt2, NULL);
        }
        if (!(m_flags & F_CX))
        {
            SetDlgItemTextW(hwnd, edt3, NULL);
        }
        if (!(m_flags & F_CY))
        {
            SetDlgItemTextW(hwnd, edt4, NULL);
        }
        if (!m_item.IsStdComboBox() && !m_item.IsListBox() && !m_item.IsExtComboBox())
        {
            EnableWindow(GetDlgItem(hwnd, psh3), FALSE);
        }

        CenterWindowDx();
        SetFocus(hCmb2);
        return FALSE;
    }

    void OnOK(HWND hwnd)
    {
        DWORD flags = GetItemAndFlags(m_item);
        if (flags == 0xFFFFFFFF)
        {
            return;
        }
        if (flags & F_CLASS)
        {
            MString strClass = GetDlgItemText(cmb4);
            mstr_trim(strClass);
            if (strClass[0] == TEXT('"'))
            {
                mstr_unquote(strClass);
            }
            if (strClass.empty())
            {
                SetFocus(GetDlgItem(hwnd, cmb4));
                MsgBoxDx(LoadStringDx(IDS_ENTERCLASS), MB_ICONERROR);
                return;
            }
        }

        if (!SetInfo(flags))
            return;

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

        GetStyleSelect(m_exstyle_selection, m_exstyle_table, dwExStyle);

        HWND hLst2 = GetDlgItem(hwnd, lst2);
        m_dwExStyle = dwExStyle;
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, dwExStyle);
    }

    void UpdateClass(HWND hwnd, HWND hLst1, const MString& strClass)
    {
        MString strSuper;
        DWORD dwType = g_db.GetValue(TEXT("CONTROL.CLASSES"), strClass);
        if (dwType >= 3)
        {
            ConstantsDB::TableType table;
            table = g_db.GetTable(strClass + TEXT(".SUPERCLASS"));
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
        m_dwStyle = g_db.GetValue(str, TEXT("STYLE"));

        GetStyleSelect(m_style_selection, m_style_table, m_dwStyle);
        InitStyleListBox(hLst1, m_style_table);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        StringCchPrintf(szText, _countof(szText), TEXT("%08lX"), m_dwStyle);
        SetDlgItemText(hwnd, edt6, szText);
        m_bUpdating = FALSE;

        ListBox_SetTopIndex(hLst1, 0);

        if (strSuper.size())
            SetDlgItemText(hwnd, cmb4, strSuper.c_str());
        else
            SetDlgItemText(hwnd, cmb4, strClass.c_str());

        m_item.m_class = strClass.c_str();
        if (m_item.IsStdComboBox() || m_item.IsListBox() ||
            m_item.IsExtComboBox())
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
        MCtrlDataDlg dialog(m_item.m_extra);
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
                SetDlgItemTextW(hwnd, cmb2, NULL);
                SetDlgItemTextW(hwnd, cmb4, pszCLSID);
                CoTaskMemFree(pszCLSID);
            }
        }
    }

    void OnPsh3(HWND hwnd)
    {
        MStringListDlg dialog(m_item.m_str_list);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            m_flags |= F_SLIST;
        }
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        HWND hCmb4 = GetDlgItem(hwnd, cmb4);
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
                MString text = GetComboBoxLBText(hCmb1, nIndex);
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
                MString text = GetComboBoxLBText(hCmb4, nIndex);
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
                if (nID == g_db.GetValue(TEXT("CONTROLS.OLE.CONTROL"), TEXT("INDEX")))
                {
                    // OLE controls
                    text = g_settings.strAtlAxWin;
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
                if (nID == g_db.GetValue(TEXT("CONTROLS.OLE.CONTROL"), TEXT("INDEX")))
                {
                    // OLE controls
                    text = g_settings.strAtlAxWin;
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
