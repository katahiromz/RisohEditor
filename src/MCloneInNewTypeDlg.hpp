// MCloneInNewTypeDlg.hpp --- "Clone In New Type" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "MRisohAutoComplete.hpp"
#include "Common.hpp"

//////////////////////////////////////////////////////////////////////////////

class MCloneInNewTypeDlg : public MDialogBase
{
public:
	EntryPtr m_entry;
	MIdOrString m_old_type;
	MIdOrString m_new_type;
	MComboBoxAutoComplete m_cmb1;
	MComboBoxAutoComplete m_cmb2;
	MRisohAutoComplete *m_pAutoComplete0;

	MCloneInNewTypeDlg(EntryBase *entry)
		: MDialogBase(IDD_CLONEINNEWTYPE), m_entry(g_res.get_shared(entry))
		, m_old_type(entry->m_type)
		, m_pAutoComplete0(new MRisohAutoComplete(0))
	{
		m_new_type = BAD_TYPE;
	}

	~MCloneInNewTypeDlg()
	{
		m_pAutoComplete0->unbind();
		m_pAutoComplete0->Release();
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

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		// for Types
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		InitResTypeComboBox(hCmb1, m_old_type);
		SubclassChildDx(m_cmb1, cmb1);
		::EnableWindow(hCmb1, FALSE);

		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		InitResTypeComboBox(hCmb2, m_old_type);
		SubclassChildDx(m_cmb2, cmb2);

		COMBOBOXINFO info = { sizeof(info) };
		GetComboBoxInfo(m_cmb2, &info);
		HWND hwndEdit = info.hwndItem;
		m_pAutoComplete0->bind(hwndEdit);

		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		MIdOrString old_type;
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		if (!CheckTypeComboBox(hCmb1, old_type))
			return;

		MIdOrString new_type;
		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		if (!CheckTypeComboBox(hCmb2, new_type))
			return;

		if (old_type == new_type)
		{
			ErrorBoxDx(IDS_SAMETYPE);
			return;
		}

		if (g_res.find(ET_TYPE, new_type, BAD_NAME, BAD_LANG))
		{
			if (MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL) != IDYES)
				return;
		}

		m_new_type = new_type;
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
		case cmb1:
			if (codeNotify == CBN_EDITCHANGE)
			{
				m_cmb1.OnEditChange();
			}
			break;
		case cmb2:
			if (codeNotify == CBN_EDITCHANGE)
			{
				m_cmb2.OnEditChange();
			}
			break;
		}
	}
};
