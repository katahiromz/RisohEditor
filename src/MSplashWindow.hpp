// MSplashWindow.hpp --- RisohEditor splash window
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2026  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "MWindowBase.hpp"

//////////////////////////////////////////////////////////////////////////////

class MSplashWindow : public MWindowBase
{
public:
	enum { TIMER_ID = 999 };
	HBITMAP m_hbm = nullptr;
	static HHOOK s_hKeyboardHook;
	static HWND s_hwndSplash;

	MSplashWindow()
	{
		m_hbm = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SPLASH));
		assert(m_hbm);
	}

	~MSplashWindow()
	{
		DeleteObject(m_hbm);
	}

	BOOL CreateDx(HWND hwndParent)
	{
		if (!m_hbm)
			return FALSE;

		RegisterClassDx();

		DWORD style = WS_POPUPWINDOW;
		DWORD exstyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;

		BITMAP bm;
		if (!GetObject(m_hbm, sizeof(bm), &bm))
			return FALSE;

		RECT rc = { 0, 0, bm.bmWidth, bm.bmHeight };
		AdjustWindowRectEx(&rc, style, FALSE, exstyle);

		// High DPI support
		UINT dpi = 96;
		if (HMODULE hUser32 = GetModuleHandle(TEXT("user32.dll")))
		{
			typedef UINT (WINAPI *GETDPIFORSYSTEM)(void);
			if (auto pGetDpiForSystem = (GETDPIFORSYSTEM)GetProcAddress(hUser32, "GetDpiForSystem"))
				dpi = pGetDpiForSystem();
		}
		INT cx = MulDiv(bm.bmWidth, dpi, 96);
		INT cy = MulDiv(bm.bmHeight, dpi, 96);

		BOOL ret = CreateWindowDx(hwndParent, NULL, style, exstyle,
		                          CW_USEDEFAULT, CW_USEDEFAULT, cx, cy);
		if (ret)
		{
			CenterWindowDx(m_hwnd);
			ShowWindow(m_hwnd, SW_SHOWNORMAL);
			UpdateWindow(m_hwnd);
		}
		return ret;
	}

	LPCTSTR GetWndClassNameDx() const override
	{
		return TEXT("katahiromz's splash window");
	}

	static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
		{
			if (s_hwndSplash && IsWindow(s_hwndSplash))
				DestroyWindow(s_hwndSplash);
		}
		return CallNextHookEx(s_hKeyboardHook, nCode, wParam, lParam);
	}

	// WM_CREATE
	BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
	{
		s_hwndSplash = hwnd;
		s_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc,
		                                   GetModuleHandle(NULL), 0);
		SetTimer(hwnd, TIMER_ID, 3000, NULL);
		return TRUE;
	}

	// WM_DESTROY
	void OnDestroy(HWND hwnd)
	{
		KillTimer(hwnd, TIMER_ID);
		if (s_hKeyboardHook)
		{
			UnhookWindowsHookEx(s_hKeyboardHook);
			s_hKeyboardHook = NULL;
		}
		s_hwndSplash = NULL;
	}

	// WM_ERASEBKGND
	BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, (HBRUSH)(COLOR_3DFACE + 1));
		return TRUE;
	}

	// WM_PAINT
	void OnPaint(HWND hwnd)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);

		BITMAP bm;
		GetObject(m_hbm, sizeof(bm), &bm);

		PAINTSTRUCT ps;
		HDC hdcMem = CreateCompatibleDC(NULL);
		if (HDC hdc = BeginPaint(hwnd, &ps))
		{
			HGDIOBJ hbmOld = SelectObject(hdcMem, m_hbm);

			SetStretchBltMode(hdc, STRETCH_HALFTONE);
			StretchBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

			SelectObject(hdcMem, hbmOld);
			EndPaint(hwnd, &ps);
		}
		DeleteDC(hdcMem);
	}

	// WM_TIMER
	void OnTimer(HWND hwnd, UINT id)
	{
		if (id == TIMER_ID)
		{
			KillTimer(hwnd, id);
			DestroyWindow(hwnd);
		}
	}

	// WM_LBUTTONDOWN
	void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
	{
		KillTimer(hwnd, TIMER_ID);
		DestroyWindow(hwnd);
	}

	LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
			HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
			HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
			HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
			HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
			HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
		}
		return DefaultProcDx(hwnd, uMsg, wParam, lParam);
	}
};
