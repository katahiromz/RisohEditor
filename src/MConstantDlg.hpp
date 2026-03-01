// MConstantDlg.hpp --- "Query Constant" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "MString.hpp"
#include "RisohSettings.hpp"
#include "MComboBoxAutoComplete.hpp"
#include "ConstantsDB.hpp"
#include "Common.hpp"

class MConstantDlg;

//////////////////////////////////////////////////////////////////////////////

class MConstantDlg : public MDialogBase
{
public:
	MComboBoxAutoComplete m_cmb1;

	MConstantDlg() : MDialogBase(IDD_CONSTANT)
	{
	}

	virtual ~MConstantDlg()
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		InitConstantComboBox(hCmb1);
		SubclassChildDx(m_cmb1, cmb1);

		CenterWindowDx();
		return TRUE;
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
					WCHAR szText[MAX_PATH];
					SendMessageW(m_cmb1, CB_GETLBTEXT, iItem, (LPARAM)szText);
					name = szText;
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
					StringCbPrintfW(szText, sizeof(szText), L"0x%08lX", value);
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

	virtual INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		}
		return 0;
	}
};
