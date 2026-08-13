// MCloneInNewNameDlg.hpp --- "Clone In New Name" Dialog
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
#include "Utils.h"

//////////////////////////////////////////////////////////////////////////////

class MCloneInNewNameDlg : public MDialogBase
{
public:
	EntryPtr m_entry;
	MIdOrString m_type;
	MIdOrString m_name;
	LANGID m_lang;
	MComboBoxAutoComplete m_cmb2;
	MRisohAutoComplete *m_pAutoComplete1;

	MCloneInNewNameDlg(EntryBase *entry)
		: MDialogBase(IDD_CLONEINNEWNAME)
		, m_entry(g_res.get_shared(entry))
		, m_type(entry->m_type)
		, m_name(entry->m_name)
		, m_lang(entry->m_lang)
		, m_pAutoComplete1(new MRisohAutoComplete(1, FALSE, entry->m_type))
	{
	}

	~MCloneInNewNameDlg()
	{
		m_pAutoComplete1->unbind();
		m_pAutoComplete1->Release();
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
		InitResTypeComboBox(hCmb1, m_type);

		// for Names
		IDTYPE_ nIDTYPE_ = g_db.IDTypeFromResType(m_type);
		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		InitResNameComboBox(hCmb2, m_name, nIDTYPE_);
		SubclassChildDx(m_cmb2, cmb2);

		COMBOBOXINFO info = { sizeof(info) };
		GetComboBoxInfo(m_cmb2, &info);
		HWND hwndEdit = info.hwndItem;
		m_pAutoComplete1->bind(hwndEdit);

		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		MIdOrString type;
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		if (!CheckTypeComboBox(hCmb1, type))
			return;

		// for Names
		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		MIdOrString name;
		if (!CheckNameComboBox(hCmb2, type, name))
			return;

		if (m_name == name)
		{
			ErrorBoxDx(IDS_SAMENAME);
			return;
		}

		if (g_res.find(ET_NAME, m_type, name, m_lang))
		{
			if (MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL) != IDYES)
			{
				return;
			}
		}

		m_name = name;

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
		case psh1:
			OnPsh1(hwnd);
			break;
		case cmb2:
			if (codeNotify == CBN_EDITCHANGE)
			{
				m_cmb2.OnEditChange();
			}
			break;
		}
	}

	void OnPsh1(HWND hwnd)
	{
		SendMessage(GetParent(hwnd), WM_COMMAND, ID_IDLIST, 0);
	}
};
