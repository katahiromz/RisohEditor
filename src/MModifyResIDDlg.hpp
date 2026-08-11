// MModifyResIDDlg.hpp --- "Modify Resource ID" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "settings.h"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "Common.hpp"

//////////////////////////////////////////////////////////////////////////////

class MModifyResIDDlg : public MDialogBase
{
public:
	MString m_str1;
	MString m_str2;
	BOOL m_bIsHelpID; // TRUE if the ID type is IDTYPE_HELP (wider 32-bit range)

	MModifyResIDDlg(MString str1, MString str2, BOOL bIsHelpID = FALSE)
		: MDialogBase(IDD_MODIFYRESID), m_str1(str1), m_str2(str2),
		  m_bIsHelpID(bIsHelpID)
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		SetDlgItemText(hwnd, edt1, m_str1.c_str());

		if (m_bIsHelpID)
		{
			// Help IDs accept the full signed 32-bit range via the spinner;
			// values larger than INT_MAX must be typed manually.
			SetDlgItemText(hwnd, edt3, m_str2.c_str());
			SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE32, (WPARAM)INT_MIN, (LPARAM)INT_MAX);
		}
		else
		{
			int value = mstr_parse_int(m_str2.c_str(), true);
			SetDlgItemInt(hwnd, edt3, value, TRUE);
			SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE32, SHRT_MIN, USHRT_MAX);
		}

		UpdateText(hwnd);

		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		MString str1 = GetDlgItemText(hwnd, edt1);
		ReplaceFullWithHalf(str1);
		mstr_trim(str1);
		if (str1.empty() || mchr_is_digit(str1[0]))
		{
			HWND hEdt1 = GetDlgItem(hwnd, edt1);
			Edit_SetSel(hEdt1, 0, -1);
			SetFocus(hEdt1);
			ErrorBoxDx(IDS_ENTERTEXT);
			return;
		}
		mstr_upper(str1);
		m_str1 = std::move(str1);

		MString str3 = GetDlgItemText(hwnd, edt3);
		ReplaceFullWithHalf(str3);
		mstr_trim(str3);

		if (m_bIsHelpID)
		{
			// Help ID: accept signed [-2147483648, 2147483647] or unsigned [0, 4294967295]
			DWORD dw;
			if (str3.empty() || !IsValidHelpIDText(str3.c_str(), &dw))
			{
				dw = ULONG_MAX;
				SetDlgItemInt(hwnd, edt3, dw, FALSE);
				HWND hEdt3 = GetDlgItem(hwnd, edt3);
				Edit_SetSel(hEdt3, 0, -1);
				SetFocus(hEdt3);
				ErrorBoxDx(IDS_ENTERINT);
				return;
			}
		}
		else
		{
			INT value = mstr_parse_int(str3.c_str());
			if (str3.empty() || value < SHRT_MIN || value > USHRT_MAX)
			{
				value = std::min(std::max(value, SHRT_MIN), USHRT_MAX);
				SetDlgItemInt(hwnd, edt3, value, TRUE);
				HWND hEdt3 = GetDlgItem(hwnd, edt3);
				Edit_SetSel(hEdt3, 0, -1);
				SetFocus(hEdt3);
				ErrorBoxDx(IDS_ENTERINT);
				return;
			}
		}
		m_str2 = std::move(str3);

		EndDialog(IDOK);
	}

	void UpdateText(HWND hwnd)
	{
		MString name = GetDlgItemText(hwnd, edt1);
		mstr_trim(name);

		size_t pos = name.find(L'_');
		MString prefix = (pos == MString::npos) ? MString() : name.substr(0, pos + 1);

		std::vector<INT> indexes = GetPrefixIndexes(prefix);
		if (indexes.size() == 1)
		{
			SetDlgItemText(hwnd, edt2, MapIDType(IDTYPE_(indexes[0])).c_str());
		}
		else
		{
			SetDlgItemText(hwnd, edt2, NULL);
		}
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
		case edt1:
			if (codeNotify == EN_CHANGE)
			{
				UpdateText(hwnd);
			}
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
		}
		return 0;
	}
};
