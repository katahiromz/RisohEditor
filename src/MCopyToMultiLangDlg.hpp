// MCopyToMultiLangDlg.hpp --- "Copy to multiple languages" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "MLangAutoComplete.hpp"
#include "Common.hpp"

//////////////////////////////////////////////////////////////////////////////

class MCopyToMultiLangDlg : public MDialogBase
{
public:
	EntryBase *m_entry;
	MIdOrString m_type;
	MIdOrString m_name;
	WORD m_lang;
	MComboBoxAutoComplete m_cmb3;
	std::vector<LANGID> m_langs;
	MLangAutoComplete *m_pAutoComplete;

	MCopyToMultiLangDlg(EntryBase* entry)
		: MDialogBase(IDD_COPYTOMULTILANG), m_entry(entry),
		  m_type(entry->m_type), m_name(entry->m_name), m_lang(entry->m_lang),
		  m_pAutoComplete(new MLangAutoComplete())
	{
		m_cmb3.m_bAcceptSpace = TRUE;
		m_cmb3.m_bIgnoreCase = TRUE;
	}

	~MCopyToMultiLangDlg()
	{
		if (m_pAutoComplete)
		{
			m_pAutoComplete->unbind();
			m_pAutoComplete->Release();
			m_pAutoComplete = NULL;
		}
	}

	virtual INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_VKEYTOITEM, OnVKeyToItem);
		}
		return DefaultProcDx(hwnd, uMsg, wParam, lParam);
	}

	int OnVKeyToItem(HWND hwnd, UINT vk, HWND hwndListbox, int iCaret)
	{
		if (vk == VK_DELETE)
		{
			OnDelete(hwnd);
		}
		return SetDlgMsgResult(hwnd, WM_VKEYTOITEM, -1);
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		// for Langs
		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		InitLangComboBox(hCmb3, BAD_LANG);
		SubclassChildDx(m_cmb3, cmb3);

		// auto complete
		COMBOBOXINFO info = { sizeof(info) };
		GetComboBoxInfo(m_cmb3, &info);
		HWND hwndEdit = info.hwndItem;
		m_pAutoComplete->bind(hwndEdit);

		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		HWND hLst1 = GetDlgItem(hwnd, lst1);

		m_langs.clear();

		INT nCount = ListBox_GetCount(hLst1);
		if (nCount == 0)
		{
			MsgBoxDx(IDS_NOSELECTION, MB_ICONERROR);
			return;
		}

		for (INT iItem = 0; iItem < nCount; ++iItem)
		{
			MStringW str = GetListBoxText(hLst1, iItem);
			if (str.empty())
				continue;
			WORD wLang = LangFromText(&str[0]);
			m_langs.push_back(wLang);
		}

		BOOL bOverwrite = FALSE;
		for (auto lang : m_langs)
		{
			if (!bOverwrite && g_res.find(ET_LANG, m_type, m_name, lang))
			{
				if (MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL) != IDYES)
				{
					return;
				}
				bOverwrite = TRUE;
			}
		}

		EndDialog(IDOK);
	}

	void OnAddItem(HWND hwnd)
	{
		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		INT iItem = ComboBox_GetCurSel(hCmb3);
		MStringW str;
		if (iItem == CB_ERR)
		{
			str = ::GetDlgItemTextW(hwnd, cmb3);
		}
		else
		{
			str = GetComboBoxLBText(hCmb3, iItem);
		}

		mstr_trim(str);
		if (str.empty())
		{
			MsgBoxDx(IDS_INVALIDLANG, MB_ICONERROR);
			return;
		}

		WORD wLang = LangFromText(&str[0]);
		if (wLang != BAD_LANG)
		{
			HWND hLst1 = GetDlgItem(hwnd, lst1);
			str = TextFromLang(wLang);
			INT iItem = ListBox_FindStringExact(hLst1, -1, str.c_str());
			if (iItem == LB_ERR)
			{
				iItem = ListBox_AddString(hLst1, str.c_str());
			}
			ListBox_SelItemRange(hLst1, TRUE, iItem, iItem);
			SetDlgItemTextW(hwnd, cmb3, NULL);
		}
		else
		{
			MsgBoxDx(IDS_INVALIDLANG, MB_ICONERROR);
		}
	}

	void OnDelete(HWND hwnd)
	{
		HWND hLst1 = GetDlgItem(hwnd, lst1);

		INT iItem = ListBox_GetCurSel(hLst1);
		if (iItem == LB_ERR)
			return;

		ListBox_DeleteString(hLst1, iItem);

		INT nCount = ListBox_GetCount(hLst1);
		if (iItem == nCount)
			--iItem;

		ListBox_SetCurSel(hLst1, iItem);
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
			OnAddItem(hwnd);
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
