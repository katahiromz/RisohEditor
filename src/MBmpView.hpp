// MBmpView.hpp --- Bitmap Viewer
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

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

BOOL PlayMP3(LPCVOID ptr, size_t size);
void StopMP3(void);
BOOL PlayAvi(HWND hwnd, LPCVOID ptr, size_t size);
void StopAvi(void);

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

class MBmpView : public MWindowBase
{
public:
	BITMAP      m_bm;
	HBITMAP     m_hBitmap;
	HICON       m_hIcon;
	HWND        m_hStatic;
	HWND        m_hPlayButton;
	MBitmapDx   m_bitmap;
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
		DWORD style = WS_CHILD | SS_ICON | SS_REALSIZEIMAGE | WS_CLIPCHILDREN;
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
		UpdateScrollInfo(m_hwnd);
		DeleteTempFile();
	}

	void SetImage(const void *ptr, DWORD size)
	{
		DestroyView();
		ShowWindow(m_hStatic, SW_HIDE);
		ShowWindow(m_hPlayButton, SW_HIDE);
		if (m_bitmap.CreateFromMemory(ptr, size))
		{
			LONG cx, cy;
			m_hBitmap = m_bitmap.GetHBITMAP(cx, cy);
			UpdateScrollInfo(m_hwnd);
			SetTimer(m_hwnd, TIMER_ID, 0, NULL);
		}
		DeleteTempFile();
	}

	void SetMedia(const void *ptr, DWORD size, std::wstring media)
	{
		DestroyView();
		ShowWindow(m_hStatic, SW_HIDE);
		ShowWindow(m_hPlayButton, SW_HIDE);
		DeleteTempFile();

		if (media == L"avi") {
			ShowScrollBar(m_hwnd, SB_BOTH, FALSE);
			PlayAvi(m_hwnd, ptr, size);
			return;
		}
	}

	void SetPlay()
	{
		DestroyView();
		ShowWindow(m_hStatic, SW_HIDE);
		ShowWindow(m_hPlayButton, SW_SHOWNOACTIVATE);
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
		StopAvi();
		StopMP3();

		KillTimer(m_hwnd, TIMER_ID);
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
			return DefaultProcDx();
		}
		return 0;
	}
};
