// MEncryptTypesDlg.hpp
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MString.hpp"
#include <commdlg.h>
#include "WonResWrap.h"

extern std::vector<MString> g_encrypted_types;
MStringW get_type_label(const MIdOrString& type);

//////////////////////////////////////////////////////////////////////////////

class MEncryptTypesDlg : public MDialogBase
{
public:
	MComboBoxAutoComplete m_cmb1;
	MRisohAutoComplete *m_pAutoComplete0;

	MEncryptTypesDlg()
		: MDialogBase(IDD_ENCRYPTTYPES)
		, m_pAutoComplete0(new MRisohAutoComplete(0))
	{
	}

	~MEncryptTypesDlg()
	{
		m_pAutoComplete0->unbind();
		m_pAutoComplete0->Release();
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		// for Types
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		InitResTypeComboBox(hCmb1, BAD_TYPE);
		SubclassChildDx(m_cmb1, cmb1);

		// auto complete
		COMBOBOXINFO info = { sizeof(info) };
		GetComboBoxInfo(m_cmb1, &info);
		HWND hwndEdit = info.hwndItem;
		m_pAutoComplete0->bind(hwndEdit);

		HWND hLst1 = GetDlgItem(hwnd, lst1);
		for (size_t i = 0; i < g_encrypted_types.size(); ++i)
		{
			ListBox_AddString(hLst1, g_encrypted_types[i].c_str());
		}

		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		HWND hLst1 = GetDlgItem(hwnd, lst1);
		INT iItem, cItems = ListBox_GetCount(hLst1);
		WCHAR text[MAX_PATH];

		g_encrypted_types.clear();
		for (iItem = 0; iItem < cItems; ++iItem)
		{
			INT cch = (INT)SendMessageW(hLst1, LB_GETTEXTLEN, iItem, 0);
			if (cch + 1 < (INT)_countof(text))
			{
				SendMessageW(hLst1, LB_GETTEXT, iItem, (LPARAM)text);
				g_encrypted_types.push_back(text);
			}
		}

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
		case ID_DELETE:
			Delete(hwnd);
			break;
		case ID_SELECTALL:
			SelectAll(hwnd);
			break;
		}
	}

	void OnPsh1(HWND hwnd)
	{
		MIdOrString type;
		if (!CheckTypeComboBox(m_cmb1, type))
			return;

		auto str = get_type_label(type);

		HWND hLst1 = GetDlgItem(hwnd, lst1);
		INT iItem = ListBox_FindStringExact(hLst1, -1, str.c_str());
		if (iItem == LB_ERR)
			iItem = ListBox_AddString(hLst1, str.c_str());
		SendMessageW(hLst1, LB_SETSEL, FALSE, -1);
		SendMessageW(hLst1, LB_SETSEL, TRUE, iItem);
	}

	void SelectAll(HWND hwnd)
	{
		HWND hLst1 = GetDlgItem(hwnd, lst1);
		SendMessageW(hLst1, LB_SETSEL, TRUE, -1);
	}

	void Delete(HWND hwnd)
	{
		HWND hLst1 = GetDlgItem(hwnd, lst1);
		INT selections[80];
		INT cSelections = (INT)SendMessageW(hLst1, LB_GETSELCOUNT, 0, 0);
		if (cSelections <= 0)
			return;
		if (cSelections > (INT)_countof(selections))
			cSelections = (INT)_countof(selections);
		SendMessageW(hLst1, LB_GETSELITEMS, cSelections, (LPARAM)selections);

		for (INT i = cSelections - 1; i >= 0; --i)
			SendMessageW(hLst1, LB_DELETESTRING, (LPARAM)selections[i], 0);
	}

	int OnVkeyToItem(HWND hwnd, UINT vk, HWND hwndListbox, int iCaret)
	{
		if (GetKeyState(VK_CONTROL) < 0)
		{
			if (vk == 'A')
				SelectAll(hwnd);
		}
		else
		{
			if (vk == VK_DELETE)
				Delete(hwnd);
		}
		return SetDlgMsgResult(hwnd, WM_VKEYTOITEM, -1);
	}

	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
	{
		HWND hLst1 = GetDlgItem(hwnd, lst1);
		if (hwndContext == hLst1)
		{
			PopupMenuDx(hwnd, hwndContext, IDR_POPUPMENUS, 10, xPos, yPos);
		}
	}

	void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
	{
		HWND hLst1 = GetDlgItem(hwnd, lst1);
		if (ListBox_GetSelCount(hLst1) == 0)
		{
			EnableMenuItem(hMenu, ID_DELETE, MF_GRAYED);
		}
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		HANDLE_MSG(hwnd, WM_VKEYTOITEM, OnVkeyToItem);
		HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
		HANDLE_MSG(hwnd, WM_INITMENUPOPUP, OnInitMenuPopup);
		default:
			return 0;
		}
	}
};
