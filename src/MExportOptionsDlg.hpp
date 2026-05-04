// MExportOptionsDlg.hpp --- "Export Options" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "RisohSettings.hpp"

BOOL IsCodePageReallyUsable(UINT cp);

//////////////////////////////////////////////////////////////////////////////

class MExportOptionsDlg : public MDialogBase
{
public:
	MExportOptionsDlg(BOOL bExport) : MDialogBase(bExport ? IDD_EXP_OPTIONS : IDD_SAVE_OPTIONS)
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
		CheckDlgButton(hwnd, chx8, g_settings.bUseMSMSGTABLE ? BST_CHECKED : BST_UNCHECKED);

		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE65001));
		ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE1200));
		if (IsCodePageReallyUsable(1252)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE1252));
		if (IsCodePageReallyUsable(1250)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE1250));
		if (IsCodePageReallyUsable(1251)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE1251));
		if (IsCodePageReallyUsable(1253)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE1253));
		if (IsCodePageReallyUsable(1254)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE1254));
		if (IsCodePageReallyUsable(1255)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE1255));
		if (IsCodePageReallyUsable(1256)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE1256));
		if (IsCodePageReallyUsable(1257)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE1257));
		if (IsCodePageReallyUsable(874)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE874));
		if (IsCodePageReallyUsable(932)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE932));
		if (IsCodePageReallyUsable(936)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE936));
		if (IsCodePageReallyUsable(949)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE949));
		if (IsCodePageReallyUsable(950)) ComboBox_AddString(hCmb2, LoadStringDx(IDS_CODEPAGE950));

		WCHAR sz[MAX_PATH];
		StringCchPrintfW(sz, _countof(sz), L"%u (", g_settings.nCodePageForRC);
		INT iItem = ComboBox_FindString(hCmb2, 0, sz);
		if (iItem == CB_ERR)
		{
			StringCchPrintfW(sz, _countof(sz), L"%u", g_settings.nCodePageForRC);
			ComboBox_SetText(hCmb2, sz);
		}
		else
		{
			ComboBox_SetCurSel(hCmb2, iItem);
		}

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
		g_settings.bUseMSMSGTABLE = (IsDlgButtonChecked(hwnd, chx8) == BST_CHECKED);

		MStringW strBackupSuffix = GetDlgItemText(cmb1);
		mstr_trim(strBackupSuffix);
		g_settings.strBackupSuffix = strBackupSuffix;

		WCHAR sz[MAX_PATH];
		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		INT iItem = ComboBox_GetCurSel(hCmb2);
		if (iItem == CB_ERR)
			ComboBox_GetText(hCmb2, sz, _countof(sz));
		else
			ComboBox_GetLBText(hCmb2, iItem, sz);
		UINT nCodePage = _wtoi(sz);
		if (nCodePage > 0)
			g_settings.nCodePageForRC = nCodePage;

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
