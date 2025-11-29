// MAdviceResHDlg.hpp --- "Advice about Resource ID" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "RisohSettings.hpp"

//////////////////////////////////////////////////////////////////////////////

class MAdviceResHDlg : public MDialogBase
{
public:
	MString m_str;

	MAdviceResHDlg(const MString& str) :
		MDialogBase(IDD_ADVICERESH),  m_str(str)
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		SetDlgItemText(hwnd, edt1, m_str.c_str());

		CenterWindowDx();

		SetFocus(GetDlgItem(hwnd, IDOK));
		return FALSE;
	}

	void OnOK(HWND hwnd)
	{
		EndDialog(IDOK);
	}

	void OnPsh1(HWND hwnd)
	{
		m_str.clear();
		g_settings.added_ids.clear();
		g_settings.removed_ids.clear();
		SetDlgItemText(hwnd, edt1, NULL);
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
		case psh1:
			OnPsh1(hwnd);
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
