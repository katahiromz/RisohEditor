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
	MSplashWindow()
	{
		m_hbm = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SPLASH));
	}

	~MSplashWindow()
	{
		DeleteObject(m_hbm);
	}

	BOOL CreateDx(HWND hwndParent)
	{
		RegisterClassDx();

		DWORD style = WS_POPUPWINDOW;
		DWORD exstyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE;

		BITMAP bm;
		GetObject(m_hbm, sizeof(bm), &bm);
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
			ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
			UpdateWindow(m_hwnd);
		}
		return ret;
	}

	LPCTSTR GetWndClassNameDx() const override
	{
		return TEXT("katahiromz's splash window");
	}

	BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
	{
		SetTimer(hwnd, TIMER_ID, 3000, NULL);
		return TRUE;
	}

	BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, (HBRUSH)(COLOR_3DFACE + 1));
		return TRUE;
	}

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

			SetStretchBltMode(hdc, COLORONCOLOR);
			StretchBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

			SelectObject(hdcMem, hbmOld);
			EndPaint(hwnd, &ps);
		}
		DeleteDC(hdcMem);
	}

	void OnTimer(HWND hwnd, UINT id)
	{
		if (id == TIMER_ID)
		{
			KillTimer(hwnd, id);
			DestroyWindow(hwnd);
		}
	}

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
			HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
			HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
			HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
			HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
		}
		return DefaultProcDx(hwnd, uMsg, wParam, lParam);
	}
};
