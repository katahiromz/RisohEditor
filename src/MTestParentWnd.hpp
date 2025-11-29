// MTestParentWnd.hpp --- Test Parent Window
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

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

	MTestParentWnd(DialogRes& dialog_res,
				   MIdOrString menu, WORD lang, const MByteStreamEx& stream,
				   const std::vector<BYTE>& dlginit_data)
		: m_test_dialog(dialog_res, menu, lang, dlginit_data), m_stream(stream)
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
		m_test_dialog.SetWindowPosDx(&pt);

		ShowWindow(m_test_dialog, SW_SHOWNORMAL);
		UpdateWindow(m_test_dialog);

		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

		CenterWindowDx();

		return TRUE;
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		if (id == ID_CHILDDESTROYED)
		{
			DestroyWindow(hwnd);
		}
	}

	virtual LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		default:
			return DefaultProcDx();
		}
		return 0;
	}
};
