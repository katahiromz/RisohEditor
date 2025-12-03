// RisohEditor.cpp --- RisohEditor
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "RisohEditor.hpp"
#include "MMainWnd.hpp"
#define LINENUMEDIT_IMPL
#include "LineNumEdit.hpp"
#include "MChooseLangDlg.hpp"
#include "ToolbarRes.hpp"
#include "Utils.h"
#include <thread>

LPWSTR g_pszLogFile = NULL;
bool g_wrap_enabled = false;

//////////////////////////////////////////////////////////////////////////////
// constants

#define CX_STATUS_PART  80      // status bar part width
#define ERROR_LINE_COLOR RGB(255, 191, 191)

#define MYWM_UPDATELANGARROW (WM_USER + 114)
#define MYWM_GETDLGHEADLINES (WM_USER + 250)

// the maximum number of backup
static const UINT s_nBackupMaxCount = 5;

// contents modified?
static BOOL s_bModified = FALSE;

void DoSetFileModified(BOOL bModified)
{
	s_bModified = bModified;
}

HWND g_hMainWnd = NULL;
static INT s_ret = 0;

//////////////////////////////////////////////////////////////////////////////
// global variables

#ifdef USE_GLOBALS
	ConstantsDB g_db;           // constants database
	RisohSettings g_settings;   // settings
	EntrySet g_res;             // the set of resource items
#endif

typedef HRESULT (WINAPI *SETWINDOWTHEME)(HWND, LPCWSTR, LPCWSTR);
static SETWINDOWTHEME s_pSetWindowTheme = NULL;

//////////////////////////////////////////////////////////////////////////////
// the specialized toolbar

// buttons info #0
TBBUTTON g_buttons0[] =
{
	{ 11, ID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_GUIEDIT },
	{ 12, ID_TEST, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TEST },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
	{ 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
	{ 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
	{ 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
	{ 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
	{ 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
	{ 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
	{ 14, ID_EXPORTRES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPORT },
};

// buttons info #1
TBBUTTON g_buttons1[] =
{
	{ 12, ID_TEST, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TEST },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
	{ 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
	{ 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
	{ 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
	{ 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
	{ 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
	{ 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
	{ 14, ID_EXPORTRES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPORT },
};

// buttons info #2
TBBUTTON g_buttons2[] =
{
	{ 9, ID_COMPILE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_RECOMPILE },
	{ 10, ID_CANCELEDIT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CANCELEDIT },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
	{ 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
	{ 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
	{ 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
	{ 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
	{ 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
	{ 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
	{ 14, ID_EXPORTRES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPORT },
};

// buttons info #3
TBBUTTON g_buttons3[] =
{
	{ 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
	{ 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
	{ 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
	{ 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
	{ 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
	{ 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
	{ 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
	{ 14, ID_EXPORTRES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPORT },
};

// buttons info #4
TBBUTTON g_buttons4[] =
{
	{ 11, ID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_GUIEDIT },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
	{ 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
	{ 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
	{ 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
	{ 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
	{ 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
	{ 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
	{ 14, ID_EXPORTRES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPORT },
};

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

// check whether it needs compilation
LRESULT MMainWnd::OnCompileCheck(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// compile if necessary
	return CompileIfNecessary(TRUE);
}

// reopen the RADical window
LRESULT MMainWnd::OnReopenRad(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	OnGuiEdit(hwnd);
	return 0;
}

// report the position and size to the status bar
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

// clear the status bar
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

	WCHAR szValue[16];
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

void MMainWnd::OnCheckUpdate(HWND hwnd)
{
	std::wstring local_version = GetRisohEditorVersion();
	if (local_version.empty())
	{
		ErrorBoxDx(IDS_CANTCHECKUPDATE);
		return;
	}

	WCHAR szPath[MAX_PATH], szFile[MAX_PATH];
	GetTempPathW(_countof(szPath), szPath);
	GetTempFileNameW(szPath, L"Upd", 0, szFile);

	std::wstring page = L"https://katahiromz.web.fc2.com/re/version.html";
	DeleteUrlCacheEntryW(page.c_str());
	HRESULT hr = URLDownloadToFileW(NULL, page.c_str(), szFile, 0, NULL);
	if (FAILED(hr))
	{
		ErrorBoxDx(IDS_CANTCHECKUPDATE);
		return;
	}

	std::wstring url;
	std::wstring remote_version = ParseVersionFile(szFile, url);
	DeleteFileW(szFile);
	if (remote_version.empty() || url.empty())
	{
		ErrorBoxDx(IDS_CANTCHECKUPDATE);
		return;
	}

	WCHAR szText[256];
	if (local_version < remote_version)
	{
		StringCbPrintfW(szText, sizeof(szText), LoadStringDx(IDS_THEREISUPDATE),
						remote_version.c_str());
		if (MsgBoxDx(szText, MB_ICONINFORMATION | MB_YESNOCANCEL) == IDYES)
		{
			ShellExecuteW(hwnd, NULL, url.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
	}
	else
	{
		MsgBoxDx(IDS_NOUPDATE, MB_ICONINFORMATION);
	}
}

void MMainWnd::OnExportRes(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// get the selected entry
	auto e = g_res.get_entry();
	if (!e)
		return;     // not selected

	if (e->is_delphi_dfm())
		return OnExtractDFM(hwnd);

	// initialize OPENFILENAME structure
	WCHAR szFile[MAX_PATH] = L"";
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTRES);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"res";   // the default extension

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// extract it to a file
		if (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"res") == 0)
		{
			// it was a *.res file
			if (!g_res.extract_res(ofn.lpstrFile, e))
			{
				ErrorBoxDx(IDS_CANNOTSAVE);
			}
		}
		else
		{
			// it was not a *.res file
			if (!g_res.extract_bin(ofn.lpstrFile, e))
			{
				ErrorBoxDx(IDS_CANNOTSAVE);
			}
		}
	}
}

// extract the binary as a file
void MMainWnd::OnExtractBin(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// get the selected entry
	auto e = g_res.get_entry();
	if (!e)
		return;     // not selected

	if (e->is_delphi_dfm())
		return OnExtractDFM(hwnd);

	if (e->m_type == L"TYPELIB")
		return OnExtractTLB(hwnd);

	// initialize OPENFILENAME structure
	WCHAR szFile[MAX_PATH] = L"";
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);

	// use the prefered filter by the entry
	switch (e->m_et)
	{
	case ET_STRING:
	case ET_MESSAGE:
	case ET_TYPE:
	case ET_NAME:
		ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
		ofn.lpstrDefExt = L"res";
		break;
	case ET_LANG:
		if (e->m_type == L"PNG")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_PNGRESBINFILTER));
			ofn.lpstrDefExt = L"png";
		}
		else if (e->m_type == L"JPEG")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_JPEGRESBINFILTER));
			ofn.lpstrDefExt = L"jpg";
		}
		else if (e->m_type == L"GIF")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_GIFRESBINFILTER));
			ofn.lpstrDefExt = L"gif";
		}
		else if (e->m_type == L"TIFF")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_TIFFRESBINFILTER));
			ofn.lpstrDefExt = L"tif";
		}
		else if (e->m_type == L"AVI")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_AVIRESBINFILTER));
			ofn.lpstrDefExt = L"avi";
		}
		else if (e->m_type == L"WAVE")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_WAVERESBINFILTER));
			ofn.lpstrDefExt = L"wav";
		}
		else if (e->m_type == L"MP3")
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_MP3RESBINFILTER));
			ofn.lpstrDefExt = L"mp3";
		}
		else
		{
			ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESBINFILTER));
			ofn.lpstrDefExt = L"res";
		}
		break;
	default:
		return;
	}

	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTRES);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// extract it to a file
		if (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"res") == 0)
		{
			// it was a *.res file
			if (!g_res.extract_res(ofn.lpstrFile, e))
			{
				ErrorBoxDx(IDS_CANNOTSAVE);
			}
		}
		else
		{
			// it was not a *.res file
			if (!g_res.extract_bin(ofn.lpstrFile, e))
			{
				ErrorBoxDx(IDS_CANNOTSAVE);
			}
			else
			{
				// Notify change of file icon
				MyChangeNotify(ofn.lpstrFile);
			}
		}
	}
}

void MMainWnd::OnExtractRC(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// get the selected entry
	auto e = g_res.get_entry();
	if (!e)
		return;     // not selected

	// initialize OPENFILENAME structure
	WCHAR szFile[MAX_PATH] = L"";
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RCFILTER));
	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTRES);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"rc";   // the default extension

	// use the prefered filter by the entry
	EntrySet found;
	MIdOrString type = e->m_type;
	switch (e->m_et)
	{
	case ET_STRING:
		g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, e->m_lang);
		break;
	case ET_MESSAGE:
		g_res.search(found, ET_LANG, RT_MESSAGETABLE, BAD_NAME, e->m_lang);
		break;
	case ET_TYPE:
		if (type == RT_ICON)
			type = RT_GROUP_ICON;
		else if (type == RT_CURSOR)
			type = RT_GROUP_CURSOR;
		g_res.search(found, ET_LANG, type, BAD_NAME, BAD_LANG);
		break;
	case ET_NAME:
		g_res.search(found, ET_LANG, type, e->m_name, BAD_LANG);
		break;
	case ET_LANG:
		g_res.search(found, ET_LANG, type, e->m_name, e->m_lang);
		break;
	default:
		return;
	}

	if (found.empty())
	{
		ErrorBoxDx(IDS_DATAISEMPTY);
		return;
	}

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// show the "export options" dialog
		MExportOptionsDlg dialog;
		if (dialog.DialogBoxDx(hwnd) != IDOK)
			return;

		if (!DoExportRC(szFile, NULL, found))
		{
			ErrorBoxDx(IDS_CANTEXPORT);
		}
	}
}

void MMainWnd::PostUpdateLangArrow(HWND hwnd)
{
	PostMessage(hwnd, MYWM_UPDATELANGARROW, 0, 0);
}

// MYWM_UPDATELANGARROW
LRESULT MMainWnd::OnUpdateLangArrow(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UpdateLangArrow();
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

// extract an icon as an *.ico file
void MMainWnd::OnExtractIcon(HWND hwnd)
{
	enum IconFilterIndex  // see also: IDS_ICOFILTER
	{
		IFI_NONE = 0,
		IFI_ICO = 1,
		IFI_ANI = 2,
		IFI_ALL = 3
	};

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
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ICOFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTICO);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	// use the prefered filter by the entry
	if (entry->m_type == RT_ANIICON)
	{
		ofn.nFilterIndex = IFI_ANI;
		ofn.lpstrDefExt = L"ani";   // the default extension
	}
	else
	{
		ofn.nFilterIndex = IFI_ICO;
		ofn.lpstrDefExt = L"ico";   // the default extension
	}

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// extract it to an *.ico or *.ani file
		if (!g_res.extract_icon(ofn.lpstrFile, entry))
		{
			ErrorBoxDx(IDS_CANTEXTRACTICO);
		}
		else
		{
			// Notify change of file icon
			MyChangeNotify(ofn.lpstrFile);
		}
	}
}

// extract a cursor as an *.cur or *.ani file
void MMainWnd::OnExtractCursor(HWND hwnd)
{
	enum CursorFilterIndex      // see also: IDS_CURFILTER
	{
		CFI_NONE = 0,
		CFI_CUR = 1,
		CFI_ANI = 2,
		CFI_ALL = 3
	};

	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)     // not selected
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
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_CURFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTCUR);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	// use the prefered filter by the entry
	if (entry->m_type == RT_ANICURSOR)
	{
		ofn.nFilterIndex = CFI_ANI;
		ofn.lpstrDefExt = L"ani";   // the default extension
	}
	else
	{
		ofn.nFilterIndex = CFI_CUR;
		ofn.lpstrDefExt = L"cur";   // the default extension
	}

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// extract it to an *.cur or *.ani file
		if (!g_res.extract_cursor(ofn.lpstrFile, entry))
		{
			ErrorBoxDx(IDS_CANTEXTRACTCUR);
		}
		else
		{
			// Notify change of file icon
			MyChangeNotify(ofn.lpstrFile);
		}
	}
}

// extract a bitmap as an *.bmp file
void MMainWnd::OnExtractBitmap(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)     // not selected
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
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_BMPFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTBMP);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"bmp";   // the default extension

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// extract a bitmap as an *.bmp or *.png file
		BOOL bPNG = (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"png") == 0);
		if (!PackedDIB_Extract(ofn.lpstrFile, &(*entry)[0], (*entry).size(), bPNG))
		{
			ErrorBoxDx(IDS_CANTEXTRACTBMP);
		}
		else
		{
			// Notify change of file icon
			MyChangeNotify(ofn.lpstrFile);
		}
	}
}

// replace the resource data by a binary file
void MMainWnd::OnReplaceBin(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)
		return;

	// show the dialog
	MReplaceBinDlg dialog(entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);
	}

	DoSetFileModified(TRUE);
}

// version info
void MMainWnd::OnAbout(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MVersionInfoDlg dialog;
	dialog.DialogBoxDx(hwnd);
}

// show the MFontsDlg to allow the user to change the font settings
void MMainWnd::OnFonts(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the fonts dialog
	MFontsDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// update m_hBinFont and set it to m_hHexViewer
		DeleteObject(m_hBinFont);
		m_hBinFont = dialog.DetachBinFont();
		SetWindowFont(m_hHexViewer, m_hBinFont, TRUE);

		// update m_hSrcFont and set it to m_hCodeEditor
		DeleteObject(m_hSrcFont);
		m_hSrcFont = dialog.DetachSrcFont();
		SetWindowFont(m_hCodeEditor, m_hSrcFont, TRUE);
	}
}

// export all the resource items to an RC file
void MMainWnd::OnExport(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// initialize OPENFILENAME structure
	WCHAR file[MAX_PATH] = TEXT("");
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXPORTFILTER));
	ofn.lpstrFile = file;
	ofn.nMaxFile = _countof(file);
	ofn.lpstrTitle = LoadStringDx(IDS_EXPORT);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"rc";    // the default extension

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// do export!
		if (ofn.nFilterIndex == 2) // .res
		{
			if (!DoExportRes(file))
			{
				ErrorBoxDx(IDS_CANTEXPORT);
			}
		}
		else // .rc or .rc2
		{
			// show the "export options" dialog
			MExportOptionsDlg dialog;
			if (dialog.DialogBoxDx(hwnd) != IDOK)
				return;

			if (!DoExportRC(file))
			{
				ErrorBoxDx(IDS_CANTEXPORT);
			}
		}
	}
}

// the window class libraries
typedef std::unordered_set<HMODULE> wclib_t;
wclib_t s_wclib;

// is there a window class that is named pszName?
BOOL IsThereWndClass(const WCHAR *pszName)
{
	if (!pszName || pszName[0] == 0)
		return FALSE;   // failure

	WNDCLASSEX cls;
	if (GetClassInfoEx(NULL, pszName, &cls) ||
		GetClassInfoEx(GetModuleHandle(NULL), pszName, &cls))
	{
		return TRUE;    // already exists
	}

	// in the window class libraries?
	for (auto& library : s_wclib)
	{
		if (GetClassInfoEx(library, pszName, &cls))
			return TRUE;    // found
	}

	// CLSID?
	if (pszName[0] == L'{' &&
		pszName[9] == L'-' && pszName[14] == L'-' &&
		pszName[19] == L'-' && pszName[24] == L'-' &&
		pszName[37] == L'}')
	{
		return TRUE;        // it's a CLSID
	}

	// ATL OLE control?
	if (std::wstring(pszName).find(L"AtlAxWin") == 0)
		return TRUE;        // it's an ATL OLE control

	return FALSE;   // failure
}

// release all the window class libraries
void FreeWCLib()
{
	for (auto& library : s_wclib)
	{
		FreeLibrary(library);
		//library = NULL;
	}
	s_wclib.clear();
}

// load a window class library
void MMainWnd::OnLoadWCLib(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	WCHAR file[MAX_PATH] = TEXT("");

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_DLLFILTER));
	ofn.lpstrFile = file;
	ofn.nMaxFile = _countof(file);
	ofn.lpstrTitle = LoadStringDx(IDS_LOADWCLIB);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"dll";       // the default extension

	// let the user choose the path
	if (GetOpenFileNameW(&ofn))
	{
		// load the window class library
		HMODULE hMod = LoadLibraryW(file);
		if (hMod)
		{
			// success. add it
			s_wclib.insert(hMod);
		}
		else
		{
			ErrorBoxDx(IDS_CANNOTLOAD);
		}
	}
}

// import the resource data additionally
void MMainWnd::OnImport(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	WCHAR file[MAX_PATH] = TEXT("");

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_IMPORTFILTER));
	ofn.lpstrFile = file;
	ofn.nMaxFile = _countof(file);
	ofn.lpstrTitle = LoadStringDx(IDS_IMPORTRES);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"res";       // the default extension

	// let the user choose the path
	if (GetOpenFileNameW(&ofn))
	{
		// find the file title
		LPCWSTR pch = wcsrchr(file, L'\\');
		if (pch == NULL)
			pch = wcsrchr(file, L'/');
		if (pch == NULL)
			pch = file;
		else
			++pch;

		// find the dot extension
		pch = wcsrchr(pch, L'.');

		if (IMPORT_FAILED == DoImport(hwnd, file, pch))
		{
			ErrorBoxDx(IDS_CANNOTIMPORT);
		}
		DoSetFileModified(TRUE);
	}
}

// open a file
void MMainWnd::OnOpen(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	if (!DoQuerySaveChange(hwnd))
		return;

	// store the nominal path
	WCHAR szFile[MAX_PATH];
	StringCchCopyW(szFile, _countof(szFile), m_szFile);

	// if path was not valid, make it empty
	if (!PathFileExistsW(szFile))
		szFile[0] = 0;

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXERESRCFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_OPEN);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"exe";       // the default extension

	// let the user choose the path
	if (GetOpenFileNameW(&ofn))
	{
		// load the file
		DoLoadFile(hwnd, szFile, ofn.nFilterIndex);
	}
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

// clear all the resource data
void MMainWnd::OnNew(HWND hwnd)
{
	if (!DoQuerySaveChange(hwnd))
		return;

	// close preview
	HidePreview();

	// unload the resource.h file
	OnUnloadResH(hwnd);

	// update the file info
	UpdateFileInfo(FT_NONE, NULL, FALSE);

	// unselect
	SelectTV(NULL, FALSE);

	// clean up
	g_res.delete_all();

	// update modified flag
	DoSetFileModified(FALSE);

	// update language arrow
	PostUpdateLangArrow(hwnd);
}

enum ResFileFilterIndex     // see also: IDS_EXERESFILTER
{
	RFFI_NONE = 0,
	RFFI_EXECUTABLE = 1,
	RFFI_RC = 2,
	RFFI_RES = 3,
	RFFI_ALL = 4,
};

// save as a file or files
BOOL MMainWnd::OnSaveAs(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return FALSE;

	// store m_szFile to szFile
	WCHAR szFile[MAX_PATH];
	StringCchCopyW(szFile, _countof(szFile), m_szFile);

	// if not found, then make it empty
	if (!PathFileExistsW(szFile))
		szFile[0] = 0;

	// was it an executable?
	BOOL bWasExecutable = (m_file_type == FT_EXECUTABLE);

	// get and delete the filename extension
	WCHAR szExt[32] = L"";
	LPWSTR pch = wcsrchr(szFile, L'.');
	static const LPCWSTR s_DotExts[] =
	{
		L".exe", L".dll", L".ocx", L".cpl", L".scr", L".mui", L".rc", L".rc2", L".res"
	};
	for (auto ext : s_DotExts)
	{
		if (lstrcmpiW(pch, ext) == 0)
		{
			StringCbCopyW(szExt, sizeof(szExt), ext + 1);
			*pch = 0;
			break;
		}
	}

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXERESFILTER));

	// use the prefered filter by the entry
	ofn.nFilterIndex = g_settings.nSaveFilterIndex;
	if (bWasExecutable)
	{
		if (ofn.nFilterIndex != RFFI_EXECUTABLE)
			ofn.nFilterIndex = RFFI_EXECUTABLE;
	}
	else
	{
		if (ofn.nFilterIndex == RFFI_EXECUTABLE)
			ofn.nFilterIndex = RFFI_RC;
	}

	// use the preferred extension
	switch (ofn.nFilterIndex)
	{
	case RFFI_EXECUTABLE:
		if (szExt[0])
		{
			ofn.lpstrDefExt = szExt;
		}
		else
		{
			ofn.lpstrDefExt = L"exe";       // the default extension
		}
		break;

	case RFFI_RC:
		ofn.lpstrDefExt = L"rc";        // the default extension
		break;

	case RFFI_RES:
	default:
		ofn.lpstrDefExt = L"res";       // the default extension
		break;
	}

	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_SAVEAS);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// save the filter index to the settings
		g_settings.nSaveFilterIndex = ofn.nFilterIndex;

		if (ofn.nFilterIndex == RFFI_ALL)
		{
			ofn.nFilterIndex = RFFI_EXECUTABLE;
		}

		switch (ofn.nFilterIndex)
		{
		case RFFI_EXECUTABLE:
			// save it
			if (DoSaveAs(szFile))
			{
				m_nStatusStringID = IDS_FILESAVED;
				return TRUE;
			}
			ErrorBoxDx(IDS_CANNOTSAVE);
			break;

		case RFFI_RC:
			// export and save it
			{
				// show "save options" dialog
				MSaveOptionsDlg save_options;
				if (save_options.DialogBoxDx(hwnd) != IDOK)
					return FALSE;

				// export
				WCHAR szResH[MAX_PATH] = L"";
				if (DoExportRC(szFile, szResH))   // succeeded
				{
					// save the resource.h path
					StringCchCopyW(m_szResourceH, _countof(m_szResourceH), szResH);

					// update the file info
					UpdateFileInfo(FT_RC, szFile, FALSE);

					m_nStatusStringID = IDS_FILESAVED;
					return TRUE;
				}
				else
				{
					ErrorBoxDx(IDS_CANNOTSAVE);
				}
			}
			break;

		case RFFI_RES:
			// save the *.res file
			if (DoSaveResAs(szFile))
			{
				m_nStatusStringID = IDS_FILESAVED;
				return TRUE;
			}
			ErrorBoxDx(IDS_CANNOTSAVE);
			break;

		default:
			assert(0);
			break;
		}
	}

	return FALSE;
}

BOOL MMainWnd::OnSave(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return FALSE;

	if (!m_szFile[0])
	{
		return OnSaveAs(hwnd);
	}

	LPWSTR pchDotExt = PathFindExtensionW(m_szFile);
	if (lstrcmpiW(pchDotExt, L".res") == 0)
	{
		g_settings.nSaveFilterIndex = RFFI_RES;
	}
	else if (lstrcmpiW(pchDotExt, L".rc") == 0 || lstrcmpiW(pchDotExt, L".rc2") == 0)
	{
		g_settings.nSaveFilterIndex = RFFI_RC;
	}
	else
	{
		g_settings.nSaveFilterIndex = RFFI_EXECUTABLE;
	}

	switch (g_settings.nSaveFilterIndex)
	{
	case RFFI_EXECUTABLE:
		// save it
		if (DoSaveAs(m_szFile))
		{
			m_nStatusStringID = IDS_FILESAVED;
			return TRUE;
		}
		ErrorBoxDx(IDS_CANNOTSAVE);
		break;

	case RFFI_RC:
		// export and save it
		{
			// show "save options" dialog
			MSaveOptionsDlg save_options;
			if (save_options.DialogBoxDx(hwnd) != IDOK)
				return FALSE;

			// export
			WCHAR szResH[MAX_PATH] = L"";
			if (DoExportRC(m_szFile, szResH))   // succeeded
			{
				// save the resource.h path
				StringCchCopyW(m_szResourceH, _countof(m_szResourceH), szResH);

				// update the file info
				UpdateFileInfo(FT_RC, m_szFile, FALSE);

				m_nStatusStringID = IDS_FILESAVED;
				return TRUE;
			}
			else
			{
				ErrorBoxDx(IDS_CANNOTSAVE);
			}
		}
		break;

	case RFFI_RES:
		// save the *.res file
		if (DoSaveResAs(m_szFile))
		{
			m_nStatusStringID = IDS_FILESAVED;
			return TRUE;
		}
		ErrorBoxDx(IDS_CANNOTSAVE);
		break;

	default:
		assert(0);
		break;
	}

	return FALSE;
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

	// initialize LOGFONT structures
	LOGFONTW lfBin, lfSrc;
	ZeroMemory(&lfBin, sizeof(lfBin));
	ZeroMemory(&lfSrc, sizeof(lfSrc));

	// set lfFaceName from settings
	StringCchCopy(lfBin.lfFaceName, _countof(lfBin.lfFaceName), g_settings.strBinFont.c_str());
	StringCchCopy(lfSrc.lfFaceName, _countof(lfSrc.lfFaceName), g_settings.strSrcFont.c_str());

	// calculate the height
	if (HDC hDC = CreateCompatibleDC(NULL))
	{
		lfBin.lfHeight = -MulDiv(g_settings.nBinFontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
		lfSrc.lfHeight = -MulDiv(g_settings.nSrcFontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
		DeleteDC(hDC);
	}

	// create the fonts
	m_hBinFont = CreateFontIndirectW(&lfBin);
	assert(m_hBinFont);
	m_hSrcFont = ::CreateFontIndirectW(&lfSrc);
	assert(m_hSrcFont);

	// set the fonts to the controls
	SetWindowFont(m_hHexViewer, m_hBinFont, TRUE);
	SetWindowFont(m_hCodeEditor, m_hSrcFont, TRUE);
}

// check the text for item search
static bool CheckTextForSearch(ITEM_SEARCH *pSearch, EntryBase *entry, MString text)
{
	// make the text uppercase to ignore case
	if (pSearch->bIgnoreCases)
		CharUpperW(&text[0]);

	// find?
	if (text.find(pSearch->strText) == MString::npos)
		return false;   // not found

	if (pSearch->bDownward)     // go downward
	{
		// check the position
		if (pSearch->pCurrent == NULL || *pSearch->pCurrent < *entry)
		{
			if (pSearch->pFound)    // already found
			{
				if (*entry < *pSearch->pFound)  // compare with the found one
				{
					pSearch->pFound = entry;
					return true;    // found
				}
			}
			else    // not found yet
			{
				pSearch->pFound = entry;    // found
				return true;    // found
			}
		}
	}
	else    // go upward
	{
		// check the position
		if (pSearch->pCurrent == NULL || *entry < *pSearch->pCurrent)
		{
			if (pSearch->pFound)    // already found
			{
				if (*pSearch->pFound < *entry)  // compare with the found one
				{
					pSearch->pFound = entry;
					return true;    // found
				}
			}
			else
			{
				pSearch->pFound = entry;
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
		case ET_MESSAGE:
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
		CharUpperW(&search.strText[0]);

	search_proc(&search);

	return search.pFound != NULL;
}

// clone the resource item in new name
void MMainWnd::OnCopyAsNewName(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected name entry
	auto entry = g_res.get_entry();
	if (!entry || entry->m_et != ET_NAME)
		return;

	// show the dialog
	MCloneInNewNameDlg dialog(entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// search the ET_LANG entries
		EntrySet found;
		g_res.search(found, ET_LANG, entry->m_type, entry->m_name);

		if (entry->m_type == RT_GROUP_ICON)     // group icon
		{
			for (auto e : found)
			{
				g_res.copy_group_icon(e, dialog.m_name, e->m_lang);
			}
		}
		else if (entry->m_type == RT_GROUP_CURSOR)  // group cursor
		{
			for (auto e : found)
			{
				g_res.copy_group_cursor(e, dialog.m_name, e->m_lang);
			}
		}
		else    // otherwise
		{
			for (auto e : found)
			{
				g_res.add_lang_entry(e->m_type, dialog.m_name, e->m_lang, e->m_data);
			}
		}

		// select the entry
		SelectTV(ET_NAME, dialog.m_type, dialog.m_name, BAD_LANG, FALSE);
		DoSetFileModified(TRUE);
	}
}

// clone the resource item in new language
void MMainWnd::OnCopyAsNewLang(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	switch (entry->m_et)
	{
	case ET_LANG: case ET_STRING: case ET_MESSAGE:
		break;      // ok

	default:
		return;     // unable to copy the language
	}

	// show the dialog
	MCloneInNewLangDlg dialog(entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		if (entry->m_type == RT_GROUP_ICON)     // group icon
		{
			// search the group icons
			EntrySet found;
			g_res.search(found, ET_LANG, RT_GROUP_ICON, entry->m_name, entry->m_lang);

			// copy them
			for (auto e : found)
			{
				g_res.copy_group_icon(e, e->m_name, dialog.m_lang);
			}

			// select the entry
			SelectTV(ET_LANG, dialog.m_type, dialog.m_name, dialog.m_lang, FALSE);
		}
		else if (entry->m_type == RT_GROUP_CURSOR)
		{
			// search the group cursors
			EntrySet found;
			g_res.search(found, ET_LANG, RT_GROUP_CURSOR, entry->m_name, entry->m_lang);

			// copy them
			for (auto e : found)
			{
				g_res.copy_group_cursor(e, e->m_name, dialog.m_lang);
			}

			// select the entry
			SelectTV(ET_LANG, dialog.m_type, dialog.m_name, dialog.m_lang, FALSE);
		}
		else if (entry->m_et == ET_STRING)
		{
			// search the strings
			EntrySet found;
			g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, entry->m_lang);

			// copy them
			for (auto e : found)
			{
				g_res.add_lang_entry(e->m_type, e->m_name, dialog.m_lang, e->m_data);
			}

			// select the entry
			SelectTV(ET_STRING, dialog.m_type, BAD_NAME, dialog.m_lang, FALSE);
		}
		else if (entry->m_et == ET_MESSAGE)
		{
			// search the messagetables
			EntrySet found;
			g_res.search(found, ET_LANG, RT_MESSAGETABLE, BAD_NAME, entry->m_lang);

			// copy them
			for (auto e : found)
			{
				g_res.add_lang_entry(e->m_type, e->m_name, dialog.m_lang, e->m_data);
			}

			// select the entry
			SelectTV(ET_MESSAGE, dialog.m_type, BAD_NAME, dialog.m_lang, FALSE);
		}
		else
		{
			// search the entries
			EntrySet found;
			g_res.search(found, ET_LANG, entry->m_type, entry->m_name, entry->m_lang);

			// copy them
			for (auto e : found)
			{
				g_res.add_lang_entry(e->m_type, e->m_name, dialog.m_lang, e->m_data);
			}

			// select the entry
			SelectTV(ET_LANG, dialog.m_type, dialog.m_name, dialog.m_lang, FALSE);
		}
		DoSetFileModified(TRUE);
	}
}

void MMainWnd::OnCopyToMultiLang(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	switch (entry->m_et)
	{
	case ET_LANG: case ET_STRING: case ET_MESSAGE:
		break;      // ok

	default:
		return;     // unable to copy the language
	}

	// show the dialog
	MCopyToMultiLangDlg dialog(entry);
	WORD wLang = BAD_LANG;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		for (auto& lang : dialog.m_langs)
		{
			wLang = lang;
			if (entry->m_type == RT_GROUP_ICON)     // group icon
			{
				// search the group icons
				EntrySet found;
				g_res.search(found, ET_LANG, RT_GROUP_ICON, entry->m_name, entry->m_lang);

				// copy them
				for (auto e : found)
				{
					g_res.copy_group_icon(e, e->m_name, lang);
				}
			}
			else if (entry->m_type == RT_GROUP_CURSOR)
			{
				// search the group cursors
				EntrySet found;
				g_res.search(found, ET_LANG, RT_GROUP_CURSOR, entry->m_name, entry->m_lang);

				// copy them
				for (auto e : found)
				{
					g_res.copy_group_cursor(e, e->m_name, lang);
				}
			}
			else if (entry->m_et == ET_STRING)
			{
				// search the strings
				EntrySet found;
				g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, entry->m_lang);

				// copy them
				for (auto e : found)
				{
					g_res.add_lang_entry(e->m_type, e->m_name, lang, e->m_data);
				}
			}
			else if (entry->m_et == ET_MESSAGE)
			{
				// search the messagetables
				EntrySet found;
				g_res.search(found, ET_LANG, RT_MESSAGETABLE, BAD_NAME, entry->m_lang);

				// copy them
				for (auto e : found)
				{
					g_res.add_lang_entry(e->m_type, e->m_name, lang, e->m_data);
				}
			}
			else
			{
				// search the entries
				EntrySet found;
				g_res.search(found, ET_LANG, entry->m_type, entry->m_name, entry->m_lang);

				// copy them
				for (auto e : found)
				{
					g_res.add_lang_entry(e->m_type, e->m_name, lang, e->m_data);
				}
			}
		}

		DoSetFileModified(TRUE);

		// select the entry
		SelectTV(ET_LANG, entry->m_type, entry->m_name, wLang, FALSE);
	}
}

// show the item search dialog
void MMainWnd::OnItemSearch(HWND hwnd)
{
	// is there "item search" dialogs?
	if (MItemSearchDlg::Dialog())
	{
		HWND hDlg = *MItemSearchDlg::Dialog();
		if (::IsWindow(hDlg))
		{
			// bring it to the top
			SetForegroundWindow(hDlg);
			SetFocus(hDlg);
			return;
		}
		// erase hDlg from dialogs
		MItemSearchDlg::Dialog() = nullptr;
	}

	// create dialog
	auto pDialog = std::make_shared<MItemSearchDlg>(m_search);
	pDialog->CreateDialogDx(hwnd);
	MItemSearchDlg::Dialog() = pDialog;

	// set the window handles to m_search.res2text
	m_search.res2text.m_hwnd = hwnd;
	m_search.res2text.m_hwndDialog = *pDialog;

	// show it
	ShowWindow(*pDialog, SW_SHOWNORMAL);
	UpdateWindow(*pDialog);
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

	if (index >= (INT)g_langs.size())
		return FALSE; // reject

	WORD wNewLang = g_langs[index].LangID;

	auto entry = g_res.get_entry();
	if (!entry || entry->m_et == ET_TYPE || entry->m_et == ET_NAME)
		return FALSE;   // reject

	WORD wOldLang = entry->m_lang;
	if (wNewLang == BAD_LANG || wOldLang == wNewLang)
		return FALSE;   // reject

	// check if it already exists
	if (g_res.find(ET_LANG, entry->m_type, entry->m_name, wNewLang))
	{
		ErrorBoxDx(IDS_ALREADYEXISTS);
		return FALSE;   // reject
	}

	PostUpdateLangArrow(hwnd);

	WCHAR szText[MAX_PATH];
	MString strLang = TextFromLang(wNewLang);
	StringCbCopy(szText, sizeof(szText), strLang.c_str());
	DoRelangEntry(szText, entry, wOldLang, wNewLang);
	DoSetFileModified(TRUE);
	return TRUE; // accepted
}

BOOL MMainWnd::DoInnerSearch(HWND hwnd)
{
	DWORD ich, ichEnd;
	SendMessageW(m_hCodeEditor, EM_GETSEL, (WPARAM)&ich, (LPARAM)&ichEnd);

	MString strText = MWindowBase::GetWindowText(m_hCodeEditor);

	MString strTarget = m_search.strText;
	if (m_search.bIgnoreCases)
	{
		CharUpperW(&strText[0]);
		CharUpperW(&strTarget[0]);
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

void search_worker_thread(MMainWnd* pThis, HWND hwnd, MItemSearchDlg* pDialog)
{
	pThis->search_worker_thread_outer(hwnd, pDialog);
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
		CharUpperW(&m_search.strText[0]);

	// initialize
	m_search.bCancelled = FALSE;
	m_search.pFound = NULL;
	m_search.pCurrent = entry;

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

	std::thread t1(::search_worker_thread, this, hwnd, pDialog);
	t1.detach();

	return FALSE;
}

// delete a resource item
void MMainWnd::OnDeleteRes(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected entry
	if (auto entry = g_res.get_entry())
	{
		// delete it
		if (g_res.super()->find(entry) != g_res.end())
			TreeView_DeleteItem(m_hwndTV, entry->m_hItem);
	}
	DoSetFileModified(TRUE);
}

// play the sound
void MMainWnd::OnPlay(HWND hwnd)
{
	// is it a WAVE sound?
	auto entry = g_res.get_lang_entry();
	if (entry && entry->m_type == L"WAVE")
	{
		// play the sound
		PlaySound(reinterpret_cast<LPCTSTR>(&(*entry)[0]), NULL,
				  SND_ASYNC | SND_NODEFAULT | SND_MEMORY);
		return;
	}
	// Is it an MP3 sound?
	if (entry && entry->m_type == L"MP3")
	{
		PlayMP3(&(*entry)[0], entry->size());
		return;
	}
}

void MMainWnd::OnSelChange(HWND hwnd, INT iSelected)
{
	if (iSelected != m_tab.GetCurSel())
	{
		// update tab control selection
		m_tab.SetCurSel(iSelected);
	}

	// update g_settings.bShowBinEdit
	switch (iSelected)
	{
	case 0:
		g_settings.bShowBinEdit = FALSE;
		break;
	case 1:
		g_settings.bShowBinEdit = TRUE;
		break;
	}

	// update show
	SetShowMode(m_nShowMode);

	// relayout
	PostMessage(hwnd, WM_SIZE, 0, 0);
}

// cancel edit
void MMainWnd::OnCancelEdit(HWND hwnd)
{
	// clear modification flag
	Edit_SetModify(m_hCodeEditor, FALSE);
	Edit_SetReadOnly(m_hCodeEditor, FALSE);

	// reselect to update the m_hCodeEditor
	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE);
}

// set error message
void MMainWnd::SetErrorMessage(const MStringA& strOutput, BOOL bBox)
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

// compile the source text
void MMainWnd::OnCompile(HWND hwnd)
{
	// needs reopen?
	BOOL bReopen = IsWindowVisible(m_rad_window);

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	// is it not modified?
	if (!Edit_GetModify(m_hCodeEditor))
	{
		// select the entry
		SelectTV(entry, FALSE);
		return;
	}

	ChangeStatusText(IDS_COMPILING);

	// m_hCodeEditor --> strWide
	MStringW strWide = MWindowBase::GetWindowTextW(m_hCodeEditor);

	// compile the strWide text
	MStringA strOutput;
	if (CompileParts(strOutput, entry->m_type, entry->m_name, entry->m_lang, strWide, bReopen))
	{
		m_nStatusStringID = IDS_RECOMPILEOK;

		// clear the control selection
		MRadCtrl::GetTargets().clear();
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);

		// clear the modification flag
		Edit_SetModify(m_hCodeEditor, FALSE);

		// select the entry
		SelectTV(entry, FALSE);
	}
	else
	{
		// failed
		m_nStatusStringID = IDS_RECOMPILEFAILED;
		SetErrorMessage(strOutput);
	}
}

// do GUI edit
void MMainWnd::OnGuiEdit(HWND hwnd)
{
	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry->is_editable(m_szVCBat))
		return;     // not editable

	if (!entry->can_gui_edit())
		return;     // unable to edit by GUI?

	// compile if necessary
	if (!CompileIfNecessary(FALSE))
	{
		return;
	}

	if (entry->m_type == RT_ACCELERATOR)
	{
		// entry->m_data --> accel_res
		AccelRes accel_res;
		MByteStreamEx stream(entry->m_data);
		if (accel_res.LoadFromStream(stream))
		{
			// editing...
			ChangeStatusText(IDS_EDITINGBYGUI);

			// show the dialog
			MEditAccelDlg dialog(accel_res);
			INT nID = (INT)dialog.DialogBoxDx(hwnd);
			if (nID == IDOK && entry == g_res.get_entry())
			{
				DoSetFileModified(TRUE);

				// update accel_res
				accel_res.Update();

				// accel_res --> entry->m_data
				entry->m_data = accel_res.data();
			}
		}

		// make it non-read-only
		Edit_SetReadOnly(m_hCodeEditor, FALSE);

		// select the entry
		if (entry == g_res.get_entry())
			SelectTV(entry, FALSE);

		// ready
		ChangeStatusText(IDS_READY);
	}
	else if (entry->m_type == RT_MENU)
	{
		// entry->m_data --> menu_res
		MenuRes menu_res;
		MByteStreamEx stream(entry->m_data);
		if (menu_res.LoadFromStream(stream))
		{
			// editing...
			ChangeStatusText(IDS_EDITINGBYGUI);

			// show the dialog
			MEditMenuDlg dialog(menu_res);
			INT nID = (INT)dialog.DialogBoxDx(hwnd);
			if (nID == IDOK && entry == g_res.get_entry())
			{
				DoSetFileModified(TRUE);

				// update menu_res
				menu_res.Update();

				// menu_res --> entry->m_data
				entry->m_data = menu_res.data();
			}
		}

		// make it non-read-only
		Edit_SetReadOnly(m_hCodeEditor, FALSE);

		// select the entry
		if (entry == g_res.get_entry())
			SelectTV(entry, FALSE);

		// ready
		ChangeStatusText(IDS_READY);
	}
	else if (entry->m_type == RT_TOOLBAR)
	{
		// entry->m_data --> toolbar_res
		ToolbarRes toolbar_res;
		MByteStreamEx stream(entry->m_data);
		if (toolbar_res.LoadFromStream(stream))
		{
			// editing...
			ChangeStatusText(IDS_EDITINGBYGUI);

			// show the dialog
			MEditToolbarDlg dialog(toolbar_res);
			INT nID = (INT)dialog.DialogBoxDx(hwnd);
			if (nID == IDOK && entry == g_res.get_entry())
			{
				DoSetFileModified(TRUE);

				// toolbar_res --> entry->m_data
				entry->m_data = toolbar_res.data();
			}
		}

		// make it non-read-only
		Edit_SetReadOnly(m_hCodeEditor, FALSE);

		// select the entry
		if (entry == g_res.get_entry())
			SelectTV(entry, FALSE);

		// ready
		ChangeStatusText(IDS_READY);
	}
	else if (entry->m_type == RT_DIALOG)
	{
		// editing...
		ChangeStatusText(IDS_EDITINGBYGUI);

		// entry->m_data --> m_rad_window.m_dialog_res
		MByteStreamEx stream(entry->m_data);
		m_rad_window.m_dialog_res.LoadFromStream(stream);
		m_rad_window.m_dialog_res.m_lang = entry->m_lang;
		m_rad_window.m_dialog_res.m_name = entry->m_name;
		m_rad_window.clear_maps();
		m_rad_window.create_maps(entry->m_lang);

		// load RT_DLGINIT
		if (auto e = g_res.find(ET_LANG, RT_DLGINIT, entry->m_name, entry->m_lang))
		{
			m_rad_window.m_dialog_res.LoadDlgInitData(e->m_data);
		}
		else if (auto e = g_res.find(ET_LANG, RT_DLGINIT, entry->m_name, BAD_LANG))
		{
			m_rad_window.m_dialog_res.LoadDlgInitData(e->m_data);
		}

		// recreate the RADical dialog
		if (::IsWindow(m_rad_window))
		{
			m_rad_window.ReCreateRadDialog(m_rad_window);
		}
		else
		{
			if (!m_rad_window.CreateDx(m_hwnd))
			{
				ErrorBoxDx(IDS_DLGFAIL);
			}
		}

		// make it non-read-only
		Edit_SetReadOnly(m_hCodeEditor, FALSE);
	}
	else if (entry->m_type == RT_DLGINIT)
	{
		// entry->m_data --> dlginit_res
		DlgInitRes dlginit_res;
		MByteStreamEx stream(entry->m_data);
		if (dlginit_res.LoadFromStream(stream))
		{
			// editing...
			ChangeStatusText(IDS_EDITINGBYGUI);

			// show the dialog
			MDlgInitDlg dialog(dlginit_res);
			INT nID = (INT)dialog.DialogBoxDx(hwnd);
			if (nID == IDOK && entry == g_res.get_entry())
			{
				DoSetFileModified(TRUE);

				// dlginit_res --> entry->m_data
				entry->m_data = dlginit_res.data();
			}
		}

		// make it non-read-only
		Edit_SetReadOnly(m_hCodeEditor, FALSE);

		// select the entry
		if (entry == g_res.get_entry())
			SelectTV(entry, FALSE);

		// ready
		ChangeStatusText(IDS_READY);
	}
	else if (entry->m_type == RT_STRING && entry->m_et == ET_STRING)
	{
		// g_res --> found
		WORD lang = entry->m_lang;
		EntrySet found;
		g_res.search(found, ET_LANG, RT_STRING, BAD_NAME, lang);

		// found --> str_res
		StringRes str_res;
		for (auto e : found)
		{
			MByteStreamEx stream(e->m_data);
			if (!str_res.LoadFromStream(stream, e->m_name.m_id))
			{
				ErrorBoxDx(IDS_CANNOTLOAD);
				return;
			}
		}

		// editing...
		ChangeStatusText(IDS_EDITINGBYGUI);

		// show the dialog
		MStringsDlg dialog(str_res);
		INT nID = (INT)dialog.DialogBoxDx(hwnd);
		if (nID == IDOK)
		{
			DoSetFileModified(TRUE);

			// dialog --> str_res
			str_res = dialog.m_str_res;

			// dump (with disabling macro IDs)
			bool shown = !g_settings.bHideID;
			g_settings.bHideID = false;
			MStringW strWide = str_res.Dump();
			g_settings.bHideID = !shown;

			// compile the dumped result
			MStringA strOutput;
			if (CompileParts(strOutput, RT_STRING, BAD_NAME, lang, strWide))
			{
				m_nStatusStringID = IDS_RECOMPILEOK;

				// select the entry to update the source
				SelectTV(ET_STRING, RT_STRING, BAD_NAME, lang, FALSE);
			}
			else
			{
				m_nStatusStringID = IDS_RECOMPILEFAILED;
				SetErrorMessage(strOutput);
			}
		}

		// make it non-read-only
		Edit_SetReadOnly(m_hCodeEditor, FALSE);

		// ready
		ChangeStatusText(IDS_READY);
	}
	else if (entry->m_type == RT_MESSAGETABLE && entry->m_et == ET_MESSAGE)
	{
		if (g_settings.bUseMSMSGTABLE)
			return;

		// g_res --> found
		WORD lang = entry->m_lang;
		EntrySet found;
		g_res.search(found, ET_LANG, RT_MESSAGETABLE, BAD_NAME, lang);

		// found --> msg_res
		MessageRes msg_res;
		for (auto e : found)
		{
			MByteStreamEx stream(e->m_data);
			if (!msg_res.LoadFromStream(stream, e->m_name.m_id))
			{
				ErrorBoxDx(IDS_CANNOTLOAD);
				return;
			}
		}

		// editing...
		ChangeStatusText(IDS_EDITINGBYGUI);

		// show the dialog
		MMessagesDlg dialog(msg_res);
		INT nID = (INT)dialog.DialogBoxDx(hwnd);
		if (nID == IDOK)
		{
			DoSetFileModified(TRUE);

			// dialog --> msg_res
			msg_res = dialog.m_msg_res;

			// dump (with disabling macro IDs)
			bool shown = !g_settings.bHideID;
			g_settings.bHideID = false;
			MStringW strWide = msg_res.Dump();
			g_settings.bHideID = !shown;

			// compile the dumped result
			MStringA strOutput;
			if (CompileParts(strOutput, RT_MESSAGETABLE, WORD(1), lang, strWide))
			{
				m_nStatusStringID = IDS_RECOMPILEOK;

				// select the entry
				SelectTV(ET_MESSAGE, RT_MESSAGETABLE, BAD_NAME, lang, FALSE);
			}
			else
			{
				m_nStatusStringID = IDS_RECOMPILEFAILED;
				SetErrorMessage(strOutput);
			}
		}

		Edit_SetReadOnly(m_hCodeEditor, g_settings.bUseMSMSGTABLE);

		// ready
		ChangeStatusText(IDS_READY);
	}
}

MStringW
GetResTypeEncoding(const MIdOrString& type)
{
	MStringW name;

	if (type.m_id)
	{
		name = g_db.GetName(L"RESOURCE", type.m_id);
		if (name.empty())
			name = mstr_dec_word(type.m_id);
	}
	else
	{
		name = type.str();
	}

	auto it = g_settings.encoding_map.find(name);
	if (it != g_settings.encoding_map.end())
		return it->second;

	return L"";
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

// do text edit
void MMainWnd::OnEdit(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(::IsWindowVisible(m_rad_window)))
		return;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!IsEntryTextEditable(entry))
		return;

	// make it non-read-only
	Edit_SetReadOnly(m_hCodeEditor, FALSE);

	// select the entry
	SelectTV(entry, TRUE);
}

void MMainWnd::OnOpenLocalFile(HWND hwnd, LPCWSTR filename)
{
	// compile if necessary
	if (!CompileIfNecessary(::IsWindowVisible(m_rad_window)))
		return;

	// get the module path filename of this application module
	WCHAR szPath[MAX_PATH];
	GetModuleFileNameW(NULL, szPath, _countof(szPath));

	LPWSTR pch = PathFindFileNameW(szPath);
	*pch = 0;

	for (INT m = 0; m <= 3; ++m)
	{
		MString strPath = szPath;
		for (INT n = 0; n < m; ++n)
			strPath += L"..\\";

		strPath += filename;

		if (PathFileExistsW(strPath.c_str()))
		{
			// open it
			ShellExecuteW(hwnd, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
			return;
		}
	}
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
	pmaker.SetShowWindow(SW_HIDE);
	pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

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
	pmaker.SetShowWindow(SW_HIDE);
	pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

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

// for debugging purpose
void MMainWnd::OnDebugTreeNode(HWND hwnd)
{
	WCHAR sz[MAX_PATH * 2 + 32];

	// show the file paths
	StringCchPrintfW(sz, _countof(sz), L"%s\n\n%s", m_szFile, m_szResourceH);
	MsgBoxDx(sz, MB_ICONINFORMATION);

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	static const LPCWSTR apszI_[] =
	{
		L"ET_ANY",
		L"ET_TYPE",
		L"ET_STRING",
		L"ET_MESSAGE",
		L"ET_NAME",
		L"ET_LANG"
	};

	MStringW type = entry->m_type.str();
	MStringW name = entry->m_name.str();
	StringCchPrintfW(sz, _countof(sz),
		L"%s: type:%s, name:%s, lang:0x%04X, entry:%p, hItem:%p, strLabel:%s", apszI_[entry->m_et],
		type.c_str(), name.c_str(), entry->m_lang, entry, entry->m_hItem, entry->m_strLabel.c_str());
	MsgBoxDx(sz, MB_ICONINFORMATION);
}

void MMainWnd::SetShowMode(SHOW_MODE mode)
{
	m_nShowMode = mode;
	if (g_settings.bShowBinEdit)
	{
		ShowWindow(m_hCodeEditor, SW_HIDE);
		ShowWindow(m_hBmpView, SW_HIDE);
		ShowWindow(m_hHexViewer, SW_SHOWNOACTIVATE);
		m_splitter2.SetPaneCount(1);
		m_splitter2.SetPane(0, m_hHexViewer);
	}
	else
	{
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

// WM_INITMENU: update the menus
void MMainWnd::OnInitMenu(HWND hwnd, HMENU hMenu)
{
	if (g_settings.bWordWrap)
		CheckMenuItem(hMenu, ID_WORD_WRAP, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_WORD_WRAP, MF_BYCOMMAND | MF_UNCHECKED);

	if (m_szResourceH[0])
		EnableMenuItem(hMenu, ID_UNLOADRESH, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(hMenu, ID_UNLOADRESH, MF_BYCOMMAND | MF_GRAYED);

	// search the language entries
	EntrySet found;
	g_res.search(found, ET_LANG);

	if (found.empty())
	{
		EnableMenuItem(hMenu, ID_ITEMSEARCH, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXPAND_ALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COLLAPSE_ALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FIND, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FINDUPWARD, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACE, MF_GRAYED);
	}
	else
	{
		EnableMenuItem(hMenu, ID_ITEMSEARCH, MF_ENABLED);
		EnableMenuItem(hMenu, ID_EXPAND_ALL, MF_ENABLED);
		EnableMenuItem(hMenu, ID_COLLAPSE_ALL, MF_ENABLED);
		EnableMenuItem(hMenu, ID_FIND, MF_ENABLED);
		EnableMenuItem(hMenu, ID_FINDUPWARD, MF_ENABLED);
		EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_ENABLED);
		EnableMenuItem(hMenu, ID_REPLACE, MF_ENABLED);
	}

	if (g_settings.bShowToolBar)
		CheckMenuItem(hMenu, ID_SHOWHIDETOOLBAR, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_SHOWHIDETOOLBAR, MF_BYCOMMAND | MF_UNCHECKED);

	if (g_settings.bUseBeginEnd)
		CheckMenuItem(hMenu, ID_USEBEGINEND, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_USEBEGINEND, MF_BYCOMMAND | MF_UNCHECKED);

	if (g_settings.bUseMSMSGTABLE)
		CheckMenuItem(hMenu, ID_USEMSMSGTBL, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_USEMSMSGTBL, MF_BYCOMMAND | MF_UNCHECKED);

	BOOL bCanEditLabel = TRUE;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry || entry->m_et == ET_TYPE)
	{
		bCanEditLabel = FALSE;
	}

	if (bCanEditLabel)
	{
		if (entry)
		{
			if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
			{
				if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
				{
					bCanEditLabel = FALSE;
				}
			}
		}
		else
		{
			bCanEditLabel = FALSE;
		}
	}

	if (bCanEditLabel)
		EnableMenuItem(hMenu, ID_EDITLABEL, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(hMenu, ID_EDITLABEL, MF_BYCOMMAND | MF_GRAYED);

	if (IsWindowVisible(m_hStatusBar))
		CheckMenuItem(hMenu, ID_STATUSBAR, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_STATUSBAR, MF_UNCHECKED);

	if (g_settings.bShowBinEdit)
		CheckMenuItem(hMenu, ID_BINARYPANE, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_BINARYPANE, MF_UNCHECKED);

	if (g_settings.bAlwaysControl)
		CheckMenuItem(hMenu, ID_ALWAYSCONTROL, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_ALWAYSCONTROL, MF_UNCHECKED);

	if (g_settings.bHideID)
		CheckMenuItem(hMenu, ID_HIDEIDMACROS, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_HIDEIDMACROS, MF_UNCHECKED);

	if (g_settings.bUseIDC_STATIC)
		CheckMenuItem(hMenu, ID_USEIDC_STATIC, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_USEIDC_STATIC, MF_UNCHECKED);

	if (!entry || !entry->m_hItem)
	{
		EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_DELETERES, MF_GRAYED);
		EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EDIT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GUIEDIT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTRC, MF_GRAYED);
		return;
	}

	EnableMenuItem(hMenu, ID_EXTRACTRC, MF_ENABLED);

	BOOL bEditable = entry && entry->is_editable(m_szVCBat);
	if (bEditable)
	{
		EnableMenuItem(hMenu, ID_EDIT, MF_ENABLED);
		if (entry->can_gui_edit())
		{
			EnableMenuItem(hMenu, ID_GUIEDIT, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_GUIEDIT, MF_GRAYED);
		}
	}
	else
	{
		EnableMenuItem(hMenu, ID_EDIT, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GUIEDIT, MF_GRAYED);
	}

	switch (entry ? entry->m_et : ET_ANY)
	{
	case ET_TYPE:
		EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
		EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_GRAYED);
		break;
	case ET_NAME:
		EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
		EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_ENABLED);
		EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_GRAYED);
		break;

	case ET_LANG:
		if (entry->m_type == RT_GROUP_ICON || entry->m_type == RT_ICON ||
			entry->m_type == RT_ANIICON)
		{
			EnableMenuItem(hMenu, ID_EXTRACTICON, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
		}
		if (entry->m_type == RT_GROUP_ICON || entry->m_type == RT_ANIICON)
		{
			EnableMenuItem(hMenu, ID_REPLACEICON, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
		}

		if (entry->m_type == RT_BITMAP)
		{
			EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_ENABLED);
			EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
			EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
		}

		if (entry->m_type == RT_GROUP_CURSOR || entry->m_type == RT_CURSOR ||
			entry->m_type == RT_ANICURSOR)
		{
			EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
		}
		if (entry->m_type == RT_GROUP_CURSOR || entry->m_type == RT_ANICURSOR)
		{
			EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
		}

		if (entry->is_testable())
		{
			EnableMenuItem(hMenu, ID_TEST, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
		}

		EnableMenuItem(hMenu, ID_REPLACEBIN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
		EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
		if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
		{
			EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
			EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_GRAYED);
		}
		else
		{
			EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_ENABLED);
			EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_ENABLED);
		}
		break;

	case ET_STRING: case ET_MESSAGE:
		EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
		EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
		EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_ENABLED);
		EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_GRAYED);
		break;

	default:
		EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
		EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_GRAYED);
		EnableMenuItem(hMenu, ID_DELETERES, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
		EnableMenuItem(hMenu, ID_COPYTOMULTILANG, MF_GRAYED);
		break;
	}
}

// WM_CONTEXTMENU: the context menu is shown
void MMainWnd::OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
	if (hwndContext != m_hwndTV)
		return;     // ignore

	// if RADical window is displayed
	if (IsWindowVisible(m_rad_window))
	{
		// destroy it
		m_rad_window.DestroyWindow();
	}

	// get screen coordinates from xPos and yPos
	POINT pt = {(SHORT)xPos, (SHORT)yPos};
	HTREEITEM hItem;
	if (pt.x == -1 && pt.y == -1)
	{
		// context menu from keyboard
		hItem = TreeView_GetSelection(hwndContext);

		RECT rc;
		TreeView_GetItemRect(hwndContext, hItem, &rc, FALSE);
		pt.x = (rc.left + rc.right) / 2;
		pt.y = (rc.top + rc.bottom) / 2;
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
	HMENU hMenu = LoadMenuW(m_hInst, MAKEINTRESOURCEW(IDR_POPUPMENUS));
	OnInitMenu(hwnd, hMenu);
	HMENU hSubMenu = ::GetSubMenu(hMenu, 0);
	if (hMenu == NULL || hSubMenu == NULL)
		return;

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

// create the toolbar
BOOL MMainWnd::CreateOurToolBar(HWND hwndParent, HIMAGELIST himlTools)
{
	// create a toolbar
	HWND hwndTB = CreateWindowW(TOOLBARCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | CCS_TOP | TBSTYLE_LIST | TBSTYLE_TOOLTIPS,
		0, 0, 0, 0, hwndParent, (HMENU)1, GetWindowInstance(hwndParent), NULL);
	if (hwndTB == NULL)
		return FALSE;

	// initialize
	SendMessageW(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessageW(hwndTB, TB_SETBITMAPSIZE, 0, MAKELPARAM(0, 0));
	SendMessageW(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)himlTools);
	SendMessageW(hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

	ToolBar_StoreStrings(hwndTB, _countof(g_buttons0), g_buttons0);
	ToolBar_StoreStrings(hwndTB, _countof(g_buttons1), g_buttons1);
	ToolBar_StoreStrings(hwndTB, _countof(g_buttons2), g_buttons2);
	ToolBar_StoreStrings(hwndTB, _countof(g_buttons3), g_buttons3);
	ToolBar_StoreStrings(hwndTB, _countof(g_buttons4), g_buttons4);

	m_hToolBar = hwndTB;
	UpdateOurToolBarButtons(3);

	return TRUE;
}

void MMainWnd::UpdateToolBarStatus()
{
	if (TreeView_GetCount(m_hwndTV) == 0)
	{
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_EXPAND_ALL, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_COLLAPSE_ALL, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_EXPORTRES, 0);
	}
	else
	{
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_EXPAND_ALL, TBSTATE_ENABLED);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_COLLAPSE_ALL, TBSTATE_ENABLED);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_EXPORTRES, TBSTATE_ENABLED);
	}

	BOOL bCanEditLabel = TRUE;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (entry && !entry->valid())
		entry = NULL;
	if (!entry || entry->m_et == ET_TYPE)
	{
		bCanEditLabel = FALSE;
	}

	if (bCanEditLabel)
	{
		if (entry)
		{
			if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
			{
				if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
				{
					bCanEditLabel = FALSE;
				}
			}
		}
		else
		{
			bCanEditLabel = FALSE;
		}
	}

	if (bCanEditLabel)
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_EDITLABEL, TBSTATE_ENABLED);
	else
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_EDITLABEL, 0);

	if (!entry || !entry->m_hItem)
	{
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_GUIEDIT, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, 0);
		return;
	}

	BOOL bEditable = entry && entry->is_editable(m_szVCBat);
	if (bEditable)
	{
		if (entry->can_gui_edit())
		{
			SendMessageW(m_hToolBar, TB_SETSTATE, ID_GUIEDIT, TBSTATE_ENABLED);
		}
		else
		{
			SendMessageW(m_hToolBar, TB_SETSTATE, ID_GUIEDIT, 0);
		}
	}
	else
	{
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_GUIEDIT, 0);
	}

	switch (entry ? entry->m_et : ET_ANY)
	{
	case ET_TYPE:
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, TBSTATE_ENABLED);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, 0);
		break;

	case ET_NAME:
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, TBSTATE_ENABLED);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, TBSTATE_ENABLED);
		break;

	case ET_LANG:
		if (entry->is_testable())
		{
			SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, TBSTATE_ENABLED);
		}
		else
		{
			SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, 0);
		}

		SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, TBSTATE_ENABLED);
		if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
			SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, 0);
		else
			SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, TBSTATE_ENABLED);
		break;

	case ET_STRING: case ET_MESSAGE:
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, TBSTATE_ENABLED);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, TBSTATE_ENABLED);
		break;

	default:
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, 0);
		break;
	}
}

// update the toolbar buttons
void MMainWnd::UpdateOurToolBarButtons(INT iType)
{
	// delete all the buttons of toolbar
	while (SendMessageW(m_hToolBar, TB_DELETEBUTTON, 0, 0))
		;

	// add the buttons
	switch (iType)
	{
	case 0:
		SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons0), (LPARAM)g_buttons0);
		break;
	case 1:
		SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons1), (LPARAM)g_buttons1);
		break;
	case 2:
		SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons2), (LPARAM)g_buttons2);
		break;
	case 3:
		SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons3), (LPARAM)g_buttons3);
		break;
	case 4:
		SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons4), (LPARAM)g_buttons4);
		break;
	}

	UpdateToolBarStatus();

	// show/hide the toolbar by settings
	if (g_settings.bShowToolBar)
		ShowWindow(m_hToolBar, SW_SHOWNOACTIVATE);
	else
		ShowWindow(m_hToolBar, SW_HIDE);
}

// select an item in the tree control
void
MMainWnd::SelectTV(EntryType et, const MIdOrString& type,
				   const MIdOrString& name, WORD lang,
				   BOOL bDoubleClick, STV stv)
{
	// close the preview
	HidePreview(stv);

	// find the entry
	if (auto entry = g_res.find(et, type, name, lang))
	{
		// select it
		SelectTV(entry, bDoubleClick, stv);
	}
}

// select an item in the tree control
void MMainWnd::SelectTV(EntryBase *entry, BOOL bDoubleClick, STV stv)
{
	BOOL bModified = Edit_GetModify(m_hCodeEditor);

	// close the preview
	HidePreview(stv);

	if (!entry)     // not selected
	{
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

		if (stv != STV_DONTRESET)
		{
			// show the string table
			PreviewStringTable(m_hwnd, *entry);
		}

		// hide the binary EDIT control
		SetWindowTextW(m_hHexViewer, NULL);

		// it's editable
		bEditable = TRUE;
		break;

	case ET_MESSAGE:
		// clean up m_hBmpView
		m_hBmpView.DestroyView();

		if (stv != STV_DONTRESET)
		{
			// show the message table
			PreviewMessageTable(m_hwnd, *entry);
		}

		// hide the binary EDIT control
		SetWindowTextW(m_hHexViewer, NULL);

		// editable?
		bEditable = !g_settings.bUseMSMSGTABLE;
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
		SetShowMode(SHOW_CODEONLY);
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
	SetShowMode(m_nShowMode);

	// recalculate the splitter
	PostMessageDx(WM_SIZE);
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

// compile the string table
BOOL MMainWnd::CompileStringTable(MStringA& strOutput, WORD lang, const MStringW& strWide)
{
	// convert strWide to UTF-8
	MStringA strUtf8 = MWideToAnsi(CP_UTF8, strWide).c_str();

	WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH], szPath3[MAX_PATH];

	// Source file #1
	StringCchCopyW(szPath1, MAX_PATH, GetTempFileNameDx(L"R1"));
	MFile r1(szPath1, TRUE);

	// Header file
	StringCchCopyW(szPath2, MAX_PATH, GetTempFileNameDx(L"R2"));

	// Output resource object file (imported)
	StringCchCopyW(szPath3, MAX_PATH, GetTempFileNameDx(L"R3"));
	MFile r3(szPath3, TRUE);    // create
	r3.CloseHandle();   // close the handle

	AutoDeleteFileW adf1(szPath1);
	AutoDeleteFileW adf3(szPath3);

	if (!g_settings.IsIDMapEmpty() && DoWriteResH(szPath2))
	{
		r1.WriteFormatA("#include \"%s\"\r\n", MWideToAnsi(CP_ACP, szPath2).c_str());
	}
	else if (m_szResourceH[0])
	{
		r1.WriteFormatA("#include \"%s\"\r\n", MWideToAnsi(CP_ACP, m_szResourceH).c_str());
	}
	r1.WriteFormatA("#include <windows.h>\r\n");
	r1.WriteFormatA("#include <commctrl.h>\r\n");
	r1.WriteFormatA("#pragma code_page(65001) // UTF-8\r\n");
	r1.WriteFormatA("LANGUAGE 0x%04X, 0x%04X\r\n", PRIMARYLANGID(lang), SUBLANGID(lang));

	// dump the macros
	for (auto& pair : g_settings.id_map)
	{
		if (pair.first == "IDC_STATIC")
		{
			r1.WriteFormatA("#undef IDC_STATIC\r\n");
			r1.WriteFormatA("#define IDC_STATIC -1\r\n");
		}
		else
		{
			r1.WriteFormatA("#undef %s\r\n", pair.first.c_str());
			r1.WriteFormatA("#define %s %s\r\n", pair.first.c_str(), pair.second.c_str());
		}
	}

	// write the UTF-8 file to Source file #2
	DWORD cbWritten, cbWrite = DWORD(strUtf8.size() * sizeof(char));
	r1.WriteFormatA("#pragma RisohEditor\r\n");
	r1.WriteFile(strUtf8.c_str(), cbWrite, &cbWritten);
	r1.CloseHandle();   // close the handle

	// build the command line text
	MStringW strCmdLine;
	strCmdLine += L'\"';
	strCmdLine += m_szWindresExe;
	strCmdLine += L"\" -DRC_INVOKED ";
	strCmdLine += GetMacroDump();
	strCmdLine += GetIncludesDumpForWindres();
	strCmdLine += L" -o \"";
	strCmdLine += szPath3;
	strCmdLine += L"\" -J rc -O res -F pe-i386 \"--preprocessor=";
	strCmdLine += m_szMCppExe;
	strCmdLine += L"\" \"";
	strCmdLine += szPath1;
	strCmdLine += '\"';
	//MessageBoxW(m_hwnd, strCmdLine.c_str(), NULL, 0);

	BOOL bOK = FALSE;

	// create a windres.exe process
	MProcessMaker pmaker;
	pmaker.SetShowWindow(SW_HIDE);
	pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

	MFile hInputWrite, hOutputRead;
	if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
		pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
	{
		// read all with timeout
		bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
		pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

		if (pmaker.GetExitCode() == 0 && bOK)
		{
			bOK = FALSE;

			// import res
			EntrySet res;
			if (res.import_res(szPath3))
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
			bOK = FALSE;
			// error message
			size_t ich = strOutput.find("RisohEditor.rc:");
			if (ich != strOutput.npos)
			{
				ich += 15; // "RisohEditor.rc:"
				INT iLine = INT(strtoul(&strOutput[ich], NULL, 10));
				::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
				::SendMessageW(m_hCodeEditor, LNEM_SETLINEMARK, iLine, ERROR_LINE_COLOR);
			}
			strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_COMPILEERROR));
		}
	}
	else
	{
		// error message
		strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTSTARTUP));
	}

	// recalculate the splitter
	PostMessageDx(WM_SIZE);

	AutoDeleteFileW adf2(szPath2);

	if (bOK)
		DoSetFileModified(TRUE);
	return bOK;
}

BOOL MMainWnd::CompileTYPELIB(MStringA& strOutput, const MIdOrString& name, WORD lang, const MStringW& strWide)
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

BOOL MMainWnd::CompileRCData(MStringA& strOutput, const MIdOrString& name, WORD lang, const MStringW& strWide)
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
		MWideToAnsi w2a(CP_ACP, LoadStringDx(IDS_COMPILEERROR));
		strOutput = w2a.c_str();
		if (iLine != 0)
		{
			::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
			::SendMessageW(m_hCodeEditor, LNEM_SETLINEMARK, iLine, ERROR_LINE_COLOR);
		}
		return FALSE;
	}

	entry->m_data = std::move(data);
	DoSetFileModified(TRUE);
	return TRUE;
}

// compile the message table
BOOL MMainWnd::CompileMessageTable(MStringA& strOutput, WORD lang, const MStringW& strWide)
{
	// convert strWide to UTF-8
	MStringA strUtf8 = MWideToAnsi(CP_UTF8, strWide).c_str();

	WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH], szPath3[MAX_PATH];

	// Source file #1
	StringCchCopyW(szPath1, MAX_PATH, GetTempFileNameDx(L"R1"));
	MFile r1(szPath1, TRUE);

	// Header file
	StringCchCopyW(szPath2, MAX_PATH, GetTempFileNameDx(L"R2"));

	// Output resource object file (imported)
	StringCchCopyW(szPath3, MAX_PATH, GetTempFileNameDx(L"R3"));

	MFile r3(szPath3, TRUE);    // create the file
	r3.CloseHandle();   // close the handle

	AutoDeleteFileW adf1(szPath1);
	AutoDeleteFileW adf3(szPath3);

	if (!g_settings.IsIDMapEmpty() && DoWriteResH(szPath2))
	{
		r1.WriteFormatA("#include \"%s\"\r\n", MWideToAnsi(CP_ACP, szPath2).c_str());
	}
	else if (m_szResourceH[0])
	{
		r1.WriteFormatA("#include \"%s\"\r\n", MWideToAnsi(CP_ACP, m_szResourceH).c_str());
	}
	r1.WriteFormatA("#include <windows.h>\r\n");
	r1.WriteFormatA("#include <commctrl.h>\r\n");
	r1.WriteFormatA("#pragma code_page(65001) // UTF-8\r\n");
	r1.WriteFormatA("LANGUAGE 0x%04X, 0x%04X\r\n", PRIMARYLANGID(lang), SUBLANGID(lang));

	DWORD cbWritten, cbWrite = DWORD(strUtf8.size() * sizeof(char));
	r1.WriteFormatA("#pragma RisohEditor\r\n");
	r1.WriteFile(strUtf8.c_str(), cbWrite, &cbWritten);
	r1.CloseHandle();       // close the handle

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
	strCmdLine += szPath3;
	strCmdLine += L"\" -J rc -O res \"";
	strCmdLine += szPath1;
	strCmdLine += L'\"';
	//MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

	BOOL bOK = FALSE;

	// create the mcdx.exe process
	MProcessMaker pmaker;
	pmaker.SetShowWindow(SW_HIDE);
	pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

	MFile hInputWrite, hOutputRead;
	if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
		pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
	{
		// read all with timeout
		bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
		pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

		if (pmaker.GetExitCode() == 0 && bOK)
		{
			bOK = FALSE;

			// import res
			EntrySet res;
			if (res.import_res(szPath3))
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
				}

				if (bOK)
				{
					// merge
					g_res.search_and_delete(ET_LANG, RT_MESSAGETABLE, BAD_NAME, lang);
					g_res.merge(res);
				}
			}

			// clean res up
			res.delete_all();
		}
		else
		{
			bOK = FALSE;
			size_t ich = strOutput.find("RisohEditor.rc (");
			if (ich != strOutput.npos)
			{
				ich += 16; // "RisohEditor.rc ("
				INT iLine = INT(strtoul(&strOutput[ich], NULL, 10));
				iLine += 4; // FIXME: workaround
				::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
				::SendMessageW(m_hCodeEditor, LNEM_SETLINEMARK, iLine, ERROR_LINE_COLOR);
			}
			// error message
			strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_COMPILEERROR));
		}
	}
	else
	{
		// error message
		strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTSTARTUP));
	}

	// recalculate the splitter
	PostMessageDx(WM_SIZE);

	AutoDeleteFileW adf2(szPath2);

	if (bOK)
		DoSetFileModified(TRUE);

	return bOK;
}

// compile a resource item source
BOOL MMainWnd::CompileParts(MStringA& strOutput, const MIdOrString& type, const MIdOrString& name,
							WORD lang, const MStringW& strWide, BOOL bReopen)
{
	if (type == RT_STRING)
	{
		return CompileStringTable(strOutput, lang, strWide);
	}
	if (type == RT_MESSAGETABLE && !g_settings.bUseMSMSGTABLE)
	{
		return CompileMessageTable(strOutput, lang, strWide);
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

	WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH], szPath3[MAX_PATH];

	// Source file #1
	StringCchCopyW(szPath1, MAX_PATH, GetTempFileNameDx(L"R1"));
	MFile r1(szPath1, TRUE);

	// Header file
	StringCchCopyW(szPath2, MAX_PATH, GetTempFileNameDx(L"R2"));

	// Output resource object file (imported)
	StringCchCopyW(szPath3, MAX_PATH, GetTempFileNameDx(L"R3"));
	MFile r3(szPath3, TRUE);
	r3.CloseHandle();   // close the handle

	AutoDeleteFileW adf1(szPath1);
	AutoDeleteFileW adf3(szPath3);

	// dump the head to Source file #1
	if (!g_settings.IsIDMapEmpty() && DoWriteResH(szPath2))
	{
		r1.WriteFormatA("#include \"%s\"\r\n", MWideToAnsi(CP_ACP, szPath2).c_str());
	}
	else if (m_szResourceH[0])
	{
		r1.WriteFormatA("#include \"%s\"\r\n", MWideToAnsi(CP_ACP, m_szResourceH).c_str());
	}
	r1.WriteSzA("#include <windows.h>\r\n");
	r1.WriteSzA("#include <commctrl.h>\r\n");
	r1.WriteFormatA("LANGUAGE 0x%04X, 0x%04X\r\n", PRIMARYLANGID(lang), SUBLANGID(lang));
	r1.WriteSzA("#pragma code_page(65001) // UTF-8\r\n\r\n");
	r1.WriteSzA("#ifndef IDC_STATIC\r\n");
	r1.WriteSzA("    #define IDC_STATIC (-1)\r\n");
	r1.WriteSzA("#endif\r\n\r\n");

	DWORD cbWritten, cbWrite = DWORD(strUtf8.size() * sizeof(char));
	r1.WriteSzA("#pragma RisohEditor\r\n");
	r1.WriteFile(strUtf8.c_str(), cbWrite, &cbWritten);
	r1.FlushFileBuffers();
	r1.CloseHandle();   // close the handle

	// build the command line text
	MStringW strCmdLine;
	strCmdLine += L'\"';
	strCmdLine += m_szWindresExe;
	strCmdLine += L"\" -DRC_INVOKED ";
	strCmdLine += GetMacroDump();
	strCmdLine += GetIncludesDumpForWindres();
	strCmdLine += L" -o \"";
	strCmdLine += szPath3;
	strCmdLine += L"\" -J rc -O res -F pe-i386 --preprocessor=\"";
	strCmdLine += m_szMCppExe;
	strCmdLine += L"\" \"";
	strCmdLine += szPath1;
	strCmdLine += '\"';
	//MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

	BOOL bOK = FALSE;

	// create a windres.exe process
	MProcessMaker pmaker;
	pmaker.SetShowWindow(SW_HIDE);
	pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

	MFile hInputWrite, hOutputRead;
	if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
		pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
	{
		// read all with timeout
		bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
		pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

		// check the exit code
		if (pmaker.GetExitCode() == 0 && bOK)
		{
			bOK = FALSE;

			// import res
			EntrySet res;
			if (res.import_res(szPath3))
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
			bOK = FALSE;
			// error message
			size_t ich = strOutput.find("RisohEditor.rc:");
			if (ich != strOutput.npos)
			{
				ich += 15; // "RisohEditor.rc:"
				INT iLine = INT(strtoul(&strOutput[ich], NULL, 10));
				::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
				::SendMessageW(m_hCodeEditor, LNEM_SETLINEMARK, iLine, ERROR_LINE_COLOR);
			}
			strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_COMPILEERROR));
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

	AutoDeleteFileW adf2(szPath2);

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
	if (IsWindow(m_rad_window))
	{
		m_rad_window.DestroyWindow();
	}

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
			if (IsWindow(m_rad_window))
				m_rad_window.DestroyWindow();
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

BOOL ChooseLangListBoxLang(HWND hwnd, LANGID wLangId)
{
	INT index = 0;
	for (auto& lang : g_langs)
	{
		if (lang.LangID == wLangId)
			break;
		++index;
	}

	if (index >= (INT)g_langs.size())
		return FALSE;

	index = ListBox_FindStringExact(hwnd, -1, g_langs[index].str.c_str());
	if (index < 0)
		return FALSE;

	ListBox_SetCurSel(hwnd, index);
	ListBox_SetTopIndex(hwnd, index);
	return TRUE;
}

BOOL InitLangListBox(HWND hwnd)
{
	for (auto& lang : g_langs)
	{
		INT index = ListBox_AddString(hwnd, lang.str.c_str());
		ListBox_SetItemData(hwnd, index, lang.LangID);
	}
	return TRUE;
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
		ShowLangArrow(FALSE);

		// renewal
		g_res.delete_all();
		g_res.merge(res1);

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
		ShowLangArrow(FALSE);

		// renewal
		g_res.delete_all();
		g_res.merge(res);

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

	// unload the resource.h file
	UnloadResourceH(hwnd);
	if (g_settings.bAutoLoadNearbyResH)
		CheckResourceH(hwnd, pszPath);

	// load all the resource items from the executable
	m_bLoading = TRUE;
	{
		ShowLangArrow(FALSE);
		g_res.delete_all();
		g_res.from_res(hMod);
	}
	m_bLoading = FALSE;

	// free the executable
	FreeLibrary(hMod);

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

	// update language arrow
	PostUpdateLangArrow(m_hwnd);

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

	// reset the settings of the resource.h file
	g_settings.AddIDC_STATIC();
	g_settings.id_map.clear();
	g_settings.added_ids.clear();
	g_settings.removed_ids.clear();
	m_szResourceH[0] = 0;

	// update the names
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

// find the text
void MMainWnd::OnFind(HWND hwnd)
{
	m_search.bDownward = TRUE;
	OnItemSearch(hwnd);
}

// find next
BOOL MMainWnd::OnFindNext(HWND hwnd)
{
	m_search.bDownward = TRUE;
	if (m_search.strText.empty())
	{
		OnItemSearch(hwnd);
		return TRUE;
	}
	DoItemSearchBang(hwnd, NULL);
	return TRUE;
}

// find previous
BOOL MMainWnd::OnFindPrev(HWND hwnd)
{
	m_search.bDownward = FALSE;
	if (m_search.strText.empty())
	{
		OnItemSearch(hwnd);
		return TRUE;
	}
	DoItemSearchBang(hwnd, NULL);
	return TRUE;
}

BOOL MMainWnd::DoWriteRCLangUTF8(MFile& file, ResToText& res2text, WORD lang, const EntrySet& targets)
{
	MTextToAnsi comment_sep(CP_UTF8, LoadStringDx(IDS_COMMENT_SEP));

	if (!g_settings.bSepFilesByLang && g_settings.bRedundantComments)
	{
		file.WriteSzA(comment_sep.c_str());
		file.WriteSzA("\r\n");
	}

	// dump a comment and a LANGUAGE statement
	MString strLang = ::GetLanguageStatement(lang, TRUE);
	strLang += L"\r\n";
	file.WriteSzA(MWideToAnsi(CP_ACP, strLang).c_str());

	// search the language entries
	EntrySet found;
	targets.search(found, ET_LANG, BAD_TYPE, BAD_NAME, lang);

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
		if (type == RT_MESSAGETABLE && !g_settings.bUseMSMSGTABLE)
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
				MWideToAnsi utf8(CP_UTF8, strType);
				file.WriteSzA("// ");
				file.WriteSzA(utf8.c_str());
				file.WriteSzA("\r\n\r\n");
			}

			mstr_trim(str);     // trim

			// convert the text to UTF-8
			MTextToAnsi t2a(CP_UTF8, str.c_str());
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

		// convert the text to UTF-8
		MTextToAnsi t2a(CP_UTF8, str.c_str());

		// write it
		file.WriteSzA(t2a.c_str());
	}

	// search the message tables
	found.clear();
	if (!g_settings.bUseMSMSGTABLE)
	{
		targets.search(found, ET_LANG, RT_MESSAGETABLE, BAD_NAME, lang);
		if (found.size())
		{
			if (g_settings.bRedundantComments)
			{
				file.WriteSzA(comment_sep.c_str());
				file.WriteSzA("// RT_MESSAGETABLE\r\n\r\n");
			}

			// found --> msg_res
			MessageRes msg_res;
			for (auto e : found)
			{
				if (e->m_lang != lang)
					continue;       // must be same language

				MByteStreamEx stream(e->m_data);
				if (!msg_res.LoadFromStream(stream, e->m_name.m_id))
					return FALSE;
			}

			// dump it
			MString str;
			str += L"#ifdef APSTUDIO_INVOKED\r\n";
			str += L"    #error Ap Studio cannot edit this message table.\r\n";
			str += L"#endif\r\n";
			str += L"#ifdef MCDX_INVOKED\r\n";
			str += msg_res.Dump();
			str += L"#endif\r\n\r\n";

			// convert it to UTF-8
			MTextToAnsi t2a(CP_UTF8, str.c_str());

			// write it
			file.WriteSzA(t2a.c_str());
		}
	}

	return TRUE;
}

BOOL MMainWnd::DoWriteRCLangUTF16(MFile& file, ResToText& res2text, WORD lang, const EntrySet& targets)
{
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

	// search the language entries
	EntrySet found;
	targets.search(found, ET_LANG, BAD_TYPE, BAD_NAME, lang);

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
		if (type == RT_MESSAGETABLE && !g_settings.bUseMSMSGTABLE)
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

	// search the message tables
	found.clear();
	if (!g_settings.bUseMSMSGTABLE)
	{
		targets.search(found, ET_LANG, RT_MESSAGETABLE, BAD_NAME, lang);
		if (found.size())
		{
			if (g_settings.bRedundantComments)
			{
				file.WriteSzW(comment_sep.c_str());
				file.WriteSzW(L"// RT_MESSAGETABLE\r\n\r\n");
			}

			// found --> msg_res
			MessageRes msg_res;
			for (auto e : found)
			{
				if (e->m_lang != lang)
					continue;       // must be same language

				MByteStreamEx stream(e->m_data);
				if (!msg_res.LoadFromStream(stream, e->m_name.m_id))
					return FALSE;
			}

			// dump it
			MString str;
			str += L"#ifdef APSTUDIO_INVOKED\r\n";
			str += L"    #error Ap Studio cannot edit this message table.\r\n";
			str += L"#endif\r\n";
			str += L"#ifdef MCDX_INVOKED\r\n";
			str += msg_res.Dump();
			str += L"#endif\r\n\r\n";

			// write it
			file.WriteSzW(str.c_str());
		}
	}

	return TRUE;
}

// write a language-specific RC text
BOOL MMainWnd::DoWriteRCLang(MFile& file, ResToText& res2text, WORD lang, const EntrySet& targets)
{
	if (g_settings.bRCFileUTF16)
		return DoWriteRCLangUTF16(file, res2text, lang, targets);
	else
		return DoWriteRCLangUTF8(file, res2text, lang, targets);
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

// write a RC file
BOOL MMainWnd::DoWriteRC(LPCWSTR pszFileName, LPCWSTR pszResH)
{
	EntrySet found;
	g_res.search(found, ET_LANG);

	return DoWriteRC(pszFileName, pszResH, found);
}

std::wstring generated_from(INT n)
{
	WCHAR szText[MAX_PATH];
	StringCchPrintfW(szText, _countof(szText), LoadStringDx(IDS_GENERATEDFROMTEXTINCLUDE), n);
	return szText;
}

// write a RC file
BOOL MMainWnd::DoWriteRC(LPCWSTR pszFileName, LPCWSTR pszResH, const EntrySet& found)
{
	ResToText res2text;
	res2text.m_bHumanReadable = FALSE;  // it's not human-friendly
	res2text.m_bNoLanguage = TRUE;      // no LANGUAGE statements generated

	// check not locking
	if (IsFileLockedDx(pszFileName))
	{
		WCHAR szMsg[MAX_PATH + 256];
		StringCchPrintfW(szMsg, _countof(szMsg), LoadStringDx(IDS_CANTWRITEBYLOCK), pszFileName);
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

	BOOL bRCFileUTF16 = g_settings.bRCFileUTF16;

	// Add Byte Order Mark (BOM)
	if (g_settings.bAddBomToRC)
	{
		DWORD cbWritten;
		if (bRCFileUTF16)
			file.WriteFile("\xFF\xFE", 2, &cbWritten);
		else
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
				std::wstring path = textinclude1_w;
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
		MWideToAnsi utf8(CP_UTF8, szTitle);
		file.WriteFormatA("// %s\r\n", utf8.c_str());

		MWideToAnsi utf8Notice(CP_UTF8, LoadStringDx(IDS_NOTICE));
		MWideToAnsi utf8Dagger(CP_UTF8, LoadStringDx(IDS_DAGGER));
		MWideToAnsi utf8CommentSep(CP_UTF8, LoadStringDx(IDS_COMMENT_SEP));
		file.WriteSzA(utf8Notice.c_str());
		file.WriteSzA(utf8Dagger.c_str());

		if (g_settings.bRedundantComments)
		{
			file.WriteSzA(utf8CommentSep.c_str());
			MWideToAnsi utf8GeneratedFrom1(CP_UTF8, generated_from(1));
			file.WriteSzA(utf8GeneratedFrom1);
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
			file.WriteSzA(utf8CommentSep.c_str());
			MWideToAnsi w2a(CP_UTF8, generated_from(2).c_str());
			file.WriteSzA(w2a.c_str());
			file.WriteSzA("\r\n");
		}

		file.WriteSzA(textinclude2_a.c_str());

		if (g_settings.bRedundantComments)
		{
			file.WriteSzA("\r\n");
			file.WriteSzA(utf8CommentSep.c_str());
			file.WriteSzA("\r\n");
		}

		file.WriteSzA("#pragma code_page(65001) // UTF-8\r\n\r\n");

		if (g_settings.bUseIDC_STATIC && !g_settings.bHideID)
		{
			file.WriteSzA("#ifndef IDC_STATIC\r\n");
			file.WriteSzA("    #define IDC_STATIC (-1)\r\n");
			file.WriteSzA("#endif\r\n\r\n");
		}
	}

	// get the used languages
	std::unordered_set<WORD> langs;
	typedef std::pair<WORD, MStringW> lang_pair;
	std::vector<lang_pair> lang_vec;

	for (auto res : found)
	{
		WORD lang = res->m_lang;
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
			if (!DoWriteRCLang(file, res2text, 0, found)) {
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
				MWideToAnsi utf8Notice(CP_UTF8, LoadStringDx(IDS_NOTICE));
				MWideToAnsi utf8Dagger(CP_UTF8, LoadStringDx(IDS_DAGGER));
				lang_file.WriteSzA(utf8Notice.c_str());
				lang_file.WriteSzA(utf8Dagger.c_str());
				lang_file.WriteSzA("\r\n");
				lang_file.WriteSzA("#pragma code_page(65001) // UTF-8\r\n\r\n");
			}
			if (!lang_file) {
				textinclude.delete_all();
				return FALSE;
			}
			if (!DoWriteRCLang(lang_file, res2text, lang, found)) {
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
			if (!DoWriteRCLang(file, res2text, lang, found)) {
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
				CharUpperW(&lang_name2[0]);

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
			MWideToAnsi strA(CP_UTF8, str.c_str());
			file.WriteSzA(strA.c_str());
			file.WriteSzA("\r\n");
		}

		if (p_textinclude2)
		{
			auto str = res2text.DumpEntry(*p_textinclude2);
			MWideToAnsi strA(CP_UTF8, str.c_str());
			file.WriteSzA(strA.c_str());
			file.WriteSzA("\r\n");
		}

		if (p_textinclude3)
		{
			auto str = res2text.DumpEntry(*p_textinclude3);
			MWideToAnsi strA(CP_UTF8, str.c_str());
			file.WriteSzA(strA.c_str());
			file.WriteSzA("\r\n");
		}

		file.WriteSzA("#endif // def APSTUDIO_INVOKED\r\n");

		if (g_settings.bRedundantComments)
		{
			file.WriteSzA("\r\n");
			file.WriteSzA(comment_sep.c_str());

			MWideToAnsi w2a(CP_UTF8, generated_from(3).c_str());
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
	if (IsFileLockedDx(pszResH))
	{
		WCHAR szMsg[MAX_PATH + 256];
		StringCchPrintfW(szMsg, _countof(szMsg), LoadStringDx(IDS_CANTWRITEBYLOCK), pszResH);
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
	file.WriteSzA("//{{NO_DEPENDENCIES}}\r\n");
	file.WriteSzA("// Microsoft Visual C++ Compatible\r\n");

	MWideToAnsi utf8Notice(CP_UTF8, LoadStringDx(IDS_NOTICE));
	file.WriteFormatA(utf8Notice.c_str());

	// write the RC file info if necessary
	if (pszRCFile)
	{
		// get file title
		TCHAR szFileTitle[64];
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
	prefixes[0] = g_settings.assoc_map[L"Resource.ID"];
	prefixes[1] = g_settings.assoc_map[L"Command.ID"];
	prefixes[2] = g_settings.assoc_map[L"New.Command.ID"];
	prefixes[3] = g_settings.assoc_map[L"Control.ID"];

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
			return PackedDIB_Extract(filename.c_str(), &(*entry)[0], entry->size(), FALSE);
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

struct AutoWrapEnable
{
	AutoWrapEnable()
	{
		// Wrap string literals only in UTF-16 RC. See Issue #302.
		g_wrap_enabled = g_settings.bRCFileUTF16;
	}
	~AutoWrapEnable()
	{
		g_wrap_enabled = false;
	}
};

// do export the resource data to an RC file and related files
BOOL MMainWnd::DoExportRC(LPCWSTR pszRCFile, LPWSTR pszResHFile, const EntrySet& found)
{
	AutoWrapEnable auto_wrap_enable;

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
		bOK = DoWriteResH(szPath, pszRCFile) && DoWriteRC(pszRCFile, szPath, found);

		// szPath --> pszResHFile
		if (bOK && pszResHFile)
			StringCchCopyW(pszResHFile, MAX_PATH, szPath);
	}
	else
	{
		// write the RC file
		bOK = DoWriteRC(pszRCFile, NULL, found);
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

BOOL MMainWnd::DoSaveAsCompression(LPCWSTR pszExeFile)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return TRUE;

	return DoSaveExeAs(pszExeFile, TRUE);
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

BOOL MMainWnd::DoSaveInner(LPCWSTR pszExeFile, BOOL bCompression)
{
	// File might be updated. Wait for virus scan
	WaitForVirusScan(pszExeFile, 10 * 1000);

	// src is not exe and dest exe is respected
	DoResetCheckSum(pszExeFile);

	// The executable is updated. Wait for virus scan
	WaitForVirusScan(pszExeFile, 10 * 1000);

	if (!g_res.update_exe(pszExeFile))
		return FALSE;

	// Now the executable is updated. Wait a little for virus checker.
	Sleep(300);

	// update file info
	UpdateFileInfo(FT_EXECUTABLE, pszExeFile, m_bUpxCompressed);

	// do compress by UPX
	if (g_settings.bCompressByUPX || bCompression)
	{
		DoUpxCompress(m_szUpxExe, pszExeFile);
	}

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
BOOL MMainWnd::DoSaveExeAs(LPCWSTR pszExeFile, BOOL bCompression)
{
	LPCWSTR src = m_szFile;
	LPCWSTR dest = pszExeFile;
	WCHAR szTempFile[MAX_PATH] = L"";
	AutoDeleteFileW ad(szTempFile);

	// check not locking
	if (IsFileLockedDx(dest))
	{
		WCHAR szMsg[MAX_PATH + 256];
		StringCchPrintfW(szMsg, _countof(szMsg), LoadStringDx(IDS_CANTWRITEBYLOCK), dest);
		ErrorBoxDx(szMsg);
		return FALSE;
	}

	// is the file compressed?
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
			return DoSaveInner(dest, bCompression);
		}
	}
	else if (bDestExecutable)
	{
		return DoSaveInner(dest, bCompression);
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
				return DoSaveInner(dest, bCompression);
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
				return DoSaveInner(dest, bCompression);
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
	pmaker.SetShowWindow(SW_HIDE);
	pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

	MFile hInputWrite, hOutputRead;
	if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
		pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
	{
		// read all output
		MStringA strOutput;
		bOK = pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);
		pmaker.WaitForSingleObject(PROCESS_TIMEOUT);

		if (pmaker.GetExitCode() == 0 && bOK)
		{
			bOK = (strOutput.find("Packed") != MStringA::npos);
		}
	}

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
		ShowLangArrow(FALSE);

		// merge
		g_res.merge(res);

		// clean up
		res.delete_all();
	}
	m_bLoading = FALSE;

	// select none
	SelectTV(NULL, FALSE);

	// update language arrow
	PostUpdateLangArrow(hwnd);

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
		ShowLangArrow(FALSE);

		// renewal
		g_res.merge(res);

		// clean up
		res.delete_all();
	}
	m_bLoading = FALSE;

	// refresh the ID list window
	DoRefreshIDList(hwnd);

	// update language arrow
	PostUpdateLangArrow(hwnd);

	return IMPORTED;
}

IMPORT_RESULT MMainWnd::DoImport(HWND hwnd, LPCWSTR pszFile, LPCWSTR pchDotExt)
{
	if (!pchDotExt)
		return NOT_IMPORTABLE;

	if (lstrcmpiW(pchDotExt, L".rc") == 0 || lstrcmpiW(pchDotExt, L".rc2") == 0)
	{
		return DoImportRC(hwnd, pszFile);
	}
	else if (lstrcmpiW(pchDotExt, L".res") == 0)
	{
		return DoImportRes(hwnd, pszFile);
	}
	else if (lstrcmpiW(pchDotExt, L".ico") == 0)
	{
		// show the dialog
		MAddIconDlg dialog;
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// refresh the ID list window
			DoRefreshIDList(hwnd);

			// select the entry
			SelectTV(ET_LANG, dialog, FALSE);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".cur") == 0 || lstrcmpiW(pchDotExt, L".ani") == 0)
	{
		// show the dialog
		MAddCursorDlg dialog;
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// refresh the ID list window
			DoRefreshIDList(hwnd);

			// select the entry
			SelectTV(ET_LANG, dialog, FALSE);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".html") == 0 || lstrcmpiW(pchDotExt, L".htm") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_type = RT_HTML;
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".manifest") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_type = RT_MANIFEST;
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".wav") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_type = L"WAVE";
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".mp3") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_type = L"MP3";
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".bmp") == 0 || lstrcmpiW(pchDotExt, L".dib") == 0)
	{
		// show the dialog
		MAddBitmapDlg dialog;
		dialog.m_file = pszFile;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// refresh the ID list window
			DoRefreshIDList(hwnd);

			// select the entry
			SelectTV(ET_LANG, dialog, FALSE);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".png") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"PNG";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".gif") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"GIF";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".jpg") == 0 || lstrcmpiW(pchDotExt, L".jpeg") == 0 ||
			 lstrcmpiW(pchDotExt, L".jpe") == 0 || lstrcmpiW(pchDotExt, L".jfif") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"JPEG";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".tif") == 0 || lstrcmpiW(pchDotExt, L".tiff") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"TIFF";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".avi") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"AVI";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".wmf") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"WMF";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".emf") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"EMF";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".dfm") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = RT_RCDATA;
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}
	else if (lstrcmpiW(pchDotExt, L".tlb") == 0)
	{
		// show the dialog
		MAddResDlg dialog;
		dialog.m_file = pszFile;
		dialog.m_type = L"TYPELIB";
		if (dialog.DialogBoxDx(hwnd) == IDOK)
		{
			// add a resource item
			DoAddRes(hwnd, dialog);

			DoSetFileModified(TRUE);

			return IMPORTED;
		}
		return IMPORT_CANCELLED;
	}

	return NOT_IMPORTABLE;
}

// WM_DROPFILES: file(s) has been dropped
void MMainWnd::OnDropFiles(HWND hwnd, HDROP hdrop)
{
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

	// update language arrow
	PostUpdateLangArrow(hwnd);

	// remove the command lock
	--m_nCommandLock;

	// show "ready" if just unlocked
	if (m_nCommandLock == 0 && !::IsWindow(m_rad_window))
		ChangeStatusText(IDS_READY);
}

// open the dialog to load the resource.h
void MMainWnd::OnLoadResH(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
	{
		return;
	}

	// (resource.h file path) --> szFile
	WCHAR szFile[MAX_PATH];
	if (m_szResourceH[0])
		StringCchCopyW(szFile, _countof(szFile), m_szResourceH);
	else
		StringCchCopyW(szFile, _countof(szFile), L"resource.h");

	// if it does not exist, clear the file path
	if (!PathFileExistsW(szFile))
		szFile[0] = 0;

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_HEADFILTER));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_LOADRESH);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"h";     // the default extension

	// let the user choose the path
	if (GetOpenFileNameW(&ofn))
	{
		// load the resource.h file
		DoLoadResH(hwnd, szFile);

		// is the resource.h file loaded?
		if (m_szResourceH[0])
		{
			// show the ID list window
			ShowIDList(hwnd, TRUE);
		}

		// update the names
		UpdateNames();
	}
}

// load the resource.h file
void MMainWnd::OnLoadResHBang(HWND hwnd)
{
	if (m_szResourceH[0])
	{
		MString strFile = m_szResourceH;
		DoLoadResH(hwnd, strFile.c_str());

		if (m_szResourceH[0])
		{
			ShowIDList(hwnd, TRUE);
		}
	}
}

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
	StopMP3();
	StopAvi();

	// Try to cancel searching
	m_search.bCancelled = FALSE;
	::Sleep(100);

	// release auto complete
	DoLangEditAutoCompleteRelease(hwnd);

	// close preview
	HidePreview();

	// unload the resource.h file
	OnUnloadResH(hwnd);

	// update the file info
	UpdateFileInfo(FT_NONE, NULL, FALSE);

	// unselect
	SelectTV(NULL, FALSE);

	// clean up
	g_res.delete_all();

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

	// destroy the window's
	m_rad_window.DestroyWindow();
	DestroyWindow(m_hHexViewer);
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
					char sz[32];
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
	pmaker.SetShowWindow(SW_HIDE);
	pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

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
	pmaker.SetShowWindow(SW_HIDE);
	pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

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
	ShowIDList(hwnd, IsWindow(m_id_list_dlg));
}

// refresh the treeview
void MMainWnd::DoRefreshTV(HWND hwnd)
{
	// refresh the resource items
	EntrySet res;
	res.merge(g_res);
	g_res.delete_all();
	g_res.merge(res);

	// clean up
	res.delete_all();

	// redraw
	InvalidateRect(m_hwndTV, NULL, TRUE);
}

// advice the change for resource.h file
void MMainWnd::OnAdviceResH(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	MString str;

	if (!g_settings.removed_ids.empty() &&
		(g_settings.removed_ids.size() != 1 ||
		 g_settings.removed_ids.find("IDC_STATIC") == g_settings.removed_ids.end()))
	{
		str += LoadStringDx(IDS_DELETENEXTIDS);

		for (auto& pair : g_settings.removed_ids)
		{
			if (pair.first == "IDC_STATIC")
				continue;

			str += TEXT("#define ");
			str += MAnsiToText(CP_ACP, pair.first).c_str();
			str += TEXT(" ");
			str += MAnsiToText(CP_ACP, pair.second).c_str();
			str += TEXT("\r\n");
		}
		str += TEXT("\r\n");
	}

	if (!g_settings.added_ids.empty() &&
		(g_settings.added_ids.size() != 1 ||
		 g_settings.added_ids.find("IDC_STATIC") == g_settings.added_ids.end() ||
		 !g_settings.bUseIDC_STATIC))
	{
		str += LoadStringDx(IDS_ADDNEXTIDS);

		for (auto& pair : g_settings.added_ids)
		{
			str += TEXT("#define ");
			str += MAnsiToText(CP_ACP, pair.first).c_str();
			str += TEXT(" ");
			str += MAnsiToText(CP_ACP, pair.second).c_str();
			str += TEXT("\r\n");
		}
		str += TEXT("\r\n");
	}

	if (str.empty())
	{
		str += LoadStringDx(IDS_NOCHANGE);
		str += TEXT("\r\n");
	}

	// show the dialog
	MAdviceResHDlg dialog(str);
	dialog.DialogBoxDx(hwnd);
}

// unload the resource.h info
void MMainWnd::OnUnloadResH(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// unload the resource.h file
	UnloadResourceH(hwnd);
}

// the configuration dialog
void MMainWnd::OnConfig(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MConfigDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// refresh PATHs
		ReSetPaths(hwnd);

		// update word wrapping
		ReCreateSrcEdit(hwnd);

		// update the fonts
		ReCreateFonts(hwnd);

		// update the labels of the entries
		UpdateNames(FALSE);

		// select the entry to update the text
		auto entry = g_res.get_entry();
		SelectTV(entry, FALSE);

		// update title bar
		UpdateTitleBar();
	}

	// update menu
	UpdateMenu();
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

// use IDC_STATIC macro or not
void MMainWnd::OnUseIDC_STATIC(HWND hwnd)
{
	// toggle the flag
	g_settings.bUseIDC_STATIC = !g_settings.bUseIDC_STATIC;

	// select the entry to update the text
	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE, STV_RESETTEXT);
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

	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE);

	if (bModified)
		DoSetFileModified(TRUE);
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

// show/hide the ID macros
void MMainWnd::OnHideIDMacros(HWND hwnd)
{
	BOOL bListOpen = IsWindow(m_id_list_dlg);

	// toggle the flag
	g_settings.bHideID = !g_settings.bHideID;

	UpdateNames(FALSE);

	ShowIDList(hwnd, bListOpen);

	// select the entry to update the text
	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE);
}

void MMainWnd::OnDfmSettings(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	MDfmSettingsDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		g_settings.nDfmCodePage = dialog.m_nCodePage;
		g_settings.bDfmRawTextComments = dialog.m_bComments;
		g_settings.bDfmNoUnicode = dialog.m_bNoUnicode;

		// select the entry to update the text
		auto entry = g_res.get_entry();
		SelectTV(entry, FALSE);
	}
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

// show the ID list window
void MMainWnd::OnIDList(HWND hwnd)
{
	ShowIDList(hwnd, TRUE);
}

// show the ID association dialog
void MMainWnd::OnIdAssoc(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(::IsWindowVisible(m_rad_window)))
		return;

	// show the dialog
	MIdAssocDlg dialog;
	dialog.DialogBoxDx(hwnd);

	// update the prefix database
	UpdatePrefixDB(hwnd);
}

// show the language list
void MMainWnd::OnShowLangs(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MLangsDlg dialog;
	dialog.DialogBoxDx(hwnd);
}

// show/hide the toolbar
void MMainWnd::OnShowHideToolBar(HWND hwnd)
{
	// toggle the flag
	g_settings.bShowToolBar = !g_settings.bShowToolBar;

	if (g_settings.bShowToolBar)
		ShowWindow(m_hToolBar, SW_SHOWNOACTIVATE);
	else
		ShowWindow(m_hToolBar, SW_HIDE);

	// recalculate the splitter
	PostMessageDx(WM_SIZE);
}

// the paths dialog
void MMainWnd::OnSetPaths(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MPathsDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		ReSetPaths(hwnd);
	}
}

// start changing the resource name/language
void MMainWnd::OnEditLabel(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected type entry
	auto entry = g_res.get_entry();
	if (!entry || entry->m_et == ET_TYPE)
	{
		return;
	}

	if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
	{
		if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
		{
			return;
		}
	}

	HTREEITEM hItem = TreeView_GetSelection(m_hwndTV);
	TreeView_EditLabel(m_hwndTV, hItem);
}

// the predefined macro dialog
void MMainWnd::OnPredefMacros(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MMacrosDlg dialog;
	INT_PTR nID = dialog.DialogBoxDx(hwnd);
	switch (INT(nID))
	{
	case IDOK:
		g_settings.macros = dialog.m_map;
		break;
	case psh6:
		g_settings.ResetMacros();
		break;
	}
}

// expand all the tree control items
void MMainWnd::OnExpandAll(HWND hwnd)
{
	ShowLangArrow(FALSE);

	// get the selected entry
	auto entry = g_res.get_entry();

	HTREEITEM hItem = TreeView_GetRoot(m_hwndTV);
	do
	{
		Expand(hItem);
		hItem = TreeView_GetNextSibling(m_hwndTV, hItem);
	} while (hItem);

	// select the entry
	SelectTV(entry, FALSE);

	// update language arrow
	PostUpdateLangArrow(hwnd);
}

// unexpand all the tree control items
void MMainWnd::OnCollapseAll(HWND hwnd)
{
	ShowLangArrow(FALSE);

	HTREEITEM hItem = TreeView_GetRoot(m_hwndTV);
	do
	{
		Collapse(hItem);
		hItem = TreeView_GetNextSibling(m_hwndTV, hItem);
	} while (hItem);

	// select the entry
	SelectTV(NULL, FALSE);

	// update language arrow
	PostUpdateLangArrow(hwnd);
}

void MMainWnd::OnAddBang(HWND hwnd, NMTOOLBAR *pToolBar)
{
	// TODO: If you edited "Edit" menu, then you may have to update the below codes
	HMENU hMenu = GetMenu(hwnd);
	HMENU hEditMenu = GetSubMenu(hMenu, 1);
	HMENU hAddMenu = GetSubMenu(hEditMenu, 2);

	// the button rectangle
	RECT rcItem = pToolBar->rcButton;

	// get the hot point
	POINT pt;
	pt.x = rcItem.left;
	pt.y = rcItem.bottom;
	ClientToScreen(m_hToolBar, &pt);

	// get the monitor info
	HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	ZeroMemory(&mi, sizeof(mi));
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMon, &mi);

	// by the hot point, change the menu alignment
	UINT uFlags = TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL;
	if (pt.y >= (mi.rcWork.top + mi.rcWork.bottom) / 2)
	{
		uFlags |= TPM_BOTTOMALIGN;
		pt.x = rcItem.left;
		pt.y = rcItem.top;
		ClientToScreen(m_hToolBar, &pt);
	}
	else
	{
		uFlags |= TPM_TOPALIGN;
	}

	// See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648002.aspx
	SetForegroundWindow(m_hwnd);

	// show the popup menu
	TPMPARAMS params;
	ZeroMemory(&params, sizeof(params));
	params.cbSize = sizeof(params);
	params.rcExclude = rcItem;
	TrackPopupMenuEx(hAddMenu, uFlags, pt.x, pt.y, m_hwnd, &params);

	// See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648002.aspx
	SendMessageDx(WM_NULL);
}

void MMainWnd::OnClone(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	auto entry = g_res.get_entry();
	if (!entry)
		return;

	switch (entry->m_et)
	{
	case ET_TYPE:
		break;

	case ET_NAME:
		OnCopyAsNewName(hwnd);
		break;

	case ET_LANG:
	case ET_STRING:
	case ET_MESSAGE:
		OnCopyAsNewLang(hwnd);
		break;

	default:
		break;
	}
}

void MMainWnd::OnGuide(HWND hwnd)
{
	static const WCHAR szJapaneseURL[] =
		L"https://katahiromz.web.fc2.com/colony3rd/risoheditor/";
	static const WCHAR szEnglishURL[] =
		L"https://katahiromz.web.fc2.com/colony3rd/risoheditor/en/";

	if (PRIMARYLANGID(GetThreadUILanguage()) == LANG_JAPANESE)
		ShellExecuteW(hwnd, NULL, szJapaneseURL, NULL, NULL, SW_SHOWNORMAL);
	else
		ShellExecuteW(hwnd, NULL, szEnglishURL, NULL, NULL, SW_SHOWNORMAL);
}

void MMainWnd::OnEncoding(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	MEncodingDlg dialog;
	if (IDOK == dialog.DialogBoxDx(hwnd))
	{
		// select the entry
		auto entry = g_res.get_entry();
		SelectTV(entry, FALSE);
	}
}

void MMainWnd::OnQueryConstant(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	MConstantDlg dialog;
	dialog.DialogBoxDx(hwnd);
}

void MMainWnd::OnExtractBang(HWND hwnd)
{
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	switch (entry->m_et)
	{
	case ET_TYPE:
	case ET_NAME:
	case ET_STRING:
	case ET_MESSAGE:
		OnExtractBin(hwnd);
		break;

	case ET_LANG:
		if (entry->m_type == RT_ICON || entry->m_type == RT_GROUP_ICON ||
			entry->m_type == RT_ANIICON)
		{
			OnExtractIcon(hwnd);
		}
		else if (entry->m_type == RT_CURSOR || entry->m_type == RT_GROUP_CURSOR ||
				 entry->m_type == RT_ANICURSOR)
		{
			OnExtractCursor(hwnd);
		}
		else if (entry->m_type == RT_BITMAP)
		{
			OnExtractBitmap(hwnd);
		}
		else if (entry->m_type == RT_RCDATA && entry->is_delphi_dfm())
		{
			OnExtractDFM(hwnd);
		}
		else if (entry->m_type == L"TYPELIB")
		{
			OnExtractTLB(hwnd);
		}
		else
		{
			OnExtractBin(hwnd);
		}
		break;

	default:
		break;
	}
}

void MMainWnd::OnSaveAsWithCompression(HWND hwnd)
{
	enum ResFileFilterIndex     // see also: IDS_EXEFILTER
	{
		RFFI2_NONE = 0,
		RFFI2_EXECUTABLE = 1,
		RFFI2_ALL = 2
	};

	// compile if necessary
	if (!CompileIfNecessary(TRUE))
		return;

	// store m_szFile to szFile
	WCHAR szFile[MAX_PATH];
	StringCchCopyW(szFile, _countof(szFile), m_szFile);

	// if not found, then make it empty
	if (!PathFileExistsW(szFile))
		szFile[0] = 0;

	// initialize OPENFILENAME structure
	OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
	ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXEFILTER));

	// use the prefered filter by the entry
	ofn.nFilterIndex = RFFI2_EXECUTABLE;

	// use the preferred extension
	WCHAR szExt[32];
	LPWSTR pchDotExt = PathFindExtensionW(m_szFile);
	if (pchDotExt && *pchDotExt == L'.')
	{
		StringCbCopyW(szExt, sizeof(szExt), pchDotExt + 1);
		ofn.lpstrDefExt = szExt;
	}
	else
	{
		ofn.lpstrDefExt = L"exe";       // the default extension
	}

	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrTitle = LoadStringDx(IDS_SAVEASCOMPRESS);
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
				OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

	// let the user choose the path
	if (GetSaveFileNameW(&ofn))
	{
		// save it
		if (!DoSaveAsCompression(szFile))
		{
			ErrorBoxDx(IDS_CANNOTSAVE);
		}
	}
}

// toggle the word wrapping of the source EDIT control
void MMainWnd::OnWordWrap(HWND hwnd)
{
	// save the modified flag
	BOOL bModified = Edit_GetModify(m_hCodeEditor);

	// switch the flag
	g_settings.bWordWrap = !g_settings.bWordWrap;

	// get text
	MString strText = GetWindowTextW(m_hCodeEditor);

	// create the source EDIT control
	ReCreateSrcEdit(hwnd);

	// reset fonts
	ReCreateFonts(hwnd);

	// restore text
	SetWindowTextW(m_hCodeEditor, strText.c_str());

	// restore the modified flag
	Edit_SetModify(m_hCodeEditor, bModified);

	// select the entry to refresh
	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE, STV_DONTRESET);
}

void MMainWnd::OnUseMSMSGTBL(HWND hwnd)
{
	g_settings.bUseMSMSGTABLE = !g_settings.bUseMSMSGTABLE;

	// create the source EDIT control
	ReCreateSrcEdit(hwnd);

	// reset fonts
	ReCreateFonts(hwnd);

	// select the entry to refresh
	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE, STV_RESETTEXT);
}

void MMainWnd::OnUseBeginEnd(HWND hwnd)
{
	g_settings.bUseBeginEnd = !g_settings.bUseBeginEnd;

	// create the source EDIT control
	ReCreateSrcEdit(hwnd);

	// reset fonts
	ReCreateFonts(hwnd);

	// select the entry to refresh
	auto entry = g_res.get_entry();
	SelectTV(entry, FALSE, STV_RESETTEXT);
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

void MMainWnd::UpdateLangArrow()
{
	EntryBase *entry = g_res.get_entry();
	if (!entry)
	{
		HTREEITEM hItem = TreeView_GetSelection(m_hwndTV);
		ShowLangArrow(FALSE, hItem);
		return;
	}

	HTREEITEM hItem = entry->m_hItem;

	switch (entry->m_et)
	{
	case ET_LANG:
		if (entry->m_type != RT_STRING && entry->m_type != RT_MESSAGETABLE)
			ShowLangArrow(TRUE, hItem);
		else
			ShowLangArrow(FALSE, hItem);
		break;
	case ET_STRING:
	case ET_MESSAGE:
		ShowLangArrow(TRUE, hItem);
		break;
	default:
		ShowLangArrow(FALSE, hItem);
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

void MMainWnd::OnRefreshAll(HWND hwnd)
{
	BOOL bModifiedOld = s_bModified;
	DoRefreshTV(hwnd);
	DoRefreshIDList(hwnd);
	SelectTV(g_res.get_entry(), FALSE);

	s_bModified = bModifiedOld;

	PostUpdateLangArrow(hwnd);
}

// WM_COMMAND
void MMainWnd::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	MWaitCursor wait;

	if (codeNotify == EN_CHANGE && m_hCodeEditor == hwndCtl)
	{
		// the source EDIT control was modified.
		// change the toolbar
		UpdateOurToolBarButtons(2);

		// show "ready" status
		ChangeStatusText(IDS_READY);
		return;
	}

	// show "executing command..." status
	if (!::IsWindow(m_rad_window) && id >= 100)
		ChangeStatusText(IDS_EXECUTINGCMD);

	// add a command lock
	++m_nCommandLock;

	// execute the command
	BOOL bUpdateStatus = TRUE;
	switch (id)
	{
	case ID_NEW:
		OnNew(hwnd);
		break;
	case ID_OPEN:
		OnOpen(hwnd);
		break;
	case ID_SAVEAS:
		OnSaveAs(hwnd);
		break;
	case ID_IMPORT:
		OnImport(hwnd);
		break;
	case ID_EXIT:
		DestroyWindow(hwnd);
		break;
	case ID_ADDICON:
		OnAddIcon(hwnd);
		break;
	case ID_ADDCURSOR:
		OnAddCursor(hwnd);
		break;
	case ID_ADDBITMAP:
		OnAddBitmap(hwnd);
		break;
	case ID_ADDRES:
		OnAddRes(hwnd);
		break;
	case ID_REPLACEICON:
		OnReplaceIcon(hwnd);
		break;
	case ID_REPLACECURSOR:
		OnReplaceCursor(hwnd);
		break;
	case ID_REPLACEBITMAP:
		OnReplaceBitmap(hwnd);
		break;
	case ID_REPLACEBIN:
		OnReplaceBin(hwnd);
		break;
	case ID_DELETERES:
		OnDeleteRes(hwnd);
		break;
	case ID_EDIT:
		OnEdit(hwnd);
		break;
	case ID_EXTRACTICON:
		OnExtractIcon(hwnd);
		break;
	case ID_EXTRACTCURSOR:
		OnExtractCursor(hwnd);
		break;
	case ID_EXTRACTBITMAP:
		OnExtractBitmap(hwnd);
		break;
	case ID_EXTRACTBIN:
		OnExtractBin(hwnd);
		break;
	case ID_ABOUT:
		OnAbout(hwnd);
		break;
	case ID_TEST:
		OnTest(hwnd);
		break;
	case ID_CANCELEDIT:
		OnCancelEdit(hwnd);
		break;
	case ID_COMPILE:
		OnCompile(hwnd);
		break;
	case ID_GUIEDIT:
		OnGuiEdit(hwnd);
		break;
	case ID_DESTROYRAD:
		OnCancelEdit(hwnd);
		break;
	case ID_DELCTRL:
		MRadCtrl::DeleteSelection();
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
		break;
	case ID_ADDCTRL:
		m_rad_window.OnAddCtrl(m_rad_window);
		break;
	case ID_CTRLPROP:
		m_rad_window.OnCtrlProp(m_rad_window);
		break;
	case ID_DLGPROP:
		m_rad_window.OnDlgProp(m_rad_window);
		break;
	case ID_CTRLINDEXTOP:
		m_rad_window.IndexTop(m_rad_window);
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
		break;
		break;
	case ID_CTRLINDEXBOTTOM:
		m_rad_window.IndexBottom(m_rad_window);
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
		break;
		break;
	case ID_CTRLINDEXMINUS:
		m_rad_window.IndexMinus(m_rad_window);
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
		break;
		break;
	case ID_CTRLINDEXPLUS:
		m_rad_window.IndexPlus(m_rad_window);
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
		break;
	case ID_SHOWHIDEINDEX:
		m_rad_window.OnShowHideIndex(m_rad_window);
		break;
	case ID_TOPALIGN:
		m_rad_window.OnTopAlign(m_rad_window);
		break;
	case ID_BOTTOMALIGN:
		m_rad_window.OnBottomAlign(m_rad_window);
		break;
	case ID_LEFTALIGN:
		m_rad_window.OnLeftAlign(m_rad_window);
		break;
	case ID_RIGHTALIGN:
		m_rad_window.OnRightAlign(m_rad_window);
		break;
	case ID_FITTOGRID:
		m_rad_window.OnFitToGrid(m_rad_window);
		bUpdateStatus = FALSE;
		break;
	case ID_STATUSBAR:
		// toggle the flag
		g_settings.bShowStatusBar = !g_settings.bShowStatusBar;

		// show/hide the scroll bar
		ShowStatusBar(g_settings.bShowStatusBar);

		// relayout
		PostMessageDx(WM_SIZE);
		break;
	case ID_BINARYPANE:
		// toggle the flag
		g_settings.bShowBinEdit = !g_settings.bShowBinEdit;
		// show/hide the binary
		m_tab.SetCurSel(!!g_settings.bShowBinEdit);
		OnSelChange(hwnd, !!g_settings.bShowBinEdit);
		break;
	case ID_ALWAYSCONTROL:
		{
			// toggle the flag
			g_settings.bAlwaysControl = !g_settings.bAlwaysControl;

			// select the entry to update the text
			auto entry = g_res.get_entry();
			SelectTV(entry, FALSE);
		}
		break;
	case ID_MRUFILE0:
	case ID_MRUFILE1:
	case ID_MRUFILE2:
	case ID_MRUFILE3:
	case ID_MRUFILE4:
	case ID_MRUFILE5:
	case ID_MRUFILE6:
	case ID_MRUFILE7:
	case ID_MRUFILE8:
	case ID_MRUFILE9:
	case ID_MRUFILE10:
	case ID_MRUFILE11:
	case ID_MRUFILE12:
	case ID_MRUFILE13:
	case ID_MRUFILE14:
	case ID_MRUFILE15:
		{
			DWORD i = id - ID_MRUFILE0;
			if (i < g_settings.vecRecentlyUsed.size())
			{
				DoLoadFile(hwnd, g_settings.vecRecentlyUsed[i].c_str());
			}
		}
		break;
	case ID_PLAY:
		OnPlay(hwnd);
		break;
	case ID_READY:
		if (g_bNoGuiMode)
		{
			std::vector<MStringW> commands;
			mstr_split(commands, m_commands, L"\n");
			for (auto& command : commands)
			{
				if (command.empty())
					continue;

				if (command.find(L"load:") == 0)
				{
					command = command.substr(5);
					if (!DoResLoad(command, m_load_options))
						s_ret = 3;
					continue;
				}

				if (command.find(L"save:") == 0)
				{
					command = command.substr(5);
					if (!DoResSave(command, m_save_options))
						s_ret = 4;
					continue;
				}
			}

			PostMessageW(hwnd, WM_CLOSE, 0, 0);
		}
		break;
	case ID_IDASSOC:
		OnIdAssoc(hwnd);
		break;
	case ID_LOADRESH:
		OnLoadResH(hwnd);
		break;
	case ID_IDLIST:
		OnIDList(hwnd);
		break;
	case ID_UNLOADRESH:
		OnUnloadResH(hwnd);
		break;
	case ID_HIDEIDMACROS:
		OnHideIDMacros(hwnd);
		break;
	case ID_USEIDC_STATIC:
		OnUseIDC_STATIC(hwnd);
		break;
	case ID_CONFIG:
		OnConfig(hwnd);
		break;
	case ID_ADVICERESH:
		OnAdviceResH(hwnd);
		break;
	case ID_UPDATEID:
		UpdateNames();
		break;
	case ID_OPENREADME:
		OnOpenLocalFile(hwnd, L"README.txt");
		break;
	case ID_OPENREADMEJP:
		OnOpenLocalFile(hwnd, L"README_ja.txt");
		break;
	case ID_OPENREADMEES:
		OnOpenLocalFile(hwnd, L"README_es.txt");
		break;
	case ID_LOADWCLIB:
		OnLoadWCLib(hwnd);
		break;
	case ID_FIND:
		OnFind(hwnd);
		break;
	case ID_FINDDOWNWARD:
		OnFindNext(hwnd);
		break;
	case ID_FINDUPWARD:
		OnFindPrev(hwnd);
		break;
	case ID_REPLACE:
		break;
	case ID_ADDMENU:
		OnAddMenu(hwnd);
		break;
	case ID_ADDTOOLBAR:
		OnAddToolbar(hwnd);
		break;
	case ID_ADDVERINFO:
		OnAddVerInfo(hwnd);
		break;
	case ID_ADDMANIFEST:
		OnAddManifest(hwnd);
		break;
	case ID_ADDDIALOG:
		OnAddDialog(hwnd);
		break;
	case ID_ADDSTRINGTABLE:
		OnAddStringTable(hwnd);
		break;
	case ID_ADDMESSAGETABLE:
		OnAddMessageTable(hwnd);
		break;
	case ID_ADDHTML:
		OnAddHtml(hwnd);
		break;
	case ID_ADDACCEL:
		OnAddAccel(hwnd);
		break;
	case ID_COPYASNEWNAME:
		OnCopyAsNewName(hwnd);
		break;
	case ID_COPYASNEWLANG:
		OnCopyAsNewLang(hwnd);
		break;
	case ID_COPYTOMULTILANG:
		OnCopyToMultiLang(hwnd);
		break;
	case ID_ITEMSEARCH:
		OnItemSearch(hwnd);
		break;
	case ID_UPDATERESHBANG:
		OnUpdateResHBang(hwnd);
		break;
	case ID_OPENLICENSE:
		OnOpenLocalFile(hwnd, L"LICENSE.txt");
		break;
	case ID_OPENHISTORY:
		OnOpenLocalFile(hwnd, L"CHANGELOG.txt");
		break;
	case ID_OPENHISTORYITA:
		OnOpenLocalFile(hwnd, L"CHANGELOG_it.txt");
		break;
	case ID_OPENHISTORYJPN:
		OnOpenLocalFile(hwnd, L"CHANGELOG_ja.txt");
		break;
	case ID_OPENHISTORYKOR:
		OnOpenLocalFile(hwnd, L"CHANGELOG_ko.txt");
		break;
	case ID_OPENHISTORYES:
		OnOpenLocalFile(hwnd, L"CHANGELOG_es.txt");
		break;
	case ID_OPENHYOJUNKA:
		OnOpenLocalFile(hwnd, L"HYOJUNKA.md");
		break;
	case ID_DEBUGTREENODE:
		OnDebugTreeNode(hwnd);
		break;
	case ID_LOADRESHBANG:
		OnLoadResHBang(hwnd);
		break;
	case ID_REFRESHDIALOG:
		m_rad_window.OnRefresh(m_rad_window);
		break;
	case ID_REFRESHALL:
		OnRefreshAll(hwnd);
		break;
	case ID_EXPORT:
		OnExport(hwnd);
		break;
	case ID_FONTS:
		OnFonts(hwnd);
		break;
	case ID_REFRESH:
		DoRefreshTV(hwnd);
		break;
	case ID_PREDEFMACROS:
		OnPredefMacros(hwnd);
		break;
	case ID_EDITLABEL:
		OnEditLabel(hwnd);
		break;
	case ID_SETPATHS:
		OnSetPaths(hwnd);
		break;
	case ID_SETDEFAULTS:
		SetDefaultSettings(hwnd);
		break;
	case ID_SHOWLANGS:
		OnShowLangs(hwnd);
		break;
	case ID_SHOWHIDETOOLBAR:
		OnShowHideToolBar(hwnd);
		break;
	case ID_EXPAND_ALL:
		OnExpandAll(hwnd);
		break;
	case ID_COLLAPSE_ALL:
		OnCollapseAll(hwnd);
		break;
	case ID_WORD_WRAP:
		OnWordWrap(hwnd);
		break;
	case ID_SAVEASCOMPRESS:
		OnSaveAsWithCompression(hwnd);
		break;
	case ID_CLONE:
		OnClone(hwnd);
		break;
	case ID_ADDBANG:
		break;
	case ID_EXTRACTBANG:
		OnExtractBang(hwnd);
		break;
	case ID_GUIDE:
		OnGuide(hwnd);
		break;
	case ID_ENCODING:
		OnEncoding(hwnd);
		break;
	case ID_QUERYCONSTANT:
		OnQueryConstant(hwnd);
		break;
	case ID_USEBEGINEND:
		OnUseBeginEnd(hwnd);
		break;
	case ID_USEMSMSGTBL:
		OnUseMSMSGTBL(hwnd);
		break;
	case ID_SAVE:
		OnSave(hwnd);
		break;
	case ID_EGA:
		OnEga(hwnd);
		break;
	case ID_EGA_PROGRAM:
		OnEgaProgram(hwnd);
		break;
	case ID_OPENREADMEIT:
		OnOpenLocalFile(hwnd, L"README_it.txt");
		break;
	case ID_OPEN_EGA_MANUAL:
		OnOpenLocalFile(hwnd, L"EGA\\EGA-Manual.pdf");
		break;
	case ID_DIALOG_FONT_SUBSTITUTES:
		OnDialogFontSubst(hwnd);
		break;
	case ID_HELP:
		OnHelp(hwnd);
		break;
	case ID_NEXTPANE:
		OnNextPane(hwnd, TRUE);
		break;
	case ID_PREVPANE:
		OnNextPane(hwnd, FALSE);
		break;
	case ID_EXTRACTRC:
		OnExtractRC(hwnd);
		break;
	case ID_EXPORTRES:
		OnExportRes(hwnd);
		break;
	case ID_CHECKUPDATE:
		OnCheckUpdate(hwnd);
		break;
	case ID_DFMSETTINGS:
		OnDfmSettings(hwnd);
		break;
	case ID_AUTOCOMPLETE:
		{
			HWND hwndEdit = TreeView_GetEditControl(m_hwndTV);
			DoLangEditAutoComplete(hwnd, hwndEdit);
		}
		break;
	case ID_AUTOCOMPLETEDONE:
		DoLangEditAutoCompleteRelease(hwnd);
		break;
	case ID_OPENREADMEKO:
		OnOpenLocalFile(hwnd, L"README_ko.txt");
		break;
	case ID_CHOOSEUILANG:
		{
			MChooseLangDlg dialog;
			if (dialog.DialogBoxDx(hwnd) == IDOK)
				g_settings.ui_lang = dialog.m_langid;
		}
		break;
	case ID_OPENREADMETR:
		OnOpenLocalFile(hwnd, L"README_tr.txt");
		break;
	case ID_OPENREADMEID:
		OnOpenLocalFile(hwnd, L"README_id.txt");
		break;
	case ID_OPENHISTORYID:
		OnOpenLocalFile(hwnd, L"CHANGELOG_id.txt");
		break;
	case ID_OPENREADMEPTB:
		OnOpenLocalFile(hwnd, L"README_pt.txt");
		break;
	case ID_OPENREADMETR:
		OnOpenLocalFile(hwnd, L"README_tr.txt");
		break;
	case ID_OPENHISTORYPTB:
		OnOpenLocalFile(hwnd, L"CHANGELOG_pt.txt");
		break;
	case ID_EGAFINISH:
		{
			BOOL bModifiedOld = s_bModified;
			DoRefreshTV(hwnd);
			DoRefreshIDList(hwnd);
			s_bModified = bModifiedOld;
		}
		if (!g_RES_select_type.is_zero() ||
			g_RES_select_name != BAD_TYPE ||
			g_RES_select_lang != BAD_LANG)
		{
			EntrySet found;
			g_res.search(found, ET_LANG, g_RES_select_type, g_RES_select_name, g_RES_select_lang);

			for (auto e : found)
			{
				SelectTV(e, FALSE);
				break;
			}

			g_RES_select_type = BAD_TYPE;
			g_RES_select_name = BAD_NAME;
			g_RES_select_lang = BAD_LANG;
		}
		PostUpdateLangArrow(hwnd);
		break;
	case ID_INTERNALTEST:
		OnInternalTest(hwnd);
		break;
	default:
		bUpdateStatus = FALSE;
		break;
	}

	// remove the command lock
	--m_nCommandLock;

	if (m_nCommandLock == 0)
		g_res.delete_invalid();     // clean up invalids

	UpdateToolBarStatus();

	// show "ready" status if ready
	if (m_nCommandLock == 0 && bUpdateStatus && !::IsWindow(m_rad_window))
	{
		if (m_nStatusStringID != 0)
		{
			ChangeStatusText(m_nStatusStringID);
			m_nStatusStringID = 0;
		}
		else
		{
			ChangeStatusText(IDS_READY);
		}
	}

#if 0 && !defined(NDEBUG) && (WINVER >= 0x0500)
	// show object counts (for debugging purpose)
	HANDLE hProcess = GetCurrentProcess();
	TCHAR szText[64];
	StringCchPrintf(szText, _countof(szText), TEXT("GDI:%ld, USER:%ld"),
			 GetGuiResources(hProcess, GR_GDIOBJECTS),
			 GetGuiResources(hProcess, GR_USEROBJECTS));
	ChangeStatusText(szText);
#endif
}

// get the resource name from text
MIdOrString GetNameFromText(const WCHAR *pszText)
{
	// pszText --> szText
	WCHAR szText[128];
	StringCchCopyW(szText, _countof(szText), pszText);

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

		// string
		return MIdOrString(szText);
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
			auto nIDTYPE_ = IDTYPE_(g_db.GetValue(L"RESOURCE.ID.TYPE", pair.first));
			ret.push_back(nIDTYPE_);
		}
	}
	return ret;
}

BOOL MMainWnd::ShowLangArrow(BOOL bShow, HTREEITEM hItem)
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
		m_arrow.ChooseLang(entry->m_lang);
	}
	else
	{
		if (IsWindow(m_arrow))
		{
			ShowWindow(m_arrow, SW_HIDE);
			InvalidateRect(m_hwndTV, &rc, TRUE);
		}
	}

	return TRUE;
}

void MMainWnd::DoLangEditAutoCompleteRelease(HWND hwnd)
{
	if (m_pAutoComplete)
	{
		m_pAutoComplete->unbind();
		m_pAutoComplete->Release();
		m_pAutoComplete = NULL;
	}

	m_auto_comp_edit.unhook();
}

void MMainWnd::DoLangEditAutoComplete(HWND hwnd, HWND hwndEdit)
{
	DoLangEditAutoCompleteRelease(hwnd);

	m_pAutoComplete = new MLangAutoComplete();
	if (!m_pAutoComplete)
		return;

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
		MWaitCursor wait;
		auto ptv = (NM_TREEVIEW *)pnmhdr;
		auto entry = (EntryBase *)ptv->itemOld.lParam;
		g_res.on_delete_item(entry);
		DoSetFileModified(TRUE);
	}
	else if (pnmhdr->code == NM_DBLCLK)
	{
		MWaitCursor wait;
		if (pnmhdr->hwndFrom == m_hwndTV && entry)
		{
			switch (entry->m_et)
			{
			case ET_LANG:
				OnEdit(hwnd);
				if (g_settings.bGuiByDblClick)
				{
					OnGuiEdit(hwnd);
				}
				return 1;
			case ET_STRING:
				OnEdit(hwnd);
				if (g_settings.bGuiByDblClick)
				{
					OnGuiEdit(hwnd);
				}
				return 1;
			case ET_MESSAGE:
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
		if (IsWindow(m_rad_window))
		{
			m_rad_window.DestroyWindow();
		}
	}
	else if (pnmhdr->code == TVN_SELCHANGED)
	{
		MWaitCursor wait;
		if (!m_bLoading && entry)
		{
			// select the entry to update the text
			SelectTV(entry, FALSE);
			OnSelChange(hwnd, 0);

			PostUpdateLangArrow(hwnd);
		}
	}
	else if (pnmhdr->code == TVN_ITEMEXPANDING)
	{
		m_arrow.ShowDropDownList(m_arrow, FALSE);
		ShowLangArrow(FALSE);
	}
	else if (pnmhdr->code == TVN_ITEMEXPANDED)
	{
		PostUpdateLangArrow(hwnd);
	}
	else if (pnmhdr->code == NM_RETURN)
	{
		MWaitCursor wait;
		if (pnmhdr->hwndFrom == m_hwndTV && entry)
		{
			switch (entry->m_et)
			{
			case ET_LANG:
				OnEdit(hwnd);
				if (g_settings.bGuiByDblClick)
				{
					OnGuiEdit(hwnd);
				}
				return 1;
			case ET_STRING:
				OnEdit(hwnd);
				if (g_settings.bGuiByDblClick)
				{
					OnGuiEdit(hwnd);
				}
				return 1;
			case ET_MESSAGE:
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
			ShowLangArrow(FALSE);
			PostUpdateLangArrow(hwnd);
			break;
		case VK_F2:
			{
				// compile if necessary
				if (!CompileIfNecessary(FALSE))
					return TRUE;

				// get the selected type entry
				auto entry = g_res.get_entry();
				if (!entry || entry->m_et == ET_TYPE)
				{
					return TRUE;
				}

				if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
				{
					if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
					{
						return TRUE;
					}
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
		static WORD old_lang = BAD_LANG;
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
			{
				return TRUE;    // prevent
			}

			auto entry = (EntryBase *)lParam;

			if (entry->m_et == ET_TYPE)
			{
				return TRUE;    // prevent
			}

			if (entry->m_et == ET_NAME || entry->m_et== ET_LANG)
			{
				if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
				{
					return TRUE;    // prevent
				}
			}

			StringCchCopyW(szOldText, _countof(szOldText), pszOldText);
			mstr_trim(szOldText);

			if (entry->m_et == ET_LANG || entry->m_et == ET_STRING ||
				entry->m_et == ET_MESSAGE)
			{
				old_lang = LangFromText(szOldText);
				if (old_lang == BAD_LANG)
				{
					return TRUE;    // prevent
				}
			}

			m_arrow.ShowDropDownList(m_arrow, FALSE);
			ShowLangArrow(FALSE);

			switch (entry->m_et)
			{
			case ET_LANG:
			case ET_MESSAGE:
			case ET_STRING:
				PostMessage(hwnd, WM_COMMAND, ID_AUTOCOMPLETE, 0);
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

			auto entry = (EntryBase *)lParam;

			switch (entry->m_et)
			{
			case ET_LANG:
			case ET_MESSAGE:
			case ET_STRING:
				PostMessage(hwnd, WM_COMMAND, ID_AUTOCOMPLETEDONE, 0);
				break;
			}

			if (pszNewText == NULL)
			{
				PostUpdateLangArrow(hwnd);
				return FALSE;   // reject
			}

			if (!entry || entry->m_et == ET_TYPE)
			{
				return FALSE;   // reject
			}

			if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
			{
				if (entry->m_type == RT_STRING || entry->m_type == RT_MESSAGETABLE)
				{
					return FALSE;   // reject
				}
			}

			WCHAR szNewText[MAX_PATH];
			StringCchCopyW(szNewText, _countof(szNewText), pszNewText);
			mstr_trim(szNewText);
			if (entry->m_et == ET_NAME && !szNewText[0])
			{
				ErrorBoxDx(IDS_INVALIDNAME);
				return FALSE;   // reject
			}

			if (entry->m_et == ET_NAME)
			{
				// rename the name
				MIdOrString old_name = GetNameFromText(szOldText);
				MIdOrString new_name = GetNameFromText(szNewText);

				if (old_name == new_name)
					return FALSE;   // reject

				// check if it already exists
				if (g_res.find(ET_LANG, entry->m_type, new_name))
				{
					ErrorBoxDx(IDS_ALREADYEXISTS);
					return FALSE;   // reject
				}

				DoRenameEntry(pszNewText, entry, old_name, new_name);
				DoSetFileModified(TRUE);
				return TRUE;   // accept
			}
			else if (entry->m_et == ET_LANG)
			{
				PostUpdateLangArrow(hwnd);

				old_lang = LangFromText(szOldText);
				if (old_lang == BAD_LANG)
					return FALSE;   // reject

				WORD new_lang = LangFromText(szNewText);
				if (new_lang == BAD_LANG)
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
			else if (entry->m_et == ET_STRING || entry->m_et == ET_MESSAGE)
			{
				PostUpdateLangArrow(hwnd);

				old_lang = LangFromText(szOldText);
				if (old_lang == BAD_LANG)
					return FALSE;   // reject

				WORD new_lang = LangFromText(szNewText);
				if (new_lang == BAD_LANG)
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

// change the name of the resource entries
void MMainWnd::DoRenameEntry(LPWSTR pszText, EntryBase *entry, const MIdOrString& old_name, const MIdOrString& new_name)
{
	// search the old named language entries
	EntrySet found;
	g_res.search(found, ET_LANG, entry->m_type, old_name);

	// rename them
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
void MMainWnd::DoRelangEntry(LPWSTR pszText, EntryBase *entry, WORD old_lang, WORD new_lang)
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

	case ET_MESSAGE:
		// serach the resource messages
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

void MMainWnd::OnNextPane(HWND hwnd, BOOL bNext)
{
	HWND hwndCodeEditor = m_hCodeEditor;
	HWND hwndHexViewer = m_hHexViewer;
	HWND hwndRad = IsWindow(m_rad_window) ? (HWND)m_rad_window : NULL;
	HWND hwndIDList = IsWindow(m_id_list_dlg) ? (HWND)m_id_list_dlg : NULL;
	HWND hwndFind = IsWindow(m_hFindReplaceDlg) ? (HWND)m_hFindReplaceDlg : NULL;

	HWND hwndFocus = GetFocus();

	if (hwndRad != NULL && GetParent(hwndFocus) == hwndRad)
		hwndFocus = hwndRad;

	if (hwndIDList != NULL && GetParent(hwndFocus) == hwndIDList)
		hwndFocus = hwndIDList;

	if (hwndFind != NULL && GetParent(hwndFocus) == hwndFind)
		hwndFocus = hwndFind;

	if (hwndFocus == NULL)
	{
		SetFocus(m_hwndTV);
		return;
	}

	HWND ahwnd[] =
	{
		m_hwndTV, m_hCodeEditor, m_hHexViewer, hwndRad, m_hFindReplaceDlg, hwndIDList
	};

	UINT i;
	for (i = 0; i < _countof(ahwnd); ++i)
	{
		if (ahwnd[i] == hwndFocus)
			break;
	}

	if (i == _countof(ahwnd))
	{
		SetFocus(m_hwndTV);
		return;
	}

	if (bNext)
	{
		do
		{
			++i;
			if (i == _countof(ahwnd))
				i = 0;
		} while (ahwnd[i] == NULL);
	}
	else
	{
		do
		{
			if (i == 0)
				i = _countof(ahwnd) - 1;
			else
				--i;
		} while (ahwnd[i] == NULL);
	}

	if (hwndCodeEditor == ahwnd[i])
	{
		OnSelChange(hwnd, 0);
		SetFocus(m_hCodeEditor);
	}
	else if (hwndHexViewer == ahwnd[i])
	{
		OnSelChange(hwnd, 1);
		SetFocus(m_hHexViewer);
	}
	else
	{
		SetFocus(ahwnd[i]);
	}
}

void MMainWnd::OnHelp(HWND hwnd)
{
	ShellExecuteW(hwnd, NULL, LoadStringDx(IDS_HOMEPAGE), NULL, NULL, SW_SHOWNORMAL);
}

void MMainWnd::OnDialogFontSubst(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// if RADical window is displayed
	if (IsWindowVisible(m_rad_window))
	{
		// destroy it
		m_rad_window.DestroyWindow();
	}

	MDialogFontSubstDlg dialog;
	dialog.DialogBoxDx(hwnd);
}

// do resource test
void MMainWnd::OnTest(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(::IsWindowVisible(m_rad_window)))
		return;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	if (entry->m_type == RT_DIALOG)
	{
		// entry->m_data --> stream --> dialog_res
		DialogRes dialog_res;
		MByteStreamEx stream(entry->m_data);
		dialog_res.LoadFromStream(stream);

		if (!dialog_res.m_class.empty())
		{
			// TODO: support the classed dialogs
			ErrorBoxDx(IDS_CANTTESTCLASSDLG);
		}
		else
		{
			// detach the dialog template menu (it will be used after)
			MIdOrString menu = dialog_res.m_menu;
			dialog_res.m_menu.clear();
			stream.clear();

			// fixup for "AtlAxWin*" and/or "{...}" window classes.
			// see also: DialogRes::FixupForTest
			dialog_res.FixupForTest(false);
			dialog_res.SaveToStream(stream);
			dialog_res.FixupForTest(true);

			// load RT_DLGINIT if any
			std::vector<BYTE> dlginit_data;
			if (auto e = g_res.find(ET_LANG, RT_DLGINIT, entry->m_name, entry->m_lang))
			{
				dlginit_data = e->m_data;
			}

			// show test dialog
			if (dialog_res.m_style & WS_CHILD)
			{
				// dialog_res is a child dialog. create its parent (with menu if any)
				auto window = new MTestParentWnd(dialog_res, menu, entry->m_lang,
												 stream, dlginit_data);
				window->CreateWindowDx(hwnd, LoadStringDx(IDS_PARENTWND),
					WS_DLGFRAME | WS_POPUPWINDOW, WS_EX_APPWINDOW);

				// show it
				ShowWindow(*window, g_bNoGuiMode ? SW_HIDE : SW_SHOWNORMAL);
				UpdateWindow(*window);
			}
			else
			{
				// it's a non-child dialog. show the test dialog (with menu if any)
				MTestDialog dialog(dialog_res, menu, entry->m_lang, dlginit_data);
				dialog.DialogBoxIndirectDx(hwnd, stream.ptr());
				stream = std::move(stream);
			}
		}
	}
	else if (entry->m_type == RT_MENU)
	{
		// load a menu from memory
		HMENU hMenu = LoadMenuIndirect(&(*entry)[0]);
		if (hMenu)
		{
			// show the dialog
			MTestMenuDlg dialog(hMenu);
			dialog.DialogBoxDx(hwnd, IDD_MENUTEST);

			// unload the menu
			DestroyMenu(hMenu);
		}
	}
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
		WCHAR title[64];
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

// do save or update the resource.h file
void MMainWnd::OnUpdateResHBang(HWND hwnd)
{
	// check whether the ID list window is open or not
	BOOL bListOpen = IsWindow(m_id_list_dlg);

	// destroy the ID list window
	DestroyWindow(m_id_list_dlg);

	// // query update to the user
	// if (MsgBoxDx(IDS_UPDATERESH, MB_ICONINFORMATION | MB_YESNO) == IDNO)    // don't update
	// {
	//     ShowIDList(hwnd, bListOpen);
	//     return;
	// }

	if (1)
	{
		// build new "resource.h" file path
		WCHAR szResH[MAX_PATH];

		if (m_szResourceH[0])
		{
			StringCchCopyW(szResH, _countof(szResH), m_szResourceH);
		}
		else if (m_szFile[0])
		{
			StringCchCopyW(szResH, _countof(szResH), m_szFile);

			WCHAR *pch = wcsrchr(szResH, L'\\');
			if (pch == NULL)
				pch = wcsrchr(szResH, L'/');
			if (pch == NULL)
				return; // failure

			*pch = 0;
			StringCchCatW(szResH, _countof(szResH), L"\\resource.h");
		}
		else
		{
			StringCchCopyW(szResH, _countof(szResH), L"resource.h");
		}

		// initialize OPENFILENAME structure
		OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
		ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_HEADFILTER));
		ofn.lpstrFile = szResH;
		ofn.nMaxFile = _countof(szResH);
		ofn.lpstrTitle = LoadStringDx(IDS_SAVERESH);
		ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
					OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = L"h";

		// let the user choose the path
		if (!GetSaveFileNameW(&ofn))
		{
			return;     // cancelled
		}

		// create new
		if (!DoWriteResH(szResH))
		{
			ErrorBoxDx(IDS_CANTWRITERESH);
			ShowIDList(hwnd, bListOpen);
			return;     // failure
		}

		// szResH --> m_szResourceH
		StringCchCopyW(m_szResourceH, _countof(m_szResourceH), szResH);
	}
	else    // update the resource.h file by modification
	{
		// do backup the resource.h file
		if (g_settings.bBackup)
			DoBackupFile(m_szResourceH);

		// open file
		FILE *fp = _wfopen(m_szResourceH, L"r");
		if (!fp)
		{
			ErrorBoxDx(IDS_CANTWRITERESH);
			ShowIDList(hwnd, bListOpen);
			return;
		}

		// read the resource.h lines
		std::vector<MStringA> lines;
		ReadResHLines(fp, lines);
		fclose(fp);     // close the files

		// modify the lines
		UpdateResHLines(lines);

		// reopen the file to write
		fp = _wfopen(m_szResourceH, L"w");
		if (!fp)
		{
			ErrorBoxDx(IDS_CANTWRITERESH);
			ShowIDList(hwnd, bListOpen);
			return;
		}

		// write now
		for (size_t i = 0; i < lines.size(); ++i)
		{
			fprintf(fp, "%s\n", lines[i].c_str());
		}

		fflush(fp);
		fclose(fp);     // close the files
	}

	// clear modification of IDs
	g_settings.added_ids.clear();
	g_settings.removed_ids.clear();

	// reopen the ID list window if necessary
	ShowIDList(hwnd, bListOpen);

	DoSetFileModified(TRUE);
}

// add an icon resource
void MMainWnd::OnAddIcon(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddIconDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// refresh the ID list window
		DoRefreshIDList(hwnd);

		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);

		DoSetFileModified(TRUE);
	}
}

// replace the icon resource
void MMainWnd::OnReplaceIcon(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)
		return;

	// show the dialog
	MReplaceIconDlg dialog(entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);

		DoSetFileModified(TRUE);
	}
}

// replace the cursor resource
void MMainWnd::OnReplaceCursor(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry)
		return;

	// show the dialog
	MReplaceCursorDlg dialog(entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);

		DoSetFileModified(TRUE);
	}
}

// add a bitmap resource
void MMainWnd::OnAddBitmap(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddBitmapDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// refresh the ID list window
		DoRefreshIDList(hwnd);

		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);

		DoSetFileModified(TRUE);
	}
}

// replace the bitmap resource
void MMainWnd::OnReplaceBitmap(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (!entry)
		return;

	// show the dialog
	MReplaceBitmapDlg dialog(*entry);
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);

		DoSetFileModified(TRUE);
	}
}

// add a cursor
void MMainWnd::OnAddCursor(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddCursorDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// refresh the ID list window
		DoRefreshIDList(hwnd);

		// select the entry
		SelectTV(ET_LANG, dialog, FALSE);

		DoSetFileModified(TRUE);
	}
}

// add a resource item
void MMainWnd::OnAddRes(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// add a menu
void MMainWnd::OnAddMenu(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_MENU;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// add a TOOLBAR
void MMainWnd::OnAddToolbar(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_TOOLBAR;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// add a string table
void MMainWnd::OnAddStringTable(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_STRING;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// add a message table resource
void MMainWnd::OnAddMessageTable(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_MESSAGETABLE;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// add an HTML resource
void MMainWnd::OnAddHtml(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_HTML;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// add an RT_ACCELERATOR resource
void MMainWnd::OnAddAccel(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_ACCELERATOR;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// add a version info resource
void MMainWnd::OnAddVerInfo(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_VERSION;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// add a manifest resource
void MMainWnd::OnAddManifest(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_MANIFEST;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

// add a resource item
void MMainWnd::DoAddRes(HWND hwnd, MAddResDlg& dialog)
{
	if (dialog.m_strTemplate.empty())   // already added
	{
		// refresh the ID list window
		DoRefreshIDList(hwnd);

		// select it
		SelectTV(ET_LANG, dialog, FALSE);

		// clear the modification flag
		Edit_SetModify(m_hCodeEditor, FALSE);
	}
	else        // use dialog.m_strTemplate
	{
		// dialog.m_strTemplate --> m_hCodeEditor
		SetWindowTextW(m_hCodeEditor, dialog.m_strTemplate.c_str());
		::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);

		// workaround to edit the Microsoft message table
		if (dialog.m_type == RT_MESSAGETABLE && g_settings.bUseMSMSGTABLE)
		{
			g_settings.bUseMSMSGTABLE = FALSE;
		}

		// compile dialog.m_strTemplate
		MStringA strOutput;
		if (CompileParts(strOutput, dialog.m_type, dialog.m_name, dialog.m_lang, dialog.m_strTemplate, FALSE))
		{
			// success. clear the modification flag
			Edit_SetModify(m_hCodeEditor, FALSE);
			m_nStatusStringID = IDS_RECOMPILEOK;
		}
		else
		{
			// failure
			m_nStatusStringID = IDS_RECOMPILEFAILED;
			UpdateOurToolBarButtons(2);

			// set the error message
			SetErrorMessage(strOutput, TRUE);

			// set the modification flag
			Edit_SetModify(m_hCodeEditor, TRUE);

			// make it non-read-only
			Edit_SetReadOnly(m_hCodeEditor, FALSE);
		}

		// select the added entry
		if (dialog.m_type == RT_STRING)
			SelectTV(ET_STRING, dialog.m_type, BAD_NAME, BAD_LANG, FALSE);
		else if (dialog.m_type == RT_MESSAGETABLE)
			SelectTV(ET_MESSAGE, dialog.m_type, BAD_NAME, BAD_LANG, FALSE);
		else
			SelectTV(ET_LANG, dialog, FALSE);
   }
}

// add a dialog template
void MMainWnd::OnAddDialog(HWND hwnd)
{
	// compile if necessary
	if (!CompileIfNecessary(FALSE))
		return;

	// show the dialog
	MAddResDlg dialog;
	dialog.m_type = RT_DIALOG;
	if (dialog.DialogBoxDx(hwnd) == IDOK)
	{
		// add a resource item
		DoAddRes(hwnd, dialog);

		DoSetFileModified(TRUE);
	}
}

void MMainWnd::UpdateTitleBar()
{
	if (m_szFile[0] == 0)
	{
		SetWindowTextW(m_hwnd, LoadStringDx(IDS_APPNAME));
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

	return TRUE;
}

BOOL MMainWnd::ReCreateSrcEdit(HWND hwnd)
{
	BOOL bModify = Edit_GetModify(m_hCodeEditor);

	if (IsWindow(m_hCodeEditor))
		DestroyWindow(m_hCodeEditor);

	DWORD style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP |
				  ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
				  ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN;
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
	case WM_SIZE: case WM_HSCROLL: case WM_VSCROLL:
	case WM_MOUSEWHEEL: case WM_KEYDOWN: case WM_CHAR:
		if (IsWindow(m_arrow))
		{
			// hide language arrow
			ShowLangArrow(FALSE);

			// get selected item rect
			RECT rc;
			HTREEITEM hItem = TreeView_GetSelection(hwnd);
			TreeView_GetItemRect(hwnd, hItem, &rc, FALSE);

			// default processing
			ret = CallWindowProc(s_fnTreeViewOldWndProc, hwnd, uMsg, wParam, lParam);

			// redraw the rect
			InvalidateRect(hwnd, &rc, TRUE);

			// restore language arrow
			PostUpdateLangArrow(m_hwnd);

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
	style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_TABSTOP |
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
	m_hHexViewer.CreateAsChildDx(m_splitter2, NULL, style, exstyle, 3);
	m_hHexViewer.SendMessageDx(EM_SETLIMITTEXT, 0x100000);

	// create source EDIT control
	if (!ReCreateSrcEdit(m_splitter2))
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
		DO_MESSAGE(MYWM_UPDATELANGARROW, OnUpdateLangArrow);
		DO_MESSAGE(MYWM_RADDBLCLICK, OnRadDblClick);

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

// select the message entry in the tree control
void MMainWnd::SelectMessage()
{
	// find the message table entry
	if (auto entry = g_res.find(ET_MESSAGE, RT_MESSAGETABLE))
	{
		// select the entry
		SelectTV(entry, FALSE);
	}
}

// do ID jump now!
void MMainWnd::OnIDJumpBang2(HWND hwnd, const MString& name, MString& strType)
{
	if (strType == L"Unknown.ID")
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
		if (nIDTYPE_ == IDTYPE_MESSAGE)
		{
			// select the message entry
			SelectMessage();
			return;     // done
		}
	}

	// get the type value
	MIdOrString type = WORD(g_db.GetValue(L"RESOURCE", strType));
	if (type.empty())
		type.m_str = strType;

	// name --> name_or_id
	MIdOrString name_or_id;
	if (name[0] == L'\"')
	{
		// non-numeric name
		MString name_clone = name;
		mstr_unquote(name_clone);   // unquote
		name_or_id = name_clone.c_str();
	}
	else
	{
		// numeric name
		name_or_id = WORD(g_db.GetResIDValue(name));
	}

	if (name_or_id.empty())  // name_or_id was empty
	{
		// name --> strA (ANSI)
		MStringA strA = MTextToAnsi(CP_ACP, name).c_str();

		// find strA from g_settings.id_map
		auto it = g_settings.id_map.find(strA);
		if (it != g_settings.id_map.end())  // found
		{
			MStringA strA = it->second;
			if (strA[0] == 'L')
				strA = strA.substr(1);

			// unquote
			mstr_unquote(strA);

			// resource name
			name_or_id.m_str = MAnsiToWide(CP_ACP, strA).c_str();
		}
	}

	// find the entry
	if (auto entry = g_res.find(ET_LANG, type, name_or_id))
	{
		// select the entry
		SelectTV(entry, FALSE);

		// set focus to the main window
		SetForegroundWindow(m_hwnd);
		BringWindowToTop(m_hwnd);
		SetFocus(m_hwnd);
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

LRESULT MMainWnd::OnUpdateDlgRes(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	DoSetFileModified(TRUE);

	// get the selected language entry
	auto entry = g_res.get_lang_entry();
	if (!entry || entry->m_type != RT_DIALOG)
	{
		return 0;
	}

	auto& dialog_res = m_rad_window.m_dialog_res;

	// dialog_res --> entry->m_data
	MByteStreamEx stream;
	dialog_res.SaveToStream(stream);
	entry->m_data = stream.data();

	// entry->m_lang + dialog_res --> str --> m_hCodeEditor (text)
	MString str = GetLanguageStatement(entry->m_lang);
	str += dialog_res.Dump(entry->m_name);
	SetWindowTextW(m_hCodeEditor, str.c_str());

	// entry->m_data --> m_hHexViewer (binary)
	str = DumpBinaryAsText(entry->m_data);
	SetWindowTextW(m_hHexViewer, str.c_str());

	return 0;
}

LRESULT MMainWnd::OnRadSelChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	::SendMessageW(m_hCodeEditor, LNEM_CLEARLINEMARKS, 0, 0);
	if (!IsWindow(m_rad_window))
		return 0;

	INT cHeads = INT(SendMessageW(hwnd, MYWM_GETDLGHEADLINES, 0, 0)) + 1;
	auto indeces = MRadCtrl::GetTargetIndeces();
	for (auto index : indeces)
	{
		::SendMessageW(m_hCodeEditor, LNEM_SETLINEMARK, cHeads + index, RGB(255, 255, 120));
	}
	return 0;
}

// do ID jump now!
LRESULT MMainWnd::OnIDJumpBang(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// the item index
	INT iItem = (INT)wParam;
	if (iItem == -1)
		return 0;

	// get the 1st and 2nd subitem texts
	TCHAR szText[128];
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
	if (!CreateWindowDx(NULL, MAKEINTRESOURCE(IDS_APPNAME),
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
	WCHAR szClass[16] = L"";
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

	// the default processing
	TranslateMessage(&msg);
	DispatchMessage(&msg);
}

// the main loop
INT_PTR MMainWnd::RunDx()
{
	MSG msg;

	while (BOOL bGot = ::GetMessage(&msg, NULL, 0, 0))
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

#ifdef _MSC_VER
	// the manifest information
	#pragma comment(linker, "/manifestdependency:\"type='win32' \
	  name='Microsoft.Windows.Common-Controls' \
	  version='6.0.0.0' \
	  processorArchitecture='*' \
	  publicKeyToken='6595b64144ccf1df' \
	  language='*'\"")
#endif

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
			MessageBoxW(NULL, LoadStringDx(IDS_APPNAME), LoadStringDx(IDS_APPNAME), MB_ICONINFORMATION);
			return FALSE;
		}

		if (lstrcmpiW(arg, L"-load") == 0 ||
			lstrcmpiW(arg, L"--load") == 0)
		{
			bNoGUI = TRUE;
			arg = targv[++iarg];
			m_commands += L"load:";
			m_commands += arg;
			m_commands += L"\n";
			continue;
		}
		if (lstrcmpiW(arg, L"-load-options") == 0 ||
			lstrcmpiW(arg, L"--load-options") == 0)
		{
			arg = targv[++iarg];
			m_load_options = arg;
			continue;
		}

		if (lstrcmpiW(arg, L"-save") == 0 ||
			lstrcmpiW(arg, L"--save") == 0)
		{
			bNoGUI = TRUE;
			arg = targv[++iarg];
			m_commands += L"save:";
			m_commands += arg;
			m_commands += L"\n";
		}
		if (lstrcmpiW(arg, L"-save-options") == 0 ||
			lstrcmpiW(arg, L"--save-options") == 0)
		{
			arg = targv[++iarg];
			m_save_options = arg;
			continue;
		}

		if (lstrcmpiW(arg, L"-log-file") == 0 ||
			lstrcmpiW(arg, L"--log-file") == 0)
		{
			arg = targv[++iarg];
			g_pszLogFile = arg;
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
	SetEnvironmentVariableW(L"LANG", L"en_US");
	setlocale(LC_CTYPE, "");

	// set the UI language
	g_settings.ui_lang = GetUILang();
	SetThreadUILanguage(LANGID(g_settings.ui_lang));

	// initialize the libraries
	OleInitialize(NULL);

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
	HINSTANCE hinstRichEdit = LoadLibrary(TEXT("RICHED32.DLL"));

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
	FreeLibrary(hinstRichEdit);
	FreeLibrary(hinstUXTheme);
	OleUninitialize();
	FreeWCLib();

	// check object counts
	assert(MacroParser::BaseAst::alive_count() == 0);

#if (WINVER >= 0x0500)
	HANDLE hProcess = GetCurrentProcess();
	MTRACEA("Count of GDI objects: %ld\n", GetGuiResources(hProcess, GR_GDIOBJECTS));
	MTRACEA("Count of USER objects: %ld\n", GetGuiResources(hProcess, GR_USEROBJECTS));
#endif

#if defined(_MSC_VER) && !defined(NDEBUG)
	// for detecting memory leak (MSVC only)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	return s_ret;
}

#include "WowFsRedirection.h"

// the main function of the windows application
extern "C"
INT WINAPI
wWinMain(HINSTANCE   hInstance,
		 HINSTANCE   hPrevInstance,
		 LPWSTR      lpCmdLine,
		 INT         nCmdShow)
{
	PVOID OldValue;
	BOOL bWowFsDisabled = DisableWow64FsRedirection(&OldValue);

	HRESULT hrCoInit = CoInitialize(NULL);

	INT ret = RisohEditor_Main(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

	if (bWowFsDisabled)
		RevertWow64FsRedirection(OldValue);

	if (SUCCEEDED(hrCoInit))
		CoUninitialize();

	return ret;
}

//////////////////////////////////////////////////////////////////////////////
