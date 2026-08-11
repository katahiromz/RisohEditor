// MCryptoDlg.hpp
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MString.hpp"
#include <commdlg.h>
#include "WonResWrap.h"
#include "MEncryptTypesDlg.hpp"

//////////////////////////////////////////////////////////////////////////////

class MCryptoDlg : public MDialogBase
{
public:
	MCryptoDlg() : MDialogBase(IDD_CRYPTO)
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
#ifdef ENABLE_CRYPTO
		BOOL bCrypto = (g_bEnableCrypto && g_password.size());
		CheckDlgButton(hwnd, chx1, (bCrypto ? BST_CHECKED : BST_UNCHECKED));
		EnableWindow(GetDlgItem(hwnd, edt1), bCrypto);
		EnableWindow(GetDlgItem(hwnd, edt2), bCrypto);
		EnableWindow(GetDlgItem(hwnd, edt3), bCrypto);
		EnableWindow(GetDlgItem(hwnd, psh2), bCrypto);
		for (auto& ch : g_password) ch ^= 0xFFFF;
		for (auto& ch : g_salt) ch ^= 0xFFFF;
		SetDlgItemTextW(hwnd, edt1, g_password.c_str());
		SetDlgItemTextW(hwnd, edt2, g_salt.c_str());
		for (auto& ch : g_password) ch ^= 0xFFFF;
		for (auto& ch : g_salt) ch ^= 0xFFFF;
		SendDlgItemMessageW(hwnd, edt1, EM_LIMITTEXT, MAX_PATH - 1, 0);
		SendDlgItemMessageW(hwnd, edt2, EM_LIMITTEXT, MAX_PATH - 1, 0);
#else
		EnableWindow(GetDlgItem(hwnd, chx1), FALSE);
		EnableWindow(GetDlgItem(hwnd, edt1), FALSE);
		EnableWindow(GetDlgItem(hwnd, edt2), FALSE);
		EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
		EnableWindow(GetDlgItem(hwnd, psh1), FALSE);
		EnableWindow(GetDlgItem(hwnd, psh2), FALSE);
#endif
		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
#ifdef ENABLE_CRYPTO
		g_bEnableCrypto = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
		MString str1 = GetDlgItemText(edt1);
		MString str2 = GetDlgItemText(edt2);
		if (g_bEnableCrypto)
		{
			mstr_trim(str1);
			if (str1.empty())
			{
				MessageBoxW(hwnd, LoadStringDx(IDS_ENTERPASSWORD), NULL, MB_ICONERROR);
				return;
			}
			mstr_trim(str2);
			if (str2.size() < 16)
			{
				MessageBoxW(hwnd, LoadStringDx(IDS_ENTERSALT), NULL, MB_ICONERROR);
				return;
			}
			if (g_encrypted_types.empty())
			{
				switch (MessageBoxW(hwnd, LoadStringDx(IDS_CRYPTTARGETEMPTY), NULL,
				                    MB_ICONWARNING | MB_YESNOCANCEL))
				{
				case IDYES:
					break;
				case IDNO:
				case IDCANCEL:
					return;
				}
			}
		}
		mstr_trim(str1);
		mstr_trim(str2);
		for (auto& ch : str1) ch ^= 0xFFFF;
		for (auto& ch : str2) ch ^= 0xFFFF;
		g_password = std::move(str1);
		g_salt = std::move(str2);
#endif
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
		case psh2:
			OnPsh2(hwnd);
			break;
		case chx1:
			OnChx1(hwnd);
			break;
		}
	}

	void OnChx1(HWND hwnd)
	{
#ifdef ENABLE_CRYPTO
		BOOL bCrypto = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
		EnableWindow(GetDlgItem(hwnd, edt1), bCrypto);
		EnableWindow(GetDlgItem(hwnd, edt2), bCrypto);
		EnableWindow(GetDlgItem(hwnd, edt3), bCrypto);
		EnableWindow(GetDlgItem(hwnd, psh2), bCrypto);
#endif
	}

	void OnPsh1(HWND hwnd)
	{
		GUID guid;
		WCHAR text[40];

		CoCreateGuid(&guid);
		StringFromGUID2(guid, text, _countof(text));
		SetDlgItemTextW(hwnd, edt1, text);

		CoCreateGuid(&guid);
		StringFromGUID2(guid, text, _countof(text));
		SetDlgItemTextW(hwnd, edt2, text);

		DWORD count = 500 + std::rand() % 500;
		SetDlgItemInt(hwnd, edt3, count, FALSE);
	}

	void OnPsh2(HWND hwnd)
	{
		MEncryptTypesDlg dialog;
		dialog.DialogBoxDx(hwnd);
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		default:
			return 0;
		}
	}
};
