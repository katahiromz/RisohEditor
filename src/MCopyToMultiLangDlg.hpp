// MCopyToMultiLangDlg.hpp --- "Copy to multiple languages" Dialog
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

BOOL ParseLang(const MStringW& input, LANGID& lang);

//////////////////////////////////////////////////////////////////////////////

class MCopyToMultiLangDlg : public MDialogBase
{
public:
	EntryPtr m_entry;
	MIdOrString m_type;
	MIdOrString m_name;
	LANGID m_lang;
	MComboBoxAutoComplete m_cmb3;
	std::vector<LANGID> m_langs;
	MRisohAutoComplete *m_pAutoComplete2;

	MCopyToMultiLangDlg(EntryBase* entry)
		: MDialogBase(IDD_COPYTOMULTILANG)
		, m_entry(g_res.get_shared(entry))
		, m_type(entry->m_type)
		, m_name(entry->m_name)
		, m_lang(entry->m_lang)
		, m_pAutoComplete2(new MRisohAutoComplete(2))
	{
		m_cmb3.m_bAcceptSpace = TRUE;
		m_cmb3.m_bIgnoreCase = TRUE;
	}

	~MCopyToMultiLangDlg()
	{
		m_pAutoComplete2->unbind();
		m_pAutoComplete2->Release();
	}

	int OnVKeyToItem(HWND hwnd, UINT vk, HWND hwndListbox, int iCaret)
	{
		if (vk == VK_DELETE)
		{
			OnDelete(hwnd);
		}
		if (GetKeyState(VK_CONTROL) < 0 && vk == 'A')
		{
			OnSelectAll(hwnd);
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
		m_pAutoComplete2->bind(hwndEdit);

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
			LANGID wLang;
			ParseLang(str, wLang);
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

		LANGID wLang;
		if (ParseLang(str, wLang))
		{
			HWND hLst1 = GetDlgItem(hwnd, lst1);
			str = get_lang_label(wLang);
			INT iItem = ListBox_FindStringExact(hLst1, -1, str.c_str());
			if (iItem == LB_ERR)
			{
				iItem = ListBox_AddString(hLst1, str.c_str());
			}
			::SendMessageW(hLst1, LB_SETSEL, FALSE, -1);
			ListBox_SelItemRange(hLst1, TRUE, iItem, iItem);
			SetDlgItemTextW(hwnd, cmb3, NULL);
		}
		else
		{
			MsgBoxDx(IDS_INVALIDLANG, MB_ICONERROR);
		}
	}

	void OnSelectAll(HWND hwnd)
	{
		HWND hLst1 = GetDlgItem(hwnd, lst1);
		SendMessageW(hLst1, LB_SETSEL, TRUE, -1);
	}

	void OnDelete(HWND hwnd)
	{
		HWND hLst1 = GetDlgItem(hwnd, lst1);

		INT cSelections = (INT)::SendMessage(hLst1, LB_GETSELCOUNT, 0, 0);
		if (cSelections <= 0)
			return;

		INT selections[128];
		if (cSelections > (INT)_countof(selections))
			cSelections = (INT)_countof(selections);

		::SendMessageW(hLst1, LB_GETSELITEMS, cSelections, (LPARAM)selections);

		for (INT i = cSelections - 1; i >= 0; --i)
			ListBox_DeleteString(hLst1, selections[i]);
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
		case ID_DELETE:
			OnDelete(hwnd);
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

	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
	{
		HWND hLst1 = GetDlgItem(hwnd, lst1);
		if (hwndContext == hLst1)
		{
			PopupMenuDx(hwnd, hLst1, IDR_POPUPMENUS, 9, xPos, yPos);
		}
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
			HANDLE_MSG(hwnd, WM_VKEYTOITEM, OnVKeyToItem);
		}
		return 0;
	}
};
