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
#include "Utils.h"

//////////////////////////////////////////////////////////////////////////////

class MCloneInNewLangDlg : public MDialogBase
{
public:
	EntryPtr m_entry;
	MIdOrString m_type;
	MIdOrString m_name;
	LANGID m_lang;
	MComboBoxAutoComplete m_cmb3;
	MRisohAutoComplete *m_pAutoComplete2;

	MCloneInNewLangDlg(EntryBase* entry)
		: MDialogBase(IDD_CLONEINNEWLANG)
		, m_entry(g_res.get_shared(entry))
		, m_type(entry->m_type)
		, m_name(entry->m_name)
		, m_lang(entry->m_lang)
		, m_pAutoComplete2(new MRisohAutoComplete(2))
	{
		m_cmb3.m_bAcceptSpace = TRUE;
		m_cmb3.m_bIgnoreCase = TRUE;
	}

	~MCloneInNewLangDlg()
	{
		m_pAutoComplete2->unbind();
		m_pAutoComplete2->Release();
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
		auto nIDTYPE_ = g_db.IDTypeFromResType(m_type);
		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		InitResNameComboBox(hCmb2, m_name, nIDTYPE_);

		// for Langs
		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		InitLangComboBox(hCmb3, m_lang);
		SubclassChildDx(m_cmb3, cmb3);

		COMBOBOXINFO info = { sizeof(info) };
		GetComboBoxInfo(m_cmb3, &info);
		HWND hwndEdit = info.hwndItem;
		m_pAutoComplete2->bind(hwndEdit);

		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		MIdOrString type;
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		if (!CheckTypeComboBox(hCmb1, type))
			return;

		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		LANGID lang;
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
