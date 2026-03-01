// MEncodingDlg.hpp --- "Encoding" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "MResizable.hpp"
#include "MComboBoxAutoComplete.hpp"

class MAddEncDlg;
class MModifyEncDlg;
class MEncodingDlg;

//////////////////////////////////////////////////////////////////////////////

inline MString txt2enc(const MString& txt)
{
	if (txt == LoadStringDx(IDS_ANSI))
		return L"ansi";
	if (txt == LoadStringDx(IDS_WIDE))
		return L"wide";
	if (txt == LoadStringDx(IDS_UTF8))
		return L"utf8";
	if (txt == LoadStringDx(IDS_UTF8N))
		return L"utf8n";
	if (txt == LoadStringDx(IDS_SJIS))
		return L"sjis";
	return L"";
}

inline MString enc2txt(const MString& enc)
{
	if (enc == L"ansi")
		return LoadStringDx(IDS_ANSI);
	if (enc == L"wide")
		return LoadStringDx(IDS_WIDE);
	if (enc == L"utf8")
		return LoadStringDx(IDS_UTF8);
	if (enc == L"utf8n")
		return LoadStringDx(IDS_UTF8N);
	if (enc == L"sjis")
		return LoadStringDx(IDS_SJIS);
	return L"";
}

inline MIdOrString get_type_from_text(MString str)
{
	mstr_trim(str);

	MIdOrString type;
	auto k = str.find(L" (");   // )
	if (k != MStringW::npos)
	{
		int num = mstr_parse_int(&str[k + 2]);
		type = (WORD)num;
	}
	else if (str.size() && mchr_is_digit(str[0]))
	{
		int num = mstr_parse_int(&str[0]);
		type = (WORD)num;
	}
	else
	{
		type.m_str = std::move(str);
	}
	return type;
}

// get the resource type label
inline MStringW get_type_label(MIdOrString& type)
{
	if (!type.m_id)
		return type.m_str;    // string name type

	// it was integer name type

	MStringW label = g_db.GetName(L"RESOURCE", type.m_id);
	if (label.empty())  // unable to get the label
		return mstr_dec_word(type.m_id);  // returns the numeric text

	// got the label
	if (!mchr_is_digit(label[0]))   // first character is not digit
	{
		// add a parenthesis pair and numeric text
		label += L" (";
		label += mstr_dec_word(type.m_id);
		label += L")";
	}

	return label;
}

//////////////////////////////////////////////////////////////////////////////

void InitResTypeComboBox(HWND hCmb1, const MIdOrString& type);

class MAddEncDlg : public MDialogBase
{
public:
	MIdOrString m_type;
	MString m_enc;
	MComboBoxAutoComplete m_cmb1;

	MAddEncDlg() : MDialogBase(IDD_ADDENC)
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		SubclassChildDx(m_cmb1, cmb1);

		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		InitResTypeComboBox(hCmb1, MIdOrString());

		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		ComboBox_AddString(hCmb2, LoadStringDx(IDS_ANSI));
		ComboBox_AddString(hCmb2, LoadStringDx(IDS_WIDE));
		INT k = ComboBox_AddString(hCmb2, LoadStringDx(IDS_UTF8));
		ComboBox_AddString(hCmb2, LoadStringDx(IDS_UTF8N));
		ComboBox_AddString(hCmb2, LoadStringDx(IDS_SJIS));
		//ComboBox_AddString(hCmb2, LoadStringDx(IDS_BINARY));

		ComboBox_SetCurSel(hCmb2, k);

		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		MString text = GetDlgItemText(hwnd, cmb1);
		m_type = get_type_from_text(text);
		if (m_type.empty())
		{
			ErrorBoxDx(IDS_INVALIDRESTYPE);
			return;
		}

		text = GetDlgItemText(hwnd, cmb2);
		m_enc = txt2enc(text);
		if (m_enc.empty())
		{
			return;
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
		case cmb1:
			if (codeNotify == CBN_EDITCHANGE)
			{
				m_cmb1.OnEditChange();  // input completion
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
		default:
			return DefaultProcDx();
		}
	}
};

//////////////////////////////////////////////////////////////////////////////

class MModifyEncDlg : public MDialogBase
{
public:
	MIdOrString m_type;
	MString m_enc;
	MComboBoxAutoComplete m_cmb1;

	MModifyEncDlg(const MIdOrString& type, MString enc) :
		MDialogBase(IDD_MODIFYENC),
		m_type(type),
		m_enc(enc)
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		SubclassChildDx(m_cmb1, cmb1);

		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		InitResTypeComboBox(hCmb1, m_type);
		EnableWindow(hCmb1, FALSE);

		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		ComboBox_AddString(hCmb2, LoadStringDx(IDS_ANSI));
		ComboBox_AddString(hCmb2, LoadStringDx(IDS_WIDE));
		ComboBox_AddString(hCmb2, LoadStringDx(IDS_UTF8));
		ComboBox_AddString(hCmb2, LoadStringDx(IDS_UTF8N));
		ComboBox_AddString(hCmb2, LoadStringDx(IDS_SJIS));
		//ComboBox_AddString(hCmb2, LoadStringDx(IDS_BINARY));

		MString txt = enc2txt(m_enc);
		int k = ComboBox_FindStringExact(hCmb2, -1, txt.c_str());
		ComboBox_SetCurSel(hCmb2, k);

		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		MString text = GetDlgItemText(hwnd, cmb1);
		m_type = get_type_from_text(text);
		if (m_type.empty())
		{
			ErrorBoxDx(IDS_INVALIDRESTYPE);
			return;
		}

		text = GetDlgItemText(hwnd, cmb2);
		m_enc = txt2enc(text);
		if (m_enc.empty())
		{
			return;
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
		case cmb1:
			if (codeNotify == CBN_EDITCHANGE)
			{
				m_cmb1.OnEditChange();  // input completion
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
		default:
			return DefaultProcDx();
		}
	}
};

//////////////////////////////////////////////////////////////////////////////

class MEncodingDlg : public MDialogBase
{
public:
	MResizable m_resizable;
	HWND m_hLst1;
	HICON m_hIcon;
	HICON m_hIconSm;

	MEncodingDlg() : MDialogBase(IDD_ENCODING)
	{
		m_hIcon = LoadIconDx(IDI_SMILY);
		m_hIconSm = LoadSmallIconDx(IDI_SMILY);
	}

	~MEncodingDlg()
	{
		DestroyIcon(m_hIcon);
		DestroyIcon(m_hIconSm);
	}

	void InitLst1()
	{
		ListView_DeleteAllItems(m_hLst1);

		const auto& map = g_settings.encoding_map;

		INT i = 0;
		for (auto& pair : map)
		{
			if (pair.second.empty())
				continue;

			MStringW str = pair.first;
			MIdOrString type = get_type_from_text(str);
			str = get_type_label(type);

			LV_ITEM item;
			ZeroMemory(&item, sizeof(item));
			item.iItem = i;
			item.mask = LVIF_TEXT;
			item.iSubItem = 0;
			item.pszText = &str[0];
			ListView_InsertItem(m_hLst1, &item);

			str = enc2txt(pair.second);

			ZeroMemory(&item, sizeof(item));
			item.iItem = i;
			item.mask = LVIF_TEXT;
			item.iSubItem = 1;
			item.pszText = &str[0];
			ListView_SetItem(m_hLst1, &item);

			++i;
		}
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		SendMessageDx(WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
		SendMessageDx(WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

		m_hLst1 = GetDlgItem(hwnd, lst1);
		ListView_SetExtendedListViewStyle(m_hLst1, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

		LV_COLUMN column;
		ZeroMemory(&column, sizeof(column));

		column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
		column.fmt = LVCFMT_LEFT;
		column.cx = 140;
		column.pszText = LoadStringDx(IDS_RESTYPE);
		column.iSubItem = 0;
		ListView_InsertColumn(m_hLst1, 0, &column);

		column.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
		column.fmt = LVCFMT_LEFT;
		column.cx = 200;
		column.pszText = LoadStringDx(IDS_ENCODING);
		column.iSubItem = 1;
		ListView_InsertColumn(m_hLst1, 1, &column);

		InitLst1();

		UINT state = LVIS_SELECTED | LVIS_FOCUSED;
		ListView_SetItemState(m_hLst1, 0, state, state);
		SetFocus(m_hLst1);

		OnItemChanged(hwnd);

		CenterWindowDx();
		return TRUE;
	}

	void OnReset(HWND hwnd)
	{
		g_settings.ResetEncoding();
		InitLst1();
	}

	void OnOK(HWND hwnd)
	{
		INT iItem, nCount = ListView_GetItemCount(m_hLst1);
		if (nCount == 0)
		{
			return;
		}

		auto& map = g_settings.encoding_map;
		map.clear();

		WCHAR szText1[MAX_PATH], szText2[MAX_PATH];
		for (iItem = 0; iItem < nCount; ++iItem)
		{
			ListView_GetItemText(m_hLst1, iItem, 0, szText1, _countof(szText1));
			mstr_trim(szText1);

			ListView_GetItemText(m_hLst1, iItem, 1, szText2, _countof(szText2));
			mstr_trim(szText2);

			MIdOrString type = get_type_from_text(szText1);
			MString enc = txt2enc(szText2);

			map.insert(std::make_pair(type.str(), enc));
		}

		EndDialog(IDOK);
	}

	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
	{
		if (hwndContext == m_hLst1)
		{
			PopupMenuDx(hwnd, m_hLst1, IDR_POPUPMENUS, 8, xPos, yPos);
		}
	}

	void OnDelete(HWND hwnd)
	{
		INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
		if (iItem >= 0)
		{
			ListView_DeleteItem(m_hLst1, iItem);
		}
	}

	void OnAdd(HWND hwnd)
	{
		MAddEncDlg dialog;
		if (IDOK != dialog.DialogBoxDx(hwnd))
			return;

		MString text1 = get_type_label(dialog.m_type);
		MString text2 = enc2txt(dialog.m_enc);

		INT iItem;

		LV_FINDINFO find;
		WCHAR sz[MAX_PATH];
		StringCchCopyW(sz, _countof(sz), text1.c_str());
		ZeroMemory(&find, sizeof(find));
		find.flags = LVFI_STRING;
		find.psz = sz;
		iItem = ListView_FindItem(m_hLst1, -1, &find);
		if (iItem != -1)
		{
			ListView_DeleteItem(m_hLst1, iItem);
		}

		LV_ITEM item;
		iItem = ListView_GetItemCount(m_hLst1);

		ZeroMemory(&item, sizeof(item));
		item.iItem = iItem;
		item.mask = LVIF_TEXT;
		item.iSubItem = 0;
		item.pszText = &text1[0];
		ListView_InsertItem(m_hLst1, &item);

		ZeroMemory(&item, sizeof(item));
		item.iItem = iItem;
		item.mask = LVIF_TEXT;
		item.iSubItem = 1;
		item.pszText = &text2[0];
		ListView_SetItem(m_hLst1, &item);

		UINT state = LVIS_SELECTED | LVIS_FOCUSED;
		ListView_SetItemState(m_hLst1, iItem, state, state);
		ListView_EnsureVisible(m_hLst1, iItem, FALSE);
	}

	void OnModify(HWND hwnd)
	{
		INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
		if (iItem == -1)
			return;

		WCHAR szText1[MAX_PATH], szText2[MAX_PATH];

		ListView_GetItemText(m_hLst1, iItem, 0, szText1, _countof(szText1));
		mstr_trim(szText1);

		ListView_GetItemText(m_hLst1, iItem, 1, szText2, _countof(szText2));
		mstr_trim(szText2);

		MModifyEncDlg dialog(get_type_from_text(szText1), txt2enc(szText2));
		if (IDOK != dialog.DialogBoxDx(hwnd))
			return;

		MString text2 = enc2txt(dialog.m_enc);

		LV_ITEM item;
		ZeroMemory(&item, sizeof(item));
		item.iItem = iItem;
		item.mask = LVIF_TEXT;
		item.iSubItem = 1;
		item.pszText = &text2[0];
		ListView_SetItem(m_hLst1, &item);

		UINT state = LVIS_SELECTED | LVIS_FOCUSED;
		ListView_SetItemState(m_hLst1, iItem, state, state);
		ListView_EnsureVisible(m_hLst1, iItem, FALSE);
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
		case ID_ADD:
			OnAdd(hwnd);
			OnItemChanged(hwnd);
			break;
		case psh2:
		case ID_MODIFY:
			OnModify(hwnd);
			break;
		case psh3:
		case ID_DELETE:
			OnDelete(hwnd);
			OnItemChanged(hwnd);
			break;
		case psh5:
			OnReset(hwnd);
			break;
		}
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
			if (pnmhdr->code == LVN_ITEMCHANGED)
			{
				//NM_LISTVIEW *pListView = (NM_LISTVIEW *)pnmhdr;
				OnItemChanged(hwnd);
			}
		}
		return 0;
	}

	void OnItemChanged(HWND hwnd)
	{
		INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
		BOOL bSelected = (iItem != -1);
		EnableWindow(GetDlgItem(hwnd, psh2), bSelected);
		EnableWindow(GetDlgItem(hwnd, psh3), bSelected);
	}

	void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
	{
		INT iItem = ListView_GetNextItem(m_hLst1, -1, LVNI_ALL | LVNI_SELECTED);
		if (iItem >= 0)
		{
			EnableMenuItem(hMenu, ID_MODIFY, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, ID_DELETE, MF_BYCOMMAND | MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_MODIFY, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu, ID_DELETE, MF_BYCOMMAND | MF_GRAYED);
		}
	}

	virtual INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
			HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
			HANDLE_MSG(hwnd, WM_INITMENUPOPUP, OnInitMenuPopup);
		}
		return DefaultProcDx();
	}
};
