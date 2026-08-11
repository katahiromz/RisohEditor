// MConstantDlg.hpp --- "Query Constant" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "MString.hpp"
#include "settings.h"
#include "MComboBoxAutoComplete.hpp"
#include "ConstantsDB.hpp"
#include "Common.hpp"

class MConstantDlg;

//////////////////////////////////////////////////////////////////////////////

class MConstantDlg : public MDialogBase
{
public:
	MComboBoxAutoComplete m_cmb1;
	MStringW m_str;
	WNDPROC m_fnOldEdt2WndProc = nullptr;
	WNDPROC m_fnOldEdt3WndProc = nullptr;

	MConstantDlg(PCWSTR text = L"") : MDialogBase(IDD_CONSTANT)
	{
		m_str = text;
	}

	static LRESULT CALLBACK Edt2WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		auto pThis = (MConstantDlg*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		switch (uMsg)
		{
		case WM_SETFOCUS:
		case WM_LBUTTONUP:
		case WM_COPY:
			{
				LRESULT ret = ::CallWindowProcW(pThis->m_fnOldEdt2WndProc, hwnd, uMsg, wParam, lParam);
				if (uMsg == WM_COPY)
					PostMessageW(hwnd, EM_SETSEL, -1, -1); // Select none
				else
					PostMessageW(hwnd, EM_SETSEL, 0, -1); // Select all
				return ret;
			}
			break;
		}
		return ::CallWindowProcW(pThis->m_fnOldEdt2WndProc, hwnd, uMsg, wParam, lParam);
	}

	static LRESULT CALLBACK Edt3WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		auto pThis = (MConstantDlg*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		switch (uMsg)
		{
		case WM_SETFOCUS:
		case WM_LBUTTONUP:
		case WM_COPY:
			{
				LRESULT ret = ::CallWindowProcW(pThis->m_fnOldEdt3WndProc, hwnd, uMsg, wParam, lParam);
				if (uMsg == WM_COPY)
					PostMessageW(hwnd, EM_SETSEL, -1, -1); // Select none
				else
					PostMessageW(hwnd, EM_SETSEL, 0, -1); // Select all
				return ret;
			}
			break;
		}
		return ::CallWindowProcW(pThis->m_fnOldEdt3WndProc, hwnd, uMsg, wParam, lParam);
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		InitConstantComboBox(hCmb1);
		SubclassChildDx(m_cmb1, cmb1);

		if (m_str.size())
		{
			ComboBox_SetText(m_cmb1, m_str.c_str());
			m_cmb1.OnEditChange();
		}

		// Subclassing
		HWND hEdt2 = GetDlgItem(hwnd, edt2);
		HWND hEdt3 = GetDlgItem(hwnd, edt3);
		m_fnOldEdt2WndProc = (WNDPROC)SetWindowLongPtrW(hEdt2, GWLP_WNDPROC, (LONG_PTR)Edt2WndProc);
		m_fnOldEdt3WndProc = (WNDPROC)SetWindowLongPtrW(hEdt3, GWLP_WNDPROC, (LONG_PTR)Edt3WndProc);
		SetWindowLongPtrW(hEdt2, GWLP_USERDATA, (LONG_PTR)this);
		SetWindowLongPtrW(hEdt3, GWLP_USERDATA, (LONG_PTR)this);

		CenterWindowDx();
		return TRUE;
	}

	void OnDestroy(HWND hwnd)
	{
		// Un-subclassing
		HWND hEdt2 = GetDlgItem(hwnd, edt2);
		HWND hEdt3 = GetDlgItem(hwnd, edt3);
		SetWindowLongPtrW(hEdt2, GWLP_WNDPROC, (LONG_PTR)m_fnOldEdt2WndProc);
		SetWindowLongPtrW(hEdt3, GWLP_WNDPROC, (LONG_PTR)m_fnOldEdt3WndProc);
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(id);
			break;
		case cmb1:
			if (codeNotify == CBN_EDITCHANGE)
			{
				m_cmb1.OnEditChange();
			}
			{
				MStringW name;
				if (codeNotify == CBN_SELCHANGE)
				{
					INT iItem = (INT)SendMessageW(m_cmb1, CB_GETCURSEL, 0, 0);
					if (iItem != CB_ERR)
					{
						WCHAR szText[MAX_PATH];
						SendMessageW(m_cmb1, CB_GETLBTEXT, iItem, (LPARAM)szText);
						name = szText;
					}
				}
				else
				{
					name = GetDlgItemText(hwnd, cmb1);
				}
				mstr_trim(name);

				ConstantsDB::ValueType value;
				BOOL bOK = g_db.GetValueOfName(name, value);
				if (!bOK)
				{
					for (auto& pair : g_settings.id_map)
					{
						MAnsiToWide wide(CP_ACP, pair.first.c_str());

						if (name == wide.c_str())
						{
							value = strtol(pair.second.c_str(), NULL, 0);
							bOK = TRUE;
							break;
						}
					}
				}
				if (bOK)
				{
					SetDlgItemInt(hwnd, edt2, value, FALSE);

					WCHAR szText[MAX_PATH];
					StringCbPrintfW(szText, sizeof(szText), L"0x%lX", value);
					SetDlgItemText(hwnd, edt3, szText);
				}
				else
				{
					SetDlgItemTextW(hwnd, edt2, L"");
					SetDlgItemText(hwnd, edt3, L"");
				}
			}
			break;
		}
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
		}
		return 0;
	}
};
