// MAddResIDDlg.hpp --- "Add Resource ID" Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include <unordered_map>
#include "resource.h"
#include "MWindowBase.hpp"
#include "ConstantsDB.hpp"
#include "Res.hpp"
#include "RisohSettings.hpp"
#include "Common.hpp"

extern std::unordered_map<INT, MStringW> *g_pmapIDTypeToLocalized;
extern std::unordered_map<MStringW, INT> *g_pmapLocalizedToIDType;

//////////////////////////////////////////////////////////////////////////////

class MAddResIDDlg : public MDialogBase
{
public:
	MString m_str1;
	MString m_str2;
	BOOL m_bChanging;
	BOOL m_bIsHelpID; // TRUE if the selected type is IDTYPE_HELP

	MAddResIDDlg() : MDialogBase(IDD_ADDRESID)
	{
		m_bChanging = FALSE;
		m_bIsHelpID = FALSE;
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		ConstantsDB::TableType table;

		const INT IDTYPE_default = IDTYPE_COMMAND;

		INT i = 0;
		for (auto& pair : (*g_pmapIDTypeToLocalized))
		{
			INT k = (INT)SendDlgItemMessage(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)pair.second.c_str());
			if (k == IDTYPE_default)
			{
				m_bChanging = TRUE;
				SendDlgItemMessage(hwnd, cmb1, CB_SETCURSEL, k, 0);
				m_bChanging = FALSE;
			}
			++i;
		}

		table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
		m_bChanging = TRUE;
		SetDlgItemTextW(hwnd, edt1, table[IDTYPE_default].name.c_str());
		m_bChanging = FALSE;

		SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE32, SHRT_MIN, USHRT_MAX);

		CenterWindowDx();
		return TRUE;
	}

	void OnOK(HWND hwnd)
	{
		MString str1 = GetDlgItemText(hwnd, edt1);
		ReplaceFullWithHalf(str1);
		mstr_trim(str1);
		if (str1.empty())
		{
			HWND hEdt1 = GetDlgItem(hwnd, edt1);
			Edit_SetSel(hEdt1, 0, -1);
			SetFocus(hEdt1);
			ErrorBoxDx(IDS_ENTERTEXT);
			return;
		}
		m_str1 = str1;

		// Detect whether the selected type is Help ID
		MString strType;
		{
			HWND hCmb1 = GetDlgItem(hwnd, cmb1);
			INT iSel = (INT)SendMessage(hCmb1, CB_GETCURSEL, 0, 0);
			if (iSel != CB_ERR)
				strType = GetComboBoxLBText(hCmb1, iSel);
		}
		m_bIsHelpID = (UnMapIDType(strType) == IDTYPE_HELP) ? TRUE : FALSE;

		MString str2 = GetDlgItemText(hwnd, edt2);
		ReplaceFullWithHalf(str2);
		mstr_trim(str2);

		if (m_bIsHelpID)
		{
			// Help ID: accept signed [-2147483648, 2147483647] or unsigned [0, 4294967295]
			DWORD dw;
			if (str2.empty() || !IsValidHelpIDText(str2.c_str(), &dw))
			{
				HWND hEdt2 = GetDlgItem(hwnd, edt2);
				Edit_SetSel(hEdt2, 0, -1);
				SetFocus(hEdt2);
				ErrorBoxDx(IDS_ENTERINT);
				return;
			}
		}
		else
		{
			INT value = mstr_parse_int(str2.c_str());
			if (str2.empty() || value < SHRT_MIN || value > USHRT_MAX)
			{
				value = std::min(std::max(value, SHRT_MIN), USHRT_MAX);
				SetDlgItemInt(hwnd, edt2, value, TRUE);
				HWND hEdt2 = GetDlgItem(hwnd, edt2);
				Edit_SetSel(hEdt2, 0, -1);
				SetFocus(hEdt2);
				ErrorBoxDx(IDS_ENTERINT);
				return;
			}
		}
		m_str2 = std::move(str2);

		MStringA str1a = MTextToAnsi(CP_ACP, str1).c_str();
		if (g_settings.id_map.find(str1a) != g_settings.id_map.end())
		{
			HWND hEdt1 = GetDlgItem(hwnd, edt1);
			Edit_SetSel(hEdt1, 0, -1);
			SetFocus(hEdt1);
			ErrorBoxDx(IDS_ALREADYEXISTS);
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
		case edt1:
			if (codeNotify == EN_CHANGE && !m_bChanging)
			{
				MString name = GetDlgItemText(hwnd, edt1);
				mstr_trim(name);

				MString prefix = name.substr(0, name.find(L'_') + 1);

				std::vector<INT> indexes;
				indexes = GetPrefixIndexes(prefix);
				if (indexes.empty() || indexes.size() >= 2)
				{
					m_bChanging = TRUE;
					SendDlgItemMessage(hwnd, cmb1, CB_SETCURSEL, -1, 0);
					m_bChanging = FALSE;
					break;
				}

				ConstantsDB::TableType table;
				table = g_db.GetTable(L"RESOURCE.ID.PREFIX");

				m_bChanging = TRUE;
				SendDlgItemMessage(hwnd, cmb1, CB_SETCURSEL, indexes[0], 0);
				m_bChanging = FALSE;
			}
			break;
		case cmb1:
			if (codeNotify == CBN_SELCHANGE && !m_bChanging)
			{
				HWND hCmb1 = GetDlgItem(hwnd, cmb1);
				INT k = INT(SendMessage(hCmb1, CB_GETCURSEL, 0, 0));
				if (k != -1)
				{
					ConstantsDB::TableType table;
					table = g_db.GetTable(L"RESOURCE.ID.PREFIX");

					m_bChanging = TRUE;
					SetDlgItemText(hwnd, edt1, table[k].name.c_str());
					m_bChanging = FALSE;
				}
			}
			break;
		case psh1:
			{
				SetDlgItemTextW(hwnd, edt2, NULL);

				MString name = GetDlgItemText(hwnd, edt1);
				mstr_trim(name);

				MString prefix = name.substr(0, name.find(L'_') + 1);
				if (prefix.size())
				{
					ConstantsDB::TableType table;
					table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);

					UINT nMax = 0;
					for (auto& table_entry : table)
					{
						if (table_entry.name == L"IDC_STATIC")
							continue;
						if (nMax < table_entry.value)
							nMax = table_entry.value;
					}

					INT nIDTYPE_ = IDTYPE_UNKNOWN;
					for (auto& pair : g_settings.assoc_map)
					{
						if (pair.second == prefix)
						{
							nIDTYPE_ = UnMapIDType(pair.first);
							break;
						}
					}

					INT nNextID = nMax + 1;

					switch (nIDTYPE_)
					{
					case IDTYPE_UNKNOWN:
					case IDTYPE_MESSAGE:
					case IDTYPE_WINDOW:
						break;
					default:
						if (nNextID < 100)
							nNextID = 100;
						break;
					}

					SetDlgItemInt(hwnd, edt2, nNextID, TRUE);
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
		return DefaultProcDx();
	}
};
