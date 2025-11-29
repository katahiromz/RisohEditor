// MModifyAssocDlg.hpp --- "Modify Association" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "MWindowBase.hpp"
#include "MString.hpp"

//////////////////////////////////////////////////////////////////////////////

class MModifyAssocDlg : public MDialogBase
{
public:
	MString& m_text1;
	MString& m_text2;

	MModifyAssocDlg(MString& text1, MString& text2)
		: MDialogBase(IDD_MODIFYASSOC), m_text1(text1), m_text2(text2)
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		SetDlgItemText(hwnd, edt1, m_text1.c_str());
		SetDlgItemText(hwnd, edt2, m_text2.c_str());
		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		MString str = GetDlgItemText(edt2);
		mstr_trim(str);

		if (str.empty())
		{
			HWND hEdt2 = GetDlgItem(hwnd, edt2);
			Edit_SetSel(hEdt2, 0, -1);
			SetFocus(hEdt2);
			ErrorBoxDx(IDS_EMPTYSTR);
			return;
		}

		m_text2 = str;
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
		default:
			return DefaultProcDx();
		}
	}
};
