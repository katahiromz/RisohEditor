// MStringListDlg.hpp
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "MWindowBase.hpp"
#include "MString.hpp"
#include "DlgInitRes.hpp"

//////////////////////////////////////////////////////////////////////////////

class MStringListDlg : public MDialogBase
{
public:
	std::vector<MStringA>& m_str_list;

	MStringListDlg(std::vector<MStringA>& str_list) :
		MDialogBase(IDD_STRINGLIST), m_str_list(str_list)
	{
	}

	virtual ~MStringListDlg()
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		MStringA text = mstr_join(m_str_list, "\r\n");
		SetDlgItemTextA(hwnd, edt1, text.c_str());

		MString str = LoadStringDx(IDS_DLGINIT1);
		str += LoadStringDx(IDS_DLGINIT2);
		str += LoadStringDx(IDS_DLGINIT3);
		SetDlgItemText(hwnd, stc1, str.c_str());

		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		MString str = GetDlgItemText(hwnd, edt1);
		mstr_trim(str);

		if (str.empty())
		{
			m_str_list.clear();
			EndDialog(IDOK);
			return;
		}

		MStringA strA = MTextToAnsi(CP_ACP, str.c_str()).c_str();
		mstr_trim(strA);

		std::vector<MStringA> lines;
		mstr_replace_all(strA, "\r", "");
		mstr_split(lines, strA, "\n");
		m_str_list = lines;

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
