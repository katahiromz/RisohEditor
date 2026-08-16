// MDialogFontSubstDlg --- "Replacing Dialog Fonts" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2020-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "MPropSheet.hpp"
#include "settings.h"
#include "MComboBoxAutoComplete.hpp"
#include "Utils.h"

extern std::vector<MString> g_font_names;
BOOL InitFontNames(void);

//////////////////////////////////////////////////////////////////////////////

class MDialogFontSubstDlg : public MPropSheetPage
{
public:
	MComboBoxAutoComplete m_cmb1;
	MComboBoxAutoComplete m_cmb2;
	MComboBoxAutoComplete m_cmb3;
	MComboBoxAutoComplete m_cmb4;
	MComboBoxAutoComplete m_cmb5;
	MComboBoxAutoComplete m_cmb6;

	MDialogFontSubstDlg()
		: MPropSheetPage(IDD_FONTSUBST, LoadStringDx(IDS_DIALOGFONTSUBST))
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		//InitFontComboBox(GetDlgItem(hwnd, cmb1));
		//InitFontComboBox(GetDlgItem(hwnd, cmb2));
		//InitFontComboBox(GetDlgItem(hwnd, cmb3));
		//InitFontComboBox(GetDlgItem(hwnd, cmb4));
		//InitFontComboBox(GetDlgItem(hwnd, cmb5));
		//InitFontComboBox(GetDlgItem(hwnd, cmb6));

		SetDlgItemTextW(hwnd, cmb1, g_settings.strFontReplaceFrom1.c_str());
		SetDlgItemTextW(hwnd, cmb2, g_settings.strFontReplaceTo1.c_str());
		SetDlgItemTextW(hwnd, cmb3, g_settings.strFontReplaceFrom2.c_str());
		SetDlgItemTextW(hwnd, cmb4, g_settings.strFontReplaceTo2.c_str());
		SetDlgItemTextW(hwnd, cmb5, g_settings.strFontReplaceFrom3.c_str());
		SetDlgItemTextW(hwnd, cmb6, g_settings.strFontReplaceTo3.c_str());

		SubclassChildDx(m_cmb1, cmb1);
		SubclassChildDx(m_cmb2, cmb2);
		SubclassChildDx(m_cmb3, cmb3);
		SubclassChildDx(m_cmb4, cmb4);
		SubclassChildDx(m_cmb5, cmb5);
		SubclassChildDx(m_cmb6, cmb6);

		InitFontNames();

		SendMessageW(m_cmb1, WM_SETREDRAW, FALSE, 0);
		SendMessageW(m_cmb2, WM_SETREDRAW, FALSE, 0);
		SendMessageW(m_cmb3, WM_SETREDRAW, FALSE, 0);
		SendMessageW(m_cmb4, WM_SETREDRAW, FALSE, 0);
		SendMessageW(m_cmb5, WM_SETREDRAW, FALSE, 0);
		SendMessageW(m_cmb6, WM_SETREDRAW, FALSE, 0);
		for (auto& name : g_font_names)
		{
			ComboBox_AddString(m_cmb1, name.c_str());
			ComboBox_AddString(m_cmb2, name.c_str());
			ComboBox_AddString(m_cmb3, name.c_str());
			ComboBox_AddString(m_cmb4, name.c_str());
			ComboBox_AddString(m_cmb5, name.c_str());
			ComboBox_AddString(m_cmb6, name.c_str());
		}
		SendMessageW(m_cmb1, WM_SETREDRAW, TRUE, 0);
		SendMessageW(m_cmb2, WM_SETREDRAW, TRUE, 0);
		SendMessageW(m_cmb3, WM_SETREDRAW, TRUE, 0);
		SendMessageW(m_cmb4, WM_SETREDRAW, TRUE, 0);
		SendMessageW(m_cmb5, WM_SETREDRAW, TRUE, 0);
		SendMessageW(m_cmb6, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(m_cmb1, NULL, TRUE);
		InvalidateRect(m_cmb2, NULL, TRUE);
		InvalidateRect(m_cmb3, NULL, TRUE);
		InvalidateRect(m_cmb4, NULL, TRUE);
		InvalidateRect(m_cmb5, NULL, TRUE);
		InvalidateRect(m_cmb6, NULL, TRUE);

		CenterWindowDx();
		return TRUE;
	}

	// PSN_APPLY: called by the sheet frame when the user presses OK/Apply.
	BOOL OnApply(HWND hwnd, BOOL bAllPages) override
	{
		return ApplySubst(hwnd);
	}

	BOOL ApplySubst(HWND hwnd)
	{
		auto strFontReplaceFrom1 = GetDlgItemText(cmb1);
		auto strFontReplaceTo1 = GetDlgItemText(cmb2);
		auto strFontReplaceFrom2 = GetDlgItemText(cmb3);
		auto strFontReplaceTo2 = GetDlgItemText(cmb4);
		auto strFontReplaceFrom3 = GetDlgItemText(cmb5);
		auto strFontReplaceTo3 = GetDlgItemText(cmb6);

		mstr_trim(strFontReplaceFrom1);
		mstr_trim(strFontReplaceTo1);
		mstr_trim(strFontReplaceFrom2);
		mstr_trim(strFontReplaceTo2);
		mstr_trim(strFontReplaceFrom3);
		mstr_trim(strFontReplaceTo3);

		BOOL bChanged = FALSE;
		if (!bChanged) bChanged = (g_settings.strFontReplaceFrom1 != strFontReplaceFrom1);
		if (!bChanged) bChanged = (g_settings.strFontReplaceTo1 != strFontReplaceTo1);
		if (!bChanged) bChanged = (g_settings.strFontReplaceFrom2 != strFontReplaceFrom2);
		if (!bChanged) bChanged = (g_settings.strFontReplaceTo2 != strFontReplaceTo2);
		if (!bChanged) bChanged = (g_settings.strFontReplaceFrom3 != strFontReplaceFrom3);
		if (!bChanged) bChanged = (g_settings.strFontReplaceTo3 != strFontReplaceTo3);

		if (bChanged)
		{
			g_settings.strFontReplaceFrom1 = strFontReplaceFrom1;
			g_settings.strFontReplaceTo1 = strFontReplaceTo1;
			g_settings.strFontReplaceFrom2 = strFontReplaceFrom2;
			g_settings.strFontReplaceTo2 = strFontReplaceTo2;
			g_settings.strFontReplaceFrom3 = strFontReplaceFrom3;
			g_settings.strFontReplaceTo3 = strFontReplaceTo3;

			if (IsWindowVisible(s_pMainWnd->m_rad_window))
			{
				GetWindowRect(s_pMainWnd->m_rad_window, &s_pMainWnd->m_rcRadWindow);
				s_pMainWnd->DestroyRadWindow();
				SendMessageW(*s_pMainWnd, MYWM_REOPENRAD, 0, 0);
			}
		}

		return TRUE;
	}

	void OnReset(HWND hwnd)
	{
		SetDlgItemTextW(hwnd, cmb1, L"MS Shell Dlg");
		SetDlgItemTextW(hwnd, cmb2, L"MS Shell Dlg");
		SetDlgItemTextW(hwnd, cmb3, L"MS Shell Dlg 2");
		SetDlgItemTextW(hwnd, cmb4, L"MS Shell Dlg 2");
		SetDlgItemTextW(hwnd, cmb5, L"");
		SetDlgItemTextW(hwnd, cmb6, L"");
		SetModifiedDx();
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case psh1:
			OnReset(hwnd);
			break;

		case cmb1:
			if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELENDOK)
			{
				m_cmb1.OnEditChange();
				SetModifiedDx();
			}
			break;

		case cmb2:
			if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELENDOK)
			{
				m_cmb2.OnEditChange();
				SetModifiedDx();
			}
			break;

		case cmb3:
			if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELENDOK)
			{
				m_cmb3.OnEditChange();
				SetModifiedDx();
			}
			break;

		case cmb4:
			if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELENDOK)
			{
				m_cmb4.OnEditChange();
				SetModifiedDx();
			}
			break;

		case cmb5:
			if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELENDOK)
			{
				m_cmb5.OnEditChange();
				SetModifiedDx();
			}
			break;

		case cmb6:
			if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELENDOK)
			{
				m_cmb6.OnEditChange();
				SetModifiedDx();
			}
			break;
		}
	}

	LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
	{
		return 0;
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		case WM_NOTIFY:
			{
				LPNMHDR pnmhdr = (LPNMHDR)lParam;
				if (pnmhdr->hwndFrom == ::GetParent(hwnd))
				{
					return MPropSheetPage::OnNotify(hwnd, (INT)wParam, pnmhdr);
				}
				return OnNotify(hwnd, (INT)wParam, pnmhdr);
			}
		}
		return 0;
	}
};
