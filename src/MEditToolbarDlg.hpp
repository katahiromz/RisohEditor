// MEditToolbarDlg.hpp --- Dialogs for edit of TOOLBAR resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "ToolbarRes.hpp"
#include "Common.hpp"

class MModifyTBBtnDlg;
class MEditToolbarDlg;

//////////////////////////////////////////////////////////////////////////////

class MModifyTBBtnDlg : public MDialogBase
{
public:
    std::wstring m_str;
    MComboBoxAutoComplete m_cmb1;

    MModifyTBBtnDlg(INT id, const std::wstring& str = L"") : MDialogBase(id), m_str(str)
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        InitCtrlIDComboBox(GetDlgItem(hwnd, cmb1));
        SubclassChildDx(m_cmb1, cmb1);

        SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, -1, 0);
        if (m_str[0] == L'-')
        {
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
            SetDlgItemTextW(hwnd, cmb1, L"");
        }
        else
        {
            SetDlgItemTextW(hwnd, cmb1, m_str.c_str());
        }

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        WCHAR szText[MAX_PATH];
        GetDlgItemTextW(hwnd, cmb1, szText, _countof(szText));
        ReplaceFullWithHalf(szText);
        mstr_trim(szText);

        if (szText[0] == L'-')
            szText[0] = 0;

        if (szText[0] && !CheckCommand(szText))
        {
            ErrorBoxDx(IDS_NOSUCHID);
            return;
        }

        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            szText[0] = 0;

        m_str = szText;
        EndDialog(IDOK);
    }

    BOOL m_bUpdating = FALSE;

    void OnChx1(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
        {
            m_bUpdating = TRUE;
            SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, -1, 0);
            SetDlgItemTextW(hwnd, cmb1, NULL);
            m_bUpdating = FALSE;
        }
    }

    void OnCmb1(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        INT iItem = ComboBox_GetCurSel(m_cmb1);
        if (iItem != CB_ERR)
            return;

        WCHAR sz1[MAX_PATH];
        sz1[0] = 0;
        ::GetDlgItemTextW(hwnd, cmb1, sz1, _countof(sz1));
        ReplaceFullWithHalf(sz1);
        mstr_trim(sz1);

        if (sz1[0])
        {
            m_bUpdating = TRUE;
            if (sz1[0] == L'-')
                CheckDlgButton(hwnd, chx1, BST_CHECKED);
            else
                CheckDlgButton(hwnd, chx1, BST_UNCHECKED);
            m_bUpdating = FALSE;
        }
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
        case chx1:
            OnChx1(hwnd);
            break;
        case cmb1:
            if (codeNotify == CBN_EDITCHANGE)
            {
                m_cmb1.OnEditChange();
            }
            OnCmb1(hwnd);
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

class MEditToolbarDlg : public MDialogBase
{
public:
    ToolbarRes& m_toolbar_res;
    HICON m_hIcon;
    HICON m_hIconSm;
    HWND m_hLst1;

    MEditToolbarDlg(ToolbarRes& toolbar_res)
        : MDialogBase(IDD_TOOLBARRES), m_toolbar_res(toolbar_res)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);
        m_hLst1 = NULL;
    }

    ~MEditToolbarDlg()
    {
        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemInt(hwnd, edt1, m_toolbar_res.width(), FALSE);
        SetDlgItemInt(hwnd, edt2, m_toolbar_res.height(), FALSE);

        SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM((WORD)SHRT_MAX, 3));
        SendDlgItemMessage(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM((WORD)SHRT_MAX, 3));

        m_hLst1 = GetDlgItem(hwnd, lst1);
        for (size_t i = 0; i < m_toolbar_res.size(); ++i)
        {
            DWORD id = m_toolbar_res[i];
            if (id != 0)
            {
                std::wstring str = g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, id, true);
                SendMessageW(m_hLst1, LB_ADDSTRING, 0, (LPARAM)str.c_str());
            }
            else
            {
                SendMessageW(m_hLst1, LB_ADDSTRING, 0, (LPARAM)L"---");
            }
        }
        SendMessageW(m_hLst1, LB_SETCURSEL, 0, 0);

        SendMessageW(m_hLst1, LB_SETITEMHEIGHT, 0, GetItemHeight(hwnd));

        SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

        CenterWindowDx();
        return TRUE;
    }

    INT GetItemHeight(HWND hwnd)
    {
        HFONT hFont = GetStockFont(DEFAULT_GUI_FONT);

        TEXTMETRIC tm;
        HDC hDC = CreateCompatibleDC(NULL);
        SelectObject(hDC, hFont);
        GetTextMetrics(hDC, &tm);
        DeleteDC(hDC);

        return tm.tmHeight * 2;
    }

    void OnAdd(HWND hwnd)
    {
        MModifyTBBtnDlg dialog(IDD_ADDTBBTN, L"");
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            auto& str = dialog.m_str;
            if (str.empty() || str[0] == L'-')
                str = L"---";
            INT iItem = (INT)SendMessageW(m_hLst1, LB_INSERTSTRING, -1, (LPARAM)str.c_str());
            SendMessageW(m_hLst1, LB_SETCURSEL, iItem, 0);
        }
    }

    void OnModify(HWND hwnd)
    {
        INT iItem = (INT)SendMessageW(m_hLst1, LB_GETCURSEL, 0, 0);
        if (iItem < 0)
            return;

        WCHAR sz1[MAX_PATH];
        sz1[0] = 0;
        SendMessageW(m_hLst1, LB_GETTEXT, iItem, (LPARAM)sz1);

        MModifyTBBtnDlg dialog(IDD_MODIFYTBBTN, sz1);
        if (dialog.DialogBoxDx(hwnd) == IDOK)
        {
            auto& str = dialog.m_str;
            if (str.empty() || str[0] == L'-')
                str = L"---";
            SendMessageW(m_hLst1, LB_DELETESTRING, iItem, 0);
            SendMessageW(m_hLst1, LB_INSERTSTRING, iItem, (LPARAM)str.c_str());
            SendMessageW(m_hLst1, LB_SETCURSEL, iItem, 0);
        }
    }

    void OnDelete(HWND hwnd)
    {
        INT iItem = (INT)SendMessageW(m_hLst1, LB_GETCURSEL, 0, 0);
        if (iItem >= 0)
        {
            SendMessageW(m_hLst1, LB_DELETESTRING, iItem, 0);
            INT nCount = (INT)SendMessageW(m_hLst1, LB_GETCOUNT, 0, 0);
            if (nCount == iItem)
                SendMessageW(m_hLst1, LB_SETCURSEL, iItem - 1, 0);
            else
                SendMessageW(m_hLst1, LB_SETCURSEL, iItem, 0);
        }
    }

    void OnUp(HWND hwnd)
    {
        INT iItem = (INT)SendMessageW(m_hLst1, LB_GETCURSEL, 0, 0);
        if (iItem > 0)
        {
            WCHAR sz1[MAX_PATH], sz2[MAX_PATH];
            sz1[0] = sz2[0] = 0;
            SendMessageW(m_hLst1, LB_GETTEXT, iItem - 1, (LPARAM)sz1);
            SendMessageW(m_hLst1, LB_GETTEXT, iItem, (LPARAM)sz2);
            SendMessageW(m_hLst1, LB_DELETESTRING, iItem - 1, 0);
            SendMessageW(m_hLst1, LB_DELETESTRING, iItem - 1, 0);
            SendMessageW(m_hLst1, LB_INSERTSTRING, iItem - 1, (LPARAM)sz1);
            SendMessageW(m_hLst1, LB_INSERTSTRING, iItem - 1, (LPARAM)sz2);
            SendMessageW(m_hLst1, LB_SETCURSEL, iItem - 1, 0);
        }
    }

    void OnDown(HWND hwnd)
    {
        INT iItem = (INT)SendMessageW(m_hLst1, LB_GETCURSEL, 0, 0);
        INT cItems = (INT)SendMessageW(m_hLst1, LB_GETCOUNT, 0, 0);
        if (iItem + 1 < cItems)
        {
            WCHAR sz1[MAX_PATH], sz2[MAX_PATH];
            sz1[0] = sz2[0] = 0;
            SendMessageW(m_hLst1, LB_GETTEXT, iItem, (LPARAM)sz1);
            SendMessageW(m_hLst1, LB_GETTEXT, iItem + 1, (LPARAM)sz2);
            SendMessageW(m_hLst1, LB_DELETESTRING, iItem, 0);
            SendMessageW(m_hLst1, LB_DELETESTRING, iItem, 0);
            SendMessageW(m_hLst1, LB_INSERTSTRING, iItem, (LPARAM)sz1);
            SendMessageW(m_hLst1, LB_INSERTSTRING, iItem, (LPARAM)sz2);
            SendMessageW(m_hLst1, LB_SETCURSEL, iItem + 1, 0);
        }
    }

    void OnOK(HWND hwnd)
    {
        INT nCount = (INT)SendMessageW(m_hLst1, LB_GETCOUNT, 0, 0);

        m_toolbar_res.clear();

        BOOL bTranslated;

        INT cx = GetDlgItemInt(hwnd, edt1, &bTranslated, FALSE);
        if (!bTranslated)
        {
            SetFocus(GetDlgItem(hwnd, edt1));
            SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
            ErrorBoxDx(IDS_ENTERINT);
            return;
        }
        INT cy = GetDlgItemInt(hwnd, edt2, &bTranslated, FALSE);
        if (!bTranslated)
        {
            SetFocus(GetDlgItem(hwnd, edt2));
            SendDlgItemMessageW(hwnd, edt2, EM_SETSEL, 0, -1);
            ErrorBoxDx(IDS_ENTERINT);
            return;
        }
        m_toolbar_res.width(cx);
        m_toolbar_res.height(cy);

        for (INT iItem = 0; iItem < nCount; ++iItem)
        {
            WCHAR sz1[MAX_PATH];
            sz1[0] = 0;
            SendMessageW(m_hLst1, LB_GETTEXT, iItem, (LPARAM)sz1);

            if (!sz1[0] || sz1[0] == L'-')
            {
                m_toolbar_res.push_back(0);
            }
            else
            {
                INT id = (INT)g_db.GetResIDValue(sz1);
                m_toolbar_res.push_back(id);
            }
        }

        EndDialog(IDOK);
    }

    void OnLst1DoubleClick(HWND hwnd)
    {
        OnModify(hwnd);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case psh1:
            OnAdd(hwnd);
            break;
        case psh2:
            OnModify(hwnd);
            break;
        case psh3:
            OnDelete(hwnd);
            break;
        case psh4:
            OnUp(hwnd);
            break;
        case psh5:
            OnDown(hwnd);
            break;
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(IDCANCEL);
            break;
        case lst1:
            switch (codeNotify)
            {
            case LBN_DBLCLK:
                OnLst1DoubleClick(hwnd);
                break;
            }
            break;
        }
    }

    void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
    {
        HDC hDC = lpDrawItem->hDC;
        RECT rcItem = lpDrawItem->rcItem;
        INT iItem = lpDrawItem->itemID;

        WCHAR sz1[MAX_PATH];
        sz1[0] = 0;
        SendMessageW(m_hLst1, LB_GETTEXT, iItem, (LPARAM)sz1);

        if (lpDrawItem->itemState & ODS_SELECTED)
        {
            FillRect(hDC, &rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
            SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
        else
        {
            FillRect(hDC, &rcItem, GetSysColorBrush(COLOR_WINDOW));
            SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
        }

        SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
        SetBkMode(hDC, TRANSPARENT);

        InflateRect(&rcItem, -4, -4);
        UINT uFormat = DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX;
        DrawText(hDC, sz1, -1, &rcItem, uFormat);
        InflateRect(&rcItem, 4, 4);

        if (lpDrawItem->itemState & ODS_FOCUS)
        {
            InflateRect(&rcItem, -1, -1);
            DrawFocusRect(hDC, &rcItem);
        }
    }

    int OnVkeyToItem(HWND hwnd, UINT vk, HWND hwndListbox, int iCaret)
    {
        if (vk == VK_DELETE)
        {
            OnDelete(hwnd);
        }
        return SetDlgMsgResult(hwnd, WM_VKEYTOITEM, -1);
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_DRAWITEM, OnDrawItem);
            HANDLE_MSG(hwnd, WM_VKEYTOITEM, OnVkeyToItem);
        }
        return DefaultProcDx();
    }
};
