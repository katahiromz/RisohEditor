// MEditMenuDlg.hpp --- Dialogs for edit of Menus
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "MenuRes.hpp"
#include "Common.hpp"

class MAddMItemDlg;
class MModifyMItemDlg;
class MEditMenuDlg;

//////////////////////////////////////////////////////////////////////////////

class MAddMItemDlg : public MDialogBase
{
public:
	MENU_ENTRY& m_entry;
	MComboBoxAutoComplete m_cmb2;
	MComboBoxAutoComplete m_cmb3;

	MAddMItemDlg(MENU_ENTRY& entry)
		: MDialogBase(IDD_ADDMITEM), m_entry(entry)
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		InitCtrlIDComboBox(GetDlgItem(hwnd, cmb2));
		SubclassChildDx(m_cmb2, cmb2);
		SetDlgItemText(hwnd, cmb2, L"0");

		InitResNameComboBox(GetDlgItem(hwnd, cmb3), MIdOrString(L""), IDTYPE_HELP);
		SetDlgItemInt(hwnd, cmb3, 0, TRUE);
		SubclassChildDx(m_cmb3, cmb3);

		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd);
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

	void OnPsh1(HWND hwnd)
	{
		SendMessage(GetParent(GetParent(hwnd)), WM_COMMAND, ID_IDLIST, 0);
	}

	virtual INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		}
		return DefaultProcDx();
	}
};

//////////////////////////////////////////////////////////////////////////////

class MModifyMItemDlg : public MDialogBase
{
public:
	MENU_ENTRY& m_entry;
	MComboBoxAutoComplete m_cmb2;
	MComboBoxAutoComplete m_cmb3;

	MModifyMItemDlg(MENU_ENTRY& entry)
		: MDialogBase(IDD_MODIFYMITEM), m_entry(entry)
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
	void OnOK(HWND hwnd);
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

	void OnPsh1(HWND hwnd)
	{
		SendMessage(GetParent(GetParent(hwnd)), WM_COMMAND, ID_IDLIST, 0);
	}

	virtual INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		}
		return DefaultProcDx();
	}
};

//////////////////////////////////////////////////////////////////////////////

class MEditMenuDlg : public MDialogBase
{
public:
	MenuRes& m_menu_res;
	MResizable m_resizable;
	HICON m_hIcon;
	HICON m_hIconSm;
	HWND m_hLst1;

	MEditMenuDlg(MenuRes& menu_res)
		: MDialogBase(IDD_EDITMENU), m_menu_res(menu_res)
	{
		m_hIcon = LoadIconDx(IDI_SMILY);
		m_hIconSm = LoadSmallIconDx(IDI_SMILY);
		m_hLst1 = NULL;
	}

	~MEditMenuDlg()
	{
		DestroyIcon(m_hIcon);
		DestroyIcon(m_hIconSm);
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
	void OnAdd(HWND hwnd);
	BOOL GetEntry(HWND hwnd, MENU_ENTRY& entry, INT iItem);
	BOOL SetEntry(HWND hwnd, MENU_ENTRY& entry, INT iItem);
	void OnModify(HWND hwnd);
	void OnDelete(HWND hwnd);
	void OnUp(HWND hwnd);
	void OnDown(HWND hwnd);
	void OnLeft(HWND hwnd);
	void OnRight(HWND hwnd);
	void OnOK(HWND hwnd);
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	void OnItemChanged(HWND hwnd);
	void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);
	LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr);

	virtual INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
			HANDLE_MSG(hwnd, WM_SIZE, OnSize);
			HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
			HANDLE_MSG(hwnd, WM_INITMENUPOPUP, OnInitMenuPopup);
		}
		return DefaultProcDx();
	}

	void OnSize(HWND hwnd, UINT state, int cx, int cy)
	{
		m_resizable.OnSize();
	}

	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
	{
		if (hwndContext == m_hLst1)
		{
			PopupMenuDx(hwnd, m_hLst1, IDR_POPUPMENUS, 6, xPos, yPos);
		}
	}
};
