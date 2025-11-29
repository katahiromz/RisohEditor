// MTestMenuDlg.hpp --- "Test Menu" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "MWindowBase.hpp"

//////////////////////////////////////////////////////////////////////////////

class MTestMenuDlg : public MDialogBase
{
public:
	HMENU m_hMenu;

	MTestMenuDlg(HMENU hMenu) : m_hMenu(hMenu)
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		SetMenu(hwnd, m_hMenu);
		CenterWindowDx();
		return TRUE;
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case IDOK: case IDCANCEL:
			EndDialog(id);
			break;
		}
	}

	virtual INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		}
		return DefaultProcDx();
	}
};
