// MAddResDlg.hpp --- "Add Resource" Dialog
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
#include "DlgInit.h"
#include "MRisohAutoComplete.hpp"
#include "Utils.h"

extern BOOL s_bModified;

//////////////////////////////////////////////////////////////////////////////

class MAddResDlg : public MDialogBase
{
public:
	MIdOrString m_type;
	MIdOrString m_name;
	LANGID m_lang = BAD_LANG;
	MStringW m_strTemplate;
	LPCTSTR m_file;
	MStringW m_strText;
	MComboBoxAutoComplete m_cmb1;
	MComboBoxAutoComplete m_cmb2;
	MComboBoxAutoComplete m_cmb3;
	MRisohAutoComplete *m_pAutoComplete0;
	MRisohAutoComplete *m_pAutoComplete1;
	MRisohAutoComplete *m_pAutoComplete2;
	EntryBase *m_added_entry = nullptr;
	BOOL m_bOldModified = FALSE;

	// preview (for RT_GROUP_ICON / RT_GROUP_CURSOR / RT_BITMAP)
	MIdOrString m_curType;
	HICON m_hIcon = NULL;
	HCURSOR m_hCursor = NULL;
	HBITMAP m_hBitmap = NULL;

	MAddResDlg()
		: MDialogBase(IDD_ADDRES)
		, m_type(0xFFFF)
		, m_file(NULL)
		, m_pAutoComplete0(new MRisohAutoComplete(0))
		, m_pAutoComplete1(new MRisohAutoComplete(1))
		, m_pAutoComplete2(new MRisohAutoComplete(2))
		, m_bOldModified(s_bModified)
	{
		m_cmb3.m_bAcceptSpace = TRUE;
		m_cmb3.m_bIgnoreCase = TRUE;
	}

	~MAddResDlg()
	{
		m_pAutoComplete0->unbind();
		m_pAutoComplete0->Release();
		m_pAutoComplete1->unbind();
		m_pAutoComplete1->Release();
		m_pAutoComplete2->unbind();
		m_pAutoComplete2->Release();

		if (m_hIcon)
			DestroyIcon(m_hIcon);
		if (m_hCursor)
			DestroyCursor(m_hCursor);
		if (m_hBitmap)
			DeleteObject(m_hBitmap);
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		// accept file dropping
		DragAcceptFiles(hwnd, TRUE);

		// for Types
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		InitResTypeComboBox(hCmb1, m_type);

		// enable input complete
		SubclassChildDx(m_cmb1, cmb1);
		SubclassChildDx(m_cmb2, cmb2);

		InitComboBoxPlaceholder(m_cmb2, IDS_INTEGERORIDENTIFIER);

		// set 1 to the name if it's a RT_VERSION
		if (m_type == RT_VERSION)
		{
			SetDlgItemInt(hwnd, cmb2, 1, FALSE);
		}

		// for Langs
		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		InitLangComboBox(hCmb3, GetDefaultResLanguage());
		SubclassChildDx(m_cmb3, cmb3);

		// no preview yet
		ShowWindow(GetDlgItem(hwnd, ico1), SW_HIDE);

		// for file
		if (m_file)
		{
			SetDlgItemTextW(hwnd, edt1, m_file);
			DoFile(hwnd, m_file, m_type);
		}
		FileSystemAutoComplete(GetDlgItem(hwnd, edt1));

		// do centering the dialog
		CenterWindowDx();

		// move focus to help the user input
		if (m_type == 0xFFFF)
		{
			SetFocus(GetDlgItem(hwnd, cmb1));
		}
		else
		{
			SetFocus(GetDlgItem(hwnd, cmb2));
		}

		// select the type
		OnCmb1(hwnd);

		// auto complete
		COMBOBOXINFO info = { sizeof(info) };
		GetComboBoxInfo(m_cmb1, &info);
		HWND hwndEdit = info.hwndItem;
		m_pAutoComplete0->bind(hwndEdit);

		info = { sizeof(info) };
		GetComboBoxInfo(m_cmb3, &info);
		hwndEdit = info.hwndItem;
		m_pAutoComplete2->bind(hwndEdit);

		return FALSE;
	}

	bool HasSample(const MIdOrString& type, const MIdOrString& name, LANGID wLang) const
	{
		return !GetRisohTemplate(type, name, wLang).empty();
	}

	// Handles OK for RT_GROUP_ICON, RT_GROUP_CURSOR, and RT_BITMAP, taking over
	// the role of the former MAddIconDlg, MAddCursorDlg, and MAddBitmapDlg.
	void OnOkIconCursorBitmap(HWND hwnd, HWND hEdt1, MIdOrString type,
	                          const MIdOrString& name, LANGID lang)
	{
		// the file is mandatory for these types
		std::wstring file;
		if (!Edt1_CheckFile(hEdt1, file))
			return;

		if (type == RT_GROUP_ICON)
		{
			if (auto entry = g_res.find(ET_LANG, RT_GROUP_ICON, name, lang))
			{
				INT id = MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL);
				switch (id)
				{
				case IDYES:
					g_res.delete_entry(entry);
					break;
				case IDNO:
				case IDCANCEL:
					return;
				}
			}

			if (!g_res.add_group_icon(name, lang, file))
			{
				ErrorBoxDx(IDS_CANNOTADDICON);
				return;
			}
		}
		else if (type == RT_BITMAP)
		{
			if (auto entry = g_res.find(ET_LANG, RT_BITMAP, name, lang))
			{
				INT id = MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL);
				switch (id)
				{
				case IDYES:
					g_res.delete_entry(entry);
					break;
				case IDNO:
				case IDCANCEL:
					return;
				}
			}

			if (!g_res.add_bitmap(name, lang, file))
			{
				ErrorBoxDx(IDS_CANTADDBMP);
				return;
			}
		}
		else // RT_GROUP_CURSOR
		{
			// *.ani files masquerade as RT_GROUP_CURSOR until we can inspect them
			BOOL bAni = FALSE;
			LPCWSTR pchExt = PathFindExtensionW(file.c_str());
			if (lstrcmpiW(pchExt, L".ani") == 0)
				bAni = TRUE;

			if (auto entry = g_res.find(ET_LANG, (bAni ? RT_ANICURSOR : RT_GROUP_CURSOR), name, lang))
			{
				INT id = MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL);
				switch (id)
				{
				case IDYES:
					g_res.delete_entry(entry);
					break;
				case IDNO:
				case IDCANCEL:
					return;
				}
			}

			if (!bAni)
			{
				// *.cur files can actually be RIFF-based animated cursors too
				MByteStream bs;
				if (bs.LoadFromFile(file.c_str()) && bs.size() >= 4)
				{
					if (memcmp(bs.ptr(), "RIFF", 4) == 0)
						bAni = TRUE;
				}
			}

			if (bAni)
			{
				type = RT_ANICURSOR;
				MByteStream bs;
				if (!bs.LoadFromFile(file.c_str()) ||
					!g_res.add_lang_entry(type, name, lang, bs.data()))
				{
					ErrorBoxDx(IDS_CANNOTADDCUR);
					return;
				}
			}
			else
			{
				if (!g_res.add_group_cursor(name, lang, file))
				{
					ErrorBoxDx(IDS_CANNOTADDCUR);
					return;
				}
			}
		}

		m_type = type;
		m_name = name;
		m_lang = lang;

		EndDialog(IDOK);
	}

	void OnOK(HWND hwnd)
	{
		MIdOrString type;

		// cmb1 --> (iType, type)
		if (!CheckTypeComboBox(m_cmb1, type))
			return;

		if (type == RT_STRING)
		{
			// edt1 --> sz (trimmed)
			MStringW sz = GetDlgItemText(edt1);
			mstr_trim(sz);

			// clear the name if sz is empty
			if (sz.empty())
				SetDlgItemTextW(hwnd, cmb2, NULL);
		}

		if (type == RT_VERSION)
		{
			if (::GetWindowTextLengthW(GetDlgItem(hwnd, cmb2)) == 0)
				SetDlgItemTextW(hwnd, cmb2, L"1");
		}
		else if (type == RT_MESSAGETABLE)
		{
			if (::GetWindowTextLengthW(GetDlgItem(hwnd, cmb2)) == 0)
				SetDlgItemTextW(hwnd, cmb2, L"1");
		}

		// check the name combobox cmb2
		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		MIdOrString name;
		if (!Res_HasNoName(type) && !CheckNameComboBox(hCmb2, type, name))
			return; // failure

		// check the language combobox cmb3
		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		LANGID lang;
		if (!CheckLangComboBox(hCmb3, lang))
			return;     // failure

		// get the file path from edt1
		HWND hEdt1 = GetDlgItem(hwnd, edt1);
		std::wstring file = GetWindowTextW(hEdt1);
		mstr_trim(file);

		// RT_GROUP_ICON, RT_GROUP_CURSOR, and RT_BITMAP need their own special
		// handling (this is what MAddIconDlg, MAddCursorDlg, and MAddBitmapDlg
		// used to do before they were merged into this dialog)
		if (type == RT_GROUP_ICON || type == RT_GROUP_CURSOR || type == RT_BITMAP)
		{
			OnOkIconCursorBitmap(hwnd, hEdt1, type, name, lang);
			return;
		}

		if (file.empty() && !HasSample(type, name, lang))
		{
			Edit_SetSel(hEdt1, 0, -1);  // select all
			SetFocus(hEdt1);    // set focus
			ErrorBoxDx(IDS_ENTERFILE);
			return;
		}

		if (file.size() && !Edt1_CheckFile(hEdt1, file))
			return; // failure

		// find the language entry by type, name, lang
		if (g_res.find(ET_LANG, type, name, lang))
		{
			// query overwriting
			INT id = MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL);
			switch (id)
			{
			case IDYES:
				// delete the overlapped entries
				g_res.search_and_delete(ET_LANG, type, name, lang);
				break;

			case IDNO:
			case IDCANCEL:
				return;     // cancelled
			}
		}

		bool bTemplateToAdd = false;
		bool bAdded = false;

		// if there is sample and no file was specified, then
		if (file.empty() && HasSample(type, name, lang))
		{
			bTemplateToAdd = true;     // assume OK

			if (Res_HasNoName(type))
			{
				// if this type has no name, clear the related entries
				g_res.search_and_delete(ET_NAME, type, BAD_NAME, lang);
			}

			if (HasSample(type, name, lang))
			{
				// if the type has sample, then store the template text
				m_strText = GetRisohTemplate(type, name, lang);
			}
			else
			{
				// otherwise it's not OK
				bTemplateToAdd = false;
			}

			// set one to the name if it's RT_STRING
			if (type == RT_STRING)
			{
				name = 1;   // it will be fixed later
			}

			// TEXTINCLUDE should be neutral
			if (type == L"TEXTINCLUDE")
			{
				lang = 0;
			}

			if (bTemplateToAdd)    // it's OK
			{
				// add an empty entry (data will be set later)
				m_added_entry = g_res.add_lang_entry(type, name, lang);
				bAdded = true;

				// store the results
				m_type = type;
				m_name = name;
				m_lang = lang;
				m_strTemplate = m_strText;  // the template text
			}
		}

		// try to load the file if not OK
		MByteStreamEx bs;
		if (!bTemplateToAdd && !bs.LoadFromFile(file.c_str()))
		{
			// error
			ErrorBoxDx(IDS_CANNOTADDRES);
			return;
		}

		// if not added yet, then
		if (!bAdded)
		{
			// add the data from the file
			m_added_entry = g_res.add_lang_entry(type, name, lang, bs.data());

			// store the results
			m_type = type;
			m_name = name;
			m_lang = lang;
			m_strTemplate.clear();  // no template text
		}

		// finish the dialog
		EndDialog(IDOK);
	}

	void OnPsh1(HWND hwnd)  // "browse"
	{
		// get the text (trimmed)
		MStringW strFile = GetDlgItemText(edt1);
		mstr_trim(strFile);

		// strFile --> szFile
		WCHAR szFile[MAX_PATH];
		StringCchCopyW(szFile, _countof(szFile), strFile.c_str());

		// initialize OPENFILENAME structure
		OPENFILENAMEW ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
		ofn.hwndOwner = hwnd;
		ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ALLFILES));
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = _countof(szFile);
		ofn.lpstrTitle = LoadStringDx(IDS_ADDRES);
		ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
					OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
		ofn.lpstrDefExt = L"bin";   // the default extension
		if (GetOpenFileNameW(&ofn)) // "OK" button was pressed
		{
			DoFile(hwnd, szFile, m_curType);
		}
	}

	void DoFile(HWND hwnd, LPCWSTR szFile, const MIdOrString& type)
	{
		// set the file path
		SetDlgItemTextW(hwnd, edt1, szFile);

		// If name was empty, use file title
		MString strText = GetComboBoxText(GetDlgItem(hwnd, cmb2));
		if (strText.empty())
		{
			WCHAR szText[MAX_PATH];
			StringCchCopyW(szText, _countof(szText), szFile);
			PathRemoveExtensionW(szText);
			ComboBox_SetText(GetDlgItem(hwnd, cmb2), SanitizeIdentifier(PathFindFileNameW(szText)).c_str());
		}

		// update the icon/cursor/bitmap preview, if any
		UpdatePreview(hwnd, type, szFile);
	}

	// Shows a preview of the file being added when it's an icon, a cursor, or
	// a bitmap; hides the preview for any other resource type.
	void UpdatePreview(HWND hwnd, const MIdOrString& type, LPCWSTR szFile)
	{
		HWND hIco1 = GetDlgItem(hwnd, ico1);
		if (!hIco1)
			return;

		// clear the previous preview
		SendMessage(hIco1, STM_SETIMAGE, IMAGE_ICON, 0);
		if (m_hIcon)
		{
			DestroyIcon(m_hIcon);
			m_hIcon = NULL;
		}
		if (m_hCursor)
		{
			DestroyCursor(m_hCursor);
			m_hCursor = NULL;
		}
		if (m_hBitmap)
		{
			DeleteObject(m_hBitmap);
			m_hBitmap = NULL;
		}

		LONG_PTR style = GetWindowLongPtr(hIco1, GWL_STYLE);

		if (type == RT_GROUP_ICON && szFile && *szFile)
		{
			m_hIcon = (HICON)LoadImage(NULL, szFile, IMAGE_ICON, 32, 32,
			                            LR_LOADFROMFILE | LR_CREATEDIBSECTION);
			SetWindowLongPtr(hIco1, GWL_STYLE, (style & ~SS_TYPEMASK) | SS_ICON);
			SendMessage(hIco1, STM_SETIMAGE, IMAGE_ICON, (LPARAM)m_hIcon);
			ShowWindow(hIco1, m_hIcon ? SW_SHOW : SW_HIDE);
		}
		else if (type == RT_GROUP_CURSOR && szFile && *szFile)
		{
			m_hCursor = LoadCursorFromFile(szFile);
			SetWindowLongPtr(hIco1, GWL_STYLE, (style & ~SS_TYPEMASK) | SS_ICON);
			SendMessage(hIco1, STM_SETIMAGE, IMAGE_CURSOR, (LPARAM)m_hCursor);
			ShowWindow(hIco1, m_hCursor ? SW_SHOW : SW_HIDE);
		}
		else if (type == RT_BITMAP && szFile && *szFile)
		{
			m_hBitmap = (HBITMAP)LoadImage(NULL, szFile, IMAGE_BITMAP, 32, 32,
			                                LR_LOADFROMFILE | LR_CREATEDIBSECTION);
			SetWindowLongPtr(hIco1, GWL_STYLE, (style & ~SS_TYPEMASK) | SS_BITMAP);
			SendMessage(hIco1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hBitmap);
			ShowWindow(hIco1, m_hBitmap ? SW_SHOW : SW_HIDE);
		}
		else
		{
			ShowWindow(hIco1, SW_HIDE);
		}
	}

	void OnPsh2(HWND hwnd)
	{
		// show the ID list window
		SendMessage(GetParent(hwnd), WM_COMMAND, ID_IDLIST, 0);
	}

	void OnCmb1(HWND hwnd)
	{
		// get the text of combobox cmb1
		MString strText;
		INT iItem = ComboBox_GetCurSel(m_cmb1);
		if (iItem == CB_ERR)
		{
			strText = GetComboBoxText(m_cmb1);
		}
		else
		{
			strText = GetComboBoxLBText(m_cmb1, iItem);
		}

		// strText --> strIDType (trimmed)
		MString strIDType = strText;
		mstr_trim(strIDType);

		// cut off the text from " (" to end
		size_t k = strIDType.find(L" (");
		if (k != MString::npos)
			strIDType = strIDType.substr(0, k);

		// the resource type (RT_*) --> (type, iType)
		MIdOrString type;
		WORD nRT_ = (WORD)g_db.GetValue(L"RESOURCE", strIDType);
		INT iType = g_db.IDTypeFromResType(nRT_);
		if (nRT_ != 0)
		{
			type = nRT_;
			if (iType == IDTYPE_UNKNOWN)
			{
				iType = IDTYPE_RESOURCE;
			}
		}
		else
		{
			type = MIdOrString(strIDType.c_str());
		}

		// remember the current type and refresh the preview
		m_curType = type;
		MStringW curFile = GetDlgItemTextW(hwnd, edt1);
		mstr_trim(curFile);
		UpdatePreview(hwnd, type, curFile.c_str());

		if (HasSample(type, m_name, m_lang))
		{
			// if there is a sample for this type, the file path is optional
			SetDlgItemText(hwnd, stc2, LoadStringDx(IDS_OPTIONAL));
		}
		else
		{
			// the file path is non-optional
			SetDlgItemText(hwnd, stc2, NULL);
		}

		if (type == RT_STRING || type == RT_VERSION)
		{
			// the name is optional if RT_STRING or RT_VERSION
			SetDlgItemText(hwnd, stc1, LoadStringDx(IDS_OPTIONAL));
			InitComboBoxPlaceholder(m_cmb2, IDS_NOTEXT);
		}
		else
		{
			// otherwise the name is non-optional
			SetDlgItemText(hwnd, stc1, NULL);
			InitComboBoxPlaceholder(m_cmb2, IDS_INTEGERORIDENTIFIER);
		}

		// iType (IDTYPE_*) --> prefix
		auto prefix = MapIDTypeToPrefix(IDTYPE_(iType));
		if (prefix.empty())
			return;

		// prefix --> m_cmb2
		MString strCmb2Text = GetComboBoxText(m_cmb2);
		ComboBox_ResetContent(m_cmb2);
		if (type != RT_STRING)
		{
			auto table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
			for (auto& table_entry : table)
			{
				ComboBox_AddString(m_cmb2, table_entry.name.c_str());
			}
		}
		ComboBox_SetText(m_cmb2, strCmb2Text.c_str());

		m_pAutoComplete1->unbind();
		m_pAutoComplete1->Release();
		m_pAutoComplete1 = new MRisohAutoComplete(1, FALSE, type);

		COMBOBOXINFO info = { sizeof(info) };
		GetComboBoxInfo(m_cmb2, &info);
		HWND hwndEdit = info.hwndItem;
		m_pAutoComplete1->bind(hwndEdit);
	}

	// Selects the item of cmb1 (the resource type combobox) that matches the
	// given resource type (RT_*), then refreshes the dialog as if the user had
	// chosen it. Used to guess the resource type from a dropped file's extension.
	void SelectResType(HWND hwnd, const MIdOrString& type)
	{
		INT nCount = ComboBox_GetCount(m_cmb1);
		for (INT i = 0; i < nCount; ++i)
		{
			MString strIDType = GetComboBoxLBText(m_cmb1, i);
			mstr_trim(strIDType);

			// cut off the text from " (" to end
			size_t k = strIDType.find(L" (");
			if (k != MString::npos)
				strIDType = strIDType.substr(0, k);

			WORD nRT_ = (WORD)g_db.GetValue(L"RESOURCE", strIDType);
			if (nRT_ != 0 && type == nRT_)
			{
				ComboBox_SetCurSel(m_cmb1, i);
				OnCmb1(hwnd);
				return;
			}
		}
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case IDOK:
			OnOK(hwnd);
			break;

		case IDCANCEL:
			// cancel the dialog
			EndDialog(IDCANCEL);
			break;

		case psh1:      // browse
			OnPsh1(hwnd);
			break;

		case psh2:      // show the resource ID list
			OnPsh2(hwnd);
			break;

		case cmb1:
			if (codeNotify == CBN_SELCHANGE || codeNotify == CBN_KILLFOCUS)
			{
				// selection of cmb1 was changed
				OnCmb1(hwnd);
			}
			else if (codeNotify == CBN_EDITCHANGE)
			{
				// the text of cmb1 was changed
				m_cmb1.OnEditChange();  // input completion
				OnCmb1(hwnd);
			}
			break;

		case cmb2:
			if (codeNotify == CBN_EDITCHANGE)
			{
				m_cmb2.OnEditChange();  // input completion
			}
			break;

		case cmb3:
			if (codeNotify == CBN_EDITCHANGE)
			{
				m_cmb3.OnEditChange();  // input completion
			}
			break;
		}
	}

	void OnDropFiles(HWND hwnd, HDROP hdrop)
	{
		// file(s) has dropped
		WCHAR file[MAX_PATH];
		DragQueryFileW(hdrop, 0, file, _countof(file));

		// guess the resource type from the file extension
		LPCWSTR pchExt = PathFindExtensionW(file);
		if (lstrcmpiW(pchExt, L".ico") == 0)
			SelectResType(hwnd, RT_GROUP_ICON);
		else if (lstrcmpiW(pchExt, L".cur") == 0)
			SelectResType(hwnd, RT_GROUP_CURSOR);
		else if (lstrcmpiW(pchExt, L".bmp") == 0)
			SelectResType(hwnd, RT_BITMAP);

		DoFile(hwnd, file, m_curType);
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		}
		return 0;
	}
};
