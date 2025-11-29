// MDfmSettingsDlg.hpp --- "Delphi DFM settings" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "Common.hpp"

//////////////////////////////////////////////////////////////////////////////

class MDfmSettingsDlg : public MDialogBase
{
public:
	INT m_nCodePage;
	BOOL m_bComments;
	BOOL m_bNoUnicode;

	MDfmSettingsDlg() : MDialogBase(IDD_DFMSETTINGS)
	{
		m_nCodePage = g_settings.nDfmCodePage;
		m_bComments = g_settings.bDfmRawTextComments;
		m_bNoUnicode = g_settings.bDfmNoUnicode;
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		ComboBox_AddString(hCmb1, TEXT("0"));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1252));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1250));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1251));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1253));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1254));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1255));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1256));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1257));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE874));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE932));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE936));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE949));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE950));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE65001));

		TCHAR szText[32];
		StringCbPrintf(szText, sizeof(szText), TEXT("%u"), m_nCodePage);
		ComboBox_SetText(hCmb1, szText);

		if (m_bComments)
			CheckDlgButton(hwnd, chx1, BST_CHECKED);
		else
			CheckDlgButton(hwnd, chx1, BST_UNCHECKED);

		if (m_bNoUnicode)
			CheckDlgButton(hwnd, chx2, BST_CHECKED);
		else
			CheckDlgButton(hwnd, chx2, BST_UNCHECKED);

		CenterWindowDx();

		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);

		MString strText;
		INT iItem = ComboBox_GetCurSel(hCmb1);
		if (iItem == CB_ERR)
		{
			strText = GetComboBoxText(hCmb1);
		}
		else
		{
			strText = GetComboBoxLBText(hCmb1, iItem);
		}
		m_nCodePage = _tcstoul(strText.c_str(), NULL, 0);

		if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
			m_bComments = TRUE;
		else
			m_bComments = FALSE;

		if (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED)
			m_bNoUnicode = TRUE;
		else
			m_bNoUnicode = FALSE;

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
		return DefaultProcDx();
	}
};
