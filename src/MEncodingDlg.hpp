// MEncodingDlg.hpp --- "Encoding" Dialog
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

#ifndef MZC4_MENCODINGDLG_HPP_
#define MZC4_MENCODINGDLG_HPP_

#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "MResizable.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "resource.h"

class MAddEncodingDlg;
class MModifyEncodingDlg;
class MEncodingDlg;

//////////////////////////////////////////////////////////////////////////////

class MEncodingDlg : public MDialogBase
{
public:
    MResizable m_resizable;
    HWND m_hLst1;
    HICON m_hIcon;
    HICON m_hIconSm;

    MString txt2enc(MString txt)
    {
        if (txt == LoadStringDx(IDS_ANSI))
            return L"ansi";
        if (txt == LoadStringDx(IDS_WIDE))
            return L"wide";
        if (txt == LoadStringDx(IDS_UTF8))
            return L"utf8";
        if (txt == LoadStringDx(IDS_UTF8N))
            return L"utf8n";
        if (txt == LoadStringDx(IDS_SJIS))
            return L"sjis";
        if (txt == LoadStringDx(IDS_BINARY))
            return L"bin";
        if (txt.size() && mchr_is_digit(txt[0]))
        {
            int num = mstr_parse_int(txt.c_str());
            return mstr_dec(num);
        }
        return L"";
    }

    MString enc2txt(MString enc)
    {
        if (enc == L"ansi")
            return LoadStringDx(IDS_ANSI);
        if (enc == L"wide")
            return LoadStringDx(IDS_WIDE);
        if (enc == L"utf8")
            return LoadStringDx(IDS_UTF8);
        if (enc == L"utf8n")
            return LoadStringDx(IDS_UTF8N);
        if (enc == L"sjis")
            return LoadStringDx(IDS_SJIS);
        if (enc == L"bin")
            return LoadStringDx(IDS_BINARY);
        if (enc.size() && mchr_is_digit(enc[0]))
        {
            int num = mstr_parse_int(enc.c_str());
            return mstr_dec(num);
        }
        return L"";
    }

    MEncodingDlg() : MDialogBase(IDD_ENCODING)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);
    }

    ~MEncodingDlg()
    {
        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    // get the resource type label
    MStringW get_type_label(MIdOrString& type) const
    {
        if (!type.m_id)
            return type.m_str;    // string name type

        // it was integer name type

        MStringW label = g_db.GetName(L"RESOURCE", type.m_id);
        if (label.empty())  // unable to get the label
            return mstr_dec_word(type.m_id);  // returns the numeric text

        // got the label
        if (!mchr_is_digit(label[0]))   // first character is not digit
        {
            // add a parenthesis pair and numeric text
            label += L" (";
            label += mstr_dec_word(type.m_id);
            label += L")";
        }

        return label;
    }

    void InitCtl1()
    {
        ListView_DeleteAllItems(m_hLst1);

        const auto& map = g_settings.encoding_map;

        INT i = 0;
        for (auto& pair : map)
        {
            if (pair.second.empty())
                continue;

            MStringW str = pair.first;
            auto k = str.find(L" (");   // )
            if (k != MStringW::npos)
            {
                int num = mstr_parse_int(&str[k + 1]);
                str = mstr_dec(num);
            }
            MIdOrString type(str.c_str());
            str = get_type_label(type);

            LV_ITEM item;
            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 0;
            item.pszText = &str[0];
            ListView_InsertItem(m_hLst1, &item);

            str = enc2txt(pair.second);

            ZeroMemory(&item, sizeof(item));
            item.iItem = i;
            item.mask = LVIF_TEXT;
            item.iSubItem = 1;
            item.pszText = &str[0];
            ListView_SetItem(m_hLst1, &item);

            ++i;
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

        m_hLst1 = GetDlgItem(hwnd, lst1);
        ListView_SetExtendedListViewStyle(m_hLst1, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

        LV_COLUMN column;
        ZeroMemory(&column, sizeof(column));

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 140;
        column.pszText = LoadStringDx(IDS_RESTYPE);
        column.iSubItem = 0;
        ListView_InsertColumn(m_hLst1, 0, &column);

        column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        column.fmt = LVCFMT_LEFT;
        column.cx = 200;
        column.pszText = LoadStringDx(IDS_ENCODING);
        column.iSubItem = 1;
        ListView_InsertColumn(m_hLst1, 1, &column);

        InitCtl1();

        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(m_hLst1, 0, state, state);
        SetFocus(m_hLst1);

        CenterWindowDx();
        return TRUE;
    }

    void OnModify(HWND hwnd)
    {
    }

    void OnOK(HWND hwnd)
    {
        EndDialog(IDOK);
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        if (hwndContext == m_hLst1)
        {
            PopupMenuDx(hwnd, m_hLst1, IDR_POPUPMENUS, 4, xPos, yPos);
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

#endif  // ndef MZC4_MENCODINGDLG_HPP_
