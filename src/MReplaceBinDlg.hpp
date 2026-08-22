// MReplaceBinDlg.hpp --- "Replace Binary" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "settings.h"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "Utils.h"

//////////////////////////////////////////////////////////////////////////////

class MReplaceBinDlg : public MDialogBase
{
public:
	EntryPtr m_entry;
	MIdOrString m_type;
	MIdOrString m_name;
	LANGID m_lang;

	MReplaceBinDlg(EntryBase *entry)
		: MDialogBase(IDD_REPLACERES)
		, m_entry(g_res.get_shared(entry))
		, m_type(entry->m_type)
		, m_name(entry->m_name)
		, m_lang(entry->m_lang)
	{
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

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		DragAcceptFiles(hwnd, TRUE);

		// for Types
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		EnableWindow(hCmb1, FALSE);

		InitResTypeComboBox(hCmb1, m_type);

		// for Names
		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		InitResNameComboBox(hCmb2, m_entry->m_name, IDTYPE_RESOURCE);

		// for Langs
		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		InitLangComboBox(hCmb3, m_entry->m_lang);

		FileSystemAutoComplete(GetDlgItem(hwnd, edt1));

		CenterWindowDx();
		return TRUE;
	}

	void OnPsh1(HWND hwnd)
	{
		MStringW strFile = GetDlgItemText(edt1);
		mstr_trim(strFile);

		WCHAR szFile[MAX_PATH];
		lstrcpyn(szFile, strFile.c_str(), _countof(szFile));

		OPENFILENAMEW ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
		ofn.hwndOwner = hwnd;
		ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ALLFILES));
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = _countof(szFile);
		ofn.lpstrTitle = LoadStringDx(IDS_REPLACERES);
		ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
			OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
		ofn.lpstrDefExt = L"bin";
		if (GetOpenFileNameW(&ofn))
		{
			SetDlgItemTextW(hwnd, edt1, szFile);
		}
	}

	void OnOK(HWND hwnd)
	{
		MIdOrString type;
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		if (!CheckTypeComboBox(hCmb1, type))
			return;

		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		MIdOrString name;
		if (!CheckNameComboBox(hCmb2, type, name))
			return;

		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		LANGID lang;
		if (!CheckLangComboBox(hCmb3, lang))
			return;

		std::wstring file;
		HWND hEdt1 = GetDlgItem(hwnd, edt1);
		if (!Edt1_CheckFile(hEdt1, file))
			return;

		MByteStreamEx bs;
		if (!bs.LoadFromFile(file.c_str()))
		{
			ErrorBoxDx(IDS_CANNOTREPLACE);
			return;
		}

		g_res.add_lang_entry(type, name, lang, bs.data());

		m_type = type;
		m_name = name;
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
		case psh1:
			OnPsh1(hwnd);
			break;
		}
	}

	void OnDropFiles(HWND hwnd, HDROP hdrop)
	{
		WCHAR file[MAX_PATH];
		DragQueryFileW(hdrop, 0, file, _countof(file));
		SetDlgItemTextW(hwnd, edt1, file);
	}
};
