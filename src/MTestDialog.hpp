// MTestDialog --- Test Dialog
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

#ifndef MZC4_MTESTDIALOG_HPP_
#define MZC4_MTESTDIALOG_HPP_

#include "MWindowBase.hpp"
#include "MenuRes.hpp"
#include "DialogRes.hpp"
#include "Res.hpp"

#include <map>

//////////////////////////////////////////////////////////////////////////////

class MTestDialog : public MDialogBase
{
public:
    ResEntries& m_entries;
    DialogRes& m_dialog_res;
    MIdOrString m_menu;
    WORD m_lang;
    HMENU m_hMenu;
    MTitleToBitmap  m_title_to_bitmap;
    MTitleToIcon    m_title_to_icon;
    INT             m_xDialogBaseUnit;
    INT             m_yDialogBaseUnit;

    MTestDialog(ResEntries& entries, DialogRes& dialog_res, MIdOrString menu, WORD lang)
        : m_entries(entries), m_dialog_res(dialog_res), m_menu(menu), m_lang(lang), m_hMenu(NULL)
    {
        m_xDialogBaseUnit = LOWORD(GetDialogBaseUnits());
        m_yDialogBaseUnit = HIWORD(GetDialogBaseUnits());
    }

    BOOL GetBaseUnits(INT& xDialogBaseUnit, INT& yDialogBaseUnit)
    {
        m_xDialogBaseUnit = m_dialog_res.GetBaseUnits(m_yDialogBaseUnit);
        if (m_xDialogBaseUnit == 0)
        {
            return FALSE;
        }

        xDialogBaseUnit = m_xDialogBaseUnit;
        yDialogBaseUnit = m_yDialogBaseUnit;

        return TRUE;
    }

    virtual ~MTestDialog()
    {
        if (m_hMenu)
        {
            DestroyMenu(m_hMenu);
            m_hMenu = NULL;
        }
        clear_maps();
    }

    void clear_maps()
    {
        ClearMaps(m_title_to_bitmap, m_title_to_icon);
    }

    void create_maps(WORD lang)
    {
        for (size_t i = 0; i < m_dialog_res.size(); ++i)
        {
            DialogItem& item = m_dialog_res[i];
            if (item.m_class == 0x0082 ||
                lstrcmpiW(item.m_class.c_str(), L"STATIC") == 0)
            {
                // static
                if ((item.m_style & SS_TYPEMASK) == SS_ICON)
                {
                    Res_DoIcon(m_entries, m_title_to_icon, item, lang);
                }
                else if ((item.m_style & SS_TYPEMASK) == SS_BITMAP)
                {
                    Res_DoBitmap(m_entries, m_title_to_bitmap, item, lang);
                }
            }
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        if (m_hMenu)
        {
            SetMenu(hwnd, NULL);
            DestroyMenu(m_hMenu);
            m_hMenu = NULL;
        }
        if (!m_menu.empty())
        {
            INT i = Res_Find2(m_entries, RT_MENU, m_menu, m_lang, FALSE);
            if (i != -1)
            {
                ResEntry& entry = m_entries[i];
                m_hMenu = LoadMenuIndirect(&entry[0]);
                SetMenu(hwnd, m_hMenu);

                INT cyMenu = GetSystemMetrics(SM_CYMENU);
                RECT rc;
                GetWindowRect(hwnd, &rc);
                rc.bottom += cyMenu;
                SIZE siz = SizeFromRectDx(&rc);
                SetWindowPosDx(NULL, &siz);
            }
        }

        create_maps(0xFFFF);

        GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);

        INT i = 0;
        for (HWND hCtrl = GetTopWindow(hwnd);
             hCtrl; hCtrl = GetNextWindow(hCtrl, GW_HWNDNEXT))
        {
            DWORD style = GetWindowStyle(hCtrl);
            SIZE siz;
            GetWindowPosDx(hCtrl, NULL, &siz);

            WCHAR szClass[32];
            GetClassNameW(hCtrl, szClass, 32);
            if (lstrcmpiW(szClass, L"STATIC") == 0)
            {
                WORD id = m_dialog_res[i].m_title.m_id;
                if ((style & SS_TYPEMASK) == SS_ICON)
                {
                    HICON hIcon = m_title_to_icon[id];
                    SendMessage(hCtrl, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
                    if (style & SS_REALSIZEIMAGE)
                    {
                        ICONINFO info;
                        GetIconInfo(hIcon, &info);
                        BITMAP bm;
                        GetObject(info.hbmColor, sizeof(BITMAP), &bm);
                        siz.cx = bm.bmWidth;
                        siz.cy = bm.bmHeight;
                    }
                    else if (style & SS_REALSIZECONTROL)
                    {
                        siz.cx = m_dialog_res[i].m_siz.cx * m_xDialogBaseUnit / 4;
                        siz.cy = m_dialog_res[i].m_siz.cy * m_yDialogBaseUnit / 8;
                    }
                    SetWindowPosDx(hCtrl, NULL, &siz);
                }
                else if ((style & SS_TYPEMASK) == SS_BITMAP)
                {
                    HBITMAP hbm = m_title_to_bitmap[id];
                    SendMessage(hCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
                    if (style & SS_REALSIZECONTROL)
                    {
                        siz.cx = m_dialog_res[i].m_siz.cx * m_xDialogBaseUnit / 4;
                        siz.cy = m_dialog_res[i].m_siz.cy * m_yDialogBaseUnit / 8;
                    }
                    else
                    {
                        BITMAP bm;
                        GetObject(hbm, sizeof(BITMAP), &bm);
                        siz.cx = bm.bmWidth;
                        siz.cy = bm.bmHeight;
                    }
                    SetWindowPosDx(hCtrl, NULL, &siz);
                }
            }

            ++i;
        }

        if (m_bModal)
            CenterWindowDx();
        return TRUE;
    }

    void Destroy(HWND hwnd)
    {
        PostMessage(GetParent(hwnd), WM_NULL, 0, 0);
        if (m_bModal)
            EndDialog(IDOK);
        else
            DestroyWindow(*this);
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DWORD dwStyle;
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        case WM_LBUTTONDOWN:
            dwStyle = GetWindowStyle(hwnd);
            if (!(dwStyle & WS_SYSMENU))
            {
                Destroy(hwnd);
            }
            break;
        case WM_LBUTTONDBLCLK:
            Destroy(hwnd);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDOK: case IDCANCEL:
                Destroy(hwnd);
                break;
            }
            break;
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MTESTDIALOG_HPP_
