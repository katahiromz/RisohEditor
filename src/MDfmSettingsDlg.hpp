// MDfmSettingsDlg.hpp --- "Delphi DFM settings" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MPropSheet.hpp"
#include "settings.h"
#include "Utils.h"

BOOL IsCodePageReallyUsable(UINT cp);

//////////////////////////////////////////////////////////////////////////////

class MDfmSettingsDlg : public MPropSheetPage
{
public:
	INT m_nCodePage;
	BOOL m_bComments;
	BOOL m_bNoUnicode;

	MDfmSettingsDlg()
        : MPropSheetPage(IDD_DFMSETTINGS, LoadStringDx(IDS_DFMSETTINGS))
	{
		m_nCodePage = g_settings.nDfmCodePage;
		m_bComments = g_settings.bDfmRawTextComments;
		m_bNoUnicode = g_settings.bDfmNoUnicode;
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		ComboBox_AddString(hCmb1, TEXT("0"));
		if (IsCodePageReallyUsable(1252)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1252));
		if (IsCodePageReallyUsable(1250)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1250));
		if (IsCodePageReallyUsable(1251)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1251));
		if (IsCodePageReallyUsable(1253)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1253));
		if (IsCodePageReallyUsable(1254)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1254));
		if (IsCodePageReallyUsable(1255)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1255));
		if (IsCodePageReallyUsable(1256)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1256));
		if (IsCodePageReallyUsable(1257)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE1257));
		if (IsCodePageReallyUsable(874)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE874));
		if (IsCodePageReallyUsable(932)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE932));
		if (IsCodePageReallyUsable(936)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE936));
		if (IsCodePageReallyUsable(949)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE949));
		if (IsCodePageReallyUsable(950)) ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE950));
		ComboBox_AddString(hCmb1, LoadStringDx(IDS_CODEPAGE65001));

		TCHAR szText[MAX_PATH];
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

	// PSN_APPLY: called by the sheet frame when the user presses OK/Apply.
	BOOL OnApply(HWND hwnd, BOOL bAllPages) override
    {
        return ApplySettings(hwnd);
    }

	BOOL ApplySettings(HWND hwnd)
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

		INT nCodePage = _tcstoul(strText.c_str(), NULL, 0);
		if (nCodePage != 0 && !IsCodePageReallyUsable(nCodePage))
		{
			SetFocus(hCmb1);
			MsgBoxDx(IDS_INVALIDCODEPAGE, MB_ICONERROR);
			return FALSE;
		}
		m_nCodePage = nCodePage;

		if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
			m_bComments = TRUE;
		else
			m_bComments = FALSE;

		if (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED)
			m_bNoUnicode = TRUE;
		else
			m_bNoUnicode = FALSE;

		g_settings.nDfmCodePage = m_nCodePage;
		g_settings.bDfmRawTextComments = m_bComments;
		g_settings.bDfmNoUnicode = m_bNoUnicode;

		return TRUE;
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case chx1:
		case chx2:
			if (codeNotify == BN_CLICKED)
				SetModifiedDx();
			break;
		case cmb1:
			if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELENDOK)
				SetModifiedDx();
			break;
		}
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		case WM_NOTIFY:
			return OnNotify(hwnd, (INT)wParam, (LPNMHDR)lParam);
		}
		return 0;
	}
};
