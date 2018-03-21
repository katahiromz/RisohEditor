// MTestParentWnd.hpp --- Test Parent Window
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

#ifndef MZC4_MTESTPARENTWND_HPP_
#define MZC4_MTESTPARENTWND_HPP_

#include "MTestDialog.hpp"
#include "MByteStreamEx.hpp"

//////////////////////////////////////////////////////////////////////////////

class MTestParentWnd : public MWindowBase
{
public:
    MTestDialog m_test_dialog;
    MByteStreamEx m_stream;
    HICON m_hIcon;
    HICON m_hIconSm;

    MTestParentWnd(ResEntries& entries, DialogRes& dialog_res,
                   MIdOrString menu, WORD lang, const MByteStreamEx& stream)
        : m_test_dialog(entries, dialog_res, menu, lang), m_stream(stream)
    {
        m_hIcon = LoadIconDx(IDI_SMILY);
        m_hIconSm = LoadSmallIconDx(IDI_SMILY);
    }

    virtual ~MTestParentWnd()
    {
        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    virtual void PostNcDestroy()
    {
        delete this;
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("MZC4 MTestParentWnd Class");
    }

    virtual VOID ModifyWndClassDx(WNDCLASSEX& wcx)
    {
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        m_test_dialog.CreateDialogIndirectDx(hwnd, m_stream.ptr());

        RECT rc;
        GetWindowRect(m_test_dialog, &rc);
        AdjustWindowRectEx(&rc, GetWindowStyle(hwnd), FALSE, GetWindowExStyle(hwnd));
        POINT pt = { 0, 0 };
        SIZE siz = SizeFromRectDx(&rc);
        SetWindowPosDx(&pt, &siz);

        ShowWindow(m_test_dialog, SW_SHOWNORMAL);
        UpdateWindow(m_test_dialog);

        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

        CenterWindowDx();

        return TRUE;
    }

    void OnDestroy(HWND hwnd)
    {
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        case WM_NULL:
            DestroyWindow(hwnd);
            break;
        default:
            return DefaultProcDx();
        }
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MTESTPARENTWND_HPP_
