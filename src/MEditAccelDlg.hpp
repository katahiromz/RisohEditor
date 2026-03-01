// MEditAccelDlg.hpp --- Dialogs for edit of Accelerator Table
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "MResizable.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "AccelRes.hpp"
#include "Common.hpp"

class MAddKeyDlg;
class MModifyKeyDlg;
class MEditAccelDlg;

//////////////////////////////////////////////////////////////////////////////

class MAddKeyDlg : public MDialogBase
{
public:
	ACCEL_ENTRY& m_entry;
	MComboBoxAutoComplete m_cmb1;
	MComboBoxAutoComplete m_cmb2;

	MAddKeyDlg(ACCEL_ENTRY& entry) :
		MDialogBase(IDD_ADDKEY), m_entry(entry)
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		CheckDlgButton(hwnd, chx1, BST_CHECKED);

		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		Cmb1_InitVirtualKeys(hCmb1);
		SubclassChildDx(m_cmb1, cmb1);

		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		InitCtrlIDComboBox(hCmb2);
		SetDlgItemText(hwnd, cmb2, L"");
		SubclassChildDx(m_cmb2, cmb2);

		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		m_entry.sz0 = ::GetWindowTextW(hCmb1);

		std::wstring str = m_entry.sz0;
		BOOL bVirtKey = IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED;
		if (!Cmb1_CheckKey(hwnd, hCmb1, bVirtKey, str))
		{
			ErrorBoxDx(IDS_INVALIDKEY);
			return;
		}
		m_entry.sz0 = str;

		WORD wFlags = 0;
		if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
			wFlags |= FVIRTKEY;
		if (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED)
			wFlags |= FNOINVERT;
		if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
			wFlags |= FCONTROL;
		if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
			wFlags |= FSHIFT;
		if (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED)
			wFlags |= FALT;

		m_entry.sz1 = GetKeyFlags(wFlags);

		m_entry.sz2 = ::GetDlgItemTextW(hwnd, cmb2);
		ReplaceFullWithHalf(m_entry.sz2);
		mstr_trim(m_entry.sz2);
		if (!CheckCommand(m_entry.sz2))
		{
			ErrorBoxDx(IDS_NOSUCHID);
			return;
		}

		EndDialog(IDOK);
	}

	void OnPsh1(HWND hwnd)
	{
		SendMessage(GetParent(GetParent(hwnd)), WM_COMMAND, ID_IDLIST, 0);
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

class MModifyKeyDlg : public MDialogBase
{
public:
	ACCEL_ENTRY& m_entry;
	MComboBoxAutoComplete m_cmb1;
	MComboBoxAutoComplete m_cmb2;

	MModifyKeyDlg(ACCEL_ENTRY& entry) :
		MDialogBase(IDD_MODIFYKEY), m_entry(entry)
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		InitCtrlIDComboBox(hCmb2);
		SubclassChildDx(m_cmb2, cmb2);
		SetDlgItemTextW(hwnd, cmb2, m_entry.sz2.c_str());

		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		Cmb1_InitVirtualKeys(hCmb1);
		SetDlgItemTextW(hwnd, cmb1, m_entry.sz0.c_str());
		SubclassChildDx(m_cmb1, cmb1);

		WORD wFlags;
		SetKeyFlags(wFlags, m_entry.sz1);
		if (wFlags & FVIRTKEY)
			CheckDlgButton(hwnd, chx1, BST_CHECKED);
		if (wFlags & FNOINVERT)
			CheckDlgButton(hwnd, chx2, BST_CHECKED);
		if (wFlags & FCONTROL)
			CheckDlgButton(hwnd, chx3, BST_CHECKED);
		if (wFlags & FSHIFT)
			CheckDlgButton(hwnd, chx4, BST_CHECKED);
		if (wFlags & FALT)
			CheckDlgButton(hwnd, chx5, BST_CHECKED);

		if (wFlags & FVIRTKEY)
		{
			HWND hCmb1 = GetDlgItem(hwnd, cmb1);
			Cmb1_InitVirtualKeys(hCmb1);

			INT i = ComboBox_FindStringExact(hCmb1, -1, m_entry.sz0.c_str());
			if (i != CB_ERR)
			{
				ComboBox_SetCurSel(hCmb1, i);
			}
		}

		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		m_entry.sz0 = ::GetWindowTextW(hCmb1);

		std::wstring str = m_entry.sz0;
		BOOL bVirtKey = IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED;
		if (!Cmb1_CheckKey(hwnd, hCmb1, bVirtKey, str))
		{
			ErrorBoxDx(IDS_INVALIDKEY);
			return;
		}
		m_entry.sz0 = str;

		WORD wFlags = 0;
		if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
			wFlags |= FVIRTKEY;
		if (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED)
			wFlags |= FNOINVERT;
		if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
			wFlags |= FCONTROL;
		if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
			wFlags |= FSHIFT;
		if (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED)
			wFlags |= FALT;

		m_entry.sz1 = GetKeyFlags(wFlags);

		m_entry.sz2 = ::GetDlgItemTextW(hwnd, cmb2);
		ReplaceFullWithHalf(m_entry.sz2);
		mstr_trim(m_entry.sz2);
		if (!CheckCommand(m_entry.sz2))
		{
			ErrorBoxDx(IDS_NOSUCHID);
			return;
		}

		EndDialog(IDOK);
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case chx1:
			if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
			{
				Cmb1_InitVirtualKeys(GetDlgItem(hwnd, cmb1));
			}
			else
			{
				SetDlgItemTextW(hwnd, cmb1, NULL);
			}
			break;
		case IDOK:
			OnOK(hwnd);
			break;
		case IDCANCEL:
			EndDialog(IDCANCEL);
			break;
		case psh1:
			OnPsh1(hwnd);
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

class MEditAccelDlg : public MDialogBase
{
public:
	AccelRes& m_accel_res;
	MResizable m_resizable;
	HICON m_hIcon;
	HICON m_hIconSm;
	HWND m_hLst1;

	MEditAccelDlg(AccelRes& accel_res)
		: MDialogBase(IDD_EDITACCEL), m_accel_res(accel_res)
	{
		m_hIcon = LoadIconDx(IDI_SMILY);
		m_hIconSm = LoadSmallIconDx(IDI_SMILY);
		m_hLst1 = NULL;
	}

	~MEditAccelDlg()
	{
		DestroyIcon(m_hIcon);
		DestroyIcon(m_hIconSm);
	}

	void OnUp(HWND hwnd)
	{
		INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
		if (iItem == 0)
			return;

		ACCEL_ENTRY ae0, ae1;
		ae0.sz0 = GetListViewItemText(m_hLst1, iItem - 1, 0);
		ae0.sz1 = GetListViewItemText(m_hLst1, iItem - 1, 1);
		ae0.sz2 = GetListViewItemText(m_hLst1, iItem - 1, 2);
		ae1.sz0 = GetListViewItemText(m_hLst1, iItem, 0);
		ae1.sz1 = GetListViewItemText(m_hLst1, iItem, 1);
		ae1.sz2 = GetListViewItemText(m_hLst1, iItem, 2);

		ListView_SetItemText(m_hLst1, iItem - 1, 0, const_cast<LPWSTR>(ae1.sz0.c_str()));
		ListView_SetItemText(m_hLst1, iItem - 1, 1, const_cast<LPWSTR>(ae1.sz1.c_str()));
		ListView_SetItemText(m_hLst1, iItem - 1, 2, const_cast<LPWSTR>(ae1.sz2.c_str()));
		ListView_SetItemText(m_hLst1, iItem, 0, const_cast<LPWSTR>(ae0.sz0.c_str()));
		ListView_SetItemText(m_hLst1, iItem, 1, const_cast<LPWSTR>(ae0.sz1.c_str()));
		ListView_SetItemText(m_hLst1, iItem, 2, const_cast<LPWSTR>(ae0.sz2.c_str()));

		UINT state = LVIS_SELECTED | LVIS_FOCUSED;
		ListView_SetItemState(m_hLst1, iItem - 1, state, state);
	}

	void OnDown(HWND hwnd)
	{
		INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
		if (iItem + 1 == ListView_GetItemCount(m_hLst1))
			return;

		ACCEL_ENTRY ae0, ae1;
		ae0.sz0 = GetListViewItemText(m_hLst1, iItem, 0);
		ae0.sz1 = GetListViewItemText(m_hLst1, iItem, 1);
		ae0.sz2 = GetListViewItemText(m_hLst1, iItem, 2);
		ae1.sz0 = GetListViewItemText(m_hLst1, iItem + 1, 0);
		ae1.sz1 = GetListViewItemText(m_hLst1, iItem + 1, 1);
		ae1.sz2 = GetListViewItemText(m_hLst1, iItem + 1, 2);

		ListView_SetItemText(m_hLst1, iItem, 0, const_cast<LPWSTR>(ae1.sz0.c_str()));
		ListView_SetItemText(m_hLst1, iItem, 1, const_cast<LPWSTR>(ae1.sz1.c_str()));
		ListView_SetItemText(m_hLst1, iItem, 2, const_cast<LPWSTR>(ae1.sz2.c_str()));
		ListView_SetItemText(m_hLst1, iItem + 1, 0, const_cast<LPWSTR>(ae0.sz0.c_str()));
		ListView_SetItemText(m_hLst1, iItem + 1, 1, const_cast<LPWSTR>(ae0.sz1.c_str()));
		ListView_SetItemText(m_hLst1, iItem + 1, 2, const_cast<LPWSTR>(ae0.sz2.c_str()));

		UINT state = LVIS_SELECTED | LVIS_FOCUSED;
		ListView_SetItemState(m_hLst1, iItem + 1, state, state);
	}

	void OnDelete(HWND hwnd)
	{
		INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
		if (iItem >= 0)
		{
			ListView_DeleteItem(m_hLst1, iItem);
		}
	}

	void OnDeleteAll(HWND hwnd)
	{
		ListView_DeleteAllItems(m_hLst1);
	}

	void OnAdd(HWND hwnd)
	{
		ACCEL_ENTRY entry;

		MAddKeyDlg dialog(entry);
		if (IDOK != dialog.DialogBoxDx(hwnd))
		{
			return;
		}

		INT iItem = ListView_GetItemCount(m_hLst1);

		LV_ITEM item;

		ZeroMemory(&item, sizeof(item));
		item.iItem = iItem;
		item.mask = LVIF_TEXT;
		item.iSubItem = 0;
		item.pszText = const_cast<LPWSTR>(entry.sz0.c_str());
		ListView_InsertItem(m_hLst1, &item);

		ZeroMemory(&item, sizeof(item));
		item.iItem = iItem;
		item.mask = LVIF_TEXT;
		item.iSubItem = 1;
		item.pszText = const_cast<LPWSTR>(entry.sz1.c_str());
		ListView_SetItem(m_hLst1, &item);

		ZeroMemory(&item, sizeof(item));
		item.iItem = iItem;
		item.mask = LVIF_TEXT;
		item.iSubItem = 2;
		item.pszText = const_cast<LPWSTR>(entry.sz2.c_str());
		ListView_SetItem(m_hLst1, &item);

		UINT state = LVIS_SELECTED | LVIS_FOCUSED;
		ListView_SetItemState(m_hLst1, iItem, state, state);
		ListView_EnsureVisible(m_hLst1, iItem, FALSE);
	}

	void OnModify(HWND hwnd)
	{
		INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
		if (iItem < 0)
		{
			return;
		}

		ACCEL_ENTRY a_entry;
		a_entry.sz0 = GetListViewItemText(m_hLst1, iItem, 0);
		a_entry.sz1 = GetListViewItemText(m_hLst1, iItem, 1);
		a_entry.sz2 = GetListViewItemText(m_hLst1, iItem, 2);

		MModifyKeyDlg dialog(a_entry);
		if (IDOK == dialog.DialogBoxDx(hwnd))
		{
			ListView_SetItemText(m_hLst1, iItem, 0, const_cast<LPWSTR>(a_entry.sz0.c_str()));
			ListView_SetItemText(m_hLst1, iItem, 1, const_cast<LPWSTR>(a_entry.sz1.c_str()));
			ListView_SetItemText(m_hLst1, iItem, 2, const_cast<LPWSTR>(a_entry.sz2.c_str()));
		}
	}

	void OnOK(HWND hwnd)
	{
		INT i, nCount = ListView_GetItemCount(m_hLst1);

		if (nCount == 0)
		{
			ErrorBoxDx(IDS_DATAISEMPTY);
			return;
		}

		m_accel_res.entries().clear();
		for (i = 0; i < nCount; ++i)
		{
			ACCEL_ENTRY a_entry;
			a_entry.sz0 = GetListViewItemText(m_hLst1, i, 0);
			a_entry.sz1 = GetListViewItemText(m_hLst1, i, 1);
			a_entry.sz2 = GetListViewItemText(m_hLst1, i, 2);

			WORD wFlags;
			SetKeyFlags(wFlags, a_entry.sz1);

			AccelTableEntry entry;
			entry.fFlags = wFlags;
			if (wFlags & FVIRTKEY)
			{
				entry.wAscii = (WORD)g_db.GetValue(L"VIRTUALKEYS", a_entry.sz0);
			}
			else
			{
				std::wstring str, str2 = a_entry.sz0;
				LPCWSTR pch = str2.c_str();
				if (guts_quote(str, pch))
				{
					entry.wAscii = str[0];
				}
				else
				{
					entry.wAscii = (WORD)mstr_parse_int(a_entry.sz0.c_str());
				}
			}
			entry.wId = (WORD)g_db.GetResIDValue(a_entry.sz2);

			m_accel_res.entries().push_back(entry);
		}

		EndDialog(IDOK);
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case psh1:
			OnAdd(hwnd);
			break;
		case psh2:
			OnModify(hwnd);
			break;
		case psh3:
			OnDelete(hwnd);
			break;
		case psh4:
			OnUp(hwnd);
			break;
		case psh5:
			OnDown(hwnd);
			break;
		case psh6:
			OnDeleteAll(hwnd);
			break;
		case IDOK:
			OnOK(hwnd);
			break;
		case IDCANCEL:
			EndDialog(IDCANCEL);
			break;
		}
	}

	void OnItemChanged(HWND hwnd)
	{
		INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
		BOOL bSelected = (iItem != -1);
		EnableWindow(GetDlgItem(hwnd, psh2), bSelected);
		EnableWindow(GetDlgItem(hwnd, psh3), bSelected);
		EnableWindow(GetDlgItem(hwnd, psh4), bSelected);
		EnableWindow(GetDlgItem(hwnd, psh5), bSelected);
	}

	void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
	{
		INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
		BOOL bSelected = (iItem != -1);
		EnableMenuItem(hMenu, psh2, bSelected ? MF_ENABLED : MF_GRAYED);
		EnableMenuItem(hMenu, psh3, bSelected ? MF_ENABLED : MF_GRAYED);
		EnableMenuItem(hMenu, psh4, bSelected ? MF_ENABLED : MF_GRAYED);
		EnableMenuItem(hMenu, psh5, bSelected ? MF_ENABLED : MF_GRAYED);
	}

	LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
	{
		if (idFrom == lst1)
		{
			if (pnmhdr->code == LVN_KEYDOWN)
			{
				LV_KEYDOWN *KeyDown = (LV_KEYDOWN *)pnmhdr;
				if (KeyDown->wVKey == VK_DELETE)
				{
					OnDelete(hwnd);
					return 0;
				}
			}
			if (pnmhdr->code == NM_DBLCLK)
			{
				OnModify(hwnd);
				return 0;
			}
			if (pnmhdr->code == LVN_GETINFOTIP)
			{
				NMLVGETINFOTIP *pGetInfoTip = (NMLVGETINFOTIP *)pnmhdr;
				INT iItem = pGetInfoTip->iItem;
				INT iSubItem = pGetInfoTip->iSubItem;
				TCHAR szText[MAX_PATH];
				ListView_GetItemText(m_hLst1, iItem, iSubItem, szText, _countof(szText));
				StringCchCopy(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, szText);
			}
			if (pnmhdr->code == LVN_ITEMCHANGED)
			{
				//NM_LISTVIEW *pListView = (NM_LISTVIEW *)pnmhdr;
				OnItemChanged(hwnd);
			}
		}
		return 0;
	}

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

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		m_hLst1 = GetDlgItem(hwnd, lst1);
		ListView_SetExtendedListViewStyle(m_hLst1, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

		LV_COLUMN column;
		ZeroMemory(&column, sizeof(column));

		column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
		column.fmt = LVCFMT_LEFT;
		column.cx = 105;
		column.pszText = LoadStringDx(IDS_KEY);
		column.iSubItem = 0;
		ListView_InsertColumn(m_hLst1, 0, &column);

		column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
		column.fmt = LVCFMT_LEFT;
		column.cx = 75;
		column.pszText = LoadStringDx(IDS_FLAGS);
		column.iSubItem = 1;
		ListView_InsertColumn(m_hLst1, 1, &column);

		column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
		column.fmt = LVCFMT_LEFT;
		column.cx = 185;
		column.pszText = LoadStringDx(IDS_COMMANDID);
		column.iSubItem = 2;
		ListView_InsertColumn(m_hLst1, 2, &column);

		typedef AccelRes::entries_type entries_type;
		const entries_type& entries = m_accel_res.entries();

		INT i = 0;
		for (auto& entry : entries)
		{
			std::wstring str;
			if (entry.fFlags & FVIRTKEY)
			{
				str = g_db.GetName(L"VIRTUALKEYS", entry.wAscii);
			}
			else
			{
				str += (WCHAR)entry.wAscii;
				str = mstr_quote(str);
			}

			LV_ITEM item;
			ZeroMemory(&item, sizeof(item));
			item.iItem = i;
			item.mask = LVIF_TEXT;
			item.iSubItem = 0;
			item.pszText = &str[0];
			ListView_InsertItem(m_hLst1, &item);

			str = GetKeyFlags(entry.fFlags);

			ZeroMemory(&item, sizeof(item));
			item.iItem = i;
			item.mask = LVIF_TEXT;
			item.iSubItem = 1;
			item.pszText = &str[0];
			ListView_SetItem(m_hLst1, &item);

			str = GetKeyID(entry.wId);

			ZeroMemory(&item, sizeof(item));
			item.iItem = i;
			item.mask = LVIF_TEXT;
			item.iSubItem = 2;
			item.pszText = &str[0];
			ListView_SetItem(m_hLst1, &item);

			++i;
		}

		UINT state = LVIS_SELECTED | LVIS_FOCUSED;
		ListView_SetItemState(m_hLst1, 0, state, state);
		SetFocus(m_hLst1);

		m_resizable.OnParentCreate(hwnd);

		m_resizable.SetLayoutAnchor(lst1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
		m_resizable.SetLayoutAnchor(psh1, mzcLA_TOP_RIGHT);
		m_resizable.SetLayoutAnchor(psh2, mzcLA_TOP_RIGHT);
		m_resizable.SetLayoutAnchor(psh3, mzcLA_TOP_RIGHT);
		m_resizable.SetLayoutAnchor(psh4, mzcLA_TOP_RIGHT);
		m_resizable.SetLayoutAnchor(psh5, mzcLA_TOP_RIGHT);
		m_resizable.SetLayoutAnchor(psh6, mzcLA_BOTTOM_LEFT);
		m_resizable.SetLayoutAnchor(IDOK, mzcLA_BOTTOM_RIGHT);
		m_resizable.SetLayoutAnchor(IDCANCEL, mzcLA_BOTTOM_RIGHT);

		SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
		SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

		CenterWindowDx();
		return TRUE;
	}

	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
	{
		if (hwndContext == m_hLst1)
		{
			PopupMenuDx(hwnd, m_hLst1, IDR_POPUPMENUS, 7, xPos, yPos);
		}
	}
};
