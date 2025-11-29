// MHyperLinkCtrl.hpp --- hyper link control
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MHYPERLINKCTRL_HPP_
#define MZC4_MHYPERLINKCTRL_HPP_        4   /* Version 4 */

#include "MWindowBase.hpp"

class MHyperLinkCtrl;

//////////////////////////////////////////////////////////////////////////////

class MHyperLinkCtrl : public MWindowBase
{
public:
	MHyperLinkCtrl() :
		m_bGotFocus(FALSE),
		m_hHandCursor(LoadCursor(NULL, IDC_HAND))
	{
	}

	virtual ~MHyperLinkCtrl()
	{
		// NOTE: m_hHandCursor is a shared system cursor loaded with LoadCursor(NULL, ...)
		// and should NOT be destroyed.
	}

	virtual void OnJump(HWND hwnd)
	{
		WPARAM wParam = MAKEWPARAM(GetDlgCtrlID(hwnd), STN_CLICKED);
		LPARAM lParam = (LPARAM)hwnd;
		SendMessage(GetParent(hwnd), WM_COMMAND, wParam, lParam);
	}

	void OnPaint(HWND hwnd)
	{
		ModifyStyleDx(0, SS_NOTIFY | WS_TABSTOP);

		TCHAR szClass[64];
		GetClassName(hwnd, szClass, 64);
		assert(lstrcmpi(szClass, TEXT("STATIC")) == 0);

		PAINTSTRUCT ps;
		if (HDC hDC = BeginPaint(hwnd, &ps))
		{
			HFONT hDefaultFont = (HFONT)SendMessageDx(WM_GETFONT, 0, 0);

			LOGFONT lf;
			GetObject(hDefaultFont, sizeof(lf), &lf);
			lf.lfUnderline = TRUE;

			HFONT hFont = CreateFontIndirect(&lf);
			if (hFont)
			{
				HGDIOBJ hFontOld = SelectObject(hDC, hFont);

				RECT rcWindow, rcClient;
				GetWindowRect(hwnd, &rcWindow);
				GetClientRect(hwnd, &rcClient);

				POINT pt;
				GetCursorPos(&pt);

				if (m_bGotFocus || PtInRect(&rcWindow, pt))
				{
					SetTextColor(hDC, RGB(255, 0, 0));
					SetBkColor(hDC, RGB(255, 255, 0));
				}
				else
				{
					SetTextColor(hDC, RGB(0, 0, 255));
					SetBkColor(hDC, GetSysColor(COLOR_3DFACE));
				}
				SetBkMode(hDC, OPAQUE);

				FillRect(hDC, &rcClient, (HBRUSH)(COLOR_3DFACE + 1));

				UINT uFormat = DT_NOPREFIX;
				DWORD style = GetStyleDx();
				if (style & SS_CENTER)
					uFormat |= DT_CENTER;
				if (style & SS_RIGHT)
					uFormat |= DT_RIGHT;
				if (style & SS_CENTERIMAGE)
					uFormat |= DT_VCENTER | DT_SINGLELINE;

				MString text = GetWindowText();
				DrawText(hDC, text.c_str(), int(text.size()), &rcClient, uFormat);

				if (m_bGotFocus)
				{
					InflateRect(&rcClient, -1, -1);
					DrawFocusRect(hDC, &rcClient);
				}

				SelectObject(hDC, hFontOld);
				DeleteObject(hFont);
			}

			EndPaint(hwnd, &ps);
		}
	}

	void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
	{
		InvalidateRect(hwnd, NULL, TRUE);
		SetTimer(hwnd, 999, 100, NULL);
		SetCursor(m_hHandCursor);
	}

	void OnTimer(HWND hwnd, UINT id)
	{
		RECT rc;
		GetWindowRect(hwnd, &rc);

		POINT pt;
		GetCursorPos(&pt);

		if (!PtInRect(&rc, pt))
		{
			InvalidateRect(hwnd, NULL, TRUE);
			KillTimer(hwnd, 999);
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		}
	}

	void OnSetFocus(HWND hwnd, HWND hwndOldFocus)
	{
		m_bGotFocus = TRUE;
		InvalidateRect(hwnd, NULL, TRUE);
	}

	void OnKillFocus(HWND hwnd, HWND hwndNewFocus)
	{
		m_bGotFocus = FALSE;
		InvalidateRect(hwnd, NULL, TRUE);
	}

	void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
	{
		if (vk == VK_SPACE)
		{
			OnJump(hwnd);
		}
	}

	void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
	{
		OnJump(hwnd);
	}

	virtual LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
		HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
		HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
		HANDLE_MSG(hwnd, WM_SETFOCUS, OnSetFocus);
		HANDLE_MSG(hwnd, WM_KILLFOCUS, OnKillFocus);
		HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
		HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
		default:
			return DefaultProcDx();
		}
	}

protected:
	BOOL m_bGotFocus;
	HCURSOR m_hHandCursor;
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MHYPERLINKCTRL_HPP_
