// MBmpView.hpp --- Bitmap Viewer
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
#include "MWindowBase.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "MBitmapDx.hpp"
#ifndef _INC_VFW
	#include <vfw.h>
#endif
#include <mmsystem.h>  // for mciGetErrorString

class MBmpView;

// Helper to log MCI errors for debugging purposes
inline void LogMCIError(DWORD dwError, LPCTSTR pszContext)
{
	TCHAR szError[256];
	if (mciGetErrorString(dwError, szError, _countof(szError)))
	{
		TCHAR szMsg[512];
		wsprintf(szMsg, TEXT("MCI Error in %s: %s (code %lu)\n"), pszContext, szError, dwError);
		OutputDebugString(szMsg);
	}
	else
	{
		TCHAR szMsg[128];
		wsprintf(szMsg, TEXT("MCI Error in %s: code %lu\n"), pszContext, dwError);
		OutputDebugString(szMsg);
	}
}

//////////////////////////////////////////////////////////////////////////////

class MMciSubclassed : public MWindowBase
{
public:
	virtual LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_ERASEBKGND:
			return (LRESULT)GetStockBrush(LTGRAY_BRUSH);
		}
		return DefaultProcDx();
	}
};

class MBmpView : public MWindowBase
{
public:
	BITMAP      m_bm;
	HBITMAP     m_hBitmap;
	HICON       m_hIcon;
	HWND        m_hStatic;
	HWND        m_hPlayButton;
	MBitmapDx   m_bitmap;
	HWND        m_mci_window;
	MMciSubclassed  m_mci;
	TCHAR       m_szTempFile[MAX_PATH];
	enum { TIMER_ID = 999 };

	MBmpView()
	{
		ZeroMemory(&m_bm, sizeof(m_bm));
		m_szTempFile[0] = 0;
	}

	~MBmpView()
	{
		DeleteTempFile();
		DestroyView();
	}

	BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
	{
		DWORD style = WS_CHILD | WS_BORDER | MCIWNDF_NOMENU | MCIWNDF_NOPLAYBAR |
					  MCIWNDF_NOAUTOSIZEWINDOW | MCIWNDF_NOAUTOSIZEMOVIE |
					  MCIWNDF_NOTIFYALL;
		m_mci_window = MCIWndCreate(hwnd, GetModuleHandle(NULL), style, NULL);
		if (m_mci_window == NULL)
			return FALSE;

		m_mci.SubclassDx(m_mci_window);

		style = WS_CHILD | SS_ICON | SS_REALSIZEIMAGE;
		m_hStatic = CreateWindowEx(0, TEXT("STATIC"), NULL,
			style, 0, 0, 32, 32, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);
		if (m_hStatic == NULL)
			return FALSE;

		style = WS_CHILD | BS_PUSHBUTTON | BS_CENTER | BS_ICON;
		m_hPlayButton = CreateWindowEx(0, TEXT("BUTTON"), TEXT("Play"),
			style, 0, 0, 64, 65, hwnd, (HMENU)2, GetModuleHandle(NULL), NULL);
		if (m_hPlayButton == NULL)
			return FALSE;

		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PLAY));
		SendMessage(m_hPlayButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		SetWindowFont(m_hPlayButton, GetStockFont(DEFAULT_GUI_FONT), TRUE);
		return TRUE;
	}

	void OnDestroy(HWND hwnd)
	{
		DestroyView();

		DestroyWindow(m_mci_window);
		m_mci_window = NULL;

		DestroyWindow(m_hStatic);
		m_hStatic = NULL;

		DestroyWindow(m_hPlayButton);
		m_hPlayButton = NULL;
	}

	virtual LPCTSTR GetWndClassNameDx() const
	{
		return TEXT("RisohEditor MBmpView Class");
	}

	virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
	{
		wcx.hIcon = NULL;
		wcx.hCursor = LoadCursor(NULL, IDC_CROSS);
		wcx.hbrBackground = GetStockBrush(LTGRAY_BRUSH);
		wcx.lpszMenuName = NULL;
	}

	void SetBitmap(HBITMAP hbm)
	{
		DestroyView();
		m_hBitmap = hbm;
		ShowWindow(m_hStatic, SW_HIDE);
		ShowWindow(m_hPlayButton, SW_HIDE);
		ShowWindow(m_mci_window, SW_HIDE);
		UpdateScrollInfo(m_hwnd);
		DeleteTempFile();
	}

	void SetIcon(HICON hIcon, BOOL bIcon)
	{
		DestroyView();
		m_hIcon = hIcon;
		SendMessage(m_hStatic, STM_SETIMAGE, (bIcon ? IMAGE_ICON : IMAGE_CURSOR), (LPARAM)hIcon);
		ShowWindow(m_hStatic, SW_SHOWNOACTIVATE);
		ShowWindow(m_hPlayButton, SW_HIDE);
		ShowWindow(m_mci_window, SW_HIDE);
		UpdateScrollInfo(m_hwnd);
		DeleteTempFile();
	}

	void SetImage(const void *ptr, DWORD size)
	{
		DestroyView();
		ShowWindow(m_hStatic, SW_HIDE);
		ShowWindow(m_hPlayButton, SW_HIDE);
		ShowWindow(m_mci_window, SW_HIDE);
		if (m_bitmap.CreateFromMemory(ptr, size))
		{
			LONG cx, cy;
			m_hBitmap = m_bitmap.GetHBITMAP(cx, cy);
			UpdateScrollInfo(m_hwnd);
			SetTimer(m_hwnd, TIMER_ID, 0, NULL);
		}
		DeleteTempFile();
	}

	void SetMedia(const void *ptr, DWORD size)
	{
		DestroyView();
		ShowWindow(m_hStatic, SW_HIDE);
		ShowWindow(m_hPlayButton, SW_HIDE);
		ShowWindow(m_mci_window, SW_HIDE);
		DeleteTempFile();

		TCHAR szTempPath[MAX_PATH];
		GetTempPath(MAX_PATH, szTempPath);
		GetTempFileName(szTempPath, TEXT("avi"), 0, m_szTempFile);

		// MCI relies on file extension to determine media type; rename .tmp to .avi
		// so that compressed AVI files are properly recognized on some systems.
		LPTSTR pszDot = _tcsrchr(m_szTempFile, TEXT('.'));
		if (pszDot != NULL && pszDot + 5 <= m_szTempFile + _countof(m_szTempFile))
		{
			TCHAR szOldPath[MAX_PATH];
			_tcscpy_s(szOldPath, m_szTempFile);
			_tcscpy_s(pszDot, 5, TEXT(".avi"));  // ".avi" + null = 5 chars
			// Rename the temp file created by GetTempFileName
			if (!MoveFile(szOldPath, m_szTempFile))
			{
				// Rename failed; revert to original path
				_tcscpy_s(m_szTempFile, szOldPath);
			}
		}

		ShowScrollBar(m_hwnd, SB_BOTH, FALSE);

		MByteStreamEx stream;
		stream.WriteData(ptr, size);
		if (stream.SaveToFile(m_szTempFile))
		{
			ShowWindow(m_mci_window, SW_SHOWNOACTIVATE);
			DWORD dwError = (DWORD)MCIWndOpen(m_mci_window, m_szTempFile, 0);
			if (dwError != 0)
			{
				LogMCIError(dwError, TEXT("MCIWndOpen"));
				ShowWindow(m_mci_window, SW_HIDE);
				return;
			}
			dwError = (DWORD)MCIWndPlay(m_mci_window);
			if (dwError != 0)
			{
				LogMCIError(dwError, TEXT("MCIWndPlay"));
				ShowWindow(m_mci_window, SW_HIDE);
			}
		}
	}

	void SetPlay()
	{
		DestroyView();
		ShowWindow(m_hStatic, SW_HIDE);
		ShowWindow(m_hPlayButton, SW_SHOWNOACTIVATE);
		ShowWindow(m_mci_window, SW_HIDE);
		DeleteTempFile();
	}

	void DeleteTempFile()
	{
		if (m_szTempFile[0])
		{
			if (DeleteFile(m_szTempFile) ||
				GetFileAttributes(m_szTempFile) == 0xFFFFFFFF)
			{
				m_szTempFile[0] = 0;
			}
		}
	}

	void DestroyView()
	{
		KillTimer(m_hwnd, TIMER_ID);
		ShowWindow(m_mci_window, SW_HIDE);
		MCIWndStop(m_mci_window);
		MCIWndClose(m_mci_window);
		if (m_hBitmap)
		{
			DeleteObject(m_hBitmap);
			m_hBitmap = NULL;
		}
		if (m_hIcon)
		{
			DestroyIcon(m_hIcon);
			m_hIcon = NULL;
		}
		m_bitmap.SetBitmap(NULL);
	}

	BOOL CreateDx(HWND hwndParent, INT CtrlID = 4, BOOL bVisible = FALSE)
	{
		DWORD dwStyle = WS_CHILD | WS_HSCROLL | WS_VSCROLL;
		if (bVisible)
			dwStyle |= WS_VISIBLE;
		DWORD dwExStyle = WS_EX_CLIENTEDGE;
		return CreateAsChildDx(hwndParent, NULL, dwStyle, dwExStyle, CtrlID);
	}

	void OnPaint(HWND hwnd)
	{
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hwnd, &ps);
		if (hDC == NULL)
			return;

		HDC hMemDC = CreateCompatibleDC(NULL);
		{
			SelectObject(hMemDC, m_hBitmap);
			INT dx = GetScrollPos(hwnd, SB_HORZ);
			INT dy = GetScrollPos(hwnd, SB_VERT);
			BitBlt(hDC, -dx, -dy, m_bm.bmWidth, m_bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
		}
		DeleteDC(hMemDC);
		EndPaint(hwnd, &ps);
	}

	BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, GetStockBrush(COLOR_BACKGROUND));
		return TRUE;
	}

	void OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);

		SCROLLINFO info;
		ZeroMemory(&info, sizeof(info));
		info.cbSize = sizeof(info);
		info.fMask = SIF_POS | SIF_PAGE | SIF_DISABLENOSCROLL;
		info.nPage = rc.right - rc.left;
		switch (code)
		{
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			info.nPos = pos;
			break;
		case SB_TOP:
			info.nPos = 0;
			break;
		case SB_BOTTOM:
			info.nPos = m_bm.bmHeight;
			break;
		case SB_ENDSCROLL:
			return;
		case SB_LINEDOWN:
			info.nPos = GetScrollPos(hwnd, SB_HORZ) + 10;
			break;
		case SB_LINEUP:
			info.nPos = GetScrollPos(hwnd, SB_HORZ) - 10;
			break;
		case SB_PAGEDOWN:
			info.nPos = GetScrollPos(hwnd, SB_HORZ) + info.nPage;
			break;
		case SB_PAGEUP:
			info.nPos = GetScrollPos(hwnd, SB_HORZ) - info.nPage;
			break;
		}
		SetScrollInfo(hwnd, SB_HORZ, &info, TRUE);
		InvalidateRect(hwnd, NULL, TRUE);
	}

	void OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);

		SCROLLINFO info;
		ZeroMemory(&info, sizeof(info));
		info.cbSize = sizeof(info);
		info.fMask = SIF_POS | SIF_PAGE | SIF_DISABLENOSCROLL;
		info.nPage = rc.bottom - rc.top;
		switch (code)
		{
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			info.nPos = pos;
			break;
		case SB_TOP:
			info.nPos = 0;
			break;
		case SB_BOTTOM:
			info.nPos = m_bm.bmHeight;
			break;
		case SB_ENDSCROLL:
			return;
		case SB_LINEDOWN:
			info.nPos = GetScrollPos(hwnd, SB_VERT) + 10;
			break;
		case SB_LINEUP:
			info.nPos = GetScrollPos(hwnd, SB_VERT) - 10;
			break;
		case SB_PAGEDOWN:
			info.nPos = GetScrollPos(hwnd, SB_VERT) + info.nPage;
			break;
		case SB_PAGEUP:
			info.nPos = GetScrollPos(hwnd, SB_VERT) - info.nPage;
			break;
		}
		SetScrollInfo(hwnd, SB_VERT, &info, TRUE);
		InvalidateRect(hwnd, NULL, TRUE);
	}

	void UpdateScrollInfo(HWND hwnd, SIZE siz)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);

		SCROLLINFO info;

		ZeroMemory(&info, sizeof(info));
		info.cbSize = sizeof(info);
		info.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
		info.nMin = 0;
		info.nMax = siz.cx;
		info.nPage = rc.right - rc.left;
		info.nPos = 0;
		SetScrollInfo(hwnd, SB_HORZ, &info, TRUE);
		ShowScrollBar(hwnd, SB_HORZ, TRUE);

		ZeroMemory(&info, sizeof(info));
		info.cbSize = sizeof(info);
		info.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
		info.nMin = 0;
		info.nMax = siz.cy;
		info.nPage = rc.bottom - rc.top;
		info.nPos = 0;
		SetScrollInfo(hwnd, SB_VERT, &info, TRUE);
		ShowScrollBar(hwnd, SB_VERT, TRUE);

		InvalidateRect(hwnd, NULL, TRUE);
	}

	void UpdateScrollInfo(HWND hwnd)
	{
		if (!GetObjectW(m_hBitmap, sizeof(m_bm), &m_bm))
			return;

		SIZE siz = { m_bm.bmWidth, m_bm.bmHeight };
		UpdateScrollInfo(hwnd, siz);
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case 999:
			UpdateScrollInfo(hwnd);
			break;
		case 1:
			break;
		case 2:
			if (codeNotify == BN_CLICKED)
			{
				PostMessage(GetParent(hwnd), WM_COMMAND, ID_PLAY, 0);
			}
			break;
		}
	}

	void OnSize(HWND hwnd, UINT state, int cx, int cy)
	{
		UpdateScrollInfo(hwnd);
		MoveWindow(m_mci_window, 0, 0, cx, cy, TRUE);
		FORWARD_WM_SIZE(hwnd, state, cx, cy, DefWindowProcW);
	}

	void OnTimer(HWND hwnd, UINT id)
	{
		KillTimer(hwnd, id);
		if (id != TIMER_ID)
			return;

		DWORD dwDelay;
		if (m_bitmap.Step(dwDelay))
		{
			LONG cx, cy;
			if (m_hBitmap)
			{
				DeleteObject(m_hBitmap);
			}
			m_hBitmap = m_bitmap.GetHBITMAP(cx, cy);
			InvalidateRect(hwnd, NULL, FALSE);
			SetTimer(hwnd, TIMER_ID, dwDelay, NULL);
		}
	}

	void OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
	{
		if (fwKeys & MK_SHIFT)
		{
			if (zDelta < 0)
			{
				for (INT i = 0; i < -zDelta; i += WHEEL_DELTA)
					FORWARD_WM_HSCROLL(hwnd, NULL, SB_LINEDOWN, 0, SendMessage);
			}
			else
			{
				for (INT i = 0; i < zDelta; i += WHEEL_DELTA)
					FORWARD_WM_HSCROLL(hwnd, NULL, SB_LINEUP, 0, SendMessage);
			}
		}
		else
		{
			if (zDelta < 0)
			{
				for (INT i = 0; i < -zDelta; i += WHEEL_DELTA)
					FORWARD_WM_VSCROLL(hwnd, NULL, SB_LINEDOWN, 0, SendMessage);
			}
			else
			{
				for (INT i = 0; i < zDelta; i += WHEEL_DELTA)
					FORWARD_WM_VSCROLL(hwnd, NULL, SB_LINEUP, 0, SendMessage);
			}
		}
	}

	virtual LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
			HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
			HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
			HANDLE_MSG(hwnd, WM_HSCROLL, OnHScroll);
			HANDLE_MSG(hwnd, WM_VSCROLL, OnVScroll);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_SIZE, OnSize);
			HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
			HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
			HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);
		default:
			if (uMsg == MCIWNDM_NOTIFYMODE)
			{
				if (lParam == MCI_MODE_STOP)
				{
					MCIWndPlayFrom(m_mci_window, 0);
					break;
				}
			}
			return DefaultProcDx();
		}
		return 0;
	}
};
