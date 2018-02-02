// MTestDialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Win32API resource editor
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

//////////////////////////////////////////////////////////////////////////////

class MTestDialog : public MDialogBase
{
public:
    ResEntries& m_entries;
    MIdOrString m_menu;
    WORD m_lang;
    HMENU m_hMenu;

    MTestDialog(ResEntries& entries, MIdOrString menu, WORD lang)
        : m_entries(entries), m_menu(menu), m_lang(lang), m_hMenu(NULL)
    {
    }

    virtual ~MTestDialog()
    {
        if (m_hMenu)
        {
            DestroyMenu(m_hMenu);
            m_hMenu = NULL;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DWORD dwStyle;
        switch (uMsg)
        {
        case WM_INITDIALOG:
            if (m_hMenu)
            {
                SetMenu(hwnd, NULL);
                DestroyMenu(m_hMenu);
                m_hMenu = NULL;
            }
            if (!m_menu.empty())
            {
                INT i = Res_Find(m_entries, RT_MENU, m_menu, m_lang, FALSE);
                if (i == -1)
                {
                    i = Res_Find(m_entries, RT_MENU, m_menu, 0xFFFF, FALSE);
                }
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
            CenterWindowDx();
            return TRUE;
        case WM_LBUTTONDOWN:
            dwStyle = GetWindowStyle(hwnd);
            if (!(dwStyle & WS_SYSMENU))
            {
                EndDialog(IDOK);
            }
            break;
        case WM_LBUTTONDBLCLK:
            EndDialog(IDOK);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDOK: case IDCANCEL:
                EndDialog(LOWORD(wParam));
                break;
            }
            break;
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MTESTDIALOG_HPP_
