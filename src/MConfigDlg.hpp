// MConfigDlg.hpp --- "Configuration" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "MPropSheet.hpp"
#include "settings.h"
#include "ConstantsDB.hpp"
#include "MPathsDlg.hpp"
#include "MFontsDlg.hpp"
#include "MCryptoDlg.hpp"
#include "MRisohAutoComplete.hpp"
#ifdef ENABLE_CRYPTO
	#include "WonResWrap.h"
#endif

//////////////////////////////////////////////////////////////////////////////

class MConfigDlg : public MPropSheetPage
{
public:
	MComboBoxAutoComplete m_cmb3;
	MRisohAutoComplete *m_pAutoComplete2;

	MConfigDlg()
		: MPropSheetPage(IDD_CONFIG, LoadStringDx(IDS_GENERAL))
		, m_pAutoComplete2(new MRisohAutoComplete(2))
	{
		m_cmb3.m_bAcceptSpace = TRUE;
		m_cmb3.m_bIgnoreCase = TRUE;
	}

	~MConfigDlg()
	{
		m_pAutoComplete2->unbind();
		m_pAutoComplete2->Release();
	}

	void Reload(HWND hwnd)
	{
		CheckDlgButton(hwnd, chx1, g_settings.bShowFullPath ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx2, g_settings.bHideID ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx3, g_settings.bResumeWindowPos ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx4, g_settings.bAutoLoadNearbyResH ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx5, g_settings.bAutoShowIDList ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx6, g_settings.bShowDotsOnDialog ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(hwnd, edt1, g_settings.nComboHeight, FALSE);
		CheckDlgButton(hwnd, chx7, g_settings.bAskUpdateResH ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx8, g_settings.bCompressByUPX ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx9, g_settings.bWordWrap ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemText(hwnd, cmb1, g_settings.strAtlAxWin.c_str());

		CheckDlgButton(hwnd, chx10, g_settings.bBackup ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, chx11, g_settings.bUseWonRes ? BST_CHECKED : BST_UNCHECKED);
		SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)L"-old");
		SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)L"-bak");
		SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)L"~");
		SetDlgItemTextW(hwnd, cmb2, g_settings.strBackupSuffix.c_str());

		EnableWindow(GetDlgItem(hwnd, psh5), g_settings.bUseWonRes);
	}

	void Cmb1_AddString(HWND hwnd, LPCWSTR text)
	{
		if ((INT)SendDlgItemMessage(hwnd, cmb1, CB_FINDSTRINGEXACT, -1, (LPARAM)text) == CB_ERR)
		{
			SendDlgItemMessage(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)text);
		}
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(9999, -9999));
		Cmb1_AddString(hwnd, TEXT("AtlAxWin"));
		Cmb1_AddString(hwnd, TEXT("AtlAxWin71"));
		Cmb1_AddString(hwnd, TEXT("AtlAxWin80"));
		Cmb1_AddString(hwnd, TEXT("AtlAxWin90"));
		Cmb1_AddString(hwnd, TEXT("AtlAxWin100"));
		Cmb1_AddString(hwnd, TEXT("AtlAxWin110"));
#ifdef ATL_SUPPORT
		Cmb1_AddString(hwnd, TEXT(ATLAXWIN_CLASS));
#endif

		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		InitLangComboBox(hCmb3, (LANGID)g_settings.nDefResLangID);
		SubclassChildDx(m_cmb3, cmb3);

#ifndef ENABLE_CRYPTO
		EnableWindow(GetDlgItem(hwnd, psh5), FALSE);
#endif

		// auto complete
		COMBOBOXINFO info = { sizeof(info) };
		GetComboBoxInfo(m_cmb3, &info);
		HWND hwndEdit = info.hwndItem;
		m_pAutoComplete2->bind(hwndEdit);

		Reload(hwnd);
		return TRUE;
	}

	// Was OnOK(HWND). Now invoked from OnApply (PSN_APPLY) instead of
	// from an IDOK button, since a property page has no OK button of
	// its own -- the sheet frame supplies one shared OK/Cancel/Apply.
	BOOL ApplySettings(HWND hwnd)
	{
		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		LANGID lang;
		if (CheckLangComboBox(hCmb3, lang, LANG_TYPE_2))
			g_settings.nDefResLangID = lang;
		else
			g_settings.nDefResLangID = BAD_LANG;

		BOOL bTranslated = FALSE;
		INT nHeight = GetDlgItemInt(hwnd, edt1, &bTranslated, FALSE);
		if (!bTranslated)
		{
			HWND hEdt1 = GetDlgItem(hwnd, edt1);
			Edit_SetSel(hEdt1, 0, -1);
			SetFocus(hEdt1);
			ErrorBoxDx(IDS_ENTERINT);
			return FALSE;
		}
		g_settings.nComboHeight = nHeight;

		g_settings.bShowFullPath = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
		g_settings.bHideID = (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
		g_settings.bResumeWindowPos = (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);
		g_settings.bAutoLoadNearbyResH = (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
		g_settings.bAutoShowIDList = (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED);
		g_settings.bShowDotsOnDialog = (IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED);
		g_settings.bAskUpdateResH = (IsDlgButtonChecked(hwnd, chx7) == BST_CHECKED);
		g_settings.bCompressByUPX = (IsDlgButtonChecked(hwnd, chx8) == BST_CHECKED);
		g_settings.bWordWrap = (IsDlgButtonChecked(hwnd, chx9) == BST_CHECKED);

		MStringW strAtlAxWin = GetDlgItemText(cmb1);
		mstr_trim(strAtlAxWin);
		g_settings.strAtlAxWin = strAtlAxWin;

		g_settings.bBackup = (IsDlgButtonChecked(hwnd, chx10) == BST_CHECKED);
		g_settings.bUseWonRes = (IsDlgButtonChecked(hwnd, chx11) == BST_CHECKED);

		MStringW strBackupSuffix = GetDlgItemText(cmb2);
		mstr_trim(strBackupSuffix);
		g_settings.strBackupSuffix = strBackupSuffix;

		if (strBackupSuffix.empty())
			g_settings.bBackup = FALSE;

		s_pMainWnd->RefreshRadBackBrush();
		s_pMainWnd->RefreshFonts();
		s_pMainWnd->RefreshCode();
		s_pMainWnd->UpdateTitleBar();
		s_pMainWnd->UpdateNames(FALSE);
		s_pMainWnd->UpdateMenu();
		return TRUE;
	}

	// PSN_APPLY: called by the sheet frame when the user presses OK/Apply.
	BOOL OnApply(HWND hwnd, BOOL bAllPages) override
	{
		return ApplySettings(hwnd);
	}

	void OnPsh5(HWND hwnd)
	{
#ifdef ENABLE_CRYPTO
		MCryptoDlg dialog;
		dialog.DialogBoxDx(hwnd);
		if (g_bEnableCrypto && g_password.size())
			CheckDlgButton(hwnd, chx11, BST_CHECKED);
#endif
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		if (chx1 <= id && id <= chx11)
		{
			SetModifiedDx();
		}
		else if (id == edt1 && codeNotify == EN_CHANGE)
		{
			SetModifiedDx();
		}
		else if ((id == cmb1 || id == cmb2 || id == cmb3) &&
			     (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELENDOK))
		{
			SetModifiedDx();
		}

		switch (id)
		{
		case psh5:
			OnPsh5(hwnd);
			break;
		case cmb3:
			if (codeNotify == CBN_EDITCHANGE)
			{
				m_cmb3.OnEditChange();  // input completion
			}
			break;
		case chx11:
			if (IsDlgButtonChecked(hwnd, chx11) == BST_CHECKED)
				EnableWindow(GetDlgItem(hwnd, psh5), TRUE);
			else
				EnableWindow(GetDlgItem(hwnd, psh5), FALSE);
			break;
		}
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
			DO_MSG(WM_INITDIALOG, OnInitDialog);
			DO_MSG(WM_COMMAND, OnCommand);
		}
		// Handles WM_NOTIFY (PSN_SETACTIVE/PSN_KILLACTIVE/PSN_APPLY/...)
		// among other defaults.
		return MPropSheetPage::DialogProcDx(hwnd, uMsg, wParam, lParam);
	}
};
