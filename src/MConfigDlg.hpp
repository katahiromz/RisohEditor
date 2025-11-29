// MConfigDlg.hpp --- "Configuration" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "MMacrosDlg.hpp"
#include "MPathsDlg.hpp"
#include "MFontsDlg.hpp"
#include "MLangAutoComplete.hpp"

//////////////////////////////////////////////////////////////////////////////

class MConfigDlg : public MDialogBase
{
public:
	MComboBoxAutoComplete m_cmb3;
	MLangAutoComplete *m_pAutoComplete;

	MConfigDlg()
		: MDialogBase(IDD_CONFIG)
		, m_pAutoComplete(new MLangAutoComplete())
	{
		m_cmb3.m_bAcceptSpace = TRUE;
		m_cmb3.m_bIgnoreCase = TRUE;
	}

	~MConfigDlg()
	{
		if (m_pAutoComplete)
		{
			m_pAutoComplete->unbind();
			m_pAutoComplete->Release();
			m_pAutoComplete = NULL;
		}
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
		SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)L"-old");
		SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)L"-bak");
		SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)L"~");
		SetDlgItemTextW(hwnd, cmb2, g_settings.strBackupSuffix.c_str());
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
		InitLangComboBox(hCmb3, g_settings.nDefResLangID);
		SubclassChildDx(m_cmb3, cmb3);

		// auto complete
		if (m_pAutoComplete)
		{
			COMBOBOXINFO info = { sizeof(info) };
			GetComboBoxInfo(m_cmb3, &info);
			HWND hwndEdit = info.hwndItem;
			m_pAutoComplete->bind(hwndEdit);
		}

		Reload(hwnd);
		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		WORD lang;
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
			return;
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

		MStringW strBackupSuffix = GetDlgItemText(cmb2);
		mstr_trim(strBackupSuffix);
		g_settings.strBackupSuffix = strBackupSuffix;

		if (strBackupSuffix.empty())
			g_settings.bBackup = FALSE;

		EndDialog(IDOK);
	}

	void OnPsh1(HWND hwnd)
	{
		MMacrosDlg dialog;
		dialog.DialogBoxDx(hwnd);
	}

	void OnPsh2(HWND hwnd)
	{
		MPathsDlg dialog;
		dialog.DialogBoxDx(hwnd);
	}

	void OnPsh3(HWND hwnd)
	{
		SendMessage(GetParent(hwnd), WM_COMMAND, ID_SETDEFAULTS, 0);
		Reload(hwnd);
		EndDialog(hwnd, IDOK);
	}

	void OnPsh4(HWND hwnd)
	{
		MFontsDlg dialog;
		dialog.DialogBoxDx(hwnd);
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
		case psh1:
			OnPsh1(hwnd);
			break;
		case psh2:
			OnPsh2(hwnd);
			break;
		case psh3:
			OnPsh3(hwnd);
			break;
		case psh4:
			OnPsh4(hwnd);
			break;
		case cmb3:
			if (codeNotify == CBN_EDITCHANGE)
			{
				m_cmb3.OnEditChange();  // input completion
			}
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
