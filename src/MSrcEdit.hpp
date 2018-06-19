// MSrcEdit.hpp --- RisohEditor the source EDIT control
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-20112 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#ifndef MZC4_MSRCEDIT_HPP_
#define MZC4_MSRCEDIT_HPP_

#include "MEditCtrl.hpp"
#include "MString.hpp"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////////

class MSrcEdit : public MEditCtrl
{
protected:
    std::set<INT> m_indeces;

public:
    enum
    {
        MARK_WIDTH = 9, MARK_HEIGHT = 9
    };

    MSrcEdit()
    {
        m_hIconMark = HICON(LoadImage(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDI_MARK),
            IMAGE_ICON, MARK_WIDTH, MARK_HEIGHT, 0));
    }

    virtual ~MSrcEdit()
    {
        if (m_hIconMark)
        {
            DestroyIcon(m_hIconMark);
        }
    }

    void ClearIndeces()
    {
        m_indeces.clear();
        InvalidateRect(m_hwnd, NULL, TRUE);
    }

    void SetIndeces(const std::set<INT>& indeces)
    {
        m_indeces = indeces;
        InvalidateRect(m_hwnd, NULL, TRUE);
    }

    void DrawMarks(HWND hwnd)
    {
        if (DefaultProcDx(hwnd, EM_GETMODIFY, 0, 0))
            return;

        for (auto& index : m_indeces)
        {
            DrawMark(hwnd, 7 + index);
        }
    }

    void DrawMark(HWND hwnd, INT iLine)
    {
        if (DefaultProcDx(hwnd, EM_GETMODIFY, 0, 0))
            return;

        MString strText = GetWindowText();
        HFONT hFont = GetWindowFont(hwnd);

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        LRESULT margins = DefaultProcDx(hwnd, EM_GETMARGINS, 0, 0);
        rcClient.left += LOWORD(margins);
        rcClient.right -= HIWORD(margins);

        SIZE sizClient = SizeFromRectDx(&rcClient);

        INT iFirstLine = DefaultProcDx(hwnd, EM_GETFIRSTVISIBLELINE, 0, 0);

        iLine -= iFirstLine;
        if (iLine < 0)
            return;

        INT ich = DefaultProcDx(hwnd, EM_LINEINDEX, iFirstLine, 0);
        strText = strText.substr(ich);

        std::vector<MString> lines;
        mstr_split(lines, strText, TEXT("\n"));

        if (HDC hDC = GetDC(hwnd))
        {
            HGDIOBJ hFontOld = SelectObject(hDC, hFont);

            INT y = rcClient.top;
            for (size_t i = 0; i < lines.size(); ++i)
            {
                if (INT(i) == iLine)
                    break;

                auto& line = lines[i];

                UINT uFormat = DT_CALCRECT | DT_EDITCONTROL | DT_EXPANDTABS |
                               DT_LEFT | DT_NOPREFIX | DT_NOPREFIX | DT_TOP;
                if (g_settings.bWordWrap)
                    uFormat |= DT_WORDBREAK;

                RECT rc = rcClient;
                DrawText(hDC, line.c_str(), -1, &rc, uFormat);

                SIZE siz = SizeFromRectDx(&rc);
                y += siz.cy;
            }

            {
                SIZE siz;
                GetTextExtentPoint32(hDC, TEXT("Mg"), 2, &siz);
                y += (siz.cy - MARK_HEIGHT + 1) / 2 + 1;
            }

            DrawIconEx(hDC, rcClient.left, y, m_hIconMark, MARK_WIDTH, MARK_HEIGHT,
                       0, NULL, DI_NORMAL);

            SelectObject(hDC, hFontOld);
            ReleaseDC(hwnd, hDC);
        }
    }

    void OnPaint(HWND hwnd)
    {
        DefaultProcDx();
        DrawMarks(hwnd);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        DefaultProcDx();
        InvalidateRect(hwnd, NULL, TRUE);
        DrawMarks(hwnd);
    }

    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        DefaultProcDx();
        InvalidateRect(hwnd, NULL, TRUE);
        DrawMarks(hwnd);
    }

    void OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
    {
        DefaultProcDx();
        InvalidateRect(hwnd, NULL, TRUE);
        DrawMarks(hwnd);
    }

    void OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
    {
        DefaultProcDx();
        InvalidateRect(hwnd, NULL, TRUE);
        DrawMarks(hwnd);
    }

    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        DefaultProcDx();
        InvalidateRect(hwnd, NULL, TRUE);
        DrawMarks(hwnd);
    }

    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
        DefaultProcDx();
        if (keyFlags & MK_LBUTTON)
        {
            InvalidateRect(hwnd, NULL, TRUE);
            DrawMarks(hwnd);
        }
    }

    void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
    {
        DefaultProcDx();
        InvalidateRect(hwnd, NULL, TRUE);
        DrawMarks(hwnd);
    }

    void OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
    {
        DefaultProcDx();
        InvalidateRect(hwnd, NULL, TRUE);
        DrawMarks(hwnd);
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result;
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
        HANDLE_MSG(hwnd, WM_HSCROLL, OnHScroll);
        HANDLE_MSG(hwnd, WM_VSCROLL, OnVScroll);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
        HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);

        case EM_SETMODIFY:
            result = DefaultProcDx();
            InvalidateRect(hwnd, NULL, TRUE);
            DrawMarks(hwnd);
            return result;

        case EM_LINESCROLL:
        case EM_REPLACESEL:
        case EM_SCROLL:
        case EM_SCROLLCARET:
        case EM_SETMARGINS:
        case EM_SETREADONLY:
        case EM_SETSEL:
            return DefaultProcDx();

        case WM_PRINTCLIENT:
            DefaultProcDx();
            DrawMarks(hwnd);
            break;

        default:
            return DefaultProcDx();
        }
        return 0;
    }

protected:
    HICON m_hIconMark;
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MSRCEDIT_HPP_
