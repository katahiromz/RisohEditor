// MVersionInfoDlg.hpp --- version information dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "DialogRes.hpp"
#include "MString.hpp"
#include "MToolBarCtrl.hpp"
#include "MHyperLinkCtrl.hpp"

class MVersionInfoDlg;

//////////////////////////////////////////////////////////////////////////////

class MVersionInfoDlg : public MDialogBase
{
public:
	MVersionInfoDlg() : MDialogBase(IDD_VERSIONINFO)
	{
		m_visited = FALSE;
		m_old_tick = 0;
	}

	virtual ~MVersionInfoDlg()
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		SetDlgItemText(hwnd, stc1, LoadStringDx(IDS_VERSIONINFO));
		SetDlgItemText(hwnd, edt1, LoadStringDx(IDS_TRANSLATORS));
		SubclassChildDx(m_hyperlink, stc2);
		CenterWindowDx();
		return TRUE;
	}

	// STN_CLICKED
	void OnStc2(HWND hwnd)
	{
		// STN_CLICKED is generated twice (WM_LBUTTONDOWN and WM_LBUTTONUP)
		bool double_generated = m_visited && (GetTickCount() - m_old_tick < 500);
		m_old_tick = GetTickCount();
		m_visited = TRUE;
		if (double_generated)
			return;

		MString url = GetDlgItemText(stc2);
		ShellExecute(hwnd, NULL, url.c_str(), NULL, NULL, SW_SHOWNORMAL);
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case IDOK:
			EndDialog(IDOK);
			break;
		case IDCANCEL:
			EndDialog(IDCANCEL);
			break;
		case stc2:
			if (codeNotify == STN_CLICKED)
				OnStc2(hwnd);
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
		default:
			return DefaultProcDx();
		}
	}

protected:
	BOOL m_visited;
	DWORD m_old_tick;
	MHyperLinkCtrl m_hyperlink;
};
