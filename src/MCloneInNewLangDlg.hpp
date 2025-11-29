// MCloneInNewLangDlg.hpp --- "Clone In New Language" Dialog
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
#include "Common.hpp"

//////////////////////////////////////////////////////////////////////////////

class MCloneInNewLangDlg : public MDialogBase
{
public:
	EntryBase *m_entry;
	MIdOrString m_type;
	MIdOrString m_name;
	WORD m_lang;
	MComboBoxAutoComplete m_cmb3;

	MCloneInNewLangDlg(EntryBase* entry)
		: MDialogBase(IDD_CLONEINNEWLANG), m_entry(entry),
		  m_type(entry->m_type), m_name(entry->m_name), m_lang(entry->m_lang)
	{
		m_cmb3.m_bAcceptSpace = TRUE;
		m_cmb3.m_bIgnoreCase = TRUE;
	}

	virtual INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		}
		return DefaultProcDx(hwnd, uMsg, wParam, lParam);
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		// for Types
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		InitResTypeComboBox(hCmb1, m_type);

		// for Names
		auto nIDTYPE_ = g_db.IDTypeFromResType(m_type);
		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		InitResNameComboBox(hCmb2, m_name, nIDTYPE_);

		// for Langs
		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		InitLangComboBox(hCmb3, m_lang);
		SubclassChildDx(m_cmb3, cmb3);

		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		MIdOrString type;
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		const ConstantsDB::TableType& table = g_db.GetTable(L"RESOURCE");
		INT iType = ComboBox_GetCurSel(hCmb1);
		if (iType != CB_ERR && iType < INT(table.size()))
		{
			type = WORD(table[iType].value);
		}
		else
		{
			if (!CheckTypeComboBox(hCmb1, type))
				return;
		}

		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		WORD lang;
		if (!CheckLangComboBox(hCmb3, lang))
			return;

		if (lang == m_lang)
		{
			ErrorBoxDx(IDS_SAMELANG);
			return;
		}

		if (g_res.find(ET_LANG, m_type, m_name, lang))
		{
			if (MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL) != IDYES)
			{
				return;
			}
		}

		m_lang = lang;

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
		case cmb3:
			if (codeNotify == CBN_EDITCHANGE)
			{
				m_cmb3.OnEditChange();
			}
			break;
		}
	}
};
