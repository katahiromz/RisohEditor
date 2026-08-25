// RisohEditor.cpp --- RisohEditor
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "MMainWnd.hpp"
#define LINENUMEDIT_IMPL
#include "LineNumEdit.hpp"
#include "MacroParser.hpp"
#include "ToolbarRes.hpp"
#include "Utils.h"
#include "WonRes.h"
#include "resource.h"
#include "protection.h"
#include <process.h>   // _beginthreadex

LPWSTR g_pszLogFile = NULL;

std::unordered_map<INT, MStringW> *g_pmapIDTypeToLocalized = NULL;
std::unordered_map<MStringW, INT> *g_pmapLocalizedToIDType = NULL;
std::vector<MString> g_types;
std::vector<MString> g_names;
std::vector<MString> g_keys;
std::vector<MString> g_ctrl_ids;
std::vector<MString> g_string_ids;
std::vector<MString> g_help_ids;
std::vector<MString> g_encrypted_types;
std::vector<MString> g_font_names;
HWND s_hwndEga = NULL;

/*static*/ HHOOK MSplashWindow::s_hKeyboardHook = NULL;
/*static*/ HWND MSplashWindow::s_hwndSplash = NULL;

#ifndef _MSC_VER
	typedef int (WINAPI *FN_GetMenuPosFromID)(HMENU hmenu, UINT id);
	static FN_GetMenuPosFromID g_fnGetMenuPosFromID = nullptr;
#endif

void InitIDTypeMaps()
{
	g_pmapIDTypeToLocalized = new std::unordered_map<INT, MStringW>();
	g_pmapLocalizedToIDType = new std::unordered_map<MStringW, INT>();

	MStringW s;
#define DEFINE_IDTYPE(index, idtype, str, wstr, ids, prefix) do { \
	s = LoadStringDx(ids); \
	(*g_pmapIDTypeToLocalized)[index] = s; \
	(*g_pmapLocalizedToIDType)[s] = index; \
} while (0);
#include "idtypes.h"
#undef DEFINE_IDTYPE
}

void UnloadIDTypeMaps()
{
	delete g_pmapIDTypeToLocalized;
	g_pmapIDTypeToLocalized = NULL;
	delete g_pmapLocalizedToIDType;
	g_pmapLocalizedToIDType = NULL;
}

MStringW MapIDType(IDTYPE_ nIDType)
{
	auto it = (*g_pmapIDTypeToLocalized).find(nIDType);
	if (it != (*g_pmapIDTypeToLocalized).end())
		return it->second;
	return (*g_pmapIDTypeToLocalized)[0];
}

IDTYPE_ UnMapIDType(const MStringW& str)
{
	auto it = (*g_pmapLocalizedToIDType).find(str);
	if (it != (*g_pmapLocalizedToIDType).end())
		return (IDTYPE_)it->second;
	return IDTYPE_UNKNOWN;
}

//////////////////////////////////////////////////////////////////////////////
// types

struct CODEPAGE_INFO
{
	UINT number;
	const char *name;
};

typedef HRESULT (WINAPI *SETWINDOWTHEME)(HWND, LPCWSTR, LPCWSTR);

//////////////////////////////////////////////////////////////////////////////
// constants

#define CX_STATUS_PART  80      // status bar part width
#define ERROR_LINE_COLOR RGB(255, 191, 191)

#define MYWM_UPDATEARROW (WM_USER + 114)
#define MYWM_GETDLGHEADLINES (WM_USER + 250)

// the maximum number of backup
static const UINT s_nBackupMaxCount = 5;

static const CODEPAGE_INFO g_codepage_info[] = {
#define DEFINE_CODEPAGE(number, name) { number, name },
#include "codepages.h"
#undef DEFINE_CODEPAGE
};

//////////////////////////////////////////////////////////////////////////////
// global variables

HWND g_hMainWnd = NULL;     // main window handle

ConstantsDB g_db;           // constants database
RisohSettings g_settings;   // settings
EntrySet g_res;             // the set of resource items

// contents modified?
BOOL s_bModified = FALSE;
INT s_ret = 0;           // return value storage
static SETWINDOWTHEME s_pSetWindowTheme = NULL;  // pointer to SetWindowTheme API

//////////////////////////////////////////////////////////////////////////////
// helper functions

LPCSTR FindCodePageName(UINT nCodePage)
{
	for (auto& entry : g_codepage_info)
	{
		if (entry.number == nCodePage)
			return entry.name;
	}
	return "(Unknown)";
}

void DoSetFileModified(BOOL bModified)
{
	s_bModified = bModified;
}

//////////////////////////////////////////////////////////////////////////////
// MMainWnd out-of-line functions

// WM_SYSCOLORCHANGE: system color settings was changed
void MMainWnd::OnSysColorChange(HWND hwnd)
{
	// notify the main window children
	m_splitter1.SendMessageDx(WM_SYSCOLORCHANGE);
	m_splitter2.SendMessageDx(WM_SYSCOLORCHANGE);
	m_rad_window.SendMessageDx(WM_SYSCOLORCHANGE);
}

// WM_SETFOCUS
void MMainWnd::OnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
	m_arrow.ShowDropDownList(m_arrow, FALSE);
}

// WM_KILLFOCUS
void MMainWnd::OnKillFocus(HWND hwnd, HWND hwndNewFocus)
{
	m_arrow.ShowDropDownList(m_arrow, FALSE);
}

// MYWM_COMPILECHECK: check whether it needs compilation
LRESULT MMainWnd::OnCompileCheck(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// compile if necessary
	return CompileIfNecessary(TRUE);
}

// MYWM_REOPENRAD: reopen the RADical window
LRESULT MMainWnd::OnReopenRad(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	OnGuiEdit(hwnd);
	if (IsWindow(m_rad_window) && !IsRectEmpty(&m_rcRadWindow))
	{
		RECT& rc = m_rcRadWindow;
		SetWindowPos(m_rad_window, nullptr, rc.left, rc.top, 0, 0,
            	     SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER);
		SetRectEmpty(&rc);
	}
	return 0;
}

// MYWM_MOVESIZEREPORT: report the position and size to the status bar
LRESULT MMainWnd::OnMoveSizeReport(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// position
	INT x = (SHORT)LOWORD(wParam);
	INT y = (SHORT)HIWORD(wParam);

	// size
	INT cx = (SHORT)LOWORD(lParam);
	INT cy = (SHORT)HIWORD(lParam);

	// set the text to status bar
	ChangeStatusText(LoadStringPrintfDx(IDS_COORD, x, y, cx, cy));

	DoSetFileModified(TRUE);
	return 0;
}

// MYWM_CLEARSTATUS: clear the status bar
LRESULT MMainWnd::OnClearStatus(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	ChangeStatusText(TEXT(""));
	return 0;
}

// WM_ACTIVATE: if activated, then set focus to m_hwndTV
void MMainWnd::OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
	static HWND s_hwndOldFocus = NULL;

	if (state == WA_ACTIVE || state == WA_CLICKACTIVE)
	{
		if (s_hwndOldFocus)
			SetFocus(s_hwndOldFocus);
		else
			SetFocus(m_hwndTV);
	}

	if (state == WA_INACTIVE)
	{
		s_hwndOldFocus = GetFocus();
	}

	// default processing
	FORWARD_WM_ACTIVATE(hwnd, state, hwndActDeact, fMinimized, CallWindowProcDx);
}

// update the menu for recently used files
void MMainWnd::UpdateMenu()
{
	// get 'File' menu
	HMENU hMenu = GetMenu(m_hwnd);
	HMENU hFileMenu = GetSubMenu(hMenu, 0);

	// get Most Recently Used menu from 'File' menu
	HMENU hMruMenu = GetSubMenu(hFileMenu, GetMenuItemCount(hFileMenu) - 3);
	assert(hMruMenu);

	// delete all the menu items from MRU menu
	while (DeleteMenu(hMruMenu, 0, MF_BYPOSITION))
		;

	TCHAR szText[MAX_PATH * 2];
	static const TCHAR szPrefix[] = TEXT("123456789ABCDEF0");

	// add the MRU menu items to the MRU menu
	INT i = 0;
	for (auto& recent : g_settings.vecRecentlyUsed)
	{
		// get the file title
		LPCTSTR pch = _tcsrchr(recent.c_str(), TEXT('\\'));
		if (pch == NULL)
			pch = _tcsrchr(recent.c_str(), TEXT('/'));
		if (pch == NULL)
			pch = recent.c_str();
		else
			++pch;

		// build the text
		StringCchPrintf(szText, _countof(szText), TEXT("&%c  %s"), szPrefix[i], pch);

		// insert an item to the MRU menu
		InsertMenu(hMruMenu, i, MF_BYPOSITION | MF_STRING, ID_MRUFILE0 + i, szText);

		++i;    // increment the index
		if (i >= (INT)_countof(szPrefix) - 1)
			break;
	}

	if (g_settings.vecRecentlyUsed.empty())
	{
		// set the "(none)" item if empty
		InsertMenu(hMruMenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, -1, LoadStringDx(IDS_NONE));
	}
}

void MMainWnd::OnExtractDFM(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)
		return;

	WCHAR szFile[MAX_PATH] = L"";
	ResToText res2text;
	MString strFile = res2text.GetEntryFileName(*entry);
	if (strFile.size())
	{
		StringCbCopyW(szFile, sizeof(szFile), strFile.c_str());
	}

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_DFMFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTDFM);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"dfm";

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		if (lstrcmpiW(PathFindExtensionW(szFile), L".txt") == 0)
		{
			if (FILE *fp = _wfopen(szFile, L"wb"))
			{
				auto ansi = dfm_text_from_binary(m_szDFMSC, entry->ptr(), entry->size(),
												 g_settings.nDfmCodePage, g_settings.bDfmRawTextComments);
				fwrite(ansi.c_str(), ansi.size(), 1, fp);
				fflush(fp);
				fclose(fp);
			}
			else
			{
				ErrorBoxDx(IDS_CANTEXTRACTDFM);
			}
		}
		else
		{
			if (!g_res.extract_bin(ofn.lpstrFile, entry))
			{
				ErrorBoxDx(IDS_CANTEXTRACTDFM);
			}
		}
	}
}

void MMainWnd::OnExtractTLB(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)
		return;

	WCHAR szFile[MAX_PATH] = L"";
	ResToText res2text;
	MString strFile = res2text.GetEntryFileName(*entry);
	if (strFile.size())
	{
		StringCbCopyW(szFile, sizeof(szFile), strFile.c_str());
	}

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_TLBRESBINFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTTLB);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"tlb";

	// let the user choose the path
	if (!GetSaveFileNameW(&ofn))
		return;

	auto dotext = PathFindExtensionW(szFile);
	if (lstrcmpiW(dotext, L".txt") == 0 || lstrcmpiW(dotext, L".idl") == 0)
	{
		std::string ansi;
		ansi = tlb_text_from_binary(m_szOleBow, entry->ptr(), entry->size());
		if (FILE *fp = _wfopen(szFile, L"wb"))
		{
			fwrite(ansi.c_str(), ansi.size(), 1, fp);
			fflush(fp);
			fclose(fp);
		}
		else
		{
			ErrorBoxDx(IDS_CANTEXTRACTTLB);
		}
	}
	else
	{
		if (!g_res.extract_bin(ofn.lpstrFile, entry))
		{
			ErrorBoxDx(IDS_CANTEXTRACTTLB);
		}
	}
}

std::wstring MMainWnd::ParseVersionFile(LPCWSTR pszFile, std::wstring& url) const
{
	std::wstring ret;
	char buf[256];
	if (FILE *fp = _wfopen(pszFile, L"rb"))
	{
		while (fgets(buf, 256, fp))
		{
			std::string str = buf;
			mstr_trim(str, " \t\r\n");
			if (str.find("VERSION:") == 0)
			{
				str = str.substr(8);
				mstr_trim(str, " \t\r\n");
				MAnsiToWide a2w(CP_ACP, str);
				ret = a2w.c_str();
			}
			else if (str.find("URL:") == 0)
			{
				str = str.substr(4);
				mstr_trim(str, " \t\r\n");
				MAnsiToWide a2w(CP_ACP, str);
				url = a2w.c_str();
			}
		}

		fclose(fp);
	}
	return ret;
}

std::wstring MMainWnd::GetRisohEditorVersion() const
{
	WCHAR szFile[MAX_PATH];
	GetModuleFileNameW(NULL, szFile, _countof(szFile));

	DWORD dwHandle;
	DWORD dwSize = GetFileVersionInfoSizeW(szFile, &dwHandle);
	if (!dwSize)
	{
		return L"";
	}

	std::vector<BYTE> data;
	data.resize(dwSize);
	if (!GetFileVersionInfoW(szFile, dwHandle, dwSize, &data[0]))
	{
		return L"";
	}

	LPVOID pValue;
	UINT uLen;

	if (!VerQueryValueW(&data[0], L"\\VarFileInfo\\Translation",
						&pValue, &uLen))
	{
		return L"";
	}

	WCHAR szValue[MAX_PATH];
	DWORD dwValue = *(LPDWORD)pValue;
	StringCbPrintfW(szValue, sizeof(szValue), L"%04X%04X", LOWORD(dwValue), HIWORD(dwValue));

	std::wstring key = L"\\StringFileInfo\\";
	key += szValue;
	key += L"\\ProductVersion";
	if (!VerQueryValueW(&data[0], key.c_str(), &pValue, &uLen))
	{
		return L"";
	}

	std::wstring ret = (LPWSTR)pValue;
	return ret;
}

void MMainWnd::PostUpdateArrow(HWND hwnd)
{
	PostMessage(hwnd, MYWM_UPDATEARROW, 0, 0);
}

// MYWM_UPDATEARROW
LRESULT MMainWnd::OnUpdateArrow(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UpdateArrow();
	return 0;
}

// MYWM_RADDBLCLICK
LRESULT MMainWnd::OnRadDblClick(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	if (lParam)
		m_rad_window.OnCtrlProp(m_rad_window);
	else
		m_rad_window.OnDlgProp(m_rad_window);
	return 0;
}

// MYWM_AUTOCOMPLETE
LRESULT MMainWnd::OnAutoComplete(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	HWND hwndEdit = TreeView_GetEditControl(m_hwndTV);
	DoTVEditAutoComplete(hwnd, hwndEdit);
    return 0;
}

// MYWM_AUTOCOMPLETEDONE
LRESULT MMainWnd::OnAutoCompleteDone(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	DoTVEditAutoCompleteRelease(hwnd);
    return 0;
}

// WM_EGA_FINISH
LRESULT MMainWnd::OnEgaFinish(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	BOOL bModifiedOld = s_bModified;
	DoRefreshTV(hwnd, TRUE);
	DoRefreshIDList(hwnd);
	s_bModified = bModifiedOld;

	if (g_RES_select_type != BAD_TYPE ||
		g_RES_select_name != BAD_TYPE ||
		g_RES_select_lang != BAD_LANG)
	{
		EntrySet found;
		g_res.search(found, ET_LANG, g_RES_select_type, g_RES_select_name, g_RES_select_lang);

		for (auto e : found)
		{
			SelectTV(e, FALSE);
			return 0;
		}

		g_RES_select_type = BAD_TYPE;
		g_RES_select_name = BAD_NAME;
		g_RES_select_lang = BAD_LANG;
	}

	PostUpdateArrow(hwnd);
	return 0;
}

void MMainWnd::RefreshCode()
{
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	Preview(m_hwnd, entry, STV_RESETTEXT, FALSE);
}

// destroy the RADical window if any
void MMainWnd::DestroyRadWindow()
{
	if (::IsWindow(m_rad_window))
		::DestroyWindow(m_rad_window);
}

void MMainWnd::RefreshRadBackBrush()
{
	m_rad_window.m_rad_dialog.DeleteBackBrush();
	if (IsWindow(m_rad_window.m_rad_dialog))
		InvalidateRect(m_rad_window.m_rad_dialog, nullptr, TRUE);
}

BOOL MMainWnd::DoQuerySaveChange(HWND hwnd)
{
	if (!s_bModified)
		return TRUE;

	INT id = MsgBoxDx(IDS_QUERYSAVECHANGE, MB_ICONINFORMATION | MB_YESNOCANCEL);
	if (id == IDCANCEL)
		return FALSE;

	if (id == IDYES)
		return OnSave(hwnd);

	return TRUE;
}

// update the fonts by the font settings
void MMainWnd::ReCreateFonts(HWND hwnd)
{
	// delete the fonts
	if (m_hBinFont)
	{
		DeleteObject(m_hBinFont);
		m_hBinFont = NULL;
	}
	if (m_hSrcFont)
	{
		DeleteObject(m_hSrcFont);
		m_hSrcFont = NULL;
	}

	m_hBinFont = CreateFontFromNameAndSize(g_settings.strBinFont.c_str(), g_settings.nBinFontSize);
	assert(m_hBinFont);
	m_hSrcFont = CreateFontFromNameAndSize(g_settings.strSrcFont.c_str(), g_settings.nSrcFontSize);
	assert(m_hSrcFont);

	// set the fonts to the controls
	SetWindowFont(m_hHexViewer, m_hBinFont, TRUE);
	SetWindowFont(m_hCodeEditor, m_hSrcFont, TRUE);
}

// check the text for item search
static bool CheckTextForSearch(ITEM_SEARCH *pSearch, EntryBase *entry, MString text)
{
	// make the text uppercase to ignore case
	MString needle = pSearch->strText;
	if (pSearch->bIgnoreCases)
	{
		mstr_upper(text);
		mstr_upper(needle);
	}

	// find?
	if (text.find(needle) == MString::npos)
		return false;   // not found

	if (pSearch->bDownward)     // go downward
	{
		// check the position
		if (pSearch->pCurrent == nullptr || *pSearch->pCurrent < *entry)
		{
			if (pSearch->pFound)    // already found
			{
				if (*entry < *pSearch->pFound)  // compare with the found one
				{
					pSearch->pFound = g_res.get_shared(entry);
					return true;    // found
				}
			}
			else    // not found yet
			{
				pSearch->pFound = g_res.get_shared(entry);    // found
				return true;    // found
			}
		}
	}
	else    // go upward
	{
		// check the position
		if (pSearch->pCurrent == nullptr || *entry < *pSearch->pCurrent)
		{
			if (pSearch->pFound)    // already found
			{
				if (*pSearch->pFound < *entry)  // compare with the found one
				{
					pSearch->pFound = g_res.get_shared(entry);
					return true;    // found
				}
			}
			else
			{
				pSearch->pFound = g_res.get_shared(entry);
				return true;    // found
			}
		}
	}

	return false;    // not found
}

// a function for item search
static unsigned __stdcall
search_proc(void *arg)
{
	auto pSearch = (ITEM_SEARCH *)arg;
	MString text;

	BOOL bFound = FALSE;
	for (auto entry : g_res)
	{
		if (!entry->valid())
			continue;

		if (pSearch->bCancelled)
			break;

		EntryBase e = *entry;

		// check label
		text = e.m_strLabel;
		if (CheckTextForSearch(pSearch, entry, text))
		{
			bFound = TRUE;
			::PostMessage(g_hMainWnd, MYWM_ITEMSEARCH, IDYES, (WPARAM)pSearch);
			//MessageBoxW(NULL, e.m_strLabel.c_str(), L"OK", 0);
			break;
		}

		// check internal text
		switch (e.m_et)
		{
		case ET_LANG:
			break;
		case ET_STRING:
			// ignore the name
			e.m_name.clear();
			break;
		default:
			continue;
		}
		text = pSearch->res2text.DumpEntry(e);
		if (CheckTextForSearch(pSearch, entry, text))
		{
			// found
			bFound = TRUE;
			::PostMessage(g_hMainWnd, MYWM_ITEMSEARCH, IDYES, (WPARAM)pSearch);
			//MessageBoxW(NULL, (e.m_strLabel + L"<>" + text).c_str(), NULL, 0);
			break;
		}
	}

	if (!bFound)
		::PostMessage(g_hMainWnd, MYWM_ITEMSEARCH, IDNO, (WPARAM)pSearch);

	pSearch->bRunning = FALSE;  // finish
	return 0;
}

// do item search
BOOL MMainWnd::DoItemSearch(ITEM_SEARCH& search)
{
	MWaitCursor wait;

	if (search.bIgnoreCases)
		mstr_upper(search.strText);

	search_proc(&search);

	return search.pFound != nullptr;
}

// MYWM_ITEMSEARCH: do item search
LRESULT MMainWnd::OnItemSearchBang(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UINT nID = (UINT)wParam;
	switch (nID)
	{
	case IDOK: // Start search
		DoItemSearchBang(hwnd, (MItemSearchDlg *)lParam);
		break;
	case IDCANCEL: // Canceled
	case IDYES: // Found
	case IDNO: // Not found
		DoEnableControls(TRUE);
		break;
	default:
		assert(0);
		break;
	}
	return 0;
}

// MYWM_COMPLEMENT
LRESULT MMainWnd::OnComplement(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	INT index = (INT)wParam;

	switch (m_arrow.m_target_type)
	{
	case TARGET_TYPE_TYPE:
		if (g_types.size())
		{
			if (index >= (INT)g_types.size())
				return FALSE;   // reject

			MString str = g_types[index];
			if (str.size() && str[0] == '"')
				mstr_unquote(str);

			MIdOrString new_type = str.c_str();

			auto entry = g_res.get_entry();
			if (!entry || entry->m_et != ET_TYPE)
				return FALSE;   // reject

			INT value1 = g_db.GetValue(L"RESOURCE", new_type.str());
			if (value1)
				new_type = (WORD)value1;

			auto old_type = entry->m_type;
			if (old_type == BAD_TYPE || old_type.str() == new_type.str())
				return FALSE;   // reject

			// check if it already exists
			if (g_res.find(ET_LANG, new_type))
			{
				ErrorBoxDx(IDS_ALREADYEXISTS);
				return FALSE;   // reject
			}

			PostUpdateArrow(hwnd);

			WCHAR szText[MAX_PATH];
			StringCchCopyW(szText, _countof(szText), new_type.c_str());

			if (!DoRetypeEntry(szText, entry, old_type, new_type))
				return FALSE; // reject

			DoSetFileModified(TRUE);
		}
		return TRUE; // accepted

	case TARGET_TYPE_NAME:
		if (g_names.size())
		{
			if (index >= (INT)g_names.size())
				return FALSE;   // reject

			MIdOrString new_name = g_names[index].c_str();

			auto entry = g_res.get_entry();
			if (!entry || entry->m_et != ET_NAME)
				return FALSE;   // reject

			// A resource ID?
			if (g_db.HasResID(new_name.str()))
				new_name = (WORD)g_db.GetResIDValue(new_name.str());

			auto old_name = entry->m_name;
			if (old_name == BAD_NAME || old_name.str() == new_name.str())
				return FALSE;   // reject

			// check if it already exists
			if (g_res.find(ET_LANG, entry->m_type, new_name, entry->m_lang))
			{
				ErrorBoxDx(IDS_ALREADYEXISTS);
				return FALSE;   // reject
			}

			PostUpdateArrow(hwnd);

			WCHAR szText[MAX_PATH];
			StringCchCopyW(szText, _countof(szText), new_name.c_str());

			DoRenameEntry(szText, entry, old_name, new_name);
			DoSetFileModified(TRUE);
		}
		return TRUE; // accepted

	case TARGET_TYPE_LANG:
		{
			if (index >= (INT)g_langs.size())
				return FALSE; // reject
			LANGID wNewLang = g_langs[index].LangID;

			auto entry = g_res.get_entry();
			if (!entry || (entry->m_et != ET_LANG && entry->m_et != ET_STRING))
				return FALSE;   // reject

			LANGID wOldLang = entry->m_lang;
			if (wNewLang == BAD_LANG || wOldLang == wNewLang)
				return FALSE;   // reject

			// check if it already exists
			if (g_res.find(ET_LANG, entry->m_type, entry->m_name, wNewLang))
			{
				ErrorBoxDx(IDS_ALREADYEXISTS);
				return FALSE;   // reject
			}

			PostUpdateArrow(hwnd);

			WCHAR szText[MAX_PATH];
			MStringW strLang = get_lang_label(wNewLang);
			StringCbCopyW(szText, sizeof(szText), strLang.c_str());
			DoRelangEntry(szText, entry, wOldLang, wNewLang);
			DoSetFileModified(TRUE);
		}
		return TRUE; // accepted
	}

	return FALSE;
}

BOOL MMainWnd::DoInnerSearch(HWND hwnd)
{
	DWORD ich, ichEnd;
	SendMessageW(m_hCodeEditor, EM_GETSEL, (WPARAM)&ich, (LPARAM)&ichEnd);

	MString strText = MWindowBase::GetWindowText(m_hCodeEditor);

	MString strTarget = m_search.strText;
	if (m_search.bIgnoreCases)
	{
		mstr_upper(strText);
		mstr_upper(strTarget);
	}

	size_t index = MString::npos;
	if (m_search.bDownward)
	{
		if (ich == ichEnd)
			index = strText.find(strTarget);
		else if (ich + 1 < strText.size())
			index = strText.find(strTarget, ich + 1);
	}
	else
	{
		if (ich == ichEnd)
			index = strText.rfind(strTarget);
		else if (ich > 0)
			index = strText.rfind(strTarget, ich - 1);
	}

	if (index != MString::npos)
	{
		SendMessageW(m_hCodeEditor, EM_SETSEL, INT(index), INT(index + strTarget.size()));
		SendMessageW(m_hCodeEditor, EM_SCROLLCARET, 0, 0);
		return TRUE;
	}

	return FALSE;
}

struct SEARCH_THREAD_PARAM
{
	MMainWnd* pThis;
	HWND hwnd;
	MItemSearchDlg* pDialog;
};

unsigned __stdcall search_worker_thread(void* pv)
{
	std::unique_ptr<SEARCH_THREAD_PARAM> param(static_cast<SEARCH_THREAD_PARAM*>(pv));
	param->pThis->search_worker_thread_outer(param->hwnd, param->pDialog);
	return 0;
}

void MMainWnd::DoEnableControls(BOOL bEnable)
{
	::EnableWindow(m_hwnd, bEnable);
	::EnableWindow(m_hCodeEditor, bEnable);
	::EnableWindow(m_hBmpView, bEnable);
	::EnableWindow(m_hHexViewer, bEnable);
	::EnableWindow(m_hToolBar, bEnable);
	::EnableWindow(m_id_list_dlg, bEnable);
	::EnableWindow(m_hwndTV, bEnable);
}

void MMainWnd::search_worker_thread_outer(HWND hwnd, MItemSearchDlg* pDialog)
{
	DoEnableControls(FALSE);

	search_worker_thread_inner(hwnd, pDialog);

	DoEnableControls(TRUE);
}

void MMainWnd::search_worker_thread_inner(HWND hwnd, MItemSearchDlg* pDialog)
{
	// get the selected entry
	auto entry = g_res.get_entry();

	if (m_search.bIgnoreCases)
		mstr_upper(m_search.strText);

	// initialize
	m_search.bCancelled = FALSE;
	m_search.pFound = nullptr;
	m_search.pCurrent = g_res.get_shared(entry);

	// start searching
	if (DoInnerSearch(hwnd))
	{
		m_search.bRunning = FALSE;

		if (pDialog)
			pDialog->Done();    // uninitialize

		return;
	}

	if (DoItemSearch(m_search) && m_search.pFound)
	{
		m_search.bRunning = FALSE;

		if (pDialog)
			pDialog->Done();    // uninitialize

		// select the found one
		TreeView_SelectItem(m_hwndTV, m_search.pFound->m_hItem);
		TreeView_EnsureVisible(m_hwndTV, m_search.pFound->m_hItem);

		DoInnerSearch(hwnd);
		return;
	}

	m_search.bRunning = FALSE;

	if (pDialog)
		pDialog->Done();    // uninitialize

	// is it not cancelled?
	if (!m_search.bCancelled)
	{
		// "no more item" message
		if (pDialog)
			EnableWindow(*pDialog, FALSE);
		MsgBoxDx(IDS_NOMOREITEM, MB_ICONINFORMATION);
		if (pDialog)
			EnableWindow(*pDialog, TRUE);
	}
}

BOOL MMainWnd::DoItemSearchBang(HWND hwnd, MItemSearchDlg *pDialog)
{
	// is it visible?
	if (pDialog == NULL || !IsWindowVisible(pDialog->m_hwnd))
	{
		pDialog = NULL;
	}

	auto param = std::make_unique<SEARCH_THREAD_PARAM>();
	param->pThis = this;
	param->hwnd = hwnd;
	param->pDialog = pDialog;

	uintptr_t hThread = _beginthreadex(nullptr, 0, ::search_worker_thread, param.get(), 0, nullptr);
	if (hThread)
	{
		param.release(); // ownership passed to the thread
		CloseHandle(reinterpret_cast<HANDLE>(hThread));
	}

	return FALSE;
}

void MMainWnd::OnSelChange(HWND hwnd, INT iSelected)
{
	// update show
	switch (iSelected)
	{
	case 0:
		SetShowMode(m_nShowMode, FALSE);
		break;
	case 1:
		SetShowMode(m_nShowMode, TRUE);
		break;
	}
}

// set error message
void MMainWnd::SetErrorMessage(const MStringA& strOutput)
{
	// show the message box
	if (strOutput.empty())
	{
		MsgBoxDx(LoadStringDx(IDS_COMPILEERROR), MB_ICONERROR);
	}
	else
	{
		MAnsiToWide wide(CP_ACP, strOutput);
		MsgBoxDx(wide.c_str(), MB_ICONERROR);
	}
}

bool MMainWnd::IsEntryTextEditable(const EntryBase *entry)
{
	if (!entry)
		return false;

	if (entry->is_editable(m_szVCBat))
		return true;

	auto enc = GetResTypeEncoding(entry->m_type);
	if (enc.size() && enc != L"bin")
		return true;

	return false;
}

// do UPX test to check whether the file is compressed or not
BOOL MMainWnd::DoUpxTest(LPCWSTR pszUpx, LPCWSTR pszFile)
{
	// build the command line text
	MStringW strCmdLine;
	strCmdLine += L"\"";
	strCmdLine += pszUpx;
	strCmdLine += L"\" -t \"";
	strCmdLine += pszFile;
	strCmdLine += L"\"";
	//MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

	BOOL bOK = FALSE;

	// create an upx.exe process
	MProcessMaker pmaker;
	pmaker.SetCreationFlags(CREATE_NO_WINDOW);

	MFile hInputWrite, hOutputRead;
	if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
		pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
	{
		// read all with timeout
		MStringA strOutput;
		pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
		pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

		if (pmaker.GetExitCode() == 0)
		{
			if (strOutput.find("[OK]") != MStringA::npos)
			{
				// success
				bOK = TRUE;
			}
		}
	}

	return bOK;
}

// do UPX extract to decompress the file
BOOL MMainWnd::DoUpxDecompress(LPCWSTR pszUpx, LPCWSTR pszFile)
{
	// build the command line text
	MStringW strCmdLine;
	strCmdLine += L"\"";
	strCmdLine += pszUpx;
	strCmdLine += L"\" -d \"";
	strCmdLine += pszFile;
	strCmdLine += L"\"";
	//MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

	BOOL bOK = FALSE;

	// create the upx.exe process
	MProcessMaker pmaker;
	pmaker.SetCreationFlags(CREATE_NO_WINDOW);

	MFile hInputWrite, hOutputRead;
	if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
		pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
	{
		// read all with timeout
		MStringA strOutput;
		bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
		pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

		if (pmaker.GetExitCode() == 0 && bOK)
		{
			MTRACEA("%s\n", strOutput.c_str());
			bOK = (strOutput.find("Unpacked") != MStringA::npos);
		}
	}

	return bOK;
}

void MMainWnd::UpdateHexViewerContent()
{
	if (m_strHexCache.empty())
	{
		auto entry = g_res.get_entry();
		if (entry && !entry->m_data.empty())
		{
			m_strHexCache = DumpBinaryAsText(entry->m_data);
		}
	}
	SetWindowTextW(m_hHexViewer, m_strHexCache.c_str());
}

void MMainWnd::SetShowMode(SHOW_MODE mode, BOOL bShowBinary)
{
	if (m_bShowBinEdit == bShowBinary && m_nShowMode == mode)
		return;
	m_bShowBinEdit = bShowBinary;
	m_nShowMode = mode;
	if (m_bShowBinEdit)
	{
		if (m_tab.GetCurSel() != 1)
			m_tab.SetCurSel(1);
		ShowWindow(m_hCodeEditor, SW_HIDE);
		ShowWindow(m_hBmpView, SW_HIDE);
		ShowWindow(m_hHexViewer, SW_SHOWNOACTIVATE);
		m_splitter2.SetPaneCount(1);
		m_splitter2.SetPane(0, m_hHexViewer);

		UpdateHexViewerContent();
	}
	else
	{
		if (m_tab.GetCurSel() != 0)
			m_tab.SetCurSel(0);
		switch (mode)
		{
		case SHOW_MOVIE:
			ShowWindow(m_hCodeEditor, SW_HIDE);
			ShowWindow(m_hBmpView, SW_SHOWNOACTIVATE);
			ShowWindow(m_hHexViewer, SW_HIDE);
			m_splitter2.SetPaneCount(1);
			m_splitter2.SetPane(0, m_hBmpView);
			break;
		case SHOW_CODEONLY:
			ShowWindow(m_hCodeEditor, SW_SHOWNOACTIVATE);
			ShowWindow(m_hBmpView, SW_HIDE);
			ShowWindow(m_hHexViewer, SW_HIDE);
			m_splitter2.SetPaneCount(1);
			m_splitter2.SetPane(0, m_hCodeEditor);
			break;
		case SHOW_CODEANDBMP:
			ShowWindow(m_hCodeEditor, SW_SHOWNOACTIVATE);
			ShowWindow(m_hBmpView, SW_SHOWNOACTIVATE);
			ShowWindow(m_hHexViewer, SW_HIDE);
			m_splitter2.SetPaneCount(2);
			m_splitter2.SetPane(0, m_hCodeEditor);
			m_splitter2.SetPane(1, m_hBmpView);
			m_splitter2.SetPaneExtent(1, g_settings.nBmpViewWidth);
			break;
		}
	}

	// We got past the early-out above, so the layout genuinely changed
	// (ShowWindow/SetPane were just called on the splitter children).
	// While Preview() is batching UI updates, just remember that a
	// relayout is needed; EndPreviewBatch() posts a single WM_SIZE for
	// the whole batch once it's done, and only because this flag is set
	// (see MMainWnd.hpp / MMainWnd_Preview.cpp).
	if (m_nPreviewBatchDepth)
		m_bPreviewLayoutDirty = TRUE;
	else
		PostMessage(m_hwnd, WM_SIZE, 0, 0);
}

// show the status bar or not
void MMainWnd::ShowStatusBar(BOOL bShow/* = TRUE*/)
{
	if (bShow)
		ShowWindow(m_hStatusBar, SW_SHOWNOACTIVATE);
	else
		ShowWindow(m_hStatusBar, SW_HIDE);
}

// WM_MOVE: the main window has moved
void MMainWnd::OnMove(HWND hwnd, int x, int y)
{
	// is the window maximized or minimized?
	if (!IsZoomed(hwnd) && !IsIconic(hwnd))
	{
		// if so, then remember the position
		RECT rc;
		GetWindowRect(hwnd, &rc);
		g_settings.nWindowLeft = rc.left;
		g_settings.nWindowTop = rc.top;
	}
}

// WM_SIZE: the main window has changed in size
void MMainWnd::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	// auto move and resize the toolbar
	SendMessageW(m_hToolBar, TB_AUTOSIZE, 0, 0);

	// auto move and resize the status bar
	SendMessageW(m_hStatusBar, WM_SIZE, 0, 0);

	RECT rc, ClientRect;

	// is the main window maximized?
	if (IsZoomed(hwnd))
	{
		g_settings.bMaximized = TRUE;
	}
	else if (IsIconic(hwnd))   // is it minimized?
	{
		;
	}
	else
	{
		// not maximized nor minimized
		// remember the size
		GetWindowRect(hwnd, &rc);
		g_settings.nWindowWidth = rc.right - rc.left;
		g_settings.nWindowHeight = rc.bottom - rc.top;
		g_settings.bMaximized = FALSE;
	}

	// get the client rectangle from the main window
	GetClientRect(hwnd, &ClientRect);
	SIZE sizClient = SizeFromRectDx(&ClientRect);

	// currently, the upper left corner of the client rectangle is (0, 0)
	INT x = 0, y = 0;

	// reserve the toolbar space
	if (::IsWindowVisible(m_hToolBar))
	{
		GetWindowRect(m_hToolBar, &rc);
		y += rc.bottom - rc.top;
		sizClient.cy -= rc.bottom - rc.top;
	}

	// reserve the status bar space
	if (::IsWindowVisible(m_hStatusBar))
	{
		INT anWidths[] = { ClientRect.right - CX_STATUS_PART, -1 };
		SendMessage(m_hStatusBar, SB_SETPARTS, 2, (LPARAM)anWidths);
		GetWindowRect(m_hStatusBar, &rc);
		sizClient.cy -= rc.bottom - rc.top;
	}

	// notify the size change to m_splitter1
	MoveWindow(m_splitter1, x, y, sizClient.cx, sizClient.cy, TRUE);

	// resize m_splitter2
	GetClientRect(m_tab, &rc);
	m_tab.AdjustRect(FALSE, &rc);
	MapWindowRect(m_tab, GetParent(m_splitter2), &rc);
	SIZE siz = SizeFromRectDx(&rc);
	MoveWindow(m_splitter2, rc.left, rc.top, siz.cx, siz.cy, TRUE);
}

// WM_CONTEXTMENU: the context menu is shown
void MMainWnd::OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
	if (hwndContext != m_hwndTV)
		return;     // ignore

	// destroy the RADical window if any
	DestroyRadWindow();

	// get screen coordinates from xPos and yPos
	POINT pt = {(SHORT)xPos, (SHORT)yPos};
	HTREEITEM hItem;
	if (pt.x == -1 && pt.y == -1)
	{
		// context menu from keyboard
		hItem = TreeView_GetSelection(hwndContext);

		RECT rc;
		if (hItem)
		{
			TreeView_GetItemRect(hwndContext, hItem, &rc, FALSE);
			pt.x = (rc.left + rc.right) / 2;
			pt.y = (rc.top + rc.bottom) / 2;
		}
		else
		{
			GetWindowRect(hwndContext, &rc);
			pt.x = rc.left;
			pt.y = rc.top;

			ScreenToClient(hwndContext, &pt);
		}
	}
	else
	{
		// context menu from mouse
		ScreenToClient(hwndContext, &pt);

		// hit test
		TV_HITTESTINFO HitTest;
		ZeroMemory(&HitTest, sizeof(HitTest));
		HitTest.pt = pt;
		TreeView_HitTest(hwndContext, &HitTest);
		hItem = HitTest.hItem;
	}

	// select the item
	TreeView_SelectItem(hwndContext, hItem);

	// load the menu
	HMENU hMenu = LoadMenuW(m_hInst, MAKEINTRESOURCEW(IDR_MAINMENU));
	OnInitMenu(hwnd, hMenu);
	HMENU hSubMenu = ::GetSubMenu(hMenu, 1);
	if (hMenu == NULL || hSubMenu == NULL)
		return;

#ifdef _MSC_VER
	UINT iItem = GetMenuPosFromID(hSubMenu, ID_DELETERES);
#else
	if (!g_fnGetMenuPosFromID)
	{
		HINSTANCE hSHLWAPI = GetModuleHandleA("shlwapi");
		FARPROC fn = GetProcAddress(hSHLWAPI, "GetMenuPosFromID");
		CopyMemory(&g_fnGetMenuPosFromID, &fn, sizeof(fn));
		assert(g_fnGetMenuPosFromID);
	}
	UINT iItem = g_fnGetMenuPosFromID(hSubMenu, ID_DELETERES);
#endif
	while (DeleteMenu(hSubMenu, iItem + 1, MF_BYPOSITION))
		;

	// convert the client coordinates to the screen coordinates
	ClientToScreen(hwndContext, &pt);

	// See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648002.aspx
	SetForegroundWindow(hwndContext);

	UINT Flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD;
	INT id = TrackPopupMenu(hSubMenu, Flags, pt.x, pt.y, 0, hwndContext, NULL);

	// See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648002.aspx
	PostMessageW(hwndContext, WM_NULL, 0, 0);

	// destroy the menu
	DestroyMenu(hMenu);

	if (id)
	{
		// execute the command
		SendMessageW(hwnd, WM_COMMAND, id, 0);
	}
}

// select an item in the tree control
void
MMainWnd::SelectTV(EntryType et, const MIdOrString& type,
				   const MIdOrString& name, LANGID lang,
				   BOOL bDoubleClick, STV stv)
{
	// find the entry
	if (auto entry = g_res.find(et, type, name, lang))
	{
		// select it
		SelectTV(entry, bDoubleClick, stv);
	}
	else
	{
		BeginPreviewBatch();
		HidePreview(stv);
		EndPreviewBatch();
		UpdateOurToolBarButtons(3);
	}
}

// select an item in the tree control
void MMainWnd::SelectTV(EntryBase *entry, BOOL bDoubleClick, STV stv)
{
	BOOL bModified = Edit_GetModify(m_hCodeEditor);

	// HidePreview() clears m_hCodeEditor, then Preview()/PreviewStringTable()
	// rewrite it. Without batching those steps together, a tree double-click
	// (OnEdit → SelectTV) flashes an empty editor between the clear and the
	// rewrite — the same class of flicker PreviewBatch was meant to stop.
	BeginPreviewBatch();

	// close the preview. If entry is non-NULL, one of the branches below
	// (Preview(), PreviewStringTable(), or the explicit SetShowMode() in
	// the default case) is about to establish the real target mode itself,
	// so tell HidePreview() to skip its own SHOW_CODEONLY forcing -- see
	// the bWillRePreview comment on HidePreview()'s declaration.
	HidePreview(stv, entry != NULL);

	if (!entry)     // not selected
	{
		EndPreviewBatch();
		UpdateOurToolBarButtons(3);
		return;
	}

	if (stv != STV_DONTRESET)
	{
		// expand the parent and ensure visible
		auto parent = g_res.get_parent(entry);
		while (parent && parent->m_hItem)
		{
			TreeView_Expand(m_hwndTV, parent->m_hItem, TVE_EXPAND);
			parent = g_res.get_parent(parent);
		}
		TreeView_SelectItem(m_hwndTV, entry->m_hItem);
		TreeView_EnsureVisible(m_hwndTV, entry->m_hItem);
	}

	m_type = entry->m_type;
	m_name = entry->m_name;
	m_lang = entry->m_lang;

	BOOL bEditable;
	switch (entry->m_et)
	{
	case ET_LANG:
		// do preview
		bEditable = Preview(m_hwnd, entry, stv);
		break;

	case ET_STRING:
		// clean up m_hBmpView
		m_hBmpView.DestroyView();

		bEditable = TRUE;
		if (stv != STV_DONTRESET)
		{
			// show the string table
			bEditable = PreviewStringTable(m_hwnd, *entry);
		}

		// hide the binary EDIT control
		SetWindowTextW(m_hHexViewer, NULL);
		break;

	default:
		// otherwise
		// clean up m_hBmpView
		m_hBmpView.DestroyView();

		// hide the binary EDIT control
		SetWindowTextW(m_hHexViewer, NULL);

		// it's non editable
		bEditable = FALSE;

		// update show mode
		SetShowMode(SHOW_CODEONLY, m_bShowBinEdit);
	}

	if (stv == STV_DONTRESET || stv == STV_RESETTEXT)
	{
		// restore the modified flag
		Edit_SetModify(m_hCodeEditor, bModified);
	}

	if (bEditable)  // editable
	{
		// make it not read-only
		Edit_SetReadOnly(m_hCodeEditor, FALSE);

		// update the toolbar
		if (Edit_GetModify(m_hCodeEditor))
		{
			UpdateOurToolBarButtons(2);
		}
		else if (entry->is_testable())
		{
			UpdateOurToolBarButtons(0);
		}
		else if (entry->can_gui_edit())
		{
			UpdateOurToolBarButtons(4);
		}
		else
		{
			UpdateOurToolBarButtons(3);
		}
	}
	else
	{
		// make it read-only
		Edit_SetReadOnly(m_hCodeEditor, TRUE);

		// update the toolbar
		UpdateOurToolBarButtons(3);
	}

	// update show
	SetShowMode(m_nShowMode, m_bShowBinEdit);

	// EndPreviewBatch posts a single WM_SIZE when the depth returns to zero;
	// do not post another one here or the splitter/layout will animate twice.
	EndPreviewBatch();
}

// dump all the macros
MStringW MMainWnd::GetMacroDump(BOOL bApStudio) const
{
	MStringW ret;
	for (auto& macro : g_settings.macros)   // for each predefined macros
	{
		// " -Dmacro=contents"
		ret += L" -D";
		ret += macro.first;
		if (macro.second.size())
		{
			ret += L"=";
			ret += macro.second;
		}
	}

	if (bApStudio)
		ret += L" -DAPSTUDIO_INVOKED=1";

	ret += L" ";
	return ret;
}

// dump all the #include's
MStringW MMainWnd::GetIncludesDump() const
{
	MStringW ret;
	for (auto& path : g_settings.includes)
	{
		// " -Ipath"
		auto& str = path;
		if (str.empty())
			continue;

		ret += L" \"-I";
		ret += str;
		ret += L"\"";
	}
	if (m_szIncludeDir[0])
	{
		ret += L" \"-I";
		ret += m_szIncludeDir;
		ret += L"\"";
	}
	ret += L" ";
	return ret;
}

// dump all the #include's
MStringW MMainWnd::GetIncludesDumpForWindres() const
{
	MStringW ret;
	for (auto& path : g_settings.includes)
	{
		// " -Ipath"
		auto& str = path;
		if (str.empty())
			continue;

		ret += L" -I \"";
		ret += str;
		ret += L"\"";
	}
	if (m_szIncludeDir[0])
	{
		ret += L" -I \"";
		ret += m_szIncludeDir;
		ret += L"\"";
	}
	ret += L" ";
	return ret;
}

BOOL MMainWnd::WritePayloadLoaderRCEx(PCWSTR szHeader, PCWSTR szPayload, PCWSTR szPayloadLoader, LANGID lang)
{
	if (!g_settings.IsIDMapEmpty() && DoWriteResH(szHeader))
		return WritePayloadLoaderRC(szHeader, szPayload, szPayloadLoader, lang);
	else if (m_szResourceH[0])
		return WritePayloadLoaderRC(m_szResourceH, szPayload, szPayloadLoader, lang);
	else
		return WritePayloadLoaderRC(szHeader, szPayload, szPayloadLoader, lang);
}

// ---------------------------------------------------------------------------
// Shared plumbing for CompileStringTable / CompileMessageTable / CompileParts.
// All three build a small throw-away .rc "payload loader" that #includes the
// text being compiled, run it through windres (via mcdx/mcpp as the
// preprocessor), and then import the resulting .res file. Only the exact
// compiler executable/command-line flags and what to do with the imported
// resources differ between the three call sites, so that part is left in
// each function; only the identical setup/teardown is shared here.
// ---------------------------------------------------------------------------

// Owns the four temp files needed by a single compile attempt (header /
// imported-object / payload / payload-loader) and deletes whichever of them
// were actually created by GetTempFileNameDx, no matter how Prepare() exits.
// (Note: GetTempFileNameDx itself creates the file on disk, so even a
// failure partway through Prepare() can leave temp files behind if they
// aren't tracked here -- this fixes a couple of such leaks that existed in
// the original per-function copies.)
class MCompileTempFileSet
{
public:
	WCHAR szHeader[MAX_PATH];
	WCHAR szObject[MAX_PATH];
	WCHAR szPayload[MAX_PATH];
	WCHAR szPayloadLoader[MAX_PATH];

	MCompileTempFileSet()
	{
		szHeader[0] = szObject[0] = szPayload[0] = szPayloadLoader[0] = 0;
	}

	~MCompileTempFileSet()
	{
		if (szPayloadLoader[0]) DeleteFileW(szPayloadLoader);
		if (szPayload[0])       DeleteFileW(szPayload);
		if (szObject[0])        DeleteFileW(szObject);
		if (szHeader[0])        DeleteFileW(szHeader);
	}

	MCompileTempFileSet(const MCompileTempFileSet&) = delete;
	MCompileTempFileSet& operator=(const MCompileTempFileSet&) = delete;

	// Creates the header/object placeholder files, writes strWide (as UTF-8)
	// to the payload file, and builds the payload-loader .rc file that
	// #includes it. On failure, strOutput gets a user-facing message and
	// FALSE is returned; anything already created is still cleaned up by
	// the destructor.
	BOOL Prepare(MMainWnd *pMainWnd, LANGID lang, const MStringW& strWide, MStringA& strOutput)
	{
		StringCchCopyW(szHeader, _countof(szHeader), GetTempFileNameDx(L"R2"));
		if (!CreateEmptyFile(szHeader))
		{
			strOutput = GetCannotCreateTempFile();
			return FALSE;
		}

		StringCchCopyW(szObject, _countof(szObject), GetTempFileNameDx(L"R3"));
		if (!CreateEmptyFile(szObject))
		{
			strOutput = GetCannotCreateTempFile();
			return FALSE;
		}

		StringCchCopyW(szPayload, _countof(szPayload), GetTempFileNameDx(L"R4"));
		{
			MStringA str = MWideToAnsi(CP_UTF8, strWide.c_str()).c_str();
			FILE *fout = _wfopen(szPayload, L"wb");
			if (!fout)
			{
				strOutput = GetCannotCreateTempFile();
				return FALSE;
			}
			BOOL ok = !!fwrite(str.c_str(), str.size(), 1, fout);
			fclose(fout);
			if (!ok)
			{
				strOutput = GetCannotCreateTempFile();
				return FALSE;
			}
		}

		StringCchCopyW(szPayloadLoader, _countof(szPayloadLoader), GetTempFileNameDx(L"R1"));
		if (!pMainWnd->WritePayloadLoaderRCEx(szHeader, szPayload, szPayloadLoader, lang))
		{
			strOutput = GetCannotCreateTempFile();
			return FALSE;
		}

		return TRUE;
	}
};

// Starts strCmdLine (a windres/mcdx invocation) with its stdout/stderr
// captured into strOutput.
//   - Returns FALSE if the process could not even be started (strOutput is
//     left untouched; the caller should report IDS_CANNOTSTARTUP).
//   - Returns TRUE if it started; bSucceeded then tells the caller whether
//     it also exited with code 0 and its output was read successfully.
static BOOL RunCompilerProcess(MProcessMaker& pmaker, const MStringW& strCmdLine,
								MStringA& strOutput, BOOL& bSucceeded)
{
	pmaker.SetCreationFlags(CREATE_NO_WINDOW);
	bSucceeded = FALSE;

	MFile hInputWrite, hOutputRead;
	if (!pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) ||
		!pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
	{
		return FALSE;
	}

	BOOL bRead = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
	pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

	bSucceeded = bRead && (pmaker.GetExitCode() == 0);
	return TRUE;
}

// Finds "marker" in strOutput, scans backward from it to the nearest
// "boundary" character, and jumps the code editor to the line number found
// there. Used to locate windres/mcdx syntax errors. No-op if the marker
// text isn't present.
void MMainWnd::MarkErrorLineFromOutput(const MStringA& strOutput, const char *marker, char boundary)
{
	size_t ich = strOutput.find(marker);
	if (ich == strOutput.npos)
		return;

	while (ich > 0 && strOutput[--ich] != boundary) {}

	MStringA str = strOutput.substr(ich + 1);
	MarkErrorLine(atoi(str.c_str()));
}

// compile the string table
BOOL MMainWnd::CompileStringTable(MStringA& strOutput, LANGID lang, const MStringW& strWide)
{
	MCompileTempFileSet temp;
	if (!temp.Prepare(this, lang, strWide, strOutput))
		return FALSE;

	// build the command line text
	MStringW strCmdLine;
	strCmdLine += L'\"';
	strCmdLine += m_szWindresExe;
	strCmdLine += L"\" -DRC_INVOKED ";
	strCmdLine += GetMacroDump();
	strCmdLine += GetIncludesDumpForWindres();
	strCmdLine += L" -o \"";
	strCmdLine += temp.szObject;
	strCmdLine += L"\" -J rc -O res -F pe-i386 \"--preprocessor=";
	strCmdLine += m_szMCppExe;
	strCmdLine += L"\" \"";
	strCmdLine += temp.szPayloadLoader;
	strCmdLine += '\"';
	//MessageBoxW(m_hwnd, strCmdLine.c_str(), NULL, 0);

	BOOL bOK = FALSE;

	// create a windres.exe process
	MProcessMaker pmaker;
	BOOL bSucceeded;
	if (RunCompilerProcess(pmaker, strCmdLine, strOutput, bSucceeded))
	{
		if (bSucceeded)
		{
			// import res
			EntrySet res;
			if (res.import_res(temp.szObject))
			{
				// resource type check
				bOK = TRUE;
				for (auto entry : res)
				{
					if (entry->m_type != RT_STRING)
					{
						ErrorBoxDx(IDS_RESMISMATCH);
						bOK = FALSE;
						break;
					}
				}

				if (bOK)
				{
					// merge
					g_res.search_and_delete(ET_LANG, RT_STRING, BAD_NAME, lang);
					g_res.merge(res);
				}
			}

			// clean res up
			res.delete_all();
		}
		else
		{
			// error message
			MarkErrorLineFromOutput(strOutput, ": syntax error", ':');
			strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_COMPILEERROR)).c_str() +
				MStringA("\r\n\r\n") + strOutput;
		}
	}
	else
	{
		// error message
		strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTSTARTUP));
	}

	// recalculate the splitter
	PostMessageDx(WM_SIZE);

	if (bOK)
		DoSetFileModified(TRUE);
	return bOK;
}

BOOL MMainWnd::CompileTYPELIB(MStringA& strOutput, const MIdOrString& name, LANGID lang, const MStringW& strWide)
{
	// convert strWide to ANSI
	MStringA ansi = MWideToAnsi(CP_ACP, strWide).c_str();

	bool is_64bit = (GetMachineOfBinary(m_szFile) == IMAGE_FILE_MACHINE_AMD64);

	auto binary = tlb_binary_from_text(m_szMidlWrap, m_szVCBat, strOutput, ansi, is_64bit);
	if (binary.empty())
	{
		// error message
		if (strOutput.empty())
			strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_COMPILEERROR));

		std::vector<MStringA> lines;
		mstr_split(lines, strOutput, "\n");
		std::vector<MStringA> new_lines;
		for (auto& line : lines)
		{
			if (line.find("error") == line.npos)
				continue;
			if (line.find('*') == 0 || line.find('[') == 0)
				continue;
			if (line.find("Processing ") == 0)
				continue;
			new_lines.push_back(line);
			if (new_lines.size() >= 10)
			{
				new_lines.push_back("...");
				break;
			}
		}
		strOutput = mstr_join(new_lines, "\n");
		return FALSE;
	}

	g_res.add_lang_entry(L"TYPELIB", name, lang, binary);
	DoSetFileModified(TRUE);
	return TRUE;
}

BOOL MMainWnd::CompileRCData(MStringA& strOutput, const MIdOrString& name, LANGID lang, const MStringW& strWide)
{
	EntryBase *entry = g_res.find(ET_LANG, RT_RCDATA, name, lang);
	if (!entry || !entry->is_delphi_dfm())
		return FALSE;

	MWideToAnsi w2a(CP_UTF8, strWide);
	MStringA ansi(w2a.c_str());

	// remove "//-" ... "-//"
	for (;;)
	{
		auto i = ansi.find("//-");
		if (i == MStringA::npos)
			break;
		auto j = ansi.find("-//", i);
		auto k = ansi.find('\n', i);
		if (j != MStringA::npos && k != MStringA::npos)
		{
			if (j < k)
				ansi.erase(i, j - i + 3);
			else
				ansi.erase(i, k - i);
		}
		else if (j != MStringA::npos)
		{
			ansi.erase(i, j - i + 3);
		}
		else if (k != MStringA::npos)
		{
			ansi.erase(i, k - i);
		}
		else
		{
			ansi.erase(0, i);
			break;
		}
	}

	INT iLine = 0;
	EntryBase::data_type data =
		dfm_binary_from_text(m_szDFMSC, ansi,
							 g_settings.nDfmCodePage, g_settings.bDfmNoUnicode, iLine);
	auto text = dfm_text_from_binary(m_szDFMSC, data.data(), data.size(),
									 g_settings.nDfmCodePage, g_settings.bDfmRawTextComments);
	if (text.empty())
	{
		INT iMaxLine = (INT)::SendMessageW(m_hCodeEditor, EM_GETLINECOUNT, 0, 0);
		if (iLine > iMaxLine)
			iLine = iMaxLine;
		if (iLine <= 0)
			iLine = 1;
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
		::SendMessageW(m_hCodeEditor, LNEM_SETLINEMARK, iLine, ERROR_LINE_COLOR);
		strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_COMPILEERROR)).c_str() +
			MStringA("\r\n\r\n") + strOutput;
		return FALSE;
	}

	entry->m_data = std::move(data);
	DoSetFileModified(TRUE);
	return TRUE;
}

// compile the message table
BOOL MMainWnd::CompileMessageTable(MStringA& strOutput, const MIdOrString& name, LANGID lang, const MStringW& strWide)
{
	MCompileTempFileSet temp;
	if (!temp.Prepare(this, lang, strWide, strOutput))
		return FALSE;

	// build the command line text
	MStringW strCmdLine;
	strCmdLine += L'\"';
	strCmdLine += m_szMcdxExe;
	strCmdLine += L"\" ";
	strCmdLine += GetMacroDump();
	strCmdLine += GetIncludesDump();
	strCmdLine += L" \"--preprocessor=";
	strCmdLine += m_szMCppExe;
	strCmdLine += L"\" -o \"";
	strCmdLine += temp.szObject;
	strCmdLine += L"\" -J rc -O res \"";
	strCmdLine += temp.szPayloadLoader;
	strCmdLine += L'\"';
	//MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

	BOOL bOK = FALSE;

	// create the mcdx.exe process
	MProcessMaker pmaker;
	BOOL bSucceeded;
	if (RunCompilerProcess(pmaker, strCmdLine, strOutput, bSucceeded))
	{
		if (bSucceeded)
		{
			// import res
			EntrySet res;
			if (res.import_res(temp.szObject))
			{
				// resource type check
				bOK = TRUE;
				for (auto entry : res)
				{
					if (entry->m_type != RT_MESSAGETABLE)
					{
						ErrorBoxDx(IDS_RESMISMATCH);
						bOK = FALSE;
						break;
					}
					entry->m_name = name;
				}

				if (bOK)
				{
					// merge
					g_res.search_and_delete(ET_LANG, RT_MESSAGETABLE, name, lang);
					g_res.merge(res);
				}
			}

			// clean res up
			res.delete_all();
		}
		else
		{
			// error message
			MarkErrorLineFromOutput(strOutput, "): ERROR: Syntax error", '(');
			strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_COMPILEERROR)).c_str() +
				MStringA("\r\n\r\n") + strOutput;
		}
	}
	else
	{
		// error message
		strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTSTARTUP));
	}

	// recalculate the splitter
	PostMessageDx(WM_SIZE);

	if (bOK)
		DoSetFileModified(TRUE);

	return bOK;
}

void MMainWnd::MarkErrorLine(INT iLine)
{
	INT iMaxLine = (INT)::SendMessageW(m_hCodeEditor, EM_GETLINECOUNT, 0, 0);
	if (iLine > iMaxLine)
		iLine = iMaxLine;
	if (iLine <= 0)
		iLine = 1;
	::SendMessageW(m_hCodeEditor, WM_SETREDRAW, FALSE, 0);
	::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
	::SendMessageW(m_hCodeEditor, LNEM_SETLINEMARK, iLine, ERROR_LINE_COLOR);
	LRESULT ichLine = ::SendMessageW(m_hCodeEditor, EM_LINEINDEX, iLine - 1, 0);
	::SendMessageW(m_hCodeEditor, EM_SETSEL, ichLine, ichLine);
	::SendMessageW(m_hCodeEditor, EM_SCROLLCARET, 0, 0);
	::SendMessageW(m_hCodeEditor, WM_SETREDRAW, TRUE, 0);
	::InvalidateRect(m_hCodeEditor, nullptr, FALSE);
}

// compile a resource item source
BOOL MMainWnd::CompileParts(MStringA& strOutput, const MIdOrString& type, const MIdOrString& name,
							LANGID lang, const MStringW& strWide, BOOL bReopen)
{
	if (type == RT_STRING)
	{
		return CompileStringTable(strOutput, lang, strWide);
	}
	if (type == RT_MESSAGETABLE)
	{
		return CompileMessageTable(strOutput, name, lang, strWide);
	}
	if (type == RT_RCDATA)
	{
		return CompileRCData(strOutput, name, lang, strWide);
	}
	if (type == L"TYPELIB")
	{
		return CompileTYPELIB(strOutput, name, lang, strWide);
	}

	// add a UTF-8 BOM to data
	static const BYTE bom[] = {0xEF, 0xBB, 0xBF, 0};

	// strWide --> strUtf8 (in UTF-8)
	MStringA strUtf8 = MWideToAnsi(CP_UTF8, strWide).c_str();
	auto enc = GetResTypeEncoding(type);
	BOOL bDataOK = FALSE;
	EntryBase::data_type data;
	if (enc.size())
	{
		bDataOK = TRUE;

		if (enc == L"utf8" || enc == L"utf8n")
		{
			data.assign(strUtf8.begin(), strUtf8.end());

			if (enc != L"utf8n")
			{
				// add a UTF-8 BOM to data
				data.insert(data.begin(), &bom[0], &bom[3]);
			}
		}
		else if (enc == L"ansi")
		{
			// strWide --> data (in ANSI)
			MStringA TextAnsi = MWideToAnsi(CP_ACP, strWide).c_str();
			data.assign(TextAnsi.begin(), TextAnsi.end());
		}
		else if (enc == L"wide")
		{
			data.assign((BYTE *)&strWide[0], (BYTE *)&strWide.c_str()[strWide.size()]);
		}
		else if (enc == L"sjis")
		{
			MStringA TextSjis = MWideToAnsi(932, strWide).c_str();
			data.assign(TextSjis.begin(), TextSjis.end());
		}
		else
		{
			bDataOK = FALSE;
		}
	}
	else if (type == RT_HTML || type == RT_MANIFEST ||
			 type == L"RISOHTEMPLATE")
	{
		data.assign(strUtf8.begin(), strUtf8.end());
		data.insert(data.begin(), &bom[0], &bom[3]);

		bDataOK = TRUE;
	}

	if (bDataOK)
	{
		if (data.empty())
		{
			ErrorBoxDx(IDS_DATAISEMPTY);
			return FALSE;
		}

		// add a language entry
		auto entry = g_res.add_lang_entry(type, name, lang, data);

		// select the added entry
		SelectTV(entry, FALSE);

		DoSetFileModified(TRUE);

		return TRUE;    // success
	}

	MCompileTempFileSet temp;
	if (!temp.Prepare(this, lang, strWide, strOutput))
		return FALSE;

	// build the command line text
	MStringW strCmdLine;
	strCmdLine += L'\"';
	strCmdLine += m_szWindresExe;
	strCmdLine += L"\" -DRC_INVOKED ";
	strCmdLine += GetMacroDump();
	strCmdLine += GetIncludesDumpForWindres();
	strCmdLine += L" -o \"";
	strCmdLine += temp.szObject;
	strCmdLine += L"\" -J rc -O res -F pe-i386 --preprocessor=\"";
	strCmdLine += m_szMCppExe;
	strCmdLine += L"\" \"";
	strCmdLine += temp.szPayloadLoader;
	strCmdLine += '\"';
	//MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

	BOOL bOK = FALSE;

	// create a windres.exe process
	MProcessMaker pmaker;
	BOOL bSucceeded;
	if (RunCompilerProcess(pmaker, strCmdLine, strOutput, bSucceeded))
	{
		if (bSucceeded)
		{
			// import res
			EntrySet res;
			if (res.import_res(temp.szObject))
			{
				bOK = TRUE;
				for (auto entry : res)
				{
					if (entry->m_et != ET_LANG)
						continue;

					// resource type check
					if (entry->m_type != type)
					{
						ErrorBoxDx(IDS_RESMISMATCH);
						bOK = FALSE;
						break;
					}

					// adjust names and languages
					entry->m_name = name;
					entry->m_lang = lang;

					if (entry->m_type == RT_TOOLBAR)
					{
						ToolbarRes toolbar_res;
						MByteStreamEx stream(entry->m_data);
						toolbar_res.LoadFromStream(stream);
						entry->m_data = toolbar_res.data();
					}
				}

				if (bOK)
				{
					// merge
					g_res.search_and_delete(ET_LANG, type, name, lang);
					g_res.merge(res);
				}

				// clean res up
				res.delete_all();
			}
		}
		else
		{
			// error message
			MarkErrorLineFromOutput(strOutput, ": syntax error", ':');
			strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_COMPILEERROR)).c_str() +
				MStringA("\r\n\r\n") + strOutput;
		}
	}
	else
	{
		// error message
		strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTSTARTUP));
	}

	if (bOK)
	{
		if (bReopen && type == RT_DIALOG)
		{
			PostMessageDx(MYWM_REOPENRAD);
		}

		DoSetFileModified(TRUE);
	}

	// recalculate the splitter
	PostMessageDx(WM_SIZE);

	return bOK;
}

// recompile the resource item on selection change.
// reopen if necessary
BOOL MMainWnd::ReCompileOnSelChange(BOOL bReopen/* = FALSE*/)
{
	MStringW strWide = MWindowBase::GetWindowTextW(m_hCodeEditor);

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return FALSE;   // no selection

	// compile the entry
	MStringA strOutput;
	if (!CompileParts(strOutput, entry->m_type, entry->m_name, entry->m_lang, strWide))
	{
		// compilation failed
		m_nStatusStringID = IDS_RECOMPILEFAILED;
		SetErrorMessage(strOutput);
		return FALSE;   // failure
	}

	// compilation OK
	m_nStatusStringID = IDS_RECOMPILEOK;

	// compiled. clear the modification flag
	Edit_SetModify(m_hCodeEditor, FALSE);

	// destroy the RADical window if any
	DestroyRadWindow();

	// reopen if necessary
	if (bReopen)
	{
		if (m_type == entry->m_type &&
			m_name == entry->m_name &&
			m_lang == entry->m_lang)
		{
			if (entry->m_et == ET_LANG && entry->m_type == RT_DIALOG)
			{
				MByteStreamEx stream(entry->m_data);
				m_rad_window.m_dialog_res.LoadFromStream(stream);
				m_rad_window.CreateDx(m_hwnd);
			}
		}
	}

	return TRUE;
}

// compile the source if necessary
BOOL MMainWnd::CompileIfNecessary(BOOL bReopen/* = FALSE*/)
{
	if (Edit_GetModify(m_hCodeEditor))
	{
		// the modification flag is set. query to compile
		INT id = MsgBoxDx(IDS_COMPILENOW, MB_ICONINFORMATION | MB_YESNOCANCEL);
		switch (id)
		{
		case IDYES:
			// ok, let's compile
			return ReCompileOnSelChange(bReopen);

		case IDNO:
			// clear the modification flag
			Edit_SetModify(m_hCodeEditor, FALSE);

			// destroy the RADical window if any
			DestroyRadWindow();
			break;

		case IDCANCEL:
			return FALSE;   // cancelled
		}
	}

	return TRUE;    // go
}

// check the data folder
BOOL MMainWnd::CheckDataFolder(VOID)
{
	// get the module path filename of this application module
	WCHAR szPath[MAX_PATH], *pch;
	GetModuleFileNameW(NULL, szPath, _countof(szPath));

	// find the last '\\' or '/'
	pch = wcsrchr(szPath, L'\\');
	if (pch == NULL)
		pch = wcsrchr(szPath, L'/');
	if (pch == NULL)
		return FALSE;

	// find the data folder
	size_t diff = pch - szPath;
	StringCchCopyW(pch, diff, L"\\data");
	if (!PathFileExistsW(szPath))
	{
		StringCchCopyW(pch, diff, L"\\..\\data");
		if (!PathFileExistsW(szPath))
		{
			StringCchCopyW(pch, diff, L"\\..\\..\\data");
			if (!PathFileExistsW(szPath))
			{
				StringCchCopyW(pch, diff, L"\\..\\..\\..\\data");
				if (!PathFileExistsW(szPath))
				{
					StringCchCopyW(pch, diff, L"\\..\\..\\..\\..\\data");
					if (!PathFileExistsW(szPath))
					{
						return FALSE;   // not found
					}
				}
			}
		}
	}

	// found. store to m_szDataFolder
	StringCchCopyW(m_szDataFolder, _countof(m_szDataFolder), szPath);

	// get the PATH environment variable
	MStringW env, str;
	DWORD cch = GetEnvironmentVariableW(L"PATH", NULL, 0);
	env.resize(cch);
	GetEnvironmentVariableW(L"PATH", &env[0], cch);

	// add "data/bin" to the PATH variable
	str = m_szDataFolder;
	str += L"\\bin;";
	str += env;
	SetEnvironmentVariableW(L"PATH", str.c_str());

	return TRUE;
}

BOOL DoCheckFile(std::wstring& file, LPCWSTR psz)
{
	WCHAR szPath[MAX_PATH];
	ExpandEnvironmentStringsW(psz, szPath, _countof(szPath));
	if (PathFileExistsW(szPath))
	{
		file = szPath;
		return TRUE;
	}
	return FALSE;
}

// check the data and the helper programs
INT MMainWnd::CheckData(VOID)
{
	WCHAR szPath[MAX_PATH];

	if (!CheckDataFolder())
	{
		ErrorBoxDx(TEXT("ERROR: data folder was not found!"));
		return -1;  // failure
	}

	// Constants.txt
	StringCchCopyW(m_szConstantsFile, _countof(m_szConstantsFile), m_szDataFolder);
	StringCchCatW(m_szConstantsFile, _countof(m_szConstantsFile), L"\\Constants.txt");
	if (!g_db.LoadFromFile(m_szConstantsFile))
	{
		ErrorBoxDx(TEXT("ERROR: Unable to load Constants.txt file."));
		return -2;  // failure
	}
	g_db.m_map[L"CTRLID"].emplace_back(L"IDC_STATIC", (WORD)-1);

	// mcpp.exe
	StringCchCopyW(m_szMCppExe, _countof(m_szMCppExe), m_szDataFolder);
	StringCchCatW(m_szMCppExe, _countof(m_szMCppExe), L"\\bin\\mcpp.exe");
	// NOTE: _popen has bug: https://github.com/katahiromz/win_popen_bug
	//       To avoid this problem, we use short path.
	GetShortPathNameW(m_szMCppExe, m_szMCppExe, _countof(m_szMCppExe));
	if (!PathFileExistsW(m_szMCppExe))
	{
		ErrorBoxDx(TEXT("ERROR: No mcpp.exe found."));
		return -3;  // failure
	}
	//MessageBoxW(NULL, m_szMCppExe, NULL, 0);

	// windres.exe
	StringCchCopyW(m_szWindresExe, _countof(m_szWindresExe), m_szDataFolder);
	StringCchCatW(m_szWindresExe, _countof(m_szWindresExe), L"\\bin\\windres.exe");
	GetShortPathNameW(m_szWindresExe, m_szWindresExe, _countof(m_szWindresExe));
	if (!PathFileExistsW(m_szWindresExe))
	{
		ErrorBoxDx(TEXT("ERROR: No windres.exe found."));
		return -4;  // failure
	}
	//MessageBoxW(NULL, m_szWindresExe, NULL, 0);

	// upx.exe
	StringCchCopyW(m_szUpxExe, _countof(m_szUpxExe), m_szDataFolder);
	StringCchCatW(m_szUpxExe, _countof(m_szUpxExe), L"\\bin\\upx.exe");
	if (!PathFileExistsW(m_szUpxExe))
	{
		ErrorBoxDx(TEXT("ERROR: No upx.exe found."));
		return -5;  // failure
	}

	// include directory
	StringCchCopyW(m_szIncludeDir, _countof(m_szIncludeDir), m_szDataFolder);
	StringCchCatW(m_szIncludeDir, _countof(m_szIncludeDir), L"\\lib\\gcc\\i686-w64-mingw32\\10.2.0\\include");
	if (!PathFileExistsW(m_szIncludeDir))
	{
		ErrorBoxDx(TEXT("ERROR: No include directory found."));
		return -6;  // failure
	}

	// dfmsc.exe
	StringCchCopyW(m_szDFMSC, _countof(m_szDFMSC), m_szDataFolder);
	StringCchCatW(m_szDFMSC, _countof(m_szDFMSC), L"\\bin\\dfmsc.exe");
	if (!PathFileExistsW(m_szDFMSC))
	{
		ErrorBoxDx(TEXT("ERROR: No dfmsc.exe found."));
		return -7;  // failure
	}

	// OleBow.exe
	StringCchCopyW(m_szOleBow, _countof(m_szOleBow), m_szDataFolder);
	StringCchCatW(m_szOleBow, _countof(m_szOleBow), L"\\bin\\olebow.exe");
	if (!PathFileExistsW(m_szOleBow))
	{
		ErrorBoxDx(TEXT("ERROR: No OleBow.exe found."));
		return -7;  // failure
	}

	// midlwrap.bat
	StringCchCopyW(m_szMidlWrap, _countof(m_szMidlWrap), m_szDataFolder);
	StringCchCatW(m_szMidlWrap, _countof(m_szMidlWrap), L"\\bin\\midlwrap.bat");
	if (!PathFileExistsW(m_szMidlWrap))
	{
		ErrorBoxDx(TEXT("ERROR: No midlwrap.bat found."));
		return -8;  // failure
	}

	// vcvarsall.bat
	std::wstring file;
	m_szVCBat[0] = 0;
	if (DoCheckFile(file, LR"(C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat)") ||
		DoCheckFile(file, LR"(C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat)") ||
		DoCheckFile(file, LR"(C:\Program Files\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat)") ||
		DoCheckFile(file, LR"(C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat)") ||
		DoCheckFile(file, LR"(C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat)") ||
		DoCheckFile(file, LR"(C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat)") ||
		DoCheckFile(file, LR"(C:\Program Files\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat)") ||
		DoCheckFile(file, LR"(C:\Program Files\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat)") ||
		DoCheckFile(file, LR"(%VS140COMNTOOLS%..\..\VC\vcvarsall.bat)") ||
		DoCheckFile(file, LR"(%VS120COMNTOOLS%..\..\VC\vcvarsall.bat)") ||
		DoCheckFile(file, LR"(%VS110COMNTOOLS%..\..\VC\vcvarsall.bat)") ||
		DoCheckFile(file, LR"(%VS100COMNTOOLS%..\..\VC\vcvarsall.bat)") ||
		DoCheckFile(file, LR"(%VS90COMNTOOLS%..\..\VC\vcvarsall.bat)"))
	{
		StringCchCopyW(m_szVCBat, _countof(m_szVCBat), file.c_str());
	}

	// get the module path filename of this application module
	GetModuleFileNameW(NULL, szPath, _countof(szPath));

	// mcdx.exe
	PathRemoveFileSpecW(szPath);
	PathAppendW(szPath, L"mcdx.exe");
	if (PathFileExistsW(szPath))
	{
		StringCchCopyW(m_szMcdxExe, _countof(m_szMcdxExe), szPath);
	}
	else
	{
		StringCchCopyW(m_szMcdxExe, _countof(m_szMcdxExe), m_szDataFolder);
		StringCchCatW(m_szMcdxExe, _countof(m_szMcdxExe), L"\\bin\\mcdx.exe");
		if (!PathFileExistsW(m_szMcdxExe))
		{
			ErrorBoxDx(TEXT("ERROR: No mcdx.exe found."));
			return -8;  // failure
		}
	}

	return 0;   // success
}

// load the language information
void MMainWnd::DoLoadLangInfo(VOID)
{
	// enumerate localized languages
	EnumSystemLocalesW(EnumLocalesProc, LCID_SUPPORTED);

	// enumerate English languages
	EnumSystemLocalesW(EnumEngLocalesProc, LCID_SUPPORTED);

	// add the neutral language
	{
		LANG_ENTRY entry;
		entry.LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
		entry.str = L"Neutral";
		g_langs.push_back(entry);

		entry.str = LoadStringDx(IDS_NEUTRAL);
		g_langs.push_back(entry);
	}

	// sort
	std::sort(g_langs.begin(), g_langs.end());
}

// Helper function to get header file path from TEXTINCLUDE 1 data
static MStringW GetTextInclude1HeaderFile(const EntrySet& res, LPCWSTR szRCPath)
{
	// Find TEXTINCLUDE 1 entry
	auto p_textinclude1 = res.find(ET_LANG, L"TEXTINCLUDE", WORD(1));
	if (!p_textinclude1 || p_textinclude1->m_data.empty())
		return L"";

	// Extract the header file name from the data
	std::string data(p_textinclude1->m_data.begin(), p_textinclude1->m_data.end());

	// Remove trailing NUL characters
	while (!data.empty() && data.back() == '\0')
		data.pop_back();

	// Remove surrounding quotes if present
	if (data.size() >= 2 && data.front() == '"' && data.back() == '"')
	{
		data = data.substr(1, data.size() - 2);
	}

	// If empty, return empty
	if (data.empty())
		return L"";

	// Remove read-only marker
	if (data.find("< ") == 0)
		data = data.substr(2);

	// Convert to wide string
	MAnsiToWide a2w(CP_UTF8, data.c_str());
	MStringW strHeaderFile = a2w.c_str();

	// If it's an absolute path, use it directly
	if (PathIsRelativeW(strHeaderFile.c_str()) == FALSE)
	{
		if (PathFileExistsW(strHeaderFile.c_str()))
			return strHeaderFile;
		return L"";
	}

	// Build full path relative to RC file directory
	WCHAR szDir[MAX_PATH];
	StringCchCopyW(szDir, _countof(szDir), szRCPath);
	PathRemoveFileSpecW(szDir);

	WCHAR szFullPath[MAX_PATH];
	PathCombineW(szFullPath, szDir, strHeaderFile.c_str());

	// Check if the file exists
	if (PathFileExistsW(szFullPath))
		return szFullPath;

	return L"";
}

BOOL MMainWnd::DoLoadRC(HWND hwnd, LPCWSTR szPath)
{
	// load the RC file to the res1 variable
	EntrySet res1;
	if (!DoLoadRCEx(hwnd, szPath, res1, FALSE))
	{
		ErrorBoxDx(IDS_CANNOTOPEN);
		return FALSE;
	}

	enum {
		INCLUDE_TEXTINCLUDE3_YES = 1,
		INCLUDE_TEXTINCLUDE3_NO = 0,
		INCLUDE_TEXTINCLUDE3_UNKNOWN = -1,
	} include_textinclude3_flag = INCLUDE_TEXTINCLUDE3_UNKNOWN;

	// Load the RC file with APSTUDIO_INVOKED
	EntrySet res2;
	if (DoLoadRCEx(hwnd, szPath, res2, TRUE))
	{
		// TEXTINCLUDE 3 の項目があれば、TEXTINCLUDE 3 の項目を消すか、消さないか、いずれかの処理を行う。
retry:
		for (auto& entry1 : res1)
		{
			// res2 の中に res1 と一致する項目があるか確認する。
			bool exists_in_res2 = false;
			for (auto& entry2 : res2)
			{
				if (entry2->m_type == entry1->m_type &&
					entry2->m_name == entry1->m_name &&
					entry2->m_lang == entry1->m_lang)
				{
					exists_in_res2 = true;
					break;
				}
			}

			// 存在しない、かつ TEXTINCLUDEではない場合、
			if (!exists_in_res2 && entry1->m_type != L"TEXTINCLUDE")
			{
				// ユーザーに TEXTINCLUDE 3 を取り込むか問いただす。
				if (include_textinclude3_flag == INCLUDE_TEXTINCLUDE3_UNKNOWN)
				{
					INT id = ErrorBoxDx(IDS_INCLUDETEXTINCLUDE3, MB_YESNOCANCEL);
					if (id == IDYES)
					{
						include_textinclude3_flag = INCLUDE_TEXTINCLUDE3_YES;
					}
					else if (id == IDNO)
					{
						include_textinclude3_flag = INCLUDE_TEXTINCLUDE3_NO;
					}
					else if (id == IDCANCEL)
					{
						res1.delete_all();
						res2.delete_all();
						return FALSE;
					}
				}

				// 取り込まない場合は、該当項目を削除してやり直し。
				if (include_textinclude3_flag == INCLUDE_TEXTINCLUDE3_NO)
				{
					res1.delete_entry(entry1);
					goto retry;
				}
			}
		}
	}

	// Merge TEXTINCLUDE
	EntrySet found;
	res2.search(found, ET_LANG, L"TEXTINCLUDE");
	if (found.size())
	{
		// TEXTINCLUDE should be MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)
		for (auto& entry : found)
		{
			if (entry->m_type == L"TEXTINCLUDE")
				entry->m_lang = 0;
		}
		res1.merge(found);
	}

	res2.delete_all();

	// TEXTINCLUDE should be MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)
	for (auto& entry : res1)
	{
		if (entry->m_type == L"TEXTINCLUDE")
			entry->m_lang = 0;
	}

	// Load resource.h based on TEXTINCLUDE 1
	// Issue #301: Support TEXTINCLUDE 1
	// TN035: TEXTINCLUDE 1 contains the resource symbol header file name
	UnloadResourceH(hwnd);
	if (g_settings.bAutoLoadNearbyResH)
	{
		// First, try to load the header file specified in TEXTINCLUDE 1
		// Prefer res2 (loaded with APSTUDIO_INVOKED) as it contains TEXTINCLUDE resources
		// Fall back to res1 if res2 is empty (e.g., when APSTUDIO_INVOKED loading failed)
		MStringW strHeaderFile = GetTextInclude1HeaderFile(!res2.empty() ? res2 : res1, szPath);
		if (!strHeaderFile.empty())
		{
			// Load the header file from TEXTINCLUDE 1 value
			DoLoadResH(hwnd, strHeaderFile.c_str());
		}
		else
		{
			// Fall back to searching for resource.h in standard locations
			CheckResourceH(hwnd, szPath);
		}
	}

	// TEXTINCLUDE 3 を取り込んだら、TEXTINCLUDE 3 をリセット。
	if (include_textinclude3_flag == INCLUDE_TEXTINCLUDE3_YES)
	{
		res1.search_and_delete(ET_LANG, L"TEXTINCLUDE", WORD(3));

		std::string str = "\r\n";
		EntryBase::data_type data(str.begin(), str.end());
		res1.add_lang_entry(L"TEXTINCLUDE", WORD(3), 0, data);
	}

	// load it now
	m_bLoading = TRUE;
	{
		ShowTreeViewArrow(FALSE);

		// renewal
		SendMessageW(m_hwndTV, WM_SETREDRAW, FALSE, 0);
		g_res.delete_all();
		g_res.merge(res1);
		SendMessageW(m_hwndTV, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(m_hwndTV, NULL, TRUE);

		// clean up
		res1.delete_all();
	}
	m_bLoading = FALSE;

	// update the file info
	UpdateFileInfo(FT_RC, szPath, FALSE);

	// show ID list if necessary
	if (m_szResourceH[0] && g_settings.bAutoShowIDList)
	{
		ShowIDList(hwnd, TRUE);
	}

	// select none
	SelectTV(NULL, FALSE);

	DoSetFileModified(FALSE);
	return TRUE;
}

BOOL MMainWnd::DoLoadRES(HWND hwnd, LPCWSTR szPath)
{
	// reload the resource.h if necessary
	UnloadResourceH(hwnd);
	if (g_settings.bAutoLoadNearbyResH)
		CheckResourceH(hwnd, szPath);

	// do import to the res variable
	EntrySet res;
	if (!res.import_res(szPath))
	{
		ErrorBoxDx(IDS_CANNOTOPEN);
		return FALSE;
	}

	// load it now
	m_bLoading = TRUE;
	{
		ShowTreeViewArrow(FALSE);

		// renewal
		SendMessageW(m_hwndTV, WM_SETREDRAW, FALSE, 0);
		g_res.delete_all();
		g_res.merge(res);
		SendMessageW(m_hwndTV, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(m_hwndTV, NULL, TRUE);

		// clean up
		res.delete_all();
	}
	m_bLoading = FALSE;

	// update the file info
	UpdateFileInfo(FT_RES, szPath, FALSE);

	// show ID list if necessary
	if (m_szResourceH[0] && g_settings.bAutoShowIDList)
	{
		ShowIDList(hwnd, TRUE);
	}

	// select none
	SelectTV(NULL, FALSE);

	DoSetFileModified(FALSE);
	return TRUE;
}

BOOL MMainWnd::DoLoadEXE(HWND hwnd, LPCWSTR pszPath, BOOL bForceDecompress)
{
	WCHAR szPath[MAX_PATH];

	// check whether it was compressed
	MStringW strToOpen = pszPath;
	BOOL bCompressed = DoUpxTest(m_szUpxExe, pszPath);
	if (bCompressed)   // it was compressed
	{
		INT nID;
		if (bForceDecompress)
		{
			nID = IDYES;    // no veto for you
		}
		else
		{
			// confirm to the user to decompress
			LPWSTR szMsg = LoadStringPrintfDx(IDS_FILEISUPXED, pszPath);
			nID = MsgBoxDx(szMsg, MB_ICONINFORMATION | MB_YESNOCANCEL);
			if (nID == IDCANCEL)
				return FALSE;   // cancelled
		}

		if (nID == IDYES)   // try to decompress
		{
			// build a temporary file path
			WCHAR szTempFile[MAX_PATH];
			StringCchCopyW(szTempFile, _countof(szTempFile), GetTempFileNameDx(L"UPX"));

			// pszPath --> szTempFile (to be decompressed)
			if (!CopyFileW(pszPath, szTempFile, FALSE) ||
				!DoUpxDecompress(m_szUpxExe, szTempFile))
			{
				DeleteFileW(szTempFile);
				ErrorBoxDx(IDS_CANTUPXEXTRACT);
				return FALSE;   // failure
			}

			// decompressed
			strToOpen = szTempFile;
		}
		else
		{
			// consider as uncompressed
			bCompressed = FALSE;
		}
	}

	// load an executable files
	HMODULE hMod = LoadLibraryExW(strToOpen.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE |
								  LOAD_LIBRARY_AS_IMAGE_RESOURCE |
								  LOAD_WITH_ALTERED_SEARCH_PATH);
	if (hMod == NULL)
	{
		// replace the path
		#ifdef _WIN64
			mstr_replace_all(strToOpen,
				L"C:\\Program Files\\",
				L"C:\\Program Files (x86)\\");
		#else
			mstr_replace_all(strToOpen,
				L"C:\\Program Files (x86)\\",
				L"C:\\Program Files\\");
		#endif

		// retry to load
		hMod = LoadLibraryExW(strToOpen.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE |
			LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_WITH_ALTERED_SEARCH_PATH);
		if (hMod)
		{
			// ok, succeeded
			StringCchCopyW(szPath, _countof(szPath), strToOpen.c_str());
			pszPath = szPath;
		}
		else
		{
			// retry again
			hMod = LoadLibraryW(strToOpen.c_str());
			if (hMod == NULL)
			{
				ErrorBoxDx(IDS_CANNOTOPEN);

				// delete the decompressed file if any
				if (bCompressed)
				{
					::DeleteFileW(strToOpen.c_str());
				}

				return FALSE;       // failure
			}
		}
	}

	// load all the resource items from the executable
	m_bLoading = TRUE;
	bool is_protected = false, encrypt = false;
	EntrySet res;
	BOOL bOK;
	{
#ifdef ENABLE_CRYPTO
		if (g_res.is_protected(hMod))
		{
			is_protected = true;
			if (g_settings.bUseWonRes && g_bEnableCrypto && g_password.size())
			{
				for (auto& ch : g_password) ch ^= 0xFFFF;
				for (auto& ch : g_salt) ch ^= 0xFFFF;
				SetWonResPassword(g_password.c_str(), g_salt.c_str());
				for (auto& ch : g_password) ch ^= 0xFFFF;
				for (auto& ch : g_salt) ch ^= 0xFFFF;
			}
			else
			{
				SetWonResPassword(nullptr, nullptr);
			}
		}
		else
		{
			SetWonResPassword(nullptr, nullptr);
		}
#endif
		bOK = res.from_res(hMod);
	}
	m_bLoading = FALSE;

	// free the executable
	FreeLibrary(hMod);

	if (!bOK)
	{
		ErrorBoxDx(is_protected ? IDS_CANNOTOPENPROTECTED : IDS_CANNOTOPEN);
		return FALSE;
	}

	// unload the resource.h file
	UnloadResourceH(hwnd);
	if (g_settings.bAutoLoadNearbyResH)
		CheckResourceH(hwnd, pszPath);

	ShowTreeViewArrow(FALSE);
	SendMessageW(m_hwndTV, WM_SETREDRAW, FALSE, 0);

	g_res.delete_all();
	g_res.merge(res);
	res.delete_all();

	SendMessageW(m_hwndTV, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(m_hwndTV, NULL, TRUE);

	if (is_protected && !(g_settings.bUseWonRes && g_bEnableCrypto && g_password.size()))
		MsgBoxDx(LoadStringDx(IDS_FORCEOPENCRYPTOWARN), MB_ICONINFORMATION);

	// update the file info (using the real path)
	UpdateFileInfo(FT_EXECUTABLE, pszPath, bCompressed);

	// delete the decompressed file if any
	if (bCompressed)
	{
		::DeleteFileW(strToOpen.c_str());
	}

	// open the ID list window if necessary
	if (m_szResourceH[0] && g_settings.bAutoShowIDList)
	{
		ShowIDList(hwnd, TRUE);
	}

	// select none
	SelectTV(NULL, FALSE);

	DoSetFileModified(FALSE);

	// update drop-down arrow
	PostUpdateArrow(m_hwnd);

	return TRUE; // success
}

// load a file
BOOL MMainWnd::DoLoadFile(HWND hwnd, LPCWSTR pszFileName, DWORD nFilterIndex, BOOL bForceDecompress)
{
	MWaitCursor wait;
	WCHAR szPath[MAX_PATH], szResolvedPath[MAX_PATH], *pchPart;

	enum LoadFilterIndex        // see also: IDS_EXERESRCFILTER
	{
		LFI_NONE = 0,
		LFI_LOADABLE = 1,
		LFI_EXECUTABLE = 2,
		LFI_RES = 3,
		LFI_RC = 4,
		LFI_ALL = 5
	};

	// if it was a shortcut file, then resolve it.
	// szPath <-- pszFileName
	if (GetPathOfShortcutDx(hwnd, pszFileName, szResolvedPath))
	{
		GetFullPathNameW(szResolvedPath, _countof(szPath), szPath, &pchPart);
		nFilterIndex = LFI_NONE;
	}
	else
	{
		GetFullPathNameW(pszFileName, _countof(szPath), szPath, &pchPart);
	}

	// find the dot extension
	LPWSTR pch = PathFindExtensionW(szPath);
	if (pch)
	{
		switch (nFilterIndex)
		{
		case LFI_NONE: case LFI_ALL: case LFI_LOADABLE:
			nFilterIndex = LFI_NONE;
			if (lstrcmpiW(pch, L".res") == 0)
				nFilterIndex = LFI_RES;
			else if (lstrcmpiW(pch, L".rc") == 0 || lstrcmpiW(pch, L".rc2") == 0)
				nFilterIndex = LFI_RC;
			else if (
				lstrcmpiW(pch, L".exe") == 0 || lstrcmpiW(pch, L".dll") == 0 ||
				lstrcmpiW(pch, L".ocx") == 0 || lstrcmpiW(pch, L".cpl") == 0 ||
				lstrcmpiW(pch, L".scr") == 0 || lstrcmpiW(pch, L".mui") == 0 ||
				lstrcmpiW(pch, L".ime") == 0)
			{
				nFilterIndex = LFI_EXECUTABLE;
			}
		}
	}

	switch (nFilterIndex)
	{
	case LFI_RES:
		return DoLoadRES(hwnd, szPath);
	case LFI_RC:
		return DoLoadRC(hwnd, szPath);
	default:
		return DoLoadEXE(hwnd, szPath, bForceDecompress);
	}
}

// unload resource.h
BOOL MMainWnd::UnloadResourceH(HWND hwnd)
{
	// delete all the macro IDs
	auto it = g_db.m_map.find(L"RESOURCE.ID");
	if (it != g_db.m_map.end())
	{
		it->second.clear();
	}

	BOOL bChanged = g_settings.id_map.size() || m_szResourceH[0];

	// reset the settings of the resource.h file
	g_settings.AddIDC_STATIC();
	g_settings.id_map.clear();
	g_settings.added_ids.clear();
	g_settings.removed_ids.clear();
	m_szResourceH[0] = 0;

	if (bChanged)
		UpdateNames();

	// select the selected entry
	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE);

	// hide the ID list window
	ShowIDList(hwnd, FALSE);

	return TRUE;
}

// check the resource.h file
BOOL MMainWnd::CheckResourceH(HWND hwnd, LPCTSTR pszPath)
{
	// unload the resource.h file
	UnloadResourceH(hwnd);

	// pszPath --> szPath
	TCHAR szPath[MAX_PATH];
	StringCchCopy(szPath, _countof(szPath), pszPath);

	// find the last '\\' or '/'
	TCHAR *pch = _tcsrchr(szPath, TEXT('\\'));
	if (pch == NULL)
		pch = _tcsrchr(szPath, TEXT('/'));
	if (pch == NULL)
		pch = szPath;
	else
		++pch;

	// find the nearest resource.h file
	size_t diff = pch - szPath;
	StringCchCopy(pch, diff, TEXT("resource.h"));
	if (!PathFileExistsW(szPath))
	{
		StringCchCopy(pch, diff, TEXT("..\\resource.h"));
		if (!PathFileExistsW(szPath))
		{
			StringCchCopy(pch, diff, TEXT("..\\..\\resource.h"));
			if (!PathFileExistsW(szPath))
			{
				StringCchCopy(pch, diff, TEXT("..\\..\\..\\resource.h"));
				if (!PathFileExistsW(szPath))
				{
					StringCchCopy(pch, diff, TEXT("..\\src\\resource.h"));
					if (!PathFileExistsW(szPath))
					{
						StringCchCopy(pch, diff, TEXT("..\\..\\src\\resource.h"));
						if (!PathFileExistsW(szPath))
						{
							StringCchCopy(pch, diff, TEXT("..\\..\\..\\src\\resource.h"));
							if (!PathFileExistsW(szPath))
							{
								return FALSE;   // not found
							}
						}
					}
				}
			}
		}
	}

	// load the resource.h file
	return DoLoadResH(hwnd, szPath);
}

// load an RC file
BOOL MMainWnd::DoLoadRCEx(HWND hwnd, LPCWSTR szRCFile, EntrySet& res, BOOL bApStudio)
{
	// load the RC file to the res variable
	MStringA strOutput;
	BOOL bOK = res.load_rc(szRCFile, strOutput, m_szWindresExe,
						   m_szMCppExe, m_szMcdxExe, GetMacroDump(bApStudio),
						   GetIncludesDump());
	if (!bOK && !bApStudio)
	{
		// failed. show error message
		if (strOutput.empty())
		{
			MsgBoxDx(LoadStringDx(IDS_COMPILEERROR), MB_ICONERROR);
		}
		else
		{
			MAnsiToWide a2w(CP_ACP, strOutput);
			ErrorBoxDx(a2w.c_str());
		}
	}

	// close the preview
	HidePreview();

	// recalculate the splitter
	PostMessageDx(WM_SIZE);

	return bOK;
}

BOOL MMainWnd::DoWriteRCLangCodePage(MFile& file, ResToText& res2text, LANGID lang, const EntrySet& targets, UINT nCodePage)
{
	// search the language entries
	EntrySet found;
	targets.search(found, ET_LANG, BAD_TYPE, BAD_NAME, lang);

	bool found_effective = false;
	for (auto entry : found)
	{
		auto type = entry->m_type;
		if (type == L"TEXTINCLUDE")
			continue;
		found_effective = true;
		break;
	}
	if (!found_effective)
		return TRUE;

	MTextToAnsi comment_sep(nCodePage, LoadStringDx(IDS_COMMENT_SEP));

	if (!g_settings.bSepFilesByLang && g_settings.bRedundantComments)
	{
		file.WriteSzA(comment_sep.c_str());
		file.WriteSzA("\r\n");
	}

	// dump a comment and a LANGUAGE statement
	MString strLang = ::GetLanguageStatement(lang, TRUE);
	strLang += L"\r\n";
	file.WriteSzA(MWideToAnsi(nCodePage, strLang).c_str());

	std::vector<EntryBase *> vecFound(found.begin(), found.end());

	std::sort(vecFound.begin(), vecFound.end(),
		[](const EntryBase *a, const EntryBase *b) {
			if (a->m_type < b->m_type)
				return true;
			if (a->m_type > b->m_type)
				return false;
			return a->m_name < b->m_name;
		}
	);

	MIdOrString type, old_type;

	// for all found entries
	for (auto entry : vecFound)
	{
		old_type = type;
		type = entry->m_type;

		// ignore the string or message tables or font dir
		if (type == RT_STRING || type == RT_FONTDIR)
			continue;
		if (type == L"TEXTINCLUDE")
			continue;

		// dump the entry
		MString str = res2text.DumpEntry(*entry);
		if (!str.empty())
		{
			// output redundant comments
			if (type != old_type && g_settings.bRedundantComments)
			{
				file.WriteSzA(comment_sep.c_str());
				MStringW strType = res2text.GetResTypeName(type);
				MWideToAnsi ansi(nCodePage, strType);
				file.WriteSzA("// ");
				file.WriteSzA(ansi.c_str());
				file.WriteSzA("\r\n\r\n");
			}

			mstr_trim(str);     // trim

			// convert the text
			MTextToAnsi t2a(nCodePage, str.c_str());
			file.WriteSzA(t2a.c_str());

			// add newlines
			file.WriteSzA("\r\n\r\n");
		}
	}

	// search the string tables
	found.clear();
	targets.search(found, ET_LANG, RT_STRING, BAD_NAME, lang);
	if (found.size())
	{
		if (g_settings.bRedundantComments)
		{
			file.WriteSzA(comment_sep.c_str());
			file.WriteSzA("// RT_STRING\r\n\r\n");
		}

		// found --> str_res
		StringRes str_res;
		for (auto e : found)
		{
			if (e->m_lang != lang)
				continue;       // must be same language

			MByteStreamEx stream(e->m_data);
			if (!str_res.LoadFromStream(stream, e->m_name.m_id))
				return FALSE;
		}

		// dump
		MString str = str_res.Dump();

		// trim
		mstr_trim(str);

		// append newlines
		str += L"\r\n\r\n";

		// convert the text
		MTextToAnsi t2a(nCodePage, str.c_str());

		// write it
		file.WriteSzA(t2a.c_str());
	}

	return TRUE;
}

BOOL MMainWnd::DoWriteRCLangUTF16(MFile& file, ResToText& res2text, LANGID lang, const EntrySet& targets)
{
	// search the language entries
	EntrySet found;
	targets.search(found, ET_LANG, BAD_TYPE, BAD_NAME, lang);

	bool found_effective = false;
	for (auto entry : found)
	{
		auto type = entry->m_type;
		if (type == L"TEXTINCLUDE")
			continue;
		found_effective = true;
	}
	if (!found_effective)
		return TRUE;

	MString comment_sep(LoadStringDx(IDS_COMMENT_SEP));

	if (!g_settings.bSepFilesByLang && g_settings.bRedundantComments)
	{
		file.WriteSzW(comment_sep.c_str());
		file.WriteSzW(L"\r\n");
	}

	// dump a comment and a LANGUAGE statement
	MString strLang = ::GetLanguageStatement(lang, TRUE);
	strLang += L"\r\n";
	file.WriteSzW(strLang.c_str());

	std::vector<EntryBase *> vecFound(found.begin(), found.end());

	std::sort(vecFound.begin(), vecFound.end(),
		[](const EntryBase *a, const EntryBase *b) {
			if (a->m_type < b->m_type)
				return true;
			if (a->m_type > b->m_type)
				return false;
			return a->m_name < b->m_name;
		}
	);

	MIdOrString type, old_type;

	// for all found entries
	for (auto entry : vecFound)
	{
		old_type = type;
		type = entry->m_type;

		// ignore the string or message tables or font dir
		if (type == RT_STRING || type == RT_FONTDIR)
			continue;
		if (type == L"TEXTINCLUDE")
			continue;

		// dump the entry
		MString str = res2text.DumpEntry(*entry);
		if (!str.empty())
		{
			// output redundant comments
			if (type != old_type && g_settings.bRedundantComments)
			{
				file.WriteSzW(comment_sep.c_str());
				MStringW strType = res2text.GetResTypeName(type);
				file.WriteSzW(L"// ");
				file.WriteSzW(strType.c_str());
				file.WriteSzW(L"\r\n\r\n");
			}

			mstr_trim(str);     // trim

			// convert the text to UTF-8
			file.WriteSzW(str.c_str());

			// add newlines
			file.WriteSzW(L"\r\n\r\n");
		}
	}

	// search the string tables
	found.clear();
	targets.search(found, ET_LANG, RT_STRING, BAD_NAME, lang);
	if (found.size())
	{
		if (g_settings.bRedundantComments)
		{
			file.WriteSzW(comment_sep.c_str());
			file.WriteSzW(L"// RT_STRING\r\n\r\n");
		}

		// found --> str_res
		StringRes str_res;
		for (auto e : found)
		{
			if (e->m_lang != lang)
				continue;       // must be same language

			MByteStreamEx stream(e->m_data);
			if (!str_res.LoadFromStream(stream, e->m_name.m_id))
				return FALSE;
		}

		// dump
		MString str = str_res.Dump();

		// trim
		mstr_trim(str);

		// append newlines
		str += L"\r\n\r\n";

		// write it
		file.WriteSzW(str.c_str());
	}

	return TRUE;
}

// write a language-specific RC text
BOOL MMainWnd::DoWriteRCLang(MFile& file, ResToText& res2text, LANGID lang, const EntrySet& targets, UINT nCodePage)
{
	if (nCodePage == _CP_UTF16)
		return DoWriteRCLangUTF16(file, res2text, lang, targets);
	else
		return DoWriteRCLangCodePage(file, res2text, lang, targets, nCodePage);
}

// do backup a folder
BOOL MMainWnd::DoBackupFolder(LPCWSTR pszPath, UINT nCount)
{
	if (!PathIsDirectoryW(pszPath))
		return TRUE;    // no files to be backup'ed

	if (nCount < s_nBackupMaxCount)     // less than max count
	{
		MString strPath = pszPath;
		strPath += g_settings.strBackupSuffix;

		// do backup the "old" folder (recursively)
		DoBackupFolder(strPath.c_str(), nCount + 1);

		// rename the current folder as an "old" folder
		return MoveFileExW(pszPath, strPath.c_str(),
						   MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
	}
	else
	{
		// unable to create one more backup. delete it
		DeleteDirectoryDx(pszPath);
	}

	return TRUE;
}

// do backup a file
BOOL MMainWnd::DoBackupFile(LPCWSTR pszPath, UINT nCount)
{
	if (!PathFileExistsW(pszPath))
		return TRUE;

	if (nCount < s_nBackupMaxCount)     // less than max count
	{
		MString strPath = pszPath;
		strPath += g_settings.strBackupSuffix;

		// do backup the "old" file (recursively)
		DoBackupFile(strPath.c_str(), nCount + 1);

		// copy the current file as an "old" file
		return CopyFileW(pszPath, strPath.c_str(), FALSE);
	}
	else
	{
		// otherwise overwritten
	}

	return TRUE;
}

std::wstring generated_from(INT n)
{
	WCHAR szText[MAX_PATH];
	StringCchPrintfW(szText, _countof(szText), LoadStringDx(IDS_GENERATEDFROMTEXTINCLUDE), n);
	return szText;
}

// write a RC file
BOOL MMainWnd::DoWriteRC(LPCWSTR pszFileName, LPCWSTR pszResH, const EntrySet& found, UINT nCodePage)
{
	ResToText res2text;
	res2text.m_bHumanReadable = FALSE;  // it's not human-friendly
	res2text.m_bNoLanguage = TRUE;      // no LANGUAGE statements generated

	// check not locking
	DWORD error = IsFileLockedDx(pszFileName);
	if (error == ERROR_SHARING_VIOLATION)
	{
		WCHAR szMsg[MAX_PATH + 256];
		StringCchPrintfW(szMsg, _countof(szMsg), LoadStringDx(IDS_CANTWRITEBYLOCK), pszFileName);
		ErrorBoxDx(szMsg);
		return FALSE;
	}
	if (error == ERROR_ACCESS_DENIED)
	{
		WCHAR szMsg[MAX_PATH + 256];
		StringCchPrintfW(szMsg, _countof(szMsg), LoadStringDx(IDS_NOFILEACCESS), pszFileName);
		ErrorBoxDx(szMsg);
		return FALSE;
	}

	// Check TEXTINCLUDE write protect
	auto p_textinclude1 = found.find(ET_LANG, L"TEXTINCLUDE", WORD(1));
	if (p_textinclude1)
	{
		std::string str(p_textinclude1->m_data.begin(), p_textinclude1->m_data.end());
		if (str.find("< ") != str.npos) // write protected?
		{
			// Same file?
			WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];
			GetFullPathNameW(pszFileName, _countof(szPath1), szPath1, NULL);
			GetFullPathNameW(m_szFile, _countof(szPath2), szPath2, NULL);
			if (lstrcmpiW(szPath1, szPath2) == 0)
			{
				// Warn read-only
				if (ErrorBoxDx(LoadStringDx(IDS_TEXTINCLUDEREADONLY), MB_ICONWARNING | MB_YESNOCANCEL) != IDYES)
					return FALSE;
			}
		}
	}

	// at first, backup it
	if (g_settings.bBackup)
		DoBackupFile(pszFileName);

	// create a RC file
	MFile file(pszFileName, TRUE);
	if (!file)
		return FALSE;

	BOOL bRCFileUTF16 = (nCodePage == _CP_UTF16);

	// Add Byte Order Mark (BOM)
	if (g_settings.bAddBomToRC)
	{
		DWORD cbWritten;
		if (nCodePage == _CP_UTF16)
			file.WriteFile("\xFF\xFE", 2, &cbWritten);
		else if (nCodePage == CP_UTF8)
			file.WriteFile("\xEF\xBB\xBF", 3, &cbWritten);
	}

	WCHAR szTitle[MAX_PATH];
	GetFileTitleW(pszFileName, szTitle, _countof(szTitle));

	// Treat TEXTINCLUDE info
	EntrySet textinclude;
	textinclude.add_default_TEXTINCLUDE();

	// Issue #301: Use TEXTINCLUDE 1 from the resource if available
	p_textinclude1 = found.find(ET_LANG, L"TEXTINCLUDE", WORD(1));
	if (!p_textinclude1)
		p_textinclude1 = textinclude.find(ET_LANG, L"TEXTINCLUDE", WORD(1));

	auto p_textinclude2 = found.find(ET_LANG, L"TEXTINCLUDE", WORD(2));
	if (!p_textinclude2)
		p_textinclude2 = textinclude.find(ET_LANG, L"TEXTINCLUDE", WORD(2));

	auto p_textinclude3 = found.find(ET_LANG, L"TEXTINCLUDE", WORD(3));
	if (!p_textinclude3)
		p_textinclude3 = textinclude.find(ET_LANG, L"TEXTINCLUDE", WORD(3));

	// Get header file name from TEXTINCLUDE 1
	std::string textinclude1_a;
	if (p_textinclude1)
		textinclude1_a = p_textinclude1->to_string();
	// Remove trailing NUL characters
	while (!textinclude1_a.empty() && textinclude1_a.back() == '\0')
		textinclude1_a.pop_back();
	MAnsiToWide textinclude1_w(CP_UTF8, textinclude1_a.c_str());

	// Issue #301: Check if custom header file exists at destination, offer to copy if not
	// Note: "< " prefix indicates a write-protected/read-only file (Visual C++ convention)
	// We skip these as they are typically system headers that shouldn't be copied
	if (!textinclude1_a.empty() && textinclude1_a != "resource.h") {
		bool write_protected = (textinclude1_a.find("< ") == 0);
		std::wstring include_path = write_protected ? &textinclude1_w.c_str()[2] : textinclude1_w;
		mstr_trim(include_path);

		// Build destination path for header file
		WCHAR szDestDir[MAX_PATH];
		StringCchCopyW(szDestDir, _countof(szDestDir), pszFileName);
		PathRemoveFileSpecW(szDestDir);

		WCHAR szDestHeaderPath[MAX_PATH];
		PathCombineW(szDestHeaderPath, szDestDir, include_path.c_str());

		for (auto& ch : szDestHeaderPath) {
			if (ch == L'/') ch = L'\\';
		}

		// Try to find source header file from original RC file location
		WCHAR szSrcDir[MAX_PATH];
		StringCchCopyW(szSrcDir, _countof(szSrcDir), m_szFile);
		PathRemoveFileSpecW(szSrcDir);

		WCHAR szSrcHeaderPath[MAX_PATH];
		PathCombineW(szSrcHeaderPath, szSrcDir, include_path.c_str());

		for (auto& ch : szSrcHeaderPath) {
			if (ch == L'/') ch = L'\\';
		}

		// Source header exists?
		if (PathFileExistsW(szSrcHeaderPath)) {
			// Create parent directory if it doesn't exist
			// This handles paths like "include\resource.h" or "include/resource.h"
			WCHAR szDestHeaderDir[MAX_PATH];
			StringCchCopyW(szDestHeaderDir, _countof(szDestHeaderDir), szDestHeaderPath);
			PathRemoveFileSpecW(szDestHeaderDir);
			if (szDestHeaderDir[0] && !PathIsDirectoryW(szDestHeaderDir))
				create_directories_recursive_win32(szDestHeaderDir);

			for (auto& ch : szSrcHeaderPath) {
				if (ch == L'/') ch = L'\\';
			}
			for (auto& ch : szDestHeaderPath) {
				if (ch == L'/') ch = L'\\';
			}

			if (lstrcmpiW(szSrcHeaderPath, szDestHeaderPath) != 0 &&
				!CopyFileW(szSrcHeaderPath, szDestHeaderPath, FALSE))
			{
				// Copy failed, show error to user
				textinclude.delete_all();
				ErrorBoxDx(IDS_CANNOTSAVE);
				return FALSE;
			}
		}
	}

	std::string textinclude2_a;
	if (p_textinclude2)
		textinclude2_a = p_textinclude2->to_string();
	MAnsiToWide textinclude2_w(CP_UTF8, textinclude2_a.c_str());

	std::string textinclude3_a;
	if (p_textinclude3)
		textinclude3_a = p_textinclude3->to_string();
	MAnsiToWide textinclude3_w(CP_UTF8, textinclude3_a.c_str());

	MWideToAnsi comment_sep(CP_UTF8, LoadStringDx(IDS_COMMENT_SEP));

	// dump heading
	if (bRCFileUTF16)
	{
		file.WriteFormatW(L"// %s\r\n", szTitle);

		file.WriteSzW(LoadStringDx(IDS_NOTICE));
		file.WriteSzW(LoadStringDx(IDS_DAGGER));

		if (g_settings.bRedundantComments)
		{
			file.WriteSzW(LoadStringDx(IDS_COMMENT_SEP));
			file.WriteSzW(generated_from(1).c_str());
			file.WriteSzW(L"\r\n");
		}

		// Issue #301: Use header file name from TEXTINCLUDE 1
		if (pszResH && pszResH[0])
		{
			if (!textinclude1_a.empty())
			{
				std::wstring path = textinclude1_w.c_str();
				if (path.find(L"< ") == 0)
					path = path.substr(2);

				// Use header file name from TEXTINCLUDE 1
				file.WriteSzW(L"#include \"");
				file.WriteSzW(path.c_str());
				file.WriteSzW(L"\"\r\n");
			}
			else
			{
				file.WriteSzW(L"#include \"resource.h\"\r\n");
			}
		}

		if (g_settings.bRedundantComments)
		{
			file.WriteSzW(L"\r\n");
			file.WriteSzW(LoadStringDx(IDS_COMMENT_SEP));
			file.WriteSzW(generated_from(2).c_str());
			file.WriteSzW(L"\r\n");
		}

		file.WriteSzW(textinclude2_w.c_str());

		if (g_settings.bRedundantComments)
		{
			file.WriteSzW(L"\r\n");
			file.WriteSzW(LoadStringDx(IDS_COMMENT_SEP));
			file.WriteSzW(L"\r\n");
		}

		file.WriteSzW(L"#pragma code_page(65001) // UTF-8\r\n\r\n");

		if (g_settings.bUseIDC_STATIC && !g_settings.bHideID)
		{
			file.WriteSzW(L"#ifndef IDC_STATIC\r\n");
			file.WriteSzW(L"    #define IDC_STATIC (-1)\r\n");
			file.WriteSzW(L"#endif\r\n\r\n");
		}
	}
	else
	{
		MWideToAnsi ansi(nCodePage, szTitle);
		file.WriteFormatA("// %s\r\n", ansi.c_str());

		MWideToAnsi ansiNotice(nCodePage, LoadStringDx(IDS_NOTICE));
		MWideToAnsi ansiDagger(nCodePage, LoadStringDx(IDS_DAGGER));
		MWideToAnsi ansiCommentSep(nCodePage, LoadStringDx(IDS_COMMENT_SEP));
		file.WriteSzA(ansiNotice.c_str());
		file.WriteSzA(ansiDagger.c_str());

		if (g_settings.bRedundantComments)
		{
			file.WriteSzA(ansiCommentSep.c_str());
			MWideToAnsi ansiGeneratedFrom1(nCodePage, generated_from(1));
			file.WriteSzA(ansiGeneratedFrom1);
			file.WriteSzA("\r\n");
		}

		// Issue #301: Use header file name from TEXTINCLUDE 1
		if (pszResH && pszResH[0])
		{
			if (!textinclude1_a.empty())
			{
				std::string path = textinclude1_a;
				if (path.find("< ") == 0) path = path.substr(2);
				// Use header file name from TEXTINCLUDE 1
				file.WriteSzA("#include \"");
				file.WriteSzA(path.c_str());
				file.WriteSzA("\"\r\n");
			}
			else
			{
				file.WriteSzA("#include \"resource.h\"\r\n");
			}
		}

		if (g_settings.bRedundantComments)
		{
			file.WriteSzA("\r\n");
			file.WriteSzA(ansiCommentSep.c_str());
			MWideToAnsi w2a(nCodePage, generated_from(2).c_str());
			file.WriteSzA(w2a.c_str());
			file.WriteSzA("\r\n");
		}

		file.WriteSzA(textinclude2_a.c_str());

		if (g_settings.bRedundantComments)
		{
			file.WriteSzA("\r\n");
			file.WriteSzA(ansiCommentSep.c_str());
			file.WriteSzA("\r\n");
		}

		if (nCodePage == CP_UTF8)
			file.WriteSzA("#pragma code_page(65001) // UTF-8\r\n\r\n");
		else
			file.WriteFormatA("#pragma code_page(%u) // %s\r\n\r\n", nCodePage,
			                  FindCodePageName(nCodePage));

		if (g_settings.bUseIDC_STATIC && !g_settings.bHideID)
		{
			file.WriteSzA("#ifndef IDC_STATIC\r\n");
			file.WriteSzA("    #define IDC_STATIC (-1)\r\n");
			file.WriteSzA("#endif\r\n\r\n");
		}
	}

	// get the used languages
	std::unordered_set<LANGID> langs;
	typedef std::pair<LANGID, MStringW> lang_pair;
	std::vector<lang_pair> lang_vec;

	for (auto res : found)
	{
		LANGID lang = res->m_lang;
		if (langs.insert(lang).second)
		{
			MString lang_name = g_db.GetLangName(lang);
			lang_vec.push_back(std::make_pair(lang, lang_name));
		}
	}

	// sort by lang_name
	std::sort(lang_vec.begin(), lang_vec.end(),
		[](const lang_pair& a, const lang_pair& b) {
			return (a.second < b.second);
		}
	);

	// add "res/" to the prefix if necessary
	if (g_settings.bStoreToResFolder)
		res2text.m_strFilePrefix = L"res/";

	// Is the RC file a rc2?
	BOOL bIsRC2 = (lstrcmpiW(PathFindExtensionW(pszFileName), L".rc2") == 0);

	// use the "lang" folder?
	if (g_settings.bSepFilesByLang)
	{
		// dump neutral
		if (langs.count(0) > 0)
		{
			if (!DoWriteRCLang(file, res2text, 0, found, nCodePage)) {
				textinclude.delete_all();
				return FALSE;
			}
		}

		// create "lang" directory path
		WCHAR szLangDir[MAX_PATH];
		StringCchCopyW(szLangDir, _countof(szLangDir), pszFileName);

		// find the last '\\' or '/'
		WCHAR *pch = wcsrchr(szLangDir, L'\\');
		if (pch == NULL)
			pch = mstrrchr(szLangDir, L'/');
		if (pch == NULL) {
			textinclude.delete_all();
			return FALSE;
		}

		// build the lang directory path
		*pch = 0;
		StringCchCatW(szLangDir, _countof(szLangDir), TEXT("/lang"));

		// backup and create "lang" directory
		for (auto lang_pair : lang_vec)
		{
			if (!lang_pair.first)
				continue;

			if (g_settings.bBackup)
				DoBackupFolder(szLangDir);

			CreateDirectory(szLangDir, NULL);
			break;
		}

		// for each language
		for (auto lang_pair : lang_vec)
		{
			auto lang = lang_pair.first;
			if (!lang)
				continue;

			// create lang/XX_XX.rc file
			WCHAR szLangFile[MAX_PATH];
			StringCchCopyW(szLangFile, _countof(szLangFile), szLangDir);
			StringCchCatW(szLangFile, _countof(szLangFile), TEXT("/"));
			MStringW lang_name = lang_pair.second;
			StringCchCatW(szLangFile, _countof(szLangFile), lang_name.c_str());
			StringCchCatW(szLangFile, _countof(szLangFile), (bIsRC2 ? TEXT(".rc2") : TEXT(".rc")));
			//MessageBox(NULL, szLangFile, NULL, 0);

			if (g_settings.bBackup)
				DoBackupFile(szLangFile);

			// dump to lang/XX_XX.rc file
			MFile lang_file(szLangFile, TRUE);

			// Add Byte Order Mark (BOM)
			if (g_settings.bAddBomToRC)
			{
				DWORD cbWritten;
				if (bRCFileUTF16)
					lang_file.WriteFile("\xFF\xFE", 2, &cbWritten);
				else
					lang_file.WriteFile("\xEF\xBB\xBF", 3, &cbWritten);
			}

			if (bRCFileUTF16)
			{
				lang_file.WriteSzW(LoadStringDx(IDS_NOTICE));
				lang_file.WriteSzW(LoadStringDx(IDS_DAGGER));
				lang_file.WriteSzW(L"\r\n");
				lang_file.WriteSzW(L"#pragma code_page(65001) // UTF-8\r\n\r\n");
			}
			else
			{
				MWideToAnsi ansiNotice(nCodePage, LoadStringDx(IDS_NOTICE));
				MWideToAnsi ansiDagger(nCodePage, LoadStringDx(IDS_DAGGER));
				lang_file.WriteSzA(ansiNotice.c_str());
				lang_file.WriteSzA(ansiDagger.c_str());
				lang_file.WriteSzA("\r\n");
				if (nCodePage == CP_UTF8)
					lang_file.WriteSzA("#pragma code_page(65001) // UTF-8\r\n\r\n");
				else
					lang_file.WriteFormatA("#pragma code_page(%u) // %s\r\n\r\n", nCodePage,
					                       FindCodePageName(nCodePage));
			}
			if (!lang_file) {
				textinclude.delete_all();
				return FALSE;
			}
			if (!DoWriteRCLang(lang_file, res2text, lang, found, nCodePage)) {
				textinclude.delete_all();
				return FALSE;
			}

			if (g_settings.bRedundantComments)
			{
				if (bRCFileUTF16)
				{
					lang_file.WriteSzW(LoadStringDx(IDS_COMMENT_SEP));
				}
				else
				{
					lang_file.WriteSzA(comment_sep.c_str());
				}
			}
		}
	}
	else
	{
		// don't use the "lang" folder
		for (auto lang_pair : lang_vec)
		{
			auto lang = lang_pair.first;
			// write it for each language
			if (!DoWriteRCLang(file, res2text, lang, found, nCodePage)) {
				textinclude.delete_all();
				return FALSE;
			}
		}
	}

	bool bHasNonNeutral = false;
	for (auto& entry : found)
	{
		if (entry->m_lang != 0)
		{
			bHasNonNeutral = true;
			break;
		}
	}

	// dump language includes
	if (g_settings.bSepFilesByLang && bHasNonNeutral)
	{
		// write a C++ comment to make a section
		if (g_settings.bRedundantComments)
		{
			if (bRCFileUTF16)
			{
				file.WriteSzW(LoadStringDx(IDS_COMMENT_SEP));
				file.WriteSzW(L"// Languages\r\n\r\n");
			}
			else
			{
				file.WriteSzA(comment_sep.c_str());
				file.WriteSzA("// Languages\r\n\r\n");
			}
		}

		if (g_settings.bSelectableByMacro)
		{
			for (auto lang_pair : lang_vec)     // for each language
			{
				auto lang = lang_pair.first;
				if (!lang)
					continue;       // ignore neutral language

				// get the language name (such as en_US, ja_JP, etc.) from database
				MString lang_name1 = g_db.GetLangName(lang);

				// make uppercase one
				MString lang_name2 = lang_name1;
				mstr_upper(lang_name2);

				if (bRCFileUTF16)
				{
					// write "#ifdef LANGUAGE_...\r\n"
					file.WriteSzW(L"#ifdef LANGUAGE_");
					file.WriteSzW(lang_name2.c_str());
					file.WriteSzW(L"\r\n");

					// write "#define \"lang/....rc\"\r\n"
					file.WriteSzW(L"    #include \"lang/");
					file.WriteSzW(lang_name1.c_str());
					file.WriteSzW(bIsRC2 ? L".rc2\"\r\n" : L".rc\"\r\n");

					// write "#endif\r\n"
					file.WriteSzW(L"#endif\r\n");
				}
				else
				{
					// write "#ifdef LANGUAGE_...\r\n"
					file.WriteSzA("#ifdef LANGUAGE_");
					MWideToAnsi lang2_w2a(CP_ACP, lang_name2);
					file.WriteSzA(lang2_w2a.c_str());
					file.WriteSzA("\r\n");

					// write "#define \"lang/....rc\"\r\n"
					file.WriteSzA("    #include \"lang/");
					MWideToAnsi lang1_w2a(CP_ACP, lang_name1);
					file.WriteSzA(lang1_w2a.c_str());
					file.WriteSzA(bIsRC2 ? ".rc2\"\r\n" : ".rc\"\r\n");

					// write "#endif\r\n"
					file.WriteSzA("#endif\r\n");
				}
			}
		}
		else
		{
			for (auto lang_pair : lang_vec)
			{
				auto lang = lang_pair.first;
				if (!lang)
					continue;   // ignore the neutral language

				// get the language name (such as en_US, ja_JP, etc.) from database
				MString lang_name1 = g_db.GetLangName(lang);

				if (bRCFileUTF16)
				{
					// write "#include \"lang/....rc\"\r\n"
					file.WriteSzW(L"#include \"lang/");
					file.WriteSzW(lang_name1.c_str());
					file.WriteSzW(bIsRC2 ? L".rc2\"\r\n" : L".rc\"\r\n");
				}
				else
				{
					// write "#include \"lang/....rc\"\r\n"
					file.WriteSzA("#include \"lang/");
					file.WriteSzA(MWideToAnsi(CP_ACP, lang_name1).c_str());
					file.WriteSzA(bIsRC2 ? ".rc2\"\r\n" : ".rc\"\r\n");
				}
			}
		}

		if (bRCFileUTF16)
			file.WriteSzW(L"\r\n");
		else
			file.WriteSzA("\r\n");
	}

	if (g_settings.bRedundantComments)
	{
		if (bRCFileUTF16)
		{
			file.WriteSzW(LoadStringDx(IDS_COMMENT_SEP));
			file.WriteSzW(L"// TEXTINCLUDE\r\n\r\n");
		}
		else
		{
			file.WriteSzA(comment_sep.c_str());
			file.WriteSzA("// TEXTINCLUDE\r\n\r\n");
		}
	}

	if (bRCFileUTF16)
	{
		file.WriteSzW(L"LANGUAGE LANG_NEUTRAL, SUBLANG_NEUTRAL\r\n\r\n");
		file.WriteSzW(L"#ifdef APSTUDIO_INVOKED\r\n\r\n");

		if (p_textinclude1)
		{
			auto str = res2text.DumpEntry(*p_textinclude1);
			file.WriteSzW(str.c_str());
			file.WriteSzW(L"\r\n");
		}

		if (p_textinclude2)
		{
			auto str = res2text.DumpEntry(*p_textinclude2);
			file.WriteSzW(str.c_str());
			file.WriteSzW(L"\r\n");
		}

		if (p_textinclude3)
		{
			auto str = res2text.DumpEntry(*p_textinclude3);
			file.WriteSzW(str.c_str());
			file.WriteSzW(L"\r\n");
		}

		file.WriteSzW(L"#endif // def APSTUDIO_INVOKED\r\n");

		if (g_settings.bRedundantComments)
		{
			file.WriteSzW(L"\r\n");
			file.WriteSzW(LoadStringDx(IDS_COMMENT_SEP));
			file.WriteSzW(generated_from(3).c_str());
			file.WriteSzW(L"\r\n");
		}

		file.WriteSzW(L"#ifndef APSTUDIO_INVOKED\r\n");
		file.WriteSzW(textinclude3_w.c_str());
		file.WriteSzW(L"#endif // ndef APSTUDIO_INVOKED\r\n");

		if (g_settings.bRedundantComments)
		{
			file.WriteSzW(L"\r\n");
			file.WriteSzW(LoadStringDx(IDS_COMMENT_SEP));
		}
	}
	else
	{
		file.WriteSzA("LANGUAGE LANG_NEUTRAL, SUBLANG_NEUTRAL\r\n\r\n");
		file.WriteSzA("#ifdef APSTUDIO_INVOKED\r\n\r\n");

		if (p_textinclude1)
		{
			auto str = res2text.DumpEntry(*p_textinclude1);
			MWideToAnsi strA(nCodePage, str.c_str());
			file.WriteSzA(strA.c_str());
			file.WriteSzA("\r\n");
		}

		if (p_textinclude2)
		{
			auto str = res2text.DumpEntry(*p_textinclude2);
			MWideToAnsi strA(nCodePage, str.c_str());
			file.WriteSzA(strA.c_str());
			file.WriteSzA("\r\n");
		}

		if (p_textinclude3)
		{
			auto str = res2text.DumpEntry(*p_textinclude3);
			MWideToAnsi strA(nCodePage, str.c_str());
			file.WriteSzA(strA.c_str());
			file.WriteSzA("\r\n");
		}

		file.WriteSzA("#endif // def APSTUDIO_INVOKED\r\n");

		if (g_settings.bRedundantComments)
		{
			file.WriteSzA("\r\n");
			file.WriteSzA(comment_sep.c_str());

			MWideToAnsi w2a(nCodePage, generated_from(3).c_str());
			file.WriteSzA(w2a.c_str());

			file.WriteSzA("\r\n");
		}

		file.WriteSzA("#ifndef APSTUDIO_INVOKED\r\n");
		file.WriteSzA(textinclude3_a.c_str());
		file.WriteSzA("#endif // ndef APSTUDIO_INVOKED\r\n");

		if (g_settings.bRedundantComments)
		{
			file.WriteSzA("\r\n");
			file.WriteSzA(comment_sep.c_str());
		}
	}

	textinclude.delete_all();
	return TRUE;
}

struct MACRO_DEF
{
	MStringA prefix;
	MStringA name;
	DWORD value;
	MStringA definition;
};

void WriteMacroLine(MFile& file, const MStringA& name, const MStringA& definition)
{
	MStringA strSpace(" ");
	if (name.size() < 35)
		strSpace.assign(35 - name.size(), ' ');
	file.WriteFormatA("#define %s %s%s\r\n",
		name.c_str(), strSpace.c_str(), definition.c_str());
}

// write the resource.h file
BOOL MMainWnd::DoWriteResH(LPCWSTR pszResH, LPCWSTR pszRCFile)
{
	// check not locking
	DWORD error = IsFileLockedDx(pszResH);
	if (error == ERROR_SHARING_VIOLATION)
	{
		WCHAR szMsg[MAX_PATH + 256];
		StringCchPrintfW(szMsg, _countof(szMsg), LoadStringDx(IDS_CANTWRITEBYLOCK), pszResH);
		ErrorBoxDx(szMsg);
		return FALSE;
	}
	if (error == ERROR_ACCESS_DENIED)
	{
		WCHAR szMsg[MAX_PATH + 256];
		StringCchPrintfW(szMsg, _countof(szMsg), LoadStringDx(IDS_NOFILEACCESS), pszResH);
		ErrorBoxDx(szMsg);
		return FALSE;
	}

	// do backup the resource.h file
	if (g_settings.bBackup)
		DoBackupFile(pszResH);

	// create the resource.h file
	MFile file(pszResH, TRUE);
	if (!file)
		return FALSE;

	// write a heading
	// NOTE: resource.h must be ASCII.
	file.WriteSzA("//{{NO_DEPENDENCIES}}\r\n");
	file.WriteSzA("// Microsoft Visual C++ Compatible\r\n");
	file.WriteFormatA("// This file is automatically generated by RisohEditor %s.\r\n",
	                  PROJECT_VERSION);

	// write the RC file info if necessary
	if (pszRCFile)
	{
		// get file title
		TCHAR szFileTitle[MAX_PATH];
		GetFileTitle(pszRCFile, szFileTitle, _countof(szFileTitle));

		// change extension to .rc or .rc2
		LPTSTR pch = mstrrchr(szFileTitle, TEXT('.'));
		if (pch)
		{
			*pch = 0;
			if (lstrcmpiW(PathFindExtensionW(pszRCFile), L".rc2") == 0)
				StringCchCatW(szFileTitle, _countof(szFileTitle), TEXT(".rc2"));
			else
				StringCchCatW(szFileTitle, _countof(szFileTitle), TEXT(".rc"));
		}

		// write file title
		file.WriteSzA("// ");
		file.WriteSzA(MTextToAnsi(CP_ACP, szFileTitle).c_str());
		file.WriteSzA("\r\n");
	}

	// sort macro definitions
	std::vector<MACRO_DEF> defs;
	for (auto& pair : g_settings.id_map)
	{
		if (pair.first == "IDC_STATIC")
			continue;

		MACRO_DEF def;
		def.name = pair.first;
		def.definition = pair.second;
		def.value = mstr_parse_int(pair.second.c_str(), true, 0);
		size_t i = pair.first.find('_');
		if (i != MStringA::npos)
		{
			def.prefix = pair.first.substr(0, i + 1);
		}
		defs.push_back(def);
	}
	std::sort(defs.begin(), defs.end(),
		[](const MACRO_DEF& a, const MACRO_DEF& b) {
			if (a.prefix.empty() && b.prefix.empty())
				return a.value < b.value;
			if (a.prefix.empty())
				return false;
			if (b.prefix.empty())
				return true;
			if (a.prefix < b.prefix)
				return true;
			if (a.prefix > b.prefix)
				return false;
			if (a.definition.empty() && b.definition.empty())
				return false;
			if (a.definition.empty())
				return true;
			if (b.definition.empty())
				return false;
			if (a.definition[0] == '"' && b.definition[0] == '"')
				return a.definition < b.definition;
			if (a.definition[0] == '"')
				return false;
			if (b.definition[0] == '"')
				return true;
			if (a.value < b.value)
				return true;
			if (a.value > b.value)
				return false;
			return false;
		}
	);

	if (g_settings.bUseIDC_STATIC)
	{
		// write the macro definitions
		file.WriteSzA("\r\n");
		WriteMacroLine(file, "IDC_STATIC", "(-1)");
	}

	MStringA prefix;
	bool first = true;
	file.WriteFormatA("\r\n");
	for (auto& def : defs)
	{
		if (def.name == "IDC_STATIC")
			continue;

		if (!first && prefix != def.prefix)
			file.WriteFormatA("\r\n");

		WriteMacroLine(file, def.name, def.definition);

		prefix = def.prefix;
		first = false;
	}

	// do statistics about resource IDs
	UINT anValues[5];
	DoIDStat(anValues);

	// dump the statistics
	file.WriteFormatA("\r\n");
	file.WriteFormatA("#ifdef APSTUDIO_INVOKED\r\n");
	file.WriteFormatA("    #ifndef APSTUDIO_READONLY_SYMBOLS\r\n");
	file.WriteFormatA("        #define _APS_NO_MFC                 %u\r\n", anValues[0]);
	file.WriteFormatA("        #define _APS_NEXT_RESOURCE_VALUE    %u\r\n", anValues[1]);
	file.WriteFormatA("        #define _APS_NEXT_COMMAND_VALUE     %u\r\n", anValues[2]);
	file.WriteFormatA("        #define _APS_NEXT_CONTROL_VALUE     %u\r\n", anValues[3]);
	file.WriteFormatA("        #define _APS_NEXT_SYMED_VALUE       %u\r\n", anValues[4]);
	file.WriteFormatA("    #endif\r\n");
	file.WriteFormatA("#endif\r\n");

	return TRUE;
}

// write the resource.h file
BOOL MMainWnd::DoWriteResHOfExe(LPCWSTR pszExeFile)
{
	assert(pszExeFile);

	// pszExeFile --> szResH
	WCHAR szResH[MAX_PATH];
	StringCchCopyW(szResH, _countof(szResH), pszExeFile);

	// find the last '\\' or '/'
	LPWSTR pch = wcsrchr(szResH, L'\\');
	if (!pch)
		pch = wcsrchr(szResH, L'/');
	if (!pch)
		return FALSE;

	// build the "resource.h" file path
	++pch;
	*pch = 0;
	StringCchCatW(szResH, _countof(szResH), L"resource.h");

	// write the "resource.h" file
	if (DoWriteResH(szResH))
	{
		// szResH --> m_szResourceH
		StringCchCopyW(m_szResourceH, _countof(m_szResourceH), szResH);
		return TRUE;
	}

	return FALSE;   // failure
}

// do statistics for resource IDs
void MMainWnd::DoIDStat(UINT anValues[5])
{
	const size_t count = 4;

	INT anNext[count];
	MString prefixes[count];
	prefixes[0] = g_settings.assoc_map[MapIDType(IDTYPE_RESOURCE)];
	prefixes[1] = g_settings.assoc_map[MapIDType(IDTYPE_COMMAND)];
	prefixes[2] = g_settings.assoc_map[MapIDType(IDTYPE_NEWCOMMAND)];
	prefixes[3] = g_settings.assoc_map[MapIDType(IDTYPE_CONTROL)];

	for (size_t i = 0; i < count; ++i)
	{
		auto table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefixes[i]);

		UINT nMax = 0;
		for (auto& table_entry : table)
		{
			if (table_entry.name == L"IDC_STATIC")
				continue;

			if (i == 3)
			{
				if (g_res.find(ET_LANG, RT_CURSOR, WORD(table_entry.value)))
					continue;   // it was Cursor.ID, not Control.ID
			}

			if (nMax < table_entry.value)
				nMax = table_entry.value;
		}

		anNext[i] = nMax + 1;
	}

	anValues[0] = 1;
	anValues[1] = anNext[0];
#undef max
	anValues[2] = std::max(anNext[1], anNext[2]);
	anValues[3] = anNext[3];
	anValues[4] = 300;

	// fix for preferable values
	if (anValues[1] < 100)
		anValues[1] = 100;
	if (anValues[2] < 100)
		anValues[2] = 100;
	if (anValues[3] < 1000)
		anValues[3] = 1000;
}

// extract the resource data to a file
inline BOOL MMainWnd::DoExtract(const EntryBase *entry, BOOL bExporting)
{
	ResToText res2text;

	if (bExporting && g_settings.bStoreToResFolder)
	{
		// add "res\\" to the prefix if necessary
		res2text.m_strFilePrefix = L"res\\";
	}

	// get the entry file name
	MString filename = res2text.GetEntryFileName(*entry);
	if (filename.empty())
		return TRUE;        // no need to extract

	//MessageBox(NULL, filename.c_str(), NULL, 0);

	if (entry->m_type.is_int())
	{
		WORD wType = entry->m_type.m_id;
		if (wType == (WORD)(UINT_PTR)RT_CURSOR)
		{
			// No output file
			return TRUE;
		}
		if (wType == (WORD)(UINT_PTR)RT_BITMAP)
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (wType == (WORD)(UINT_PTR)RT_ICON)
		{
			// No output file
			return TRUE;
		}
		if (wType == (WORD)(UINT_PTR)RT_MENU)
		{
			// No output file
			return TRUE;
		}
		if (wType == (WORD)(UINT_PTR)RT_TOOLBAR)
		{
			// No output file
			return TRUE;
		}
		if (wType == (WORD)(UINT_PTR)RT_DIALOG)
		{
			// No output file
			return TRUE;
		}
		if (wType == (WORD)(UINT_PTR)RT_STRING)
		{
			// No output file
			return TRUE;
		}
		if (wType == (WORD)(UINT_PTR)RT_FONTDIR)
		{
			return TRUE;
		}
		if (wType == (WORD)(UINT_PTR)RT_FONT)
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (wType == (WORD)(UINT_PTR)RT_ACCELERATOR)
		{
			// No output file
			return TRUE;
		}
		if (wType == (WORD)(UINT_PTR)RT_RCDATA)
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (wType == (WORD)(UINT_PTR)RT_MESSAGETABLE)
		{
			if (g_settings.bUseMSMSGTABLE)
			{
				return g_res.extract_bin(filename.c_str(), entry);
			}
			else
			{
				// No output file
				return TRUE;
			}
		}
		if (wType == (WORD)(UINT_PTR)RT_GROUP_CURSOR)
		{
			return g_res.extract_cursor(filename.c_str(), entry);
		}
		if (wType == (WORD)(UINT_PTR)RT_GROUP_ICON)
		{
			return g_res.extract_icon(filename.c_str(), entry);
		}
		if (wType == (WORD)(UINT_PTR)RT_VERSION)
		{
			// No output file
			return TRUE;
		}
		if (wType == (WORD)(UINT_PTR)RT_DLGINIT)
		{
			// No output file
			return TRUE;
		}
		if (wType == (WORD)(UINT_PTR)RT_DLGINCLUDE)
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (wType == (WORD)(UINT_PTR)RT_PLUGPLAY)
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (wType == (WORD)(UINT_PTR)RT_VXD)
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (wType == (WORD)(UINT_PTR)RT_ANICURSOR)
		{
			return g_res.extract_cursor(filename.c_str(), entry);
		}
		if (wType == (WORD)(UINT_PTR)RT_ANIICON)
		{
			return g_res.extract_icon(filename.c_str(), entry);
		}
	}
	else
	{
		if (entry->m_type == L"AVI")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (entry->m_type == L"PNG")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (entry->m_type == L"GIF")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (entry->m_type == L"JPEG")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (entry->m_type == L"JPG")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (entry->m_type == L"TIFF")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (entry->m_type == L"TIF")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (entry->m_type == L"EMF")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (entry->m_type == L"ENHMETAFILE")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (entry->m_type == L"WMF")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (entry->m_type == L"WAVE")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (entry->m_type == L"MP3")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
		if (entry->m_type == L"IMAGE")
		{
			return g_res.extract_bin(filename.c_str(), entry);
		}
	}

	return g_res.extract_bin(filename.c_str(), entry);
}

// do export the resource data to an RC file and related files
BOOL MMainWnd::DoExportRC(LPCWSTR pszRCFile, LPWSTR pszResHFile)
{
	// search the language entries
	EntrySet found;
	g_res.search(found, ET_LANG);

	return DoExportRC(pszRCFile, pszResHFile, found);
}

BOOL MMainWnd::DoExportRes(LPCWSTR pszResFile)
{
	// search the language entries
	EntrySet found;
	g_res.search(found, ET_LANG);

	if (found.empty())
	{
		// unable to export the empty data
		ErrorBoxDx(IDS_DATAISEMPTY);
		return FALSE;
	}

	return g_res.extract_res(pszResFile, g_res);
}

UINT MMainWnd::DoGetCodePageOfWritingRC() const
{
	return g_settings.nCodePageForRC;
}

// do export the resource data to an RC file and related files
BOOL MMainWnd::DoExportRC(LPCWSTR pszRCFile, LPWSTR pszResHFile, const EntrySet& found)
{
	if (found.empty())
	{
		// unable to export the empty data
		ErrorBoxDx(IDS_DATAISEMPTY);
		return FALSE;
	}

	// pszRCFile --> szPath
	WCHAR szPath[MAX_PATH];
	StringCchCopy(szPath, _countof(szPath), pszRCFile);

	// find the '\\' or '/' character
	WCHAR *pch = mstrrchr(szPath, L'\\');
	if (!pch)
		pch = mstrrchr(szPath, L'/');
	if (!pch)
		return FALSE;   // failure

	*pch = 0;

	// check whether there is an external file to be extracted
	BOOL bHasExternFile = FALSE;
	for (auto e : found)
	{
		ResToText res2text;
		MString filename = res2text.GetEntryFileName(*e);
		if (filename.size())
		{
			bHasExternFile = TRUE;
			break;
		}
	}

	// if g_settings.bStoreToResFolder is set, then check the folder is not empty
	if (!IsEmptyDirectoryDx(szPath))
	{
		if (bHasExternFile && !g_settings.bStoreToResFolder)
		{
			ErrorBoxDx(IDS_MUSTBEEMPTYDIR);
			return FALSE;
		}
	}

	// save the current directory and move the current directory
	WCHAR szCurDir[MAX_PATH];
	GetCurrentDirectory(_countof(szCurDir), szCurDir);
	if (!SetCurrentDirectory(szPath))
		return FALSE;

	if (bHasExternFile)
	{
		// create the "res" folder (with backuping) if necessary
		if (g_settings.bStoreToResFolder)
		{
			MString strResDir = szPath;
			strResDir += TEXT("\\res");

			if (g_settings.bBackup)
				DoBackupFolder(strResDir.c_str());

			CreateDirectory(strResDir.c_str(), NULL);
		}

		// extract each data if necessary
		for (auto e : found)
		{
			if (e->m_type == RT_STRING || e->m_type == RT_FONTDIR)
				continue;
			if (e->m_type == RT_MESSAGETABLE && !g_settings.bUseMSMSGTABLE)
				continue;
			if (e->m_type == L"TEXTINCLUDE")
				continue;
			if (!DoExtract(e, TRUE))
				return FALSE;
		}
	}

	BOOL bOK = FALSE;
	if ((m_szResourceH[0] || !g_settings.IsIDMapEmpty()) &&
		!g_settings.bHideID)
	{
		// build the resource.h file path
		*pch = 0;
		StringCchCatW(szPath, _countof(szPath), L"\\resource.h");

		// write the resource.h file and the RC file
		bOK = DoWriteResH(szPath, pszRCFile) && DoWriteRC(pszRCFile, szPath, found, DoGetCodePageOfWritingRC());

		// szPath --> pszResHFile
		if (bOK && pszResHFile)
			StringCchCopyW(pszResHFile, MAX_PATH, szPath);
	}
	else
	{
		// write the RC file
		bOK = DoWriteRC(pszRCFile, NULL, found, DoGetCodePageOfWritingRC());
	}

	// resume the current directory
	SetCurrentDirectory(szCurDir);

	if (bOK)
	{
		DoSetFileModified(FALSE);
		m_nStatusStringID = IDS_FILESAVED;
	}

	return bOK;
}

// save the resource data as a *.res file
BOOL MMainWnd::DoSaveResAs(LPCWSTR pszResFile)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return FALSE;

	if (g_res.extract_res(pszResFile, g_res))
	{
		UpdateFileInfo(FT_RES, pszResFile, FALSE);
		DoSetFileModified(FALSE);
		return TRUE;
	}
	return FALSE;
}

BOOL MMainWnd::DoSaveFile(HWND hwnd, LPCWSTR pszFile)
{
	LPWSTR pchDotExt = PathFindExtensionW(pszFile);
	if (lstrcmpiW(pchDotExt, L".exe") == 0 ||
		lstrcmpiW(pchDotExt, L".dll") == 0 ||
		lstrcmpiW(pchDotExt, L".ocx") == 0 ||
		lstrcmpiW(pchDotExt, L".cpl") == 0 ||
		lstrcmpiW(pchDotExt, L".scr") == 0 ||
		lstrcmpiW(pchDotExt, L".mui") == 0 ||
		lstrcmpiW(pchDotExt, L".ime") == 0)
	{
		return DoSaveExeAs(pszFile);
	}
	if (lstrcmpiW(pchDotExt, L".rc") == 0 || lstrcmpiW(pchDotExt, L".rc2") == 0)
		return DoExportRC(pszFile, NULL);
	if (lstrcmpiW(pchDotExt, L".res") == 0)
		return DoSaveResAs(pszFile);
	if (*pchDotExt == 0)
	{
		WCHAR szPath[MAX_PATH];
		StringCbCopyW(szPath, sizeof(szPath), pszFile);
		PathAddExtensionW(szPath, L".rc");
		return DoExportRC(szPath, NULL);
	}
	return DoSaveExeAs(pszFile);
}

// save the file
BOOL MMainWnd::DoSaveAs(LPCWSTR pszExeFile)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return TRUE;

	return DoSaveExeAs(pszExeFile);
}

BOOL IsExeOrDll(LPCWSTR pszFileName)
{
	BYTE ab[2] = { 0, 0 };
	if (FILE *fp = _wfopen(pszFileName, L"rb"))
	{
		fread(ab, 2, 1, fp);
		fclose(fp);
	}

	if (ab[0] == 'M' && ab[1] == 'Z')
		return TRUE;
	if (ab[0] == 'P' && ab[1] == 'E')
		return TRUE;
	return FALSE;
}

BOOL IsDotExe(LPCWSTR pszFileName)
{
	return lstrcmpiW(PathFindExtensionW(pszFileName), L".exe") == 0;
}

BOOL DumpTinyExeOrDll(HINSTANCE hInst, LPCWSTR pszFileName, INT nID)
{
	if (HRSRC hRsrc = FindResourceW(hInst, MAKEINTRESOURCEW(nID), RT_RCDATA))
	{
		if (HGLOBAL hGlobal = LoadResource(hInst, hRsrc))
		{
			if (LPVOID pvData = LockResource(hGlobal))
			{
				DWORD cbData = SizeofResource(hInst, hRsrc);
				if (FILE *fp = _wfopen(pszFileName, L"wb"))
				{
					size_t nOK = fwrite(pvData, cbData, 1, fp);
					fflush(fp);
					fclose(fp);

					return !!nOK;
				}
			}
		}
	}
	return FALSE;
}

BOOL DoResetCheckSum(LPCWSTR pszExeFile)
{
	MByteStreamEx stream;
	if (!stream.LoadFromFile(pszExeFile))
	{
		assert(0);
		return FALSE;
	}

	if (stream.size() <= sizeof(IMAGE_DOS_SIGNATURE))
	{
		assert(0);
		return FALSE;
	}

	auto dos = stream.pointer<IMAGE_DOS_HEADER>();
	IMAGE_NT_HEADERS *nt;
	if (dos && dos->e_magic == IMAGE_DOS_SIGNATURE && dos->e_lfanew != 0)
		nt = stream.pointer<IMAGE_NT_HEADERS>(dos->e_lfanew);
	else
		nt = stream.pointer<IMAGE_NT_HEADERS>();

	if (!nt || nt->Signature != IMAGE_NT_SIGNATURE)
	{
		assert(0);
		return FALSE;
	}

	IMAGE_NT_HEADERS32 *nt32 = reinterpret_cast<IMAGE_NT_HEADERS32 *>(nt);
	IMAGE_NT_HEADERS64 *nt64 = reinterpret_cast<IMAGE_NT_HEADERS64 *>(nt);

	IMAGE_FILE_HEADER *file = &nt->FileHeader;
	IMAGE_OPTIONAL_HEADER32 *optional32 = NULL;
	IMAGE_OPTIONAL_HEADER64 *optional64 = NULL;

	switch (file->SizeOfOptionalHeader)
	{
	case sizeof(IMAGE_OPTIONAL_HEADER32):
		optional32 = &nt32->OptionalHeader;
		if (optional32->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			return FALSE;
		optional32->CheckSum = 0;
		break;

	case sizeof(IMAGE_OPTIONAL_HEADER64):
		optional64 = &nt64->OptionalHeader;
		if (optional64->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			return FALSE;
		optional64->CheckSum = 0;
		break;

	default:
		assert(0);
		return FALSE;
	}

	return stream.SaveToFile(pszExeFile);
}

BOOL MMainWnd::DoSaveInner(LPCWSTR pszExeFile)
{
	// File might be updated. Wait for virus scan
	WaitForVirusScan(pszExeFile, 10 * 1000);

	// src is not exe and dest exe is respected
	DoResetCheckSum(pszExeFile);

	// The executable is updated. Wait for virus scan
	WaitForVirusScan(pszExeFile, 10 * 1000);

	BOOL bEnableCrypt = FALSE;
#ifdef ENABLE_CRYPTO
	if (g_settings.bUseWonRes && g_bEnableCrypto && g_password.size())
	{
		switch (MsgBoxDx(LoadStringDx(IDS_WANTCRYPTO), MB_YESNOCANCEL))
		{
		case IDYES:
			bEnableCrypt = TRUE;
			break;
		case IDNO:
			break;
		case IDCANCEL:
			DeleteFileW(pszExeFile);
			return TRUE;
		}
	}
#else
	BOOL bEnableCrypt = FALSE;
#endif
	if (!g_res.update_exe(pszExeFile, bEnableCrypt))
		return FALSE;

	// Now the executable is updated. Wait a little for virus checker.
	Sleep(300);

	if (g_settings.bCompressByUPX || m_bUpxCompressed)
	{
		if (MsgBoxDx(IDS_COMPRESSBYUPX, MB_ICONINFORMATION | MB_YESNO) == IDYES)
			DoUpxCompress(m_szUpxExe, pszExeFile);
		else
			m_bUpxCompressed = FALSE;
	}

	// update file info
	UpdateFileInfo(FT_EXECUTABLE, pszExeFile, m_bUpxCompressed);

	// Notify change of file icon
	MyChangeNotify(pszExeFile);

	// is there any resource ID?
	if (m_szResourceH[0] || !g_settings.id_map.empty())
	{
		// query
		if (MsgBoxDx(IDS_WANNAGENRESH, MB_ICONINFORMATION | MB_YESNO) == IDYES)
		{
			// write the resource.h file
			return DoWriteResHOfExe(pszExeFile);
		}
	}

	DoSetFileModified(FALSE);

	return TRUE;    // success
}

// open the dialog to save the EXE file
BOOL MMainWnd::DoSaveExeAs(LPCWSTR pszExeFile)
{
	LPCWSTR src = m_szFile;
	LPCWSTR dest = pszExeFile;
	WCHAR szTempFile[MAX_PATH] = L"";
	AutoDeleteFileW ad(szTempFile);

	DWORD error = IsFileLockedDx(dest);
	if (error == ERROR_SHARING_VIOLATION)
	{
		WCHAR szMsg[MAX_PATH + 256];
		StringCchPrintfW(szMsg, _countof(szMsg), LoadStringDx(IDS_CANTWRITEBYLOCK), dest);
		ErrorBoxDx(szMsg);
		return FALSE;
	}
	if (error == ERROR_ACCESS_DENIED)
	{
		WCHAR szMsg[MAX_PATH + 256];
		StringCchPrintfW(szMsg, _countof(szMsg), LoadStringDx(IDS_NOFILEACCESS), dest);
		ErrorBoxDx(szMsg);
		return FALSE;
	}

	// is file UPX-compressed?
	if (m_bUpxCompressed)
	{
		// build a temporary file path
		StringCchCopyW(szTempFile, _countof(szTempFile), GetTempFileNameDx(L"UPX"));

		// src --> szTempFile (decompressed)
		if (!CopyFileW(src, szTempFile, FALSE) ||
			!DoUpxDecompress(m_szUpxExe, szTempFile))
		{
			ErrorBoxDx(IDS_CANTUPXEXTRACT);
			return FALSE;   // failure
		}

		src = szTempFile;

		// Now the executable is updated. Wait a little for virus checker.
		Sleep(300);
	}

	// do backup the dest
	if (g_settings.bBackup)
	{
		DoBackupFile(dest);

		// Now the executable is updated. Wait a little for virus checker.
		Sleep(300);
	}

	// check whether it is an executable or not
	BOOL bSrcExecutable = IsExeOrDll(src);
	BOOL bDestExecutable = IsExeOrDll(dest);

	if (bSrcExecutable)
	{
		// copy src to dest (if src and dest are not same), then update resource
		if (lstrcmpiW(src, dest) == 0 ||
			CopyFileW(src, dest, FALSE))
		{
			return DoSaveInner(dest);
		}
	}
	else if (bDestExecutable)
	{
		return DoSaveInner(dest);
	}
	else
	{
		// if src and dest are non-executable, then dump tiny exe or dll to dest
		if (IsDotExe(dest))
		{
#ifdef _WIN64
			if (DumpTinyExeOrDll(m_hInst, dest, IDR_TINYEXE64))
#else
			if (DumpTinyExeOrDll(m_hInst, dest, IDR_TINYEXE))
#endif
			{
				return DoSaveInner(dest);
			}
		}
		else
		{
#ifdef _WIN64
			if (DumpTinyExeOrDll(m_hInst, dest, IDR_TINYDLL64))
#else
			if (DumpTinyExeOrDll(m_hInst, dest, IDR_TINYDLL))
#endif
			{
				return DoSaveInner(dest);
			}
		}
	}

	return FALSE;   // failure
}

// do compress the file by UPX.exe
BOOL MMainWnd::DoUpxCompress(LPCWSTR pszUpx, LPCWSTR pszExeFile)
{
	// build the command line
	MStringW strCmdLine;
	strCmdLine += L"\"";
	strCmdLine += pszUpx;
	strCmdLine += L"\" -9 \"";
	strCmdLine += pszExeFile;
	strCmdLine += L"\"";
	//MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

	BOOL bOK = FALSE;

	// create a UPX process
	MProcessMaker pmaker;
	pmaker.SetCreationFlags(CREATE_NO_WINDOW);

	MFile hInputWrite, hOutputRead;
	MStringA strOutput;
	if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
		pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
	{
		// read all output
		bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
		pmaker.WaitForSingleObject(PROCESS_TIMEOUT);
		//MessageBoxA(NULL, strOutput.c_str(), NULL, 0);

		if (pmaker.GetExitCode() == 0 && bOK)
		{
			bOK = (strOutput.find("Packed") != MStringA::npos);
		}
	}

	if (bOK)
		m_bUpxCompressed = TRUE;
	else
		MessageBoxA(m_hwnd, strOutput.c_str(), "UPX", MB_ICONERROR);

	return bOK;
}

IMPORT_RESULT MMainWnd::DoImportRC(HWND hwnd, LPCWSTR pszFile)
{
	MWaitCursor wait;

	// load the RC file to the res variable
	EntrySet res;
	if (!DoLoadRCEx(hwnd, pszFile, res, FALSE))
	{
		ErrorBoxDx(IDS_CANNOTIMPORT);
		return IMPORT_FAILED;
	}

	bool found = false;
	for (auto entry : res)
	{
		if (entry->m_et != ET_LANG)
			continue;   // we will merge the ET_LANG entries only

		if (g_res.find(ET_LANG, entry->m_type, entry->m_name, entry->m_lang))
		{
			found = true;
			break;
		}
	}

	if (found)
	{
		if (MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL) != IDYES)
			return IMPORT_CANCELLED;
	}

	// load it now
	m_bLoading = TRUE;
	{
		ShowTreeViewArrow(FALSE);

		// merge
		g_res.merge(res);

		// clean up
		res.delete_all();
	}
	m_bLoading = FALSE;

	// select none
	SelectTV(NULL, FALSE);

	// update drop-down arrow
	PostUpdateArrow(hwnd);

	DoSetFileModified(TRUE);
	return IMPORTED;
}

IMPORT_RESULT MMainWnd::DoImportRes(HWND hwnd, LPCWSTR pszFile)
{
	// do import to the res variable
	EntrySet res;
	if (!res.import_res(pszFile))
	{
		return IMPORT_FAILED;
	}

	// is it overlapped?
	if (g_res.intersect(res))
	{
		// query overwrite
		INT nID = MsgBoxDx(IDS_EXISTSOVERWRITE, MB_ICONINFORMATION | MB_YESNOCANCEL);
		switch (nID)
		{
		case IDYES:
			// delete the overlapped entries
			for (auto entry : res)
			{
				if (entry->m_et != ET_LANG)
					continue;

				g_res.search_and_delete(ET_LANG, entry->m_type, entry->m_name, entry->m_lang);
			}
			break;

		case IDNO:
		case IDCANCEL:
			// clean up
			res.delete_all();
			return IMPORT_CANCELLED;
		}
	}

	// load it now
	m_bLoading = TRUE;
	{
		ShowTreeViewArrow(FALSE);

		// renewal
		g_res.merge(res);

		// clean up
		res.delete_all();
	}
	m_bLoading = FALSE;

	// refresh the ID list window
	DoRefreshIDList(hwnd);

	// update drop-down arrow
	PostUpdateArrow(hwnd);

	return IMPORTED;
}

// WM_DROPFILES: file(s) has been dropped
void MMainWnd::OnDropFiles(HWND hwnd, HDROP hdrop)
{
	if (IsWindow(m_splash))
		SendMessageW(m_splash, WM_CLOSE, 0, 0);

	MWaitCursor wait;
	ChangeStatusText(IDS_EXECUTINGCMD);     // executing command

	// compile if necessary
	if (!CompileIfNecessary(FALSE))
	{
		ChangeStatusText(IDS_READY);
		return;
	}

	// add the command lock
	++m_nCommandLock;

	WCHAR file[MAX_PATH], *pch;

	// get the dropped file path
	DragQueryFileW(hdrop, 0, file, _countof(file));

	// free hdrop
	DragFinish(hdrop);

	// make the window foreground
	SetForegroundWindow(hwnd);

	// find the dot extension
	pch = PathFindExtensionW(file);

	IMPORT_RESULT result = NOT_IMPORTABLE;
	if (pch && (lstrcmpiW(pch, L".rc") != 0 && lstrcmpiW(pch, L".rc2") != 0))
	{
		result = DoImport(hwnd, file, pch);
	}

	if (result == IMPORTED || result == IMPORT_CANCELLED)
	{
		// do nothing
	}
	else if (result == IMPORT_FAILED)
	{
		ErrorBoxDx(IDS_CANNOTIMPORT);
	}
	else if (pch && lstrcmpiW(pch, L".h") == 0)
	{
		// unload the resource.h file
		UnloadResourceH(hwnd);

		CheckResourceH(hwnd, file);
		if (m_szResourceH[0] && g_settings.bAutoShowIDList)
		{
			ShowIDList(hwnd, TRUE);
		}

		// update the names
		UpdateNames();
	}
	else
	{
		if (!DoQuerySaveChange(hwnd))
			return;

		// otherwise, load the file
		DoLoadFile(hwnd, file);
	}

	// update drop-down arrow
	PostUpdateArrow(hwnd);

	// remove the command lock
	--m_nCommandLock;

	// show "ready" if just unlocked
	if (m_nCommandLock == 0 && !::IsWindow(m_rad_window))
		ChangeStatusText(IDS_READY);
}

// WM_CLOSE
void MMainWnd::OnClose(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	if (DoQuerySaveChange(hwnd))
		DestroyWindow(hwnd);
}

// WM_DESTROY: the main window has been destroyed
void MMainWnd::OnDestroy(HWND hwnd)
{
	// Try to cancel searching
	m_search.bCancelled = TRUE;

	StopMP3();
	StopAvi();

	// release auto complete
	DoTVEditAutoCompleteRelease(hwnd);

	// close preview
	HidePreview();

	// unload the resource.h file
	OnUnloadResH(hwnd);

	// update the file info
	UpdateFileInfo(FT_NONE, NULL, FALSE);

	// unselect
	SelectTV(NULL, FALSE);

	// save the settings
	SaveSettings(hwnd);

	//DestroyIcon(m_hIcon);     // LR_SHARED
	DestroyIcon(m_hIconSm);
	DestroyAcceleratorTable(m_hAccel);
	ImageList_Destroy(m_hImageList);
	ImageList_Destroy(m_himlTools);
	DestroyIcon(m_hFileIcon);
	DestroyIcon(m_hFolderIcon);
	DeleteObject(m_hSrcFont);
	DeleteObject(m_hBinFont);

	//DestroyIcon(MRadCtrl::Icon());    // LR_SHARED
	DeleteObject(MRadCtrl::Bitmap());
	DestroyCursor(MSplitterWnd::CursorNS());
	DestroyCursor(MSplitterWnd::CursorWE());

	// clean up
	g_res.delete_all();
	g_res.delete_invalid();

	HWND hHexViewer = m_hHexViewer;
	m_hHexViewer.UnsubclassDx();
	DestroyWindow(hHexViewer);

	DestroyRadWindow();
	DestroyWindow(m_hCodeEditor);
	m_hBmpView.DestroyView();
	DestroyWindow(m_hBmpView);
	DestroyWindow(m_id_list_dlg);

	DestroyWindow(s_hwndEga);
	s_hwndEga = NULL;

	DestroyWindow(m_hwndTV);
	DestroyWindow(m_hToolBar);
	DestroyWindow(m_hStatusBar);
	DestroyWindow(m_hFindReplaceDlg);

	DestroyWindow(m_splitter1);
	DestroyWindow(m_splitter2);

	g_hMainWnd = NULL;

	// post WM_QUIT message to quit the application
	PostQuitMessage(0);
}

// parse the macros
BOOL MMainWnd::ParseMacros(HWND hwnd, LPCTSTR pszFile,
						   const std::vector<MStringA>& macros, MStringA& str)
{
	// split text to lines by "\n"
	std::vector<MStringA> lines;
	mstr_trim(str);
	mstr_split(lines, str, "\n");

	// erase the first line (the special pragma)
	lines.erase(lines.begin());

	// check the line count
	size_t line_count = lines.size();
	if (macros.size() < line_count)
		line_count = macros.size();

	for (size_t i = 0; i < line_count; ++i)
	{
		auto& macro = macros[i];
		auto& line = lines[i];

		// scan the line lexically
		using namespace MacroParser;
		StringScanner scanner(line);

		// tokenize it
		MacroParser::TokenStream stream(scanner);
		stream.read_tokens();

		// and parse it
		Parser parser(stream);
		if (parser.parse())     // successful
		{
			if (is_str(parser.ast()))
			{
				// it's a string value macro
				string_type value;
				if (eval_string(parser.ast(), value))   // evaluate
				{
					// add an ID mapping's pair
					g_settings.id_map[macro] = std::move(value);
				}
			}
			else
			{
				// it's an integer value macro
				int value = 0;
				if (eval_int(parser.ast(), value))  // evaluate
				{
					// convert to a string by base m_id_list_dlg.m_nBase
					CHAR sz[MAX_PATH];
					if (m_id_list_dlg.m_nBase == 16)
						StringCchPrintfA(sz, _countof(sz), "0x%X", value);
					else
						StringCchPrintfA(sz, _countof(sz), "%d", value);

					// ignore some special macros
					if (macro != "WIN32" && macro != "WINNT" && macro != "i386")
					{
						// add an ID mapping's pair
						g_settings.id_map[macro] = sz;
					}
				}
			}
		}
	}

	// clear the resource IDs in the constants database
	auto& table = g_db.m_map[L"RESOURCE.ID"];
	table.clear();

	// add the resource ID entries to the "RESOURCE.ID" table from the ID mapping
	for (auto& pair : g_settings.id_map)
	{
		MStringW str1 = MAnsiToWide(CP_ACP, pair.first).c_str();
		MStringW str2 = MAnsiToWide(CP_ACP, pair.second).c_str();
		DWORD value2 = mstr_parse_int(str2.c_str());
		ConstantsDB::EntryType entry(str1, value2);
		table.push_back(entry);
	}

	// add IDC_STATIC macro
	g_settings.AddIDC_STATIC();

	// save the resource.h file location to m_szResourceH
	StringCchCopyW(m_szResourceH, _countof(m_szResourceH), pszFile);

	return TRUE;
}

// parse the resource.h file
BOOL MMainWnd::ParseResH(HWND hwnd, LPCTSTR pszFile, const char *psz, DWORD len)
{
	// split text to lines by "\n"
	MStringA str(psz, len);
	std::vector<MStringA> lines, macros;
	mstr_split(lines, str, "\n");

	const size_t lenDefine = strlen("#define");
	for (auto& line : lines)
	{
		// trim
		mstr_trim(line);
		if (line.empty())
			continue;   // empty ones are ignored

		// the macro that begins with "_" will be ignored
		if (line.find("#define _") != MStringA::npos)
			continue;

		// find "#define "
		size_t found0 = line.find("#define");
		if (found0 == MStringA::npos)
			continue;

		// parse the line
		line = line.substr(lenDefine);
		mstr_trim(line);
		size_t found1 = line.find_first_of(" \t");
		size_t found2 = line.find('(');
		if (found1 == MStringA::npos)
			continue;   // without space is an invalid #define
		if (found2 != MStringA::npos && found2 < found1)
			continue;   // we ignore the function-like macros

		// push the macro
		macros.push_back(line.substr(0, found1));
	}

	g_settings.id_map.clear();

	if (macros.empty())
	{
		// no macros to
		return TRUE;
	}

	// (new temporary file path) --> szTempFile1
	WCHAR szTempFile1[MAX_PATH];
	StringCchCopyW(szTempFile1, _countof(szTempFile1), GetTempFileNameDx(L"R1"));

	AutoDeleteFileW adf1(szTempFile1);

	// create the temporary file
	MFile file1(szTempFile1, TRUE);

	// pszFile --> szFile
	WCHAR szFile[MAX_PATH];
	StringCchCopyW(szFile, _countof(szFile), pszFile);

	// write the heading
	DWORD cbWritten;
	file1.WriteSzA("#include \"", &cbWritten);
	file1.WriteSzA(MTextToAnsi(CP_ACP, szFile).c_str(), &cbWritten);
	file1.WriteSzA("\"\n", &cbWritten);
	file1.WriteSzA("#pragma macros\n", &cbWritten);    // the special pragma

	// write the macro names (in order to retrieve the value after)
	char buf[MAX_PATH + 64];
	for (size_t i = 0; i < macros.size(); ++i)
	{
		StringCchPrintfA(buf, _countof(buf), "%s\n", macros[i].c_str());
		file1.WriteSzA(buf, &cbWritten);
	}
	file1.FlushFileBuffers();
	file1.CloseHandle();    // close the handle

	// build the command line text
	MString strCmdLine;
	strCmdLine += L'\"';
	strCmdLine += m_szMCppExe;       // mcpp.exe
	strCmdLine += L"\" ";
	strCmdLine += GetIncludesDump();
	strCmdLine += GetMacroDump();
	strCmdLine += L" \"";
	strCmdLine += szTempFile1;
	strCmdLine += L'\"';
	//MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

	BOOL bOK = FALSE;

	// create a cpp.exe process
	MProcessMaker pmaker;
	pmaker.SetCreationFlags(CREATE_NO_WINDOW);

	MStringA strOutput;
	MFile hInputWrite, hOutputRead;
	if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
		pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
	{
		// read all with timeout
		bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
		pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

		if (bOK)
		{
			bOK = FALSE;

			// find the special pragma
			size_t pragma_found = strOutput.find("#pragma macros");
			if (pragma_found != MStringA::npos)
			{
				// get text after the special pragma
				strOutput = strOutput.substr(pragma_found);

				// parse macros
				bOK = ParseMacros(hwnd, pszFile, macros, strOutput);
			}
		}
	}

	return bOK;
}

// load the resource.h
BOOL MMainWnd::DoLoadResH(HWND hwnd, LPCTSTR pszFile)
{
	// unload the resource.h file
	UnloadResourceH(hwnd);

	// (new temporary file path) --> szTempFile
	WCHAR szTempFile[MAX_PATH];
	StringCchCopyW(szTempFile, _countof(szTempFile), GetTempFileNameDx(L"R1"));

	// create a temporary file
	MFile file(szTempFile, TRUE);
	file.CloseHandle();     // close the handle

	AutoDeleteFileW adf1(szTempFile);

	// build a command line
	MString strCmdLine;
	strCmdLine += L'"';
	strCmdLine += m_szMCppExe;
	strCmdLine += L"\" -dM -DRC_INVOKED -o \"";
	strCmdLine += szTempFile;
	strCmdLine += L"\" \"-I";
	strCmdLine += m_szIncludeDir;
	strCmdLine += L"\" \"";
	strCmdLine += pszFile;
	strCmdLine += L"\"";
	//MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

	BOOL bOK = FALSE;

	// create a cpp.exe process
	MProcessMaker pmaker;
	pmaker.SetCreationFlags(CREATE_NO_WINDOW);

	MStringA strOutput;
	MFile hInputWrite, hOutputRead;
	if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
		pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
	{
		// read all with timeout
		bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
		pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

		if (bOK && pmaker.GetExitCode() == 0)
		{
			// read all from szTempFile
			MStringA data;
			MFile file(szTempFile);
			bOK = file.ReadAll(data);
			file.CloseHandle();

			if (bOK)
			{
				// parse the resource.h
				bOK = ParseResH(hwnd, pszFile, &data[0], DWORD(data.size()));
			}
		}
	}

	return bOK;
}

// refresh the ID list window
void MMainWnd::DoRefreshIDList(HWND hwnd)
{
	if (IsWindow(m_id_list_dlg))
		::SendMessageW(m_id_list_dlg, MYWM_REFRESHIDLIST, 0, 0);
}

// refresh the treeview
void MMainWnd::DoRefreshTV(HWND hwnd, BOOL bNoDeleteAll)
{
	if (bNoDeleteAll)
	{
		DoRefreshTVEntryNames(hwnd);
	}
	else
	{
		// refresh the resource items
		EntrySet res;
		res.merge(g_res);

		SendMessageW(m_hwndTV, WM_SETREDRAW, FALSE, 0);
		g_res.delete_all();
		g_res.merge(res);
		SendMessageW(m_hwndTV, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(m_hwndTV, NULL, TRUE);

		// clean up
		res.delete_all();
	}

	// hide language drop-down arrow
	ShowWindowAsync(m_arrow, SW_HIDE);

	// redraw
	InvalidateRect(m_hwndTV, NULL, TRUE);
}

// reset the path settings
void MMainWnd::ReSetPaths(HWND hwnd)
{
	// windres.exe
	if (g_settings.strWindResExe.size())
	{
		// g_settings.strWindResExe --> m_szWindresExe
		StringCchCopyW(m_szWindresExe, _countof(m_szWindresExe), g_settings.strWindResExe.c_str());
	}
	else
	{
		// g_settings.m_szDataFolder + L"\\bin\\windres.exe" --> m_szWindresExe
		StringCchCopyW(m_szWindresExe, _countof(m_szWindresExe), m_szDataFolder);
		StringCchCatW(m_szWindresExe, _countof(m_szWindresExe), L"\\bin\\windres.exe");
	}

	// cpp.exe
	if (g_settings.strCppExe.size())
	{
		// g_settings.strCppExe --> m_szMCppExe
		StringCchCopy(m_szMCppExe, _countof(m_szMCppExe), g_settings.strCppExe.c_str());
	}
	else
	{
		// m_szDataFolder + "\\bin\\cpp.exe" --> m_szMCppExe
		StringCchCopyW(m_szMCppExe, _countof(m_szMCppExe), m_szDataFolder);
		StringCchCatW(m_szMCppExe, _countof(m_szMCppExe), L"\\bin\\mcpp.exe");
	}
}

// update the name of the tree control
void MMainWnd::UpdateNames(BOOL bModified)
{
	EntrySet found;
	g_res.search(found, ET_NAME);

	for (auto entry : found)
	{
		UpdateEntryName(entry);
	}

	if (bModified)
		DoSetFileModified(TRUE);
}

void MMainWnd::UpdateEntryType(EntryBase *e, LPWSTR pszText)
{
	// update name label
	e->m_strLabel = e->get_type_label();

	// set the label text
	TV_ITEM item;
	ZeroMemory(&item, sizeof(item));
	item.mask = TVIF_TEXT | TVIF_HANDLE;
	item.hItem = e->m_hItem;
	item.pszText = &e->m_strLabel[0];
	TreeView_SetItem(m_hwndTV, &item);

	// update pszText if any
	if (pszText)
		StringCchCopyW(pszText, MAX_PATH, item.pszText);
}

void MMainWnd::UpdateEntryName(EntryBase *e, LPWSTR pszText)
{
	// update name label
	e->m_strLabel = e->get_name_label();

	// set the label text
	TV_ITEM item;
	ZeroMemory(&item, sizeof(item));
	item.mask = TVIF_TEXT | TVIF_HANDLE;
	item.hItem = e->m_hItem;
	item.pszText = &e->m_strLabel[0];
	TreeView_SetItem(m_hwndTV, &item);

	// update pszText if any
	if (pszText)
		StringCchCopyW(pszText, MAX_PATH, item.pszText);
}

void MMainWnd::UpdateEntryLang(EntryBase *e, LPWSTR pszText)
{
	// update lang label
	e->m_strLabel = e->get_lang_label();

	// set the label text
	TV_ITEM item;
	ZeroMemory(&item, sizeof(item));
	item.mask = TVIF_TEXT | TVIF_HANDLE;
	item.hItem = e->m_hItem;
	item.pszText = &e->m_strLabel[0];
	TreeView_SetItem(m_hwndTV, &item);

	// update pszText if any
	if (pszText)
		StringCchCopyW(pszText, MAX_PATH, item.pszText);

	DoSetFileModified(TRUE);
}

// show/hide the ID list window
void MMainWnd::ShowIDList(HWND hwnd, BOOL bShow/* = TRUE*/)
{
	if (bShow)
	{
		if (IsWindow(m_id_list_dlg))
			DestroyWindow(m_id_list_dlg);
		m_id_list_dlg.CreateDialogDx(hwnd);
		ShowWindow(m_id_list_dlg, (g_bNoGuiMode ? SW_HIDE : SW_SHOWNOACTIVATE));
		UpdateWindow(m_id_list_dlg);
	}
	else
	{
		ShowWindow(m_id_list_dlg, SW_HIDE);
		DestroyWindow(m_id_list_dlg);
	}
}

void MMainWnd::DoRefreshTVEntryNames(HWND hwnd)
{
	EntrySet found;
	g_res.search(found, ET_NAME);
	for (auto entry : found)
	{
		UpdateEntryName(entry);
	}
}

PCSTR MMainWnd::GetTreeItemHelp(EntryBase *entry)
{
	if (entry->m_type == RT_CURSOR)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/resource-types";
	if (entry->m_type == RT_BITMAP)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/bitmap-resource";
	if (entry->m_type == RT_ICON)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/resource-types";
	if (entry->m_type == RT_MENU)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/menu-resource";
	if (entry->m_type == RT_DIALOG)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/dialog-resource";
	if (entry->m_type == RT_STRING)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/stringtable-resource";
	if (entry->m_type == RT_FONT)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/font-resource";
	if (entry->m_type == RT_FONTDIR)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/resource-types";
	if (entry->m_type == RT_ACCELERATOR)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/accelerators-resource";
	if (entry->m_type == RT_RCDATA)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/rcdata-resource";
	if (entry->m_type == RT_MESSAGETABLE)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/messagetable-resource";
	if (entry->m_type == RT_GROUP_CURSOR)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/cursor-resource";
	if (entry->m_type == RT_GROUP_ICON)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/icon-resource";
	if (entry->m_type == RT_VERSION)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource";
	if (entry->m_type == RT_DLGINCLUDE)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/resource-types";
	if (entry->m_type == RT_PLUGPLAY)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/resource-types";
	if (entry->m_type == RT_VXD)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/resource-types";
	if (entry->m_type == RT_ANICURSOR)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/resource-types";
	if (entry->m_type == RT_ANIICON)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/resource-types";
	if (entry->m_type == RT_HTML)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/html-resource";
	if (entry->m_type == RT_MANIFEST)
		return "https://learn.microsoft.com/en-us/windows/win32/menurc/resource-types";
	if (entry->m_type == RT_DLGINIT)
		return "https://learn.microsoft.com/en-us/cpp/mfc/tn024-mfc-defined-messages-and-resources?view=msvc-170#rt_dlginit-resource-format";
	if (entry->m_type == RT_TOOLBAR)
		return "https://github.com/katahiromz/RisohEditor/blob/master/win32-samples/ToolbarTest/ToolbarTest.cpp";
	if (entry->m_type == L"PNG")
		return "https://en.wikipedia.org/wiki/PNG";
	if (entry->m_type == L"GIF")
		return "https://en.wikipedia.org/wiki/GIF";
	if (entry->m_type == L"JPEG" || entry->m_type == L"JPG")
		return "https://en.wikipedia.org/wiki/JPEG";
	if (entry->m_type == L"TIFF" || entry->m_type == L"TIF")
		return "https://en.wikipedia.org/wiki/TIFF";
	if (entry->m_type == L"EMF" || entry->m_type == L"ENHMETAFILE" || entry->m_type == L"ENHMETAPICT")
		return "https://en.wikipedia.org/wiki/Windows_Metafile#EMF";
	if (entry->m_type == L"WMF")
		return "https://en.wikipedia.org/wiki/Windows_Metafile";
	if (entry->m_type == L"WAVE")
		return "https://en.wikipedia.org/wiki/WAV";
	if (entry->m_type == L"MP3")
		return "https://en.wikipedia.org/wiki/MP3";
	if (entry->m_type == L"AVI")
		return "https://en.wikipedia.org/wiki/Audio_Video_Interleave";
	if (entry->m_type == L"TYPELIB")
		return "https://learn.microsoft.com/en-us/previous-versions/windows/desktop/automat/contents-of-a-type-library";
	if (entry->m_type == L"TEXTINCLUDE")
		return "https://learn.microsoft.com/en-us/cpp/mfc/tn035-using-multiple-resource-files-and-header-files-with-visual-cpp#_mfcnotes_tn035_set_includes";
	return NULL;
}

// expand the treeview items
void MMainWnd::Expand(HTREEITEM hItem)
{
	TreeView_Expand(m_hwndTV, hItem, TVE_EXPAND);
	hItem = TreeView_GetChild(m_hwndTV, hItem);
	if (hItem == NULL)
		return;
	do
	{
		Expand(hItem);
		hItem = TreeView_GetNextSibling(m_hwndTV, hItem);
	} while (hItem);
}

void MMainWnd::UpdateArrow()
{
	EntryBase *entry = g_res.get_entry();
	if (!entry)
	{
		HTREEITEM hItem = TreeView_GetSelection(m_hwndTV);
		ShowTreeViewArrow(FALSE, hItem);
		return;
	}

	HTREEITEM hItem = entry->m_hItem;

	switch (entry->m_et)
	{
	case ET_TYPE:
		ShowTreeViewArrow(TRUE, hItem);
		break;
	case ET_NAME:
		ShowTreeViewArrow(TRUE, hItem);
		break;
	case ET_LANG:
		if (entry->m_type != RT_STRING)
			ShowTreeViewArrow(TRUE, hItem);
		else
			ShowTreeViewArrow(FALSE, hItem);
		break;
	case ET_STRING:
		ShowTreeViewArrow(TRUE, hItem);
		break;
	default:
		ShowTreeViewArrow(FALSE, hItem);
		break;
	}
}

// unexpand the treeview items
void MMainWnd::Collapse(HTREEITEM hItem)
{
	TreeView_Expand(m_hwndTV, hItem, TVE_COLLAPSE);
	hItem = TreeView_GetChild(m_hwndTV, hItem);
	if (hItem == NULL)
		return;
	do
	{
		Collapse(hItem);
		hItem = TreeView_GetNextSibling(m_hwndTV, hItem);
	} while (hItem);
}

// get the resource type from text
MIdOrString GetTypeFromText(const WCHAR *pszText)
{
	// pszText --> szText
	WCHAR szText[MAX_PATH];
	StringCchCopyW(szText, _countof(szText), pszText);
	mstr_trim(szText, L" \t\r\n　");

	// replace the fullwidth characters with halfwidth characters
	ReplaceFullWithHalf(szText);

	if (szText[0] == 0)
	{
		return (WORD)0;     // empty
	}
	else if (mchr_is_digit(szText[0]) || szText[0] == L'-' || szText[0] == L'+')
	{
		// numeric
		return WORD(mstr_parse_int(szText));
	}
	else
	{
		// string value
		MStringW str = szText;

		// is there parenthesis?
		size_t i = str.rfind(L'('); // ')'
		if (i != MStringW::npos && mchr_is_digit(str[i + 1]))
		{
			// parse the text after the last parenthesis
			return WORD(mstr_parse_int(&str[i + 1]));
		}

		INT value1 = g_db.GetValue(L"RESOURCE", str);
		if (value1)
			return (WORD)value1;

		// Make it Uppercase
		mstr_upper(str);

		// Retry
		INT value2 = g_db.GetValue(L"RESOURCE", str);
		if (value2)
			return (WORD)value2;

		if (str[0] == L'"') // Quoted?
			mstr_unquote(str); // Unquote

		// string
		return MIdOrString(str.c_str());
	}
}

// get the resource name from text
MIdOrString GetNameFromText(const WCHAR *pszText)
{
	// pszText --> szText
	WCHAR szText[MAX_PATH];
	StringCchCopyW(szText, _countof(szText), pszText);
	mstr_trim(szText, L" \t\r\n　");

	// replace the fullwidth characters with halfwidth characters
	ReplaceFullWithHalf(szText);

	if (szText[0] == 0)
	{
		return (WORD)0;     // empty
	}
	else if (mchr_is_digit(szText[0]) || szText[0] == L'-' || szText[0] == L'+')
	{
		// numeric
		return WORD(mstr_parse_int(szText));
	}
	else
	{
		// string value
		MStringW str = szText;

		// is there parenthesis?
		size_t i = str.rfind(L'('); // ')'
		if (i != MStringW::npos && mchr_is_digit(str[i + 1]))
		{
			// parse the text after the last parenthesis
			return WORD(mstr_parse_int(&str[i + 1]));
		}

		// A resource ID?
		if (g_db.HasResID(str))
			return (WORD)g_db.GetResIDValue(str);

		// Make it Uppercase
		mstr_upper(str);

		// Retry
		if (g_db.HasResID(str))
			return (WORD)g_db.GetResIDValue(str);

		if (str[0] == L'"') // Quoted?
			mstr_unquote(str); // Unquote

		// string
		return MIdOrString(str.c_str());
	}
}

// get the IDTYPE_ values by the specified prefix
std::vector<INT> GetPrefixIndexes(const MString& prefix)
{
	std::vector<INT> ret;
	for (auto& pair : g_settings.assoc_map)
	{
		if (prefix == pair.second && !pair.second.empty())
		{
			auto nIDTYPE_ = UnMapIDType(pair.first);
			ret.push_back(nIDTYPE_);
		}
	}
	return ret;
}

BOOL MMainWnd::ShowTreeViewArrow(BOOL bShow, HTREEITEM hItem)
{
	auto entry = g_res.get_entry();
	if (!entry)
	{
		ShowWindowAsync(m_arrow, SW_HIDE);
		return FALSE;
	}

	if (hItem == NULL)
	{
		hItem = TreeView_GetSelection(m_hwndTV);
	}

	RECT rc;
	TreeView_GetItemRect(m_hwndTV, hItem, &rc, TRUE);

	RECT rcClient;
	GetClientRect(m_hwndTV, &rcClient);
	SIZE siz = m_arrow.GetArrowSize(&rc);
	LONG x = rcClient.right - siz.cx;
	LONG y = rc.top;

	m_arrow.ShowDropDownList(m_arrow, FALSE);

	if (bShow)
	{
		if (IsWindow(m_arrow))
		{
			UINT uFlags = SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER;
			SetWindowPos(m_arrow, NULL, x, y, 0, 0, uFlags);
		}
		else
		{
			m_arrow.CreateAsChildDx(m_hwndTV, NULL, WS_CHILD, 0, -1, x, y);
		}
		m_arrow.m_hwndMain = m_hwnd;
		m_arrow.SendMessageDx(MYWM_SETITEMRECT, 0, (LPARAM)&rc);
		ShowWindowAsync(m_arrow, SW_SHOWNOACTIVATE);
		switch (entry->m_et)
		{
		case ET_TYPE:
			m_arrow.ChooseType();
			break;
		case ET_NAME:
			m_arrow.ChooseName(entry->m_type, entry->m_name);
			break;
		case ET_LANG:
		case ET_STRING:
			m_arrow.ChooseLang(entry->m_lang);
			break;
		}
	}
	else
	{
		if (IsWindow(m_arrow) && IsWindowVisible(m_arrow))
		{
			ShowWindow(m_arrow, SW_HIDE);
			InvalidateRect(m_hwndTV, &rc, TRUE);
		}
	}

	return TRUE;
}

void MMainWnd::DoTVEditAutoCompleteRelease(HWND hwnd)
{
	if (m_pAutoComplete)
	{
		m_pAutoComplete->unbind();
		m_pAutoComplete->Release();
		m_pAutoComplete = NULL;
	}

	m_auto_comp_edit.unhook();
}

void MMainWnd::DoTVEditAutoComplete(HWND hwnd, HWND hwndEdit)
{
	DoTVEditAutoCompleteRelease(hwnd);

	auto entry = g_res.get_entry();
	if (!entry)
		return;

	switch (entry->m_et)
	{
	case ET_TYPE:
		m_pAutoComplete = new MRisohAutoComplete(0);
		break;
	case ET_NAME:
		m_pAutoComplete = new MRisohAutoComplete(1);
		break;
	case ET_LANG:
	case ET_STRING:
		m_pAutoComplete = new MRisohAutoComplete(2);
		break;
	default:
		return;
	}

	m_pAutoComplete->bind(hwndEdit);
	m_auto_comp_edit.hook(hwndEdit, m_hwndTV);
	m_auto_comp_edit.m_bAdjustSize = TRUE;
}

// WM_NOTIFY
LRESULT MMainWnd::OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
{
	// get the selected entry
	auto entry = g_res.get_entry();

	if (pnmhdr->code == TCN_SELCHANGE)
	{
		if (pnmhdr->hwndFrom == m_tab)
		{
			INT iSelected = m_tab.GetCurSel();
			OnSelChange(hwnd, iSelected);
		}
	}
	else if (pnmhdr->code == MSplitterWnd::NOTIFY_CHANGED)
	{
		MWaitCursor wait;
		if (pnmhdr->hwndFrom == m_splitter1)
		{
			if (m_splitter1.GetPaneCount() >= 2)
			{
				g_settings.nTreeViewWidth = m_splitter1.GetPaneExtent(0);

				// relayout
				PostMessage(hwnd, WM_SIZE, 0, 0);
			}
		}
		else if (pnmhdr->hwndFrom == m_splitter2)
		{
			if (m_splitter2.GetPaneCount() >= 2)
				g_settings.nBmpViewWidth = m_splitter2.GetPaneExtent(1);
		}
	}
	else if (pnmhdr->code == TVN_DELETEITEM)
	{
		auto ptv = (NM_TREEVIEW *)pnmhdr;
		auto entry = (EntryBase *)ptv->itemOld.lParam;
		// During delete_all(), skip wait-cursor and modified-flag work;
		// on_delete_item is already a no-op cascade and ownership is
		// released in bulk afterwards.
		if (g_res.m_bDeletingAll)
		{
			g_res.on_delete_item(entry);
		}
		else
		{
			MWaitCursor wait;
			g_res.on_delete_item(entry);
			DoSetFileModified(TRUE);
		}
	} 
	else if (pnmhdr->code == NM_DBLCLK) 
	{
		MWaitCursor wait;
		if (pnmhdr->hwndFrom == m_hwndTV && entry)
		{
			switch (entry->m_et)
			{
			case ET_LANG:
			case ET_STRING:
				// The preceding TVN_SELCHANGED (from the first click of this
				// double-click) armed a 150ms TV_SELECTION_TIMER_ID to run a
				// deferred SelectTV(). OnEdit()/OnGuiEdit() below already
				// handle the selection synchronously, so that timer is now
				// redundant -- and dangerous: if left alone, it fires shortly
				// after OnGuiEdit() creates m_rad_window, and the deferred
				// SelectTV() -> Preview() -> HidePreview() unconditionally
				// destroys m_rad_window, making the RAD window flash open
				// and immediately close. Cancel it before handling the
				// double-click.
				::KillTimer(hwnd, TV_SELECTION_TIMER_ID);

				OnEdit(hwnd);
				if (g_settings.bGuiByDblClick)
				{
					OnGuiEdit(hwnd);
				}
				return 1;
			default:
				break;
			}
		}
	}
	else if (pnmhdr->code == TVN_SELCHANGING)
	{
		MWaitCursor wait;
		if (!m_bLoading)
		{
			// compile if necessary
			if (!CompileIfNecessary(FALSE))
				return TRUE;
		}
		DestroyRadWindow();
	}
	else if (pnmhdr->code == TVN_SELCHANGED)
	{
		MWaitCursor wait;
		if (!m_bLoading)
		{
			if (entry)
			{
				// select the entry to update the text
				::KillTimer(hwnd, TV_SELECTION_TIMER_ID);
				::SetTimer(hwnd, TV_SELECTION_TIMER_ID, 150, nullptr);
				OnSelChange(hwnd, 0);
			}
			PostUpdateArrow(hwnd);
		}
	}
	else if (pnmhdr->code == TVN_ITEMEXPANDING)
	{
		m_arrow.ShowDropDownList(m_arrow, FALSE);
		ShowTreeViewArrow(FALSE);
	}
	else if (pnmhdr->code == TVN_ITEMEXPANDED)
	{
		PostUpdateArrow(hwnd);
	}
	else if (pnmhdr->code == NM_RETURN)
	{
		MWaitCursor wait;
		if (pnmhdr->hwndFrom == m_hwndTV && entry)
		{
			switch (entry->m_et)
			{
			case ET_LANG:
			case ET_STRING:
				// See the matching comment in the NM_DBLCLK handler above:
				// cancel any pending deferred-SelectTV timer so it can't
				// destroy the m_rad_window that OnGuiEdit() is about to
				// create/show.
				::KillTimer(hwnd, TV_SELECTION_TIMER_ID);

				OnEdit(hwnd);
				if (g_settings.bGuiByDblClick)
				{
					OnGuiEdit(hwnd);
				}
				return 1;
			default:
				break;
			}
		}
	}
	else if (pnmhdr->code == TVN_KEYDOWN)
	{
		MWaitCursor wait;
		auto pTVKD = (TV_KEYDOWN *)pnmhdr;
		switch (pTVKD->wVKey)
		{
		case VK_DELETE:
			PostMessageW(hwnd, WM_COMMAND, ID_DELETERES, 0);
			DoSetFileModified(TRUE);
			return TRUE;
		case VK_LEFT:
		case VK_RIGHT:
			ShowTreeViewArrow(FALSE);
			PostUpdateArrow(hwnd);
			break;
		case VK_F2:
			{
				// compile if necessary
				if (!CompileIfNecessary(FALSE))
					return TRUE;

				// get the selected type entry
				auto entry = g_res.get_entry();
				if (!entry)
					return TRUE;

				if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
				{
					if (entry->m_type == RT_STRING)
						return TRUE;
				}

				HTREEITEM hItem = TreeView_GetSelection(m_hwndTV);
				TreeView_EditLabel(m_hwndTV, hItem);
			}
			return TRUE;
		}
	}
	else if (pnmhdr->code == TVN_GETINFOTIP)
	{
		MWaitCursor wait;
		auto pGetInfoTip = (NMTVGETINFOTIP *)pnmhdr;
		auto entry = (EntryBase *)pGetInfoTip->lParam;
		if (g_res.super()->find(entry) != g_res.super()->end())
		{
			StringCchCopyW(pGetInfoTip->pszText, pGetInfoTip->cchTextMax,
						   entry->m_strLabel.c_str());
		}
	}
	else
	{
		static LANGID old_lang = BAD_LANG;
		static WCHAR szOldText[MAX_PATH] = L"";

		if (pnmhdr->code == TBN_DROPDOWN)
		{
			auto pToolBar = (NMTOOLBAR *)pnmhdr;
			OnAddBang(hwnd, pToolBar);
		}
		else if (pnmhdr->code == TVN_BEGINLABELEDIT)
		{
			MWaitCursor wait;
			auto pInfo = (TV_DISPINFO *)pnmhdr;
			LPARAM lParam = pInfo->item.lParam;
			LPWSTR pszOldText = pInfo->item.pszText;

			MTRACEW(L"TVN_BEGINLABELEDIT: %s\n", pszOldText);

			if (IsWindow(m_arrow.m_dialog))
				return TRUE;    // prevent

			auto entry = (EntryBase*)lParam;
			if (!entry)
				return TRUE;    // prevent

			if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
			{
				if (entry->m_type == RT_STRING)
					return TRUE;    // prevent
			}

			StringCchCopyW(szOldText, _countof(szOldText), pszOldText);
			mstr_trim(szOldText);

			if (entry->m_et == ET_LANG || entry->m_et == ET_STRING)
			{
				if (!ParseLang(szOldText, old_lang))
					return TRUE;    // prevent
			}

			m_arrow.ShowDropDownList(m_arrow, FALSE);
			ShowTreeViewArrow(FALSE);

			switch (entry->m_et)
			{
			case ET_TYPE:
			case ET_NAME:
			case ET_LANG:
			case ET_STRING:
				PostMessage(hwnd, MYWM_AUTOCOMPLETE, 0, 0);
				break;
			}

			return FALSE;       // accept
		}
		else if (pnmhdr->code == TVN_ENDLABELEDIT)
		{
			MWaitCursor wait;
			auto pInfo = (TV_DISPINFO *)pnmhdr;
			LPARAM lParam = pInfo->item.lParam;
			LPWSTR pszNewText = pInfo->item.pszText;

			auto entry = (EntryBase*)lParam;
			if (!entry)
				return FALSE;   // reject

			switch (entry->m_et)
			{
			case ET_TYPE:
			case ET_NAME:
			case ET_LANG:
			case ET_STRING:
				PostMessage(hwnd, MYWM_AUTOCOMPLETEDONE, 0, 0);
				break;
			}

			if (pszNewText == NULL)
			{
				PostUpdateArrow(hwnd);
				return FALSE;   // reject
			}

			if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
			{
				if (entry->m_type == RT_STRING)
					return FALSE;   // reject
			}

			if (entry->m_et == ET_TYPE)
			{
				MIdOrString old_type = entry->m_type;
				MIdOrString new_type;
				INT ids = ParseType(pszNewText, new_type);
				if (ids)
				{
					ErrorBoxDx(ids);
					return FALSE; // failure
				}

				if (old_type == new_type)
					return FALSE;   // reject

				// check if it already exists
				if (g_res.find(ET_LANG, new_type))
				{
					ErrorBoxDx(IDS_ALREADYEXISTS);
					return FALSE;   // reject
				}

				if (!DoRetypeEntry(pszNewText, entry, old_type, new_type))
					return FALSE;   // reject

				m_arrow.ChooseType();
				DoSetFileModified(TRUE);
				return TRUE;   // accept
			}
			else if (entry->m_et == ET_NAME)
			{
				MIdOrString old_name = entry->m_name;
				MIdOrString new_name;

				INT ids = ParseName(pszNewText, entry->m_type, new_name);
				if (ids)
				{
					ErrorBoxDx(ids);
					return FALSE; // failure
				}

				if (old_name == new_name)
					return FALSE;   // reject

				// check if it already exists
				if (g_res.find(ET_LANG, entry->m_type, new_name))
				{
					ErrorBoxDx(IDS_ALREADYEXISTS);
					return FALSE;   // reject
				}

				DoRenameEntry(pszNewText, entry, old_name, new_name);
				m_arrow.ChooseName(entry->m_type, entry->m_name);
				DoSetFileModified(TRUE);
				return TRUE;   // accept
			}
			else if (entry->m_et == ET_LANG)
			{
				PostUpdateArrow(hwnd);

				LANGID new_lang;
				if (!ParseLang(pszNewText, new_lang))
				{
					ErrorBoxDx(IDS_INVALIDLANG);
					return FALSE;   // reject
				}

				if (old_lang == new_lang)
					return FALSE;   // reject

				// check if it already exists
				if (g_res.find(ET_LANG, entry->m_type, entry->m_name, new_lang))
				{
					ErrorBoxDx(IDS_ALREADYEXISTS);
					return FALSE;   // reject
				}

				DoRelangEntry(pszNewText, entry, old_lang, new_lang);
				m_arrow.ChooseLang(new_lang);
				DoSetFileModified(TRUE);

				return TRUE;   // accept
			}
			else if (entry->m_et == ET_STRING)
			{
				PostUpdateArrow(hwnd);

				LANGID new_lang;
				if (!ParseLang(pszNewText, new_lang))
				{
					ErrorBoxDx(IDS_INVALIDLANG);
					return FALSE;   // reject
				}

				if (old_lang == new_lang)
					return FALSE;   // reject

				// check if it already exists
				if (g_res.find(ET_LANG, entry->m_type, BAD_NAME, new_lang))
				{
					ErrorBoxDx(IDS_ALREADYEXISTS);
					return FALSE;   // reject
				}

				DoRelangEntry(pszNewText, entry, old_lang, new_lang);
				m_arrow.ChooseLang(new_lang);
				DoSetFileModified(TRUE);
				return TRUE;   // accept
			}

			return FALSE;   // reject
		}
	}
	return 0;
}

static int CALLBACK
TreeViewCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	auto entry1 = (const EntryBase *)lParam1;
	auto entry2 = (const EntryBase *)lParam2;
	if (*entry1 < *entry2)
		return -1;
	if (*entry1 == *entry2)
		return 0;
	return 1;
}

// change the type of the resource entries
BOOL MMainWnd::DoRetypeEntry(LPWSTR pszText, EntryBase *entry, MIdOrString& old_type, MIdOrString& new_type)
{
	if (!entry)
		return FALSE;

	if (MsgBoxDx(LoadStringDx(IDS_RETYPEWARNING), MB_ICONWARNING | MB_YESNOCANCEL) != IDYES)
		return FALSE;

	EntrySet found;
	g_res.search(found, ET_ANY, old_type, BAD_NAME, BAD_LANG);

	for (auto e : found)
	{
		assert(e->m_type == old_type);
		if (e->m_et == ET_LANG)
			g_res.add_lang_entry(new_type, e->m_name, e->m_lang, e->m_data);
		e->mark_invalid();
	}

	g_res.delete_invalid();

	entry = g_res.find(ET_TYPE, new_type, BAD_NAME, BAD_LANG);
	if (!entry)
		return FALSE;

	UpdateEntryType(entry, pszText);
	DoRefreshIDList(m_hwnd);
	Expand(entry->m_hItem);

	// select the entry to update the text
	SelectTV(entry, FALSE);

	DoSetFileModified(TRUE);

	// sort
	HTREEITEM hParent = TreeView_GetParent(m_hwndTV, entry->m_hItem);
	TV_SORTCB cb = { hParent, TreeViewCompare };
	TreeView_SortChildrenCB(m_hwndTV, &cb, 0);

	return TRUE;
}

// change the name of the resource entries
void MMainWnd::DoRenameEntry(LPWSTR pszText, EntryBase *entry, MIdOrString& old_name, MIdOrString& new_name)
{
	if (!entry)
		return;

	if (entry->m_type == L"RISOHTEMPLATE")
	{
		WORD wID = WORD(g_db.GetValue(L"RESOURCE", new_name.c_str()));
		if (wID != 0)
			new_name = wID;
	}

	// search the old named language entries
	EntrySet found;
	g_res.search(found, ET_LANG, entry->m_type, old_name);

	for (auto e : found)
	{
		assert(e->m_name == old_name);
		e->m_name = new_name;
	}

	// update the entry name
	entry->m_name = new_name;
	UpdateEntryName(entry, pszText);
	DoRefreshIDList(m_hwnd);

	// select the entry to update the text
	SelectTV(entry, FALSE);

	DoSetFileModified(TRUE);

	// sort
	HTREEITEM hParent = TreeView_GetParent(m_hwndTV, entry->m_hItem);
	TV_SORTCB cb = { hParent, TreeViewCompare };
	TreeView_SortChildrenCB(m_hwndTV, &cb, 0);
}

// change the language of the resource entries
void MMainWnd::DoRelangEntry(LPWSTR pszText, EntryBase *entry, LANGID old_lang, LANGID new_lang)
{
	EntrySet found;

	switch (entry->m_et)
	{
	case ET_STRING:
		// serach the resource strings
		g_res.search(found, ET_LANG, entry->m_type, BAD_NAME, old_lang);

		// replace the language
		for (auto e : found)
		{
			assert(e->m_lang == old_lang);
			e->m_lang = new_lang;
			UpdateEntryLang(e, pszText);
		}
		found.clear();
		break;

	case ET_LANG:
		break;

	default:
		return;
	}

	// search the old named old language entries
	g_res.search(found, ET_ANY, entry->m_type, entry->m_name, old_lang);
	for (auto e : found)
	{
		// replace the language
		assert(e->m_lang == old_lang);
		e->m_lang = new_lang;

		// update the language
		UpdateEntryLang(e, pszText);
	}

	// select the entry
	SelectTV(entry, FALSE);

	DoSetFileModified(TRUE);

	// sort
	HTREEITEM hParent = TreeView_GetParent(m_hwndTV, entry->m_hItem);
	TV_SORTCB cb = { hParent, TreeViewCompare };
	TreeView_SortChildrenCB(m_hwndTV, &cb, 0);
}

// join the lines by '\\'
void MMainWnd::JoinLinesByBackslash(std::vector<MStringA>& lines)
{
	// join by '\\'
	for (size_t i = 0; i < lines.size(); ++i)
	{
		MStringA& line = lines[i];
		if (line.size())
		{
			if (line[line.size() - 1] == '\\')
			{
				line = line.substr(0, line.size() - 1);
				lines[i] = line + lines[i + 1];
				lines.erase(lines.begin() + (i + 1));
				--i;
			}
		}
	}
}

// delete the include guard
void MMainWnd::DeleteIncludeGuard(std::vector<MStringA>& lines)
{
	size_t k0 = -1, k1 = -1;
	MStringA name0;

	for (size_t i = 0; i < lines.size(); ++i)
	{
		MStringA& line = lines[i];
		const char *pch = mstr_skip_space(&line[0]);
		if (*pch != '#')
			continue;

		++pch;
		pch = mstr_skip_space(pch);
		if (memcmp(pch, "ifndef", 6) == 0 && mchr_is_space(pch[6]))
		{
			// #ifndef
			pch += 6;
			const char *pch0 = pch = mstr_skip_space(pch);
			while (std::isalnum(*pch) || *pch == '_')
			{
				++pch;
			}

			if (name0.empty())
			{
				MStringA name(pch0, pch);
				k0 = i;
				name0 = std::move(name);
			}
			else
			{
				name0.clear();
				break;
			}
		}
		else if (memcmp(pch, "if", 2) == 0)
		{
			break;
		}
		else if (memcmp(pch, "define", 6) == 0 && mchr_is_space(pch[6]))
		{
			if (name0.empty())
				break;

			// #define
			pch += 6;
			const char *pch0 = pch = mstr_skip_space(pch);
			while (std::isalnum(*pch) || *pch == '_')
			{
				++pch;
			}
			MStringA name(pch0, pch);
			if (name0 == name)
			{
				k1 = i;
				break;
			}
		}
		else
		{
			// otherwise
			break;
		}
	}

	if (name0.empty())
		return;

	for (size_t i = lines.size(); i > 0; )
	{
		--i;
		MStringA& line = lines[i];
		const char *pch = mstr_skip_space(&line[0]);
		if (*pch != '#')
			continue;

		++pch;
		pch = mstr_skip_space(pch);
		if (memcmp(pch, "endif", 5) == 0)
		{
			lines.erase(lines.begin() + i);
			lines.erase(lines.begin() + k1);
			lines.erase(lines.begin() + k0);
			break;
		}
		else
		{
			break;
		}
	}
}

// add the head comments
void MMainWnd::AddHeadComment(std::vector<MStringA>& lines)
{
	if (m_szFile[0])
	{
		WCHAR title[MAX_PATH];
		GetFileTitleW(m_szFile, title, _countof(title));
		MStringA line = "// ";
		line += MWideToAnsi(CP_ACP, title).c_str();
		lines.insert(lines.begin(), line);
	}
	lines.insert(lines.begin(), "// Microsoft Visual C++ Compatible");
	lines.insert(lines.begin(), "//{{NO_DEPENDENCIES}}");
}

// delete the head comments
void MMainWnd::DeleteHeadComment(std::vector<MStringA>& lines)
{
	for (size_t i = 0; i < lines.size(); ++i)
	{
		MStringA& line = lines[i];
		if (line.find("//") == 0)
		{
			if (line.find("{{NO_DEPENDENCIES}}") != MStringA::npos ||
				line.find("Microsoft Visual C++") != MStringA::npos ||
				line.find(".rc") != MStringA::npos)
			{
				lines.erase(lines.begin() + i);
				--i;
			}
		}
	}
}

// delete the specific macro lines
void MMainWnd::DeleteSpecificMacroLines(std::vector<MStringA>& lines)
{
	for (size_t i = lines.size(); i > 0; )
	{
		--i;
		MStringA& line = lines[i];
		const char *pch = mstr_skip_space(&line[0]);
		if (*pch != '#')
			continue;

		++pch;
		pch = mstr_skip_space(pch);
		if (memcmp(pch, "define", 6) == 0 && mchr_is_space(pch[6]))
		{
			// #define
			pch += 6;
			const char *pch0 = pch = mstr_skip_space(pch);
			while (std::isalnum(*pch) || *pch == '_')
			{
				++pch;
			}
			MStringA name(pch0, pch);

			if (g_settings.removed_ids.find(name) != g_settings.removed_ids.end())
			{
				lines.erase(lines.begin() + i);
			}
		}
	}
}

// add additional macro lines
void MMainWnd::AddAdditionalMacroLines(std::vector<MStringA>& lines)
{
	for (auto& pair : g_settings.added_ids)
	{
		MStringA line = "#define ";
		if (pair.first == "IDC_STATIC")
		{
			line += "IDC_STATIC -1";
		}
		else
		{
			line += pair.first;
			line += " ";
			line += pair.second;
		}
		lines.push_back(line);
	}
}

// add the '#ifdef APSTUDIO_INVOKED ... #endif' block
void MMainWnd::AddApStudioBlock(std::vector<MStringA>& lines)
{
	UINT anValues[5];
	DoIDStat(anValues);

	lines.push_back("#ifdef APSTUDIO_INVOKED");
	lines.push_back("    #ifndef APSTUDIO_READONLY_SYMBOLS");

	char buf[256];
	StringCchPrintfA(buf, _countof(buf), "        #define _APS_NO_MFC                 %u", anValues[0]);
	lines.push_back(buf);
	StringCchPrintfA(buf, _countof(buf), "        #define _APS_NEXT_RESOURCE_VALUE    %u", anValues[1]);
	lines.push_back(buf);
	StringCchPrintfA(buf, _countof(buf), "        #define _APS_NEXT_COMMAND_VALUE     %u", anValues[2]);
	lines.push_back(buf);
	StringCchPrintfA(buf, _countof(buf), "        #define _APS_NEXT_CONTROL_VALUE     %u", anValues[3]);
	lines.push_back(buf);
	StringCchPrintfA(buf, _countof(buf), "        #define _APS_NEXT_SYMED_VALUE       %u", anValues[4]);
	lines.push_back(buf);
	lines.push_back("    #endif");
	lines.push_back("#endif");
}

// delete the '#ifdef APSTUDIO_INVOKED ... #endif' block
void MMainWnd::DeleteApStudioBlock(std::vector<MStringA>& lines)
{
	bool inside = false;
	size_t nest = 0;
	std::ptrdiff_t k = -1;
	for (size_t i = 0; i < lines.size(); ++i)
	{
		MStringA& line = lines[i];
		const char *pch = mstr_skip_space(&line[0]);
		if (*pch != '#')
			continue;

		++pch;
		pch = mstr_skip_space(pch);
		if (memcmp(pch, "ifdef", 5) == 0 && mchr_is_space(pch[5]))
		{
			// #ifdef
			pch += 5;
			const char *pch0 = pch = mstr_skip_space(pch);
			while (std::isalnum(*pch) || *pch == '_')
			{
				++pch;
			}
			MStringA name(pch0, pch);

			if (name == "APSTUDIO_INVOKED")
			{
				inside = true;
				k = i;
				++nest;
			}
		}
		else if (memcmp(pch, "if", 2) == 0)
		{
			++nest;
		}
		else if (memcmp(pch, "define", 6) == 0 && mchr_is_space(pch[6]))
		{
			if (!inside)
				continue;

			// #define
			pch += 6;
			const char *pch0 = pch = mstr_skip_space(pch);
			while (std::isalnum(*pch) || *pch == '_')
			{
				++pch;
			}
		}
		else if (memcmp(pch, "endif", 5) == 0)
		{
			--nest;
			if (nest == 0 && k != -1)
			{
				lines.erase(lines.begin() + k, lines.begin() + i + 1);
				break;
			}
		}
	}
}

// helper method for MMainWnd::OnUpdateResHBang
void MMainWnd::UpdateResHLines(std::vector<MStringA>& lines)
{
	JoinLinesByBackslash(lines);
	DeleteIncludeGuard(lines);
	DeleteHeadComment(lines);
	DeleteSpecificMacroLines(lines);
	AddAdditionalMacroLines(lines);
	DeleteApStudioBlock(lines);
	AddApStudioBlock(lines);
	AddHeadComment(lines);
}

// helper method for MMainWnd::OnUpdateResHBang
void MMainWnd::ReadResHLines(FILE *fp, std::vector<MStringA>& lines)
{
	// read lines
	CHAR buf[512];
	while (fgets(buf, _countof(buf), fp) != NULL)
	{
		size_t len = std::strlen(buf);
		if (len == 0)
			break;
		if (buf[len - 1] == '\n')
			buf[len - 1] = 0;
		lines.push_back(buf);
	}
}

// add a resource item
BOOL MMainWnd::DoAddRes(HWND hwnd, MAddResDlg& dialog)
{
	if (dialog.m_strTemplate.empty())   // already added
	{
		// refresh the ID list window
		DoRefreshIDList(hwnd);

		// select it
		SelectTV(ET_LANG, dialog, FALSE);

		// clear the modification flag
		Edit_SetModify(m_hCodeEditor, FALSE);
		return TRUE;
	}

	// dialog.m_strTemplate --> m_hCodeEditor
	SetWindowTextW(m_hCodeEditor, dialog.m_strTemplate.c_str());
	::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);

	// workaround to edit the Microsoft message table
	if (dialog.m_type == RT_MESSAGETABLE && g_settings.bUseMSMSGTABLE)
		g_settings.bUseMSMSGTABLE = FALSE;

	// compile dialog.m_strTemplate
	MStringA strOutput;
	if (!CompileParts(strOutput, dialog.m_type, dialog.m_name, dialog.m_lang, dialog.m_strTemplate, FALSE))
	{
		// failure
		m_nStatusStringID = IDS_RECOMPILEFAILED;
		UpdateOurToolBarButtons(2);

		// set the error message
		SetErrorMessage(strOutput);

		if (dialog.m_added_entry)
		{
			dialog.m_added_entry->mark_invalid();
			g_res.delete_invalid();

			HidePreview(STV_RESETTEXT);
			SelectTV(g_res.get_entry(), FALSE, STV_RESETTEXT);
		}

		g_res.delete_invalid();
		DoSetFileModified(dialog.m_bOldModified);
		PostUpdateArrow(hwnd);
		return FALSE;
	}

	// refresh the ID list window
	DoRefreshIDList(hwnd);
	// clear the modification flag
	Edit_SetModify(m_hCodeEditor, FALSE);
	m_nStatusStringID = IDS_RECOMPILEOK;

	// select the added entry
	if (dialog.m_type == RT_STRING)
		SelectTV(ET_STRING, dialog.m_type, BAD_NAME, BAD_LANG, FALSE);
	else
		SelectTV(ET_LANG, dialog, FALSE);

	DoSetFileModified(TRUE);
	PostUpdateArrow(hwnd);
	return TRUE;
}

void MMainWnd::UpdateTitleBar()
{
	if (m_szFile[0] == 0)
	{
		SetWindowTextW(m_hwnd, LoadStringDx(IDS_TITLE));
	}
	else if (g_settings.bShowFullPath)
	{
		// set the full file path to the title bar
		SetWindowTextW(m_hwnd, LoadStringPrintfDx(IDS_TITLEWITHFILE, m_szFile));
	}
	else
	{
		// set the file title to the title bar
		SetWindowTextW(m_hwnd, LoadStringPrintfDx(IDS_TITLEWITHFILE, PathFindFileNameW(m_szFile)));
	}
}

// set the file-related info
BOOL MMainWnd::UpdateFileInfo(FileType ft, LPCWSTR pszFile, BOOL bCompressed)
{
	m_file_type = ft;
	m_bUpxCompressed = bCompressed;

	if (pszFile == NULL || pszFile[0] == 0)
	{
		// clear the file info
		m_szFile[0] = 0;
		UpdateTitleBar();
		return TRUE;
	}

	if (m_szFile != pszFile)
	{
		// pszFile --> m_szFile (full path)
		GetFullPathNameW(pszFile, _countof(m_szFile), m_szFile, NULL);
	}

	UpdateTitleBar();

	// add to the recently used files
	g_settings.AddFile(m_szFile);

	// update the menu
	UpdateMenu();

	PostUpdateArrow(m_hwnd);
	return TRUE;
}

BOOL MMainWnd::ReCreateSrcEdit()
{
	BOOL bModify = Edit_GetModify(m_hCodeEditor);

	if (IsWindow(m_hCodeEditor))
		DestroyWindow(m_hCodeEditor);

	DWORD style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP |
				  ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
				  ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN | WS_CLIPCHILDREN;
	if (!g_settings.bWordWrap)
	{
		style |= WS_HSCROLL | ES_AUTOHSCROLL;
	}

	WNDCLASSEXW wcx;
	BOOL bLineNumEdit = ::GetClassInfoExW(NULL, L"LineNumEdit", &wcx);

	DWORD exstyle = WS_EX_CLIENTEDGE;
	HWND hSrcEdit = ::CreateWindowEx(exstyle,
		(bLineNumEdit ? L"LineNumEdit" : L"EDIT"), NULL,
		style, 0, 0, 1, 1, m_splitter2,
		(HMENU)(INT_PTR)2, GetModuleHandle(NULL), NULL);
	if (hSrcEdit)
	{
		m_hCodeEditor = hSrcEdit;
		SendMessage(m_hCodeEditor, EM_SETLIMITTEXT, 0x100000, 0);
		SendMessage(m_hCodeEditor, LNEM_SETNUMOFDIGITS, 3, 0);
		SendMessage(m_hCodeEditor, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
		Edit_SetModify(m_hCodeEditor, bModify);
		m_splitter2.SetPane(0, m_hCodeEditor);
		PostMessage(m_splitter2, WM_SIZE, 0, 0);
		return TRUE;
	}
	return FALSE;
}

static WNDPROC s_fnTreeViewOldWndProc = NULL;

LRESULT CALLBACK
TreeViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MMainWnd *this_ = (MMainWnd *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	return this_->TreeViewWndProcDx(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK
MMainWnd::TreeViewWndProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret;
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_SHIFT || wParam == VK_CONTROL)
			return 0;
		// ...FALL THROUGH...
	case WM_SIZE: case WM_HSCROLL: case WM_VSCROLL:
	case WM_MOUSEWHEEL: case WM_CHAR:
		if (IsWindow(m_arrow))
		{
			// hide drop-down arrow
			ShowTreeViewArrow(FALSE);

			// get selected item rect
			RECT rc;
			HTREEITEM hItem = TreeView_GetSelection(hwnd);
			TreeView_GetItemRect(hwnd, hItem, &rc, FALSE);

			// default processing
			ret = CallWindowProc(s_fnTreeViewOldWndProc, hwnd, uMsg, wParam, lParam);

			// redraw the rect
			InvalidateRect(hwnd, &rc, TRUE);

			// restore drop-down arrow
			PostUpdateArrow(m_hwnd);

			return ret;
		}
		break;
	case WM_SYSKEYDOWN:
		if (wParam == VK_DOWN && IsWindow(m_arrow))
		{
			m_arrow.ShowDropDownList(m_arrow, TRUE);
			return 0;
		}
		break;
	}
	return CallWindowProc(s_fnTreeViewOldWndProc, hwnd, uMsg, wParam, lParam);
}

// WM_CREATE: the main window is to be created
BOOL MMainWnd::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	MWaitCursor wait;

#if defined(NDEBUG) && defined(PROTECTION)
	if (!IsExeSigned())
		return FALSE;
#endif

	m_id_list_dlg.m_hMainWnd = hwnd;    // set the main window to the ID list window

	g_hMainWnd = hwnd;
	m_nShowMode = SHOW_CODEONLY;

	DoLoadLangInfo();   // load the language information

	// check the data
	INT nRet = CheckData();
	if (nRet)
		return FALSE;   // failure

	// load the RisohEditor settings
	LoadSettings(hwnd);

	if (g_settings.bResumeWindowPos)
	{
		// resume the main window pos
		if (g_settings.nWindowLeft != CW_USEDEFAULT)
		{
			POINT pt = { g_settings.nWindowLeft, g_settings.nWindowTop };
			SetWindowPosDx(&pt);
		}
		if (g_settings.nWindowWidth != CW_USEDEFAULT)
		{
			SIZE siz = { g_settings.nWindowWidth, g_settings.nWindowHeight };
			SetWindowPosDx(NULL, &siz);
		}
	}

	m_splash.CreateDx(hwnd);

	// create the image list for treeview
	m_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 3, 1);
	if (m_hImageList == NULL)
		return FALSE;

	// load some icons
	m_hFileIcon = LoadSmallIconDx(IDI_FILE);        // load a file icon
	m_hFolderIcon = LoadSmallIconDx(IDI_FOLDER);    // load a folder icon

	// add these icons
	ImageList_AddIcon(m_hImageList, m_hFileIcon);
	ImageList_AddIcon(m_hImageList, m_hFolderIcon);

	// create the tool image list for toolbar
	m_himlTools = (HIMAGELIST)ImageList_LoadBitmap(m_hInst, MAKEINTRESOURCE(IDB_TOOLBAR),
												   32, 8, RGB(255, 0, 255));
	if (m_himlTools == NULL)
	{
		DWORD dwError = GetLastError();
		MTRACE(TEXT("GetLastError(): %ld\n"), dwError);
		return FALSE;
	}

	// create the toolbar
	if (!CreateOurToolBar(hwnd, m_himlTools))
	{
		return FALSE;
	}

	DWORD style, exstyle;

	// create the splitter windows
	style = WS_CHILD | WS_VISIBLE | SWS_HORZ | SWS_LEFTALIGN;
	if (!m_splitter1.CreateDx(hwnd, 2, style))
		return FALSE;

	style = WS_CHILD | WS_VISIBLE | WS_BORDER | TCS_BOTTOM | TCS_TABS | TCS_TOOLTIPS |
			TCS_FOCUSNEVER | TCS_HOTTRACK | TCS_MULTILINE;
	if (!m_tab.CreateWindowDx(m_splitter1, NULL, style))
		return FALSE;
	SetWindowFont(m_tab, GetStockFont(DEFAULT_GUI_FONT), TRUE);

	m_tab.InsertItem(0, LoadStringDx(IDS_CODEEDITOR));
	m_tab.InsertItem(1, LoadStringDx(IDS_HEXVIEWER));
	m_tab.SetCurSel(0);

	style = WS_CHILD | WS_VISIBLE | SWS_HORZ | SWS_RIGHTALIGN;
	if (!m_splitter2.CreateDx(m_splitter1, 1, style))
		return FALSE;

	// create a treeview (tree control) window
	// NOTE: WS_CLIPCHILDREN is required here. m_arrow (the language
	// drop-down arrow) is a WS_CHILD window layered on top of this
	// treeview. Without WS_CLIPCHILDREN, the treeview's own DC (as
	// used internally by comctl32's ScrollWindow() call for
	// horizontal auto-scroll, see TREEVIEW_HScroll in treeview.c)
	// is not clipped to exclude m_arrow's area, so whatever is
	// currently drawn there gets dragged along by the scroll and
	// left behind as a ghost image. WS_CLIPCHILDREN makes the OS
	// exclude m_arrow's region from that DC, so this can't happen.
	style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN |
		WS_HSCROLL | WS_VSCROLL | WS_TABSTOP |
		TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_LINESATROOT |
		TVS_SHOWSELALWAYS | TVS_EDITLABELS | TVS_FULLROWSELECT | TVS_INFOTIP;
	m_hwndTV = CreateWindowExW(WS_EX_CLIENTEDGE,
		WC_TREEVIEWW, NULL, style, 0, 0, 0, 0, m_splitter1,
		(HMENU)1, m_hInst, NULL);
	if (m_hwndTV == NULL)
		return FALSE;

	SetWindowLongPtr(m_hwndTV, GWLP_USERDATA, (LONG_PTR)this);
	s_fnTreeViewOldWndProc = (WNDPROC)SetWindowLongPtrW(m_hwndTV, GWLP_WNDPROC, (LONG_PTR)TreeViewWndProc);

	// store the treeview handl to g_res (important!)
	g_res.m_hwndTV = m_hwndTV;

	if (s_pSetWindowTheme)
	{
		// apply Explorer's visual style
		(*s_pSetWindowTheme)(m_hwndTV, L"Explorer", NULL);
	}

	// set the imagelists to treeview
	TreeView_SetImageList(m_hwndTV, m_hImageList, TVSIL_NORMAL);

	// create the binary EDIT control
	style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_TABSTOP |
		ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
		ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN;
	exstyle = WS_EX_CLIENTEDGE;
	HWND hHexViewer = CreateWindowEx(exstyle, L"EDIT", NULL, style, 0, 0, 0, 0, m_splitter2, (HMENU)3,
		GetModuleHandle(NULL), 0);
	m_hHexViewer.SubclassDx(hHexViewer);
	m_hHexViewer.SendMessageDx(EM_SETLIMITTEXT, 0x100000);

	// create source EDIT control
	if (!ReCreateSrcEdit())
		return FALSE;

	// create MBmpView
	if (!m_hBmpView.CreateDx(m_splitter2, 4))
		return FALSE;

	// create status bar
	style = WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | CCS_BOTTOM;
	m_hStatusBar = CreateStatusWindow(style, LoadStringDx(IDS_STARTING), hwnd, 8);
	if (m_hStatusBar == NULL)
		return FALSE;

	// set the status text
	ChangeStatusText(IDS_STARTING);

	// setup the status bar
	INT anWidths[] = { -1 };
	SendMessage(m_hStatusBar, SB_SETPARTS, 1, (LPARAM)anWidths);

	// show the status bar or not
	if (g_settings.bShowStatusBar)
		ShowWindow(m_hStatusBar, SW_SHOWNOACTIVATE);
	else
		ShowWindow(m_hStatusBar, SW_HIDE);

	// set the pane contents of splitters
	m_splitter1.SetPane(0, m_hwndTV);
	m_splitter1.SetPane(1, m_tab);
	m_splitter1.SetPaneExtent(0, g_settings.nTreeViewWidth);
	m_splitter2.SetPane(0, m_hCodeEditor);

	// create the fonts
	ReCreateFonts(hwnd);

	if (m_argc >= 2)
	{
		if (!ParseCommandLine(hwnd, m_argc, m_targv))
		{
			PostMessageW(hwnd, WM_CLOSE, 0, 0);
		}
	}

	// enable file dropping
	DragAcceptFiles(hwnd, TRUE);

	// set focus to treeview
	SetFocus(m_hwndTV);

	// update the menu
	UpdateMenu();

	// store the file paths from settings
	if (g_settings.strWindResExe.size())
	{
		StringCchCopy(m_szWindresExe, _countof(m_szWindresExe), g_settings.strWindResExe.c_str());
		GetShortPathNameW(m_szWindresExe, m_szWindresExe, _countof(m_szWindresExe));
	}
	if (g_settings.strCppExe.size())
	{
		StringCchCopy(m_szMCppExe, _countof(m_szMCppExe), g_settings.strCppExe.c_str());
		GetShortPathNameW(m_szMCppExe, m_szMCppExe, _countof(m_szMCppExe));
	}

	// OK, ready
	PostMessageDx(WM_COMMAND, ID_READY);

	return TRUE;    // success
}

// the window procedure of the main window
/*virtual*/ LRESULT CALLBACK
MMainWnd::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		DO_MSG(WM_CREATE, OnCreate);
		DO_MSG(WM_COMMAND, OnCommand);
		DO_MSG(WM_CLOSE, OnClose);
		DO_MSG(WM_DESTROY, OnDestroy);
		DO_MSG(WM_DROPFILES, OnDropFiles);
		DO_MSG(WM_MOVE, OnMove);
		DO_MSG(WM_SIZE, OnSize);
		DO_MSG(WM_NOTIFY, OnNotify);
		DO_MSG(WM_CONTEXTMENU, OnContextMenu);
		DO_MSG(WM_INITMENU, OnInitMenu);
		DO_MSG(WM_ACTIVATE, OnActivate);
		DO_MSG(WM_SYSCOLORCHANGE, OnSysColorChange);
		DO_MSG(WM_SETFOCUS, OnSetFocus);
		DO_MSG(WM_KILLFOCUS, OnKillFocus);
		DO_MSG(WM_TIMER, OnTimer);
		DO_MESSAGE(MYWM_CLEARSTATUS, OnClearStatus);
		DO_MESSAGE(MYWM_MOVESIZEREPORT, OnMoveSizeReport);
		DO_MESSAGE(MYWM_COMPILECHECK, OnCompileCheck);
		DO_MESSAGE(MYWM_REOPENRAD, OnReopenRad);
		DO_MESSAGE(MYWM_IDJUMPBANG, OnIDJumpBang);
		DO_MESSAGE(MYWM_SELCHANGE, OnRadSelChange);
		DO_MESSAGE(MYWM_UPDATEDLGRES, OnUpdateDlgRes);
		DO_MESSAGE(MYWM_GETDLGHEADLINES, OnGetHeadLines);
		DO_MESSAGE(MYWM_DELPHI_DFM_B2T, OnDelphiDFMB2T);
		DO_MESSAGE(MYWM_TLB_B2T, OnTLB2IDL);
		DO_MESSAGE(MYWM_ITEMSEARCH, OnItemSearchBang);
		DO_MESSAGE(MYWM_COMPLEMENT, OnComplement);
		DO_MESSAGE(MYWM_UPDATEARROW, OnUpdateArrow);
		DO_MESSAGE(MYWM_RADDBLCLICK, OnRadDblClick);
		DO_MESSAGE(MYWM_AUTOCOMPLETE, OnAutoComplete);
		DO_MESSAGE(MYWM_AUTOCOMPLETEDONE, OnAutoCompleteDone);
		DO_MESSAGE(WM_EGA_FINISH, OnEgaFinish);
	default:
		return DefaultProcDx();
	}
}

// select the string entry in the tree control
void MMainWnd::SelectString(void)
{
	// find the string entry
	if (auto entry = g_res.find(ET_STRING, RT_STRING))
	{
		// select the entry
		SelectTV(entry, FALSE);
	}
}

// do ID jump now!
void MMainWnd::OnIDJumpBang2(HWND hwnd, const MString& name, MString& strType)
{
	if (strType == MapIDType(IDTYPE_UNKNOWN))
		return;     // ignore Unknown.ID jump

	// revert the resource type string
	Res_ReplaceResTypeString(strType, true);

	// get the prefix
	MString prefix = name.substr(0, name.find(L'_') + 1);

	// get the IDTYPE_'s from the prefix
	auto indexes = GetPrefixIndexes(prefix);
	for (size_t i = 0; i < indexes.size(); ++i)
	{
		INT nIDTYPE_ = indexes[i];
		if (nIDTYPE_ == IDTYPE_STRING || nIDTYPE_ == IDTYPE_PROMPT)
		{
			// select the string entry
			SelectString();
			return;     // done
		}
	}

	// get the type value
	MIdOrString type = WORD(g_db.GetValue(L"RESOURCE", strType));
	if (type.empty())
		type.m_str = strType;

	if (type == L"RISOHTEMPLATE")
	{
		MIdOrString new_name;
		ParseType(name, new_name);
		if (auto entry = g_res.find(ET_LANG, type, new_name))
		{
			// select the entry
			SelectTV(entry, FALSE);

			// set focus to the main window
			SetForegroundWindow(m_hwnd);
			BringWindowToTop(m_hwnd);
			SetFocus(m_hwnd);
		}
	}
	else
	{
		MIdOrString new_name;
		ParseName(name, type, new_name);
		if (auto entry = g_res.find(ET_LANG, type, new_name))
		{
			// select the entry
			SelectTV(entry, FALSE);

			// set focus to the main window
			SetForegroundWindow(m_hwnd);
			BringWindowToTop(m_hwnd);
			SetFocus(m_hwnd);
		}
	}
}

// MYWM_TLB_B2T
LRESULT MMainWnd::OnTLB2IDL(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	auto& str = *(MString *)wParam;
	auto& entry = *(const EntryBase *)lParam;

	std::string ansi;
	ansi = tlb_text_from_binary(m_szOleBow, entry.ptr(), entry.size());
	MAnsiToWide a2w(CP_UTF8, ansi);
	str = a2w.c_str();
	return 0;
}

// MYWM_DELPHI_DFM_B2T
LRESULT MMainWnd::OnDelphiDFMB2T(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	auto& str = *(MString *)wParam;
	auto& entry = *(const EntryBase *)lParam;

	auto ansi = dfm_text_from_binary(m_szDFMSC, entry.ptr(), entry.size(),
									 g_settings.nDfmCodePage, g_settings.bDfmRawTextComments);
	MAnsiToWide a2w(CP_UTF8, ansi);
	str = a2w.c_str();
	return 0;
}

// MYWM_GETDLGHEADLINES
LRESULT MMainWnd::OnGetHeadLines(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// get the selected entry
	auto entry = g_res.get_lang_entry();
	if (!entry)
		return -1;

	if (entry->m_type == RT_DIALOG)
	{
		DialogRes dialog_res;
		MByteStreamEx stream(entry->m_data);
		dialog_res.LoadFromStream(stream);
		return dialog_res.GetHeadLines();
	}
	return -1;
}

void MMainWnd::OnTimer(HWND hwnd, UINT id)
{
	if (id == TV_SELECTION_TIMER_ID)
	{
		::KillTimer(hwnd, TV_SELECTION_TIMER_ID);
		auto entry = g_res.get_entry();
		SelectTV(entry, FALSE);
		return;
	}
	if (id == CODE_REFRESH_TIMER_ID)
	{
		::KillTimer(hwnd, CODE_REFRESH_TIMER_ID);

		// get the selected language entry
		auto entry = g_res.get_lang_entry();
		if (!entry || entry->m_type != RT_DIALOG)
		{
			return;
		}

		auto& dialog_res = m_rad_window.m_dialog_res;

		// dialog_res --> entry->m_data
		MByteStreamEx stream;
		dialog_res.SaveToStream(stream);
		entry->m_data = stream.data();

		// entry->m_lang + dialog_res --> str --> m_hCodeEditor (text)
		// Suppress redraw on the LineNumEdit (and its line-number static, via the
		// WM_SETREDRAW forwarding in LineNumEdit) so a single coherent paint
		// happens after the text is fully replaced. Without this the gutter and
		// edit can each repaint mid-update and show as flicker, especially when
		// RAD repeatedly posts MYWM_UPDATEDLGRES while the user drags controls.
		MString str = GetLanguageStatement(entry->m_lang);
		str += dialog_res.Dump(entry->m_name);
		::SendMessageW(m_hCodeEditor, WM_SETREDRAW, FALSE, 0);
		SetWindowTextW(m_hCodeEditor, str.c_str());
		::SendMessageW(m_hCodeEditor, WM_SETREDRAW, TRUE, 0);
		// Invalidate the edit and its children (the line-number static) together.
		::RedrawWindow(m_hCodeEditor, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_NOERASE);

		// entry->m_data --> m_hHexViewer (binary)
		ClearHexCache();
		if (m_bShowBinEdit)
			UpdateHexViewerContent();
	}
}

// MYWM_UPDATEDLGRES
LRESULT MMainWnd::OnUpdateDlgRes(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	DoSetFileModified(TRUE);

	::KillTimer(hwnd, CODE_REFRESH_TIMER_ID);
	::SetTimer(hwnd, CODE_REFRESH_TIMER_ID, 150, nullptr);
	return 0;
}

// MYWM_SELCHANGE
LRESULT MMainWnd::OnRadSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	::SendMessageW(m_hCodeEditor, WM_SETREDRAW, FALSE, 0);
	::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);

	if (!IsWindow(m_rad_window))
	{
		::SendMessageW(m_hCodeEditor, WM_SETREDRAW, TRUE, 0);
		::InvalidateRect(m_hCodeEditor, nullptr, FALSE);
		return 0;
	}

	INT cHeads = INT(SendMessageW(hwnd, MYWM_GETDLGHEADLINES, 0, 0)) + 1;
	auto indeces = MRadCtrl::GetTargetIndeces();
	for (auto index : indeces)
	{
		::SendMessageW(m_hCodeEditor, LNEM_SETLINEMARK, cHeads + index, RGB(255, 255, 120));
	}
	::SendMessageW(m_hCodeEditor, WM_SETREDRAW, TRUE, 0);
	::InvalidateRect(m_hCodeEditor, nullptr, FALSE);
	return 0;
}

// MYWM_IDJUMPBANG: do ID jump now!
LRESULT MMainWnd::OnIDJumpBang(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// the item index
	INT iItem = (INT)wParam;
	if (iItem == -1)
		return 0;

	// get the 1st and 2nd subitem texts
	TCHAR szText[MAX_PATH];
	ListView_GetItemText(m_id_list_dlg.m_hLst1, iItem, 0, szText, _countof(szText));
	MString name = szText;      // 1st is name
	ListView_GetItemText(m_id_list_dlg.m_hLst1, iItem, 1, szText, _countof(szText));
	MString strTypes = szText;  // 2nd is types

	// split the text to the types by slash
	std::vector<MString> vecTypes;
	mstr_split(vecTypes, strTypes, L"/");

	// ignore if no type
	if (vecTypes.empty() || vecTypes.size() <= size_t(lParam))
		return 0;

	// do ID jump
	OnIDJumpBang2(hwnd, name, vecTypes[lParam]);

	return 0;
}

MRisohAutoComplete::MRisohAutoComplete(INT type, BOOL bUILanguage, const MIdOrString& res_type)
{
	m_type = type;
	m_nCurrentElement = 0;
	m_nRefCount = 1;
	m_fBound = FALSE;
	m_pAC = NULL;

	if (type == 0) // Types
	{
		if (InitTypes())
		{
			for (auto& type : g_types)
			{
				push_back(type);
			}
		}
	}
	else if (type == 1) // Names
	{
		if (InitNames(res_type))
		{
			for (auto& name : g_names)
			{
				push_back(name);
			}
		}
	}
	else if (type == 2) // Languages
	{
		for (auto& lang : g_langs)
		{
			if (bUILanguage && !IsValidUILang(lang.LangID))
				continue;

			push_back(lang.str);
		}
	}
	else if (type == 3) // Keys
	{
		if (InitKeys())
		{
			for (auto& key : g_keys)
			{
				push_back(key);
			}
		}
	}
	else if (type == 4) // Control IDs
	{
		if (InitCtrlIDs())
		{
			for (auto& id : g_ctrl_ids)
			{
				push_back(id);
			}
		}
	}
	else if (type == 5) // String IDs
	{
		if (InitStringIDs())
		{
			for (auto& id : g_string_ids)
			{
				push_back(id);
			}
		}
	}
	else if (type == 6) // Help IDs
	{
		if (InitHelpIDs())
		{
			for (auto& id : g_help_ids)
			{
				push_back(id);
			}
		}
	}
}

// start up the program
BOOL MMainWnd::StartDx()
{
	// get cursors for MSplitterWnd
	MSplitterWnd::CursorNS() = LoadCursor(m_hInst, MAKEINTRESOURCE(IDC_CURSORNS));
	MSplitterWnd::CursorWE() = LoadCursor(m_hInst, MAKEINTRESOURCE(IDC_CURSORWE));

	// get the main icon
	m_hIcon = LoadIconDx(IDI_MAIN);
	m_hIconSm = LoadSmallIconDx(IDI_MAIN);

	// get the access keys
	m_hAccel = ::LoadAccelerators(m_hInst, MAKEINTRESOURCE(IDR_MAINACCEL));

	// create the main window
	if (!CreateWindowDx(NULL, MAKEINTRESOURCE(IDS_TITLE),
		WS_OVERLAPPEDWINDOW, 0, CW_USEDEFAULT, CW_USEDEFAULT, 760, 480))
	{
		ErrorBoxDx(TEXT("failure of CreateWindow"));
		return FALSE;
	}
	assert(IsWindow(m_hwnd));

	if (!g_bNoGuiMode)
	{
		// maximize or not
		if (g_settings.bResumeWindowPos && g_settings.bMaximized)
		{
			ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
		}
		else
		{
			ShowWindow(m_hwnd, SW_SHOWNORMAL);
		}
	}

	UpdateWindow(m_hwnd);

	return TRUE;
}

// do one window message
void MMainWnd::DoEvents()
{
	MSG msg;
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		DoMsg(msg);
	}
}

static BOOL DoMsgCtrlA(MSG *pMsg)
{
	WCHAR szClass[MAX_PATH] = L"";
	GetClassNameW(pMsg->hwnd, szClass, _countof(szClass));

	if (lstrcmpiW(szClass, L"EDIT") == 0 || lstrcmpiW(szClass, L"LineNumEdit") == 0)
	{
		if (pMsg->message == WM_KEYDOWN)
		{
			if (pMsg->wParam == 'A' &&
				GetAsyncKeyState(VK_CONTROL) < 0 &&
				GetAsyncKeyState(VK_SHIFT) >= 0 &&
				GetAsyncKeyState(VK_MENU) >= 0)
			{
				PeekMessage(pMsg, pMsg->hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE);
				PostMessage(pMsg->hwnd, EM_SETSEL, 0, -1);
				return TRUE;
			}
		}
	}

	return FALSE;
}

// do the window messages
void MMainWnd::DoMsg(MSG& msg)
{
	// Ctrl+A on EDIT control and LineNumEdit
	if (DoMsgCtrlA(&msg))
		return;

	// do access keys
	if (IsWindow(m_hwnd))
	{
		if (::TranslateAccelerator(m_hwnd, m_hAccel, &msg))
			return;
	}

	if (IsWindow(m_arrow.m_dialog))
	{
		if (msg.message == WM_KEYDOWN)
		{
			if (m_arrow.DoComplement(m_arrow, msg.wParam))
				return;
		}
		if (::IsDialogMessage(m_arrow.m_dialog, &msg))
			return;
	}

	// do the popup windows
	if (IsWindow(m_rad_window))
	{
		if (::TranslateAccelerator(m_rad_window, m_hAccel, &msg))
			return;
	}

	if (IsWindow(m_rad_window.m_rad_dialog))
	{
		if (::TranslateAccelerator(m_rad_window.m_rad_dialog, m_hAccel, &msg))
			return;
		if (::IsDialogMessage(m_rad_window.m_rad_dialog, &msg))
			return;
	}

	if (IsWindow(m_id_list_dlg))
	{
		if (::TranslateAccelerator(m_id_list_dlg, m_hAccel, &msg))
			return;
		if (::IsDialogMessage(m_id_list_dlg, &msg))
			return;
	}

	// F1
	if (msg.message == WM_KEYDOWN && msg.wParam == VK_F1 &&
		GetKeyState(VK_CONTROL) >= 0 && GetKeyState(VK_SHIFT) >= 0 &&
		GetKeyState(VK_MENU) >= 0)
	{
		DoHelp();
	}

	// close the find/replace dialog if any
	if (IsWindow(m_hFindReplaceDlg))
	{
		if (::IsDialogMessage(m_hFindReplaceDlg, &msg))
			return;
	}

	// do the item search dialog
	if (MItemSearchDlg::Dialog())
	{
		HWND hDlg = *MItemSearchDlg::Dialog();
		if (::IsDialogMessage(hDlg, &msg))
			return; // processed
	}

	if (s_hwndEga && ::IsDialogMessageW(s_hwndEga, &msg))
		return;

	// the default processing
	TranslateMessage(&msg);
	DispatchMessage(&msg);
}

// the main loop
INT_PTR MMainWnd::RunDx()
{
	MSG msg;

	while (INT bGot = ::GetMessage(&msg, NULL, 0, 0))
	{
		if (bGot < 0)   // fatal error
		{
			MTRACE(TEXT("Application fatal error: %ld\n"), GetLastError());
			DebugBreak();
			return -1;
		}

		if (::IsWindow(s_hwndEga) && ::IsDialogMessage(s_hwndEga, &msg))
			continue;

		// do messaging
		DoMsg(msg);
	}

	return INT(msg.wParam);
}

////////////////////////////////////////////////////////////////////////////

BOOL MMainWnd::ParseCommandLine(HWND hwnd, INT argc, WCHAR **targv)
{
	LPWSTR file = NULL;
	BOOL bNoGUI = FALSE;
	m_commands.clear();
	for (INT iarg = 1; iarg < argc; ++iarg)
	{
		LPWSTR arg = targv[iarg];
		if (lstrcmpiW(arg, L"-help") == 0 ||
			lstrcmpiW(arg, L"--help") == 0 ||
			lstrcmpiW(arg, L"/?") == 0)
		{
			MessageBoxW(NULL, LoadStringDx(IDS_USAGE), LoadStringDx(IDS_APPNAME), MB_ICONINFORMATION);
			return FALSE;
		}
		if (lstrcmpiW(arg, L"-version") == 0 ||
			lstrcmpiW(arg, L"--version") == 0)
		{
			MessageBoxW(NULL, LoadStringDx(IDS_TITLE), LoadStringDx(IDS_APPNAME), MB_ICONINFORMATION);
			return FALSE;
		}

		if (lstrcmpiW(arg, L"-load") == 0 ||
			lstrcmpiW(arg, L"--load") == 0)
		{
			if (iarg + 1 < argc)
			{
				bNoGUI = TRUE;
				arg = targv[++iarg];
				m_commands += L"load:";
				m_commands += arg;
				m_commands += L"\n";
			}
			continue;
		}
		if (lstrcmpiW(arg, L"-load-options") == 0 ||
			lstrcmpiW(arg, L"--load-options") == 0)
		{
			if (iarg + 1 < argc)
			{
				arg = targv[++iarg];
				m_load_options = arg;
			}
			continue;
		}

		if (lstrcmpiW(arg, L"-save") == 0 ||
			lstrcmpiW(arg, L"--save") == 0)
		{
			if (iarg + 1 < argc)
			{
				bNoGUI = TRUE;
				arg = targv[++iarg];
				m_commands += L"save:";
				m_commands += arg;
				m_commands += L"\n";
			}
			continue;
		}
		if (lstrcmpiW(arg, L"-save-options") == 0 ||
			lstrcmpiW(arg, L"--save-options") == 0)
		{
			if (iarg + 1 < argc)
			{
				arg = targv[++iarg];
				m_save_options = arg;
			}
			continue;
		}

		if (lstrcmpiW(arg, L"-log-file") == 0 ||
			lstrcmpiW(arg, L"--log-file") == 0)
		{
			if (iarg + 1 < argc)
			{
				arg = targv[++iarg];
				g_pszLogFile = arg;
			}
			continue;
		}

		if (PathFileExistsW(arg))
		{
			if (!file)
				file = arg;
			continue;
		}
	}

	if (file && !bNoGUI)
	{
		// load the file now
		DoLoadFile(hwnd, file);
		return TRUE;
	}
	else if (bNoGUI)
	{
		++g_bNoGuiMode;
		return TRUE;
	}

	return FALSE;
}

INT
RisohEditor_Main(
	HINSTANCE   hInstance,
	HINSTANCE   hPrevInstance,
	LPWSTR      lpCmdLine,
	INT         nCmdShow)
{
#if defined(NDEBUG) && defined(PROTECTION)
#if (_WIN32_WINNT >= 0x0501)
	BOOL bDebuggerPresent;
	if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &bDebuggerPresent) && bDebuggerPresent)
		return 1;
#endif
#endif // defined(NDEBUG) && defined(PROTECTION)

	SetEnvironmentVariableW(L"LANG", L"en_US");
	setlocale(LC_CTYPE, "");

	// set the UI language
	g_settings.ui_lang = GetUILang();
	SetThreadUILanguage(LANGID(g_settings.ui_lang));

	// initialize ID Type mappings
    InitIDTypeMaps();

	// register MOleSite window class
	MOleSite::RegisterDx();

	// initialize common controls
	INITCOMMONCONTROLSEX iccx;
	iccx.dwSize = sizeof(iccx);
	iccx.dwICC = ICC_WIN95_CLASSES |
				 ICC_DATE_CLASSES |
				 ICC_USEREX_CLASSES |
				 ICC_COOL_CLASSES |
				 ICC_INTERNET_CLASSES |
				 ICC_PAGESCROLLER_CLASS |
				 ICC_NATIVEFNTCTL_CLASS |
				 ICC_STANDARD_CLASSES |
				 ICC_LINK_CLASS;
	InitCommonControlsEx(&iccx);

	// load RichEdit
	HINSTANCE hinstRichEdit = LoadLibrary(TEXT("riched20.dll"));
	HINSTANCE hinstMSFTEDIT = LoadLibrary(TEXT("msftedit.dll"));

	HINSTANCE hinstUXTheme = LoadLibrary(TEXT("UXTHEME.DLL"));
	FARPROC fn = GetProcAddress(hinstUXTheme, "SetWindowTheme");
	s_pSetWindowTheme = *reinterpret_cast<SETWINDOWTHEME *>(&fn);

	// load LineNumEdit
	LineNumEdit::SuperclassWindow();

	// load GDI+
	Gdiplus::GdiplusStartupInput gp_startup_input;
	ULONG_PTR gp_token;
	Gdiplus::GdiplusStartup(&gp_token, &gp_startup_input, NULL);

	// main process
	s_ret = 0;
	MEditCtrl::SetCtrlAHookDx(TRUE);
	{
#ifdef ATL_SUPPORT
		::AtlAxWinInit();
		CComModule _Module;
#endif
		{
			MMainWnd app(__argc, __targv, hInstance);
			s_pMainWnd = &app;

			if (app.StartDx())
			{
				// main loop
				app.RunDx();
			}
			else
			{
				s_ret = 2;
			}
		}
#ifdef ATL_SUPPORT
		::AtlAxWinTerm();
#endif
	}
	MEditCtrl::SetCtrlAHookDx(FALSE);

	// free GDI+
	Gdiplus::GdiplusShutdown(gp_token);

	// free the libraries
	FreeLibrary(hinstMSFTEDIT);
	FreeLibrary(hinstRichEdit);
	FreeLibrary(hinstUXTheme);
	FreeWCLib();

	// release ID Type mappings
    UnloadIDTypeMaps();

	// check object counts
	assert(MacroParser::BaseAst::alive_count() == 0);

#if (WINVER >= 0x0500)
	HANDLE hProcess = GetCurrentProcess();
	MTRACEA("Count of GDI objects: %ld\n", GetGuiResources(hProcess, GR_GDIOBJECTS));
	MTRACEA("Count of USER objects: %ld\n", GetGuiResources(hProcess, GR_USEROBJECTS));
#endif

	return s_ret;
}

////////////////////////////////////////////////////////////////////////////

#include "WowFsRedirection.h"

// the main function of the windows application
extern "C"
INT WINAPI
wWinMain(HINSTANCE   hInstance,
		 HINSTANCE   hPrevInstance,
		 LPWSTR      lpCmdLine,
		 INT         nCmdShow)
{
#if defined(NDEBUG) && defined(PROTECTION)
	if (IsDebuggerPresent())
		return 1;
#endif // defined(NDEBUG) && defined(PROTECTION)

#if defined(_MSC_VER) && !defined(NDEBUG)
	// for detecting memory leak (MSVC only)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// Disable WoW64 filesystem redirection
	PVOID OldValue;
	BOOL bWowFsDisabled = DisableWow64FsRedirection(&OldValue);

	std::srand(GetTickCount());

	// Initialize OLE
	HRESULT hrOleInit = OleInitialize(NULL);

	// The body
	INT ret = RisohEditor_Main(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

	// release strings
	g_types.clear();
	g_names.clear();
	g_keys.clear();
	g_ctrl_ids.clear();
	g_string_ids.clear();
	g_help_ids.clear();
	g_encrypted_types.clear();
	g_font_names.clear();

	// Revert WoW64 filesystem redirection
	if (bWowFsDisabled)
		RevertWow64FsRedirection(OldValue);

	// Uninitialize OLE
	if (SUCCEEDED(hrOleInit))
		OleUninitialize();

	// Clean up
	g_RES_select_type.clear();
	g_RES_select_name.clear();
	assert(EntryBaseBase::is_alive_zero());

	return ret;
}

//////////////////////////////////////////////////////////////////////////////
