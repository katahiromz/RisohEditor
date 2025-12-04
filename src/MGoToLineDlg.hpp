// MGoToLineDlg.hpp --- "Go To Line" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "Common.hpp"

//////////////////////////////////////////////////////////////////////////////

class MGoToLineDlg : public MDialogBase
{
public:
	INT m_line;

	MGoToLineDlg()
		: MDialogBase(IDD_GOTOLINE)
		, m_line(0)
	{
	}

	~MGoToLineDlg()
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		m_line = GetDlgItemInt(hwnd, edt1, NULL, FALSE);
		EndDialog(IDOK);
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case IDOK:
			OnOK(hwnd);
			break;
		case IDCANCEL:
			EndDialog(IDCANCEL);
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
		return 0;
	}
};
