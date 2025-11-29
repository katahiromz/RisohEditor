// MSaveOptionsDlg.hpp --- "Save Options" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "RisohSettings.hpp"

//////////////////////////////////////////////////////////////////////////////

class MSaveOptionsDlg : public MDialogBase
{
public:
	MSaveOptionsDlg() : MDialogBase(IDD_SAVE_OPTIONS)
	{
	}

	void Reload(HWND hwnd)
	{
		CheckDlgButton(hwnd, chx1, g_settings.bSepFilesByLang ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx2, g_settings.bUseBeginEnd ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx3, g_settings.bSelectableByMacro ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx4, g_settings.bBackup ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx5, g_settings.bRedundantComments ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx6, g_settings.bWrapManifest ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx7, g_settings.bRCFileUTF16 ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx8, g_settings.bUseMSMSGTABLE ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx9, g_settings.bAddBomToRC ? BST_CHECKED : BST_UNCHECKED);

		SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"-old");
		SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"-bak");
		SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"~");
		SetDlgItemTextW(hwnd, cmb1, g_settings.strBackupSuffix.c_str());
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		Reload(hwnd);

		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		g_settings.bSepFilesByLang = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
		g_settings.bUseBeginEnd = (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
		g_settings.bSelectableByMacro = (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);
		g_settings.bBackup = (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
		g_settings.bRedundantComments = (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED);
		g_settings.bWrapManifest = (IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED);
		g_settings.bRCFileUTF16 = (IsDlgButtonChecked(hwnd, chx7) == BST_CHECKED);
		g_settings.bUseMSMSGTABLE = (IsDlgButtonChecked(hwnd, chx8) == BST_CHECKED);
		g_settings.bAddBomToRC = (IsDlgButtonChecked(hwnd, chx9) == BST_CHECKED);

		MStringW strBackupSuffix = GetDlgItemText(cmb1);
		mstr_trim(strBackupSuffix);
		g_settings.strBackupSuffix = strBackupSuffix;

		if (strBackupSuffix.empty())
			g_settings.bBackup = FALSE;

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
			EndDialog(hwnd, IDCANCEL);
			break;
		}
	}

	virtual INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			DO_MSG(WM_INITDIALOG, OnInitDialog);
			DO_MSG(WM_COMMAND, OnCommand);
		}
		return DefaultProcDx();
	}
};
