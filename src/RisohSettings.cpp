// RisohSettings.cpp --- RisohEditor
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "RisohEditor.hpp"
#include "MMainWnd.hpp"
#include "Utils.h"
#ifdef PORTABLE
	#include "MRegKeyPortable.hpp"
#else
	#include "MRegKey.hpp"
#endif

#define TV_WIDTH        250     // default m_hwndTV width
#define BV_WIDTH        160     // default m_hBmpView width

// the maximum number of captions to remember
static const DWORD s_nMaxCaptions = 10;

// set the default settings
void MMainWnd::SetDefaultSettings(HWND hwnd)
{
	g_settings.ui_lang = ::GetThreadUILanguage();
	m_bShowBinEdit = FALSE;
	g_settings.bAlwaysControl = FALSE;
	g_settings.bShowStatusBar = TRUE;
	g_settings.nTreeViewWidth = TV_WIDTH;
	g_settings.nBmpViewWidth = BV_WIDTH;
	g_settings.bGuiByDblClick = TRUE;
	g_settings.bResumeWindowPos = TRUE;
	g_settings.bAutoLoadNearbyResH = TRUE;
	g_settings.bAutoShowIDList = TRUE;
	g_settings.bHideID = FALSE;
	g_settings.bUseIDC_STATIC = FALSE;
	g_settings.bShowDotsOnDialog = TRUE;
	g_settings.nComboHeight = 300;
	g_settings.vecRecentlyUsed.clear();
	g_settings.nWindowLeft = CW_USEDEFAULT;
	g_settings.nWindowTop = CW_USEDEFAULT;
	g_settings.nWindowWidth = 760;
	g_settings.nWindowHeight = 480;
	g_settings.bMaximized = FALSE;
	g_settings.nIDListLeft = CW_USEDEFAULT;
	g_settings.nIDListTop = CW_USEDEFAULT;
	g_settings.nIDListWidth = 366;
	g_settings.nIDListHeight = 490;
	g_settings.nRadLeft = CW_USEDEFAULT;
	g_settings.nRadTop = CW_USEDEFAULT;
	g_settings.bAskUpdateResH = FALSE;
	g_settings.bCompressByUPX = FALSE;
	g_settings.bAddBomToRC = FALSE;
	g_settings.bUseBeginEnd = FALSE;
	g_settings.bShowFullPath = TRUE;
	g_settings.nDfmCodePage = 0;
	g_settings.bDfmRawTextComments = TRUE;
	g_settings.bDfmNoUnicode = FALSE;
	g_settings.bUseMSMSGTABLE = FALSE;
	g_settings.nEgaX = CW_USEDEFAULT;
	g_settings.nEgaY = CW_USEDEFAULT;
	g_settings.nEgaWidth = CW_USEDEFAULT;
	g_settings.nEgaHeight = CW_USEDEFAULT;
	g_settings.nDefResLangID = BAD_LANG;

	HFONT hFont;
	LOGFONTW lf, lfBin, lfSrc;

	// get GUI font
	GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);

	ZeroMemory(&lfBin, sizeof(lfBin));
	lfBin.lfHeight = 10;
	lfBin.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	lfBin.lfCharSet = lf.lfCharSet;
	hFont = CreateFontIndirectW(&lfBin);
	GetObject(hFont, sizeof(lfBin), &lfBin);
	if (HDC hDC = CreateCompatibleDC(NULL))
	{
		SelectObject(hDC, hFont);
		GetTextFace(hDC, LF_FACESIZE, lfBin.lfFaceName);
		DeleteDC(hDC);
	}
	DeleteObject(hFont);

	ZeroMemory(&lfSrc, sizeof(lfSrc));
	lfSrc.lfHeight = 13;
	lfSrc.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	lfSrc.lfCharSet = lf.lfCharSet;
	hFont = CreateFontIndirectW(&lfSrc);
	GetObject(hFont, sizeof(lfSrc), &lfSrc);
	if (HDC hDC = CreateCompatibleDC(NULL))
	{
		SelectObject(hDC, hFont);
		GetTextFace(hDC, LF_FACESIZE, lfSrc.lfFaceName);
		DeleteDC(hDC);
	}
	DeleteObject(hFont);

	g_settings.strSrcFont = lfSrc.lfFaceName;
	g_settings.strBinFont = lfBin.lfFaceName;

	g_settings.nSrcFontSize = 12;
	g_settings.nBinFontSize = 9;

	if (HDC hDC = CreateCompatibleDC(NULL))
	{
		if (lfBin.lfHeight < 0)
		{
			g_settings.nBinFontSize = -MulDiv(lfBin.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
		}
		else
		{
			g_settings.nBinFontSize = MulDiv(lfBin.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
		}

		if (lfSrc.lfHeight < 0)
		{
			g_settings.nSrcFontSize = -MulDiv(lfSrc.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
		}
		else
		{
			g_settings.nSrcFontSize = MulDiv(lfSrc.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
		}

		DeleteDC(hDC);
	}

	auto table1 = g_db.GetTable(L"RESOURCE.ID.TYPE");
	auto table2 = g_db.GetTable(L"RESOURCE.ID.PREFIX");
	assert(table1.size() == table2.size());

	g_settings.assoc_map.clear();
	if (table1.size() && table1.size() == table2.size())
	{
		for (size_t i = 0; i < table1.size(); ++i)
		{
			g_settings.assoc_map.insert(std::make_pair(table1[i].name, table2[i].name));
		}
	}
	else
	{
		g_settings.ResetAssoc();
	}

	g_settings.id_map.clear();
	g_settings.added_ids.clear();
	g_settings.removed_ids.clear();

	g_settings.ResetMacros();

	g_settings.includes.clear();

	g_settings.strWindResExe.clear();
	g_settings.strCppExe.clear();

	// cpp.exe
	StringCchCopyW(m_szMCppExe, _countof(m_szMCppExe), m_szDataFolder);
	StringCchCatW(m_szMCppExe, _countof(m_szMCppExe), L"\\bin\\mcpp.exe");
	GetShortPathNameW(m_szMCppExe, m_szMCppExe, _countof(m_szMCppExe));

	// windres.exe
	StringCchCopyW(m_szWindresExe, _countof(m_szWindresExe), m_szDataFolder);
	StringCchCatW(m_szWindresExe, _countof(m_szWindresExe), L"\\bin\\windres.exe");
	GetShortPathNameW(m_szWindresExe, m_szWindresExe, _countof(m_szWindresExe));

	g_settings.strPrevVersion.clear();

	g_settings.bSepFilesByLang = FALSE;
	g_settings.bStoreToResFolder = TRUE;
	g_settings.bSelectableByMacro = FALSE;

	g_settings.captions.clear();

	g_settings.bShowToolBar = TRUE;
#ifdef ATL_SUPPORT
	g_settings.strAtlAxWin = TEXT(ATLAXWIN_CLASS);
#else
	g_settings.strAtlAxWin = TEXT("AtlAxWin");
#endif
	g_settings.nSaveFilterIndex = 1;
	g_settings.bWordWrap = FALSE;

	g_settings.bBackup = TRUE;
	g_settings.strBackupSuffix = L"~";

	g_settings.bRedundantComments = TRUE;
	g_settings.bWrapManifest = FALSE;

	g_settings.bRCFileUTF16 = FALSE;

	g_settings.ResetEncoding();

	g_settings.strFontReplaceFrom1 = L"MS Shell Dlg";
	g_settings.strFontReplaceTo1 = L"MS Shell Dlg";

	g_settings.strFontReplaceFrom2 = L"MS Shell Dlg 2";
	g_settings.strFontReplaceTo2 = L"MS Shell Dlg 2";

	g_settings.strFontReplaceFrom3.clear();
	g_settings.strFontReplaceTo3.clear();

	// update the menu
	UpdateMenu();
}

// update the prefix data
void MMainWnd::UpdatePrefixDB(HWND hwnd)
{
	// update "RESOURCE.ID.PREFIX" table
	auto& table = g_db.m_map[L"RESOURCE.ID.PREFIX"];
	for (size_t i = 0; i < table.size(); ++i)
	{
		for (auto& pair : g_settings.assoc_map)
		{
			if (table[i].name == pair.first)    // matched
			{
				// update the value
				table[i].value = mstr_parse_int(pair.second.c_str());
				break;
			}
		}
	}
}

// load the settings
BOOL MMainWnd::LoadSettings(HWND hwnd)
{
	SetDefaultSettings(hwnd);

#ifdef PORTABLE
	#ifdef _WIN64
		MRegKeyPortable keyRisoh(TEXT("RisohEditor64"), NULL);
	#else
		MRegKeyPortable keyRisoh(TEXT("RisohEditor"), NULL);
	#endif
#else
	#ifdef _WIN64
		MRegKey keyRisoh(HKCU, TEXT("Software\\Katayama Hirofumi MZ\\RisohEditor64"));
	#else
		MRegKey keyRisoh(HKCU, TEXT("Software\\Katayama Hirofumi MZ\\RisohEditor"));
	#endif
	if (!keyRisoh)
		return FALSE;
#endif

	keyRisoh.QueryDword(TEXT("UILanguage"), (DWORD&)g_settings.ui_lang);
	if (g_settings.ui_lang == 0)
		g_settings.ui_lang = GetThreadUILanguage();

	keyRisoh.QueryDword(TEXT("HIDE.ID"), (DWORD&)g_settings.bHideID);
	keyRisoh.QueryDword(TEXT("bUseIDC_STATIC"), (DWORD&)g_settings.bUseIDC_STATIC);
	keyRisoh.QueryDword(TEXT("ShowStatusBar"), (DWORD&)g_settings.bShowStatusBar);
	keyRisoh.QueryDword(TEXT("AlwaysControl"), (DWORD&)g_settings.bAlwaysControl);
	keyRisoh.QueryDword(TEXT("TreeViewWidth"), (DWORD&)g_settings.nTreeViewWidth);
	keyRisoh.QueryDword(TEXT("BmpViewWidth"), (DWORD&)g_settings.nBmpViewWidth);
	keyRisoh.QueryDword(TEXT("bGuiByDblClick"), (DWORD&)g_settings.bGuiByDblClick);
	keyRisoh.QueryDword(TEXT("bResumeWindowPos"), (DWORD&)g_settings.bResumeWindowPos);
	keyRisoh.QueryDword(TEXT("bAutoLoadNearbyResH"), (DWORD&)g_settings.bAutoLoadNearbyResH);
	keyRisoh.QueryDword(TEXT("bAutoShowIDList"), (DWORD&)g_settings.bAutoShowIDList);
	keyRisoh.QueryDword(TEXT("bShowDotsOnDialog"), (DWORD&)g_settings.bShowDotsOnDialog);
	keyRisoh.QueryDword(TEXT("nComboHeight"), (DWORD&)g_settings.nComboHeight);
	keyRisoh.QueryDword(TEXT("nWindowLeft"), (DWORD&)g_settings.nWindowLeft);
	keyRisoh.QueryDword(TEXT("nWindowTop"), (DWORD&)g_settings.nWindowTop);
	keyRisoh.QueryDword(TEXT("nWindowWidth"), (DWORD&)g_settings.nWindowWidth);
	keyRisoh.QueryDword(TEXT("nWindowHeight"), (DWORD&)g_settings.nWindowHeight);
	keyRisoh.QueryDword(TEXT("bMaximized"), (DWORD&)g_settings.bMaximized);
	keyRisoh.QueryDword(TEXT("nIDListLeft"), (DWORD&)g_settings.nIDListLeft);
	keyRisoh.QueryDword(TEXT("nIDListTop"), (DWORD&)g_settings.nIDListTop);
	keyRisoh.QueryDword(TEXT("nIDListWidth"), (DWORD&)g_settings.nIDListWidth);
	keyRisoh.QueryDword(TEXT("nIDListHeight"), (DWORD&)g_settings.nIDListHeight);
	keyRisoh.QueryDword(TEXT("nRadLeft"), (DWORD&)g_settings.nRadLeft);
	keyRisoh.QueryDword(TEXT("nRadTop"), (DWORD&)g_settings.nRadTop);
	keyRisoh.QueryDword(TEXT("bAskUpdateResH"), (DWORD&)g_settings.bAskUpdateResH);
	keyRisoh.QueryDword(TEXT("bCompressByUPX"), (DWORD&)g_settings.bCompressByUPX);
	keyRisoh.QueryDword(TEXT("bAddBomToRC"), (DWORD&)g_settings.bAddBomToRC);
	keyRisoh.QueryDword(TEXT("bUseBeginEnd"), (DWORD&)g_settings.bUseBeginEnd);
	keyRisoh.QueryDword(TEXT("bShowFullPath"), (DWORD&)g_settings.bShowFullPath);
	keyRisoh.QueryDword(TEXT("nDfmCodePage"), (DWORD&)g_settings.nDfmCodePage);
	keyRisoh.QueryDword(TEXT("bDfmRawTextComments"), (DWORD&)g_settings.bDfmRawTextComments);
	keyRisoh.QueryDword(TEXT("bDfmNoUnicode"), (DWORD&)g_settings.bDfmNoUnicode);
	keyRisoh.QueryDword(TEXT("bUseMSMSGTABLE"), (DWORD&)g_settings.bUseMSMSGTABLE);
	keyRisoh.QueryDword(TEXT("nEgaX"), (DWORD&)g_settings.nEgaX);
	keyRisoh.QueryDword(TEXT("nEgaY"), (DWORD&)g_settings.nEgaY);
	keyRisoh.QueryDword(TEXT("nEgaWidth"), (DWORD&)g_settings.nEgaWidth);
	keyRisoh.QueryDword(TEXT("nEgaHeight"), (DWORD&)g_settings.nEgaHeight);
	keyRisoh.QueryDword(TEXT("nDefResLangID"), (DWORD&)g_settings.nDefResLangID);

	TCHAR szText[128];
	TCHAR szValueName[128];

	// load the macros
	DWORD dwMacroCount = 0;
	if (keyRisoh.QueryDword(TEXT("dwMacroCount"), dwMacroCount) == ERROR_SUCCESS)
	{
		g_settings.macros.clear();

		for (DWORD i = 0; i < dwMacroCount; ++i)
		{
			MString key, value;

			StringCchPrintf(szValueName, _countof(szValueName), TEXT("MacroName%lu"), i);
			if (keyRisoh.QuerySz(szValueName, szText, _countof(szText)) == ERROR_SUCCESS)
				key = szText;

			StringCchPrintf(szValueName, _countof(szValueName), TEXT("MacroValue%lu"), i);
			if (keyRisoh.QuerySz(szValueName, szText, _countof(szText)) == ERROR_SUCCESS)
				value = szText;

			if (!key.empty())
				g_settings.macros.insert(std::make_pair(key, value));
		}
	}

	// load the includes
	DWORD dwNumIncludes = 0;
	if (keyRisoh.QueryDword(TEXT("dwNumIncludes"), dwNumIncludes) == ERROR_SUCCESS)
	{
		g_settings.includes.clear();

		for (DWORD i = 0; i < dwNumIncludes; ++i)
		{
			MString value;

			StringCchPrintf(szValueName, _countof(szValueName), TEXT("Include%lu"), i);
			if (keyRisoh.QuerySz(szValueName, szText, _countof(szText)) == ERROR_SUCCESS)
				value = szText;

			if (!value.empty())
				g_settings.includes.push_back(value);
		}
	}

	if (keyRisoh.QuerySz(TEXT("strSrcFont"), szText, _countof(szText)) == ERROR_SUCCESS)
		g_settings.strSrcFont = szText;
	keyRisoh.QueryDword(TEXT("nSrcFontSize"), (DWORD&)g_settings.nSrcFontSize);

	if (keyRisoh.QuerySz(TEXT("strBinFont"), szText, _countof(szText)) == ERROR_SUCCESS)
		g_settings.strBinFont = szText;
	keyRisoh.QueryDword(TEXT("nBinFontSize"), (DWORD&)g_settings.nBinFontSize);

	INT xVirtualScreen = GetSystemMetrics(SM_XVIRTUALSCREEN);
	INT yVirtualScreen = GetSystemMetrics(SM_YVIRTUALSCREEN);
	INT cxVirtualScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	INT cyVirtualScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	INT cxMin = 200, cyMin = 180;

	if (g_settings.nWindowLeft < xVirtualScreen)
		g_settings.nWindowLeft = CW_USEDEFAULT;
	if (g_settings.nWindowTop < yVirtualScreen)
		g_settings.nWindowTop = CW_USEDEFAULT;
	if (g_settings.nWindowWidth <= cxMin)
		g_settings.nWindowWidth = 760;
	if (g_settings.nWindowHeight <= cyMin)
		g_settings.nWindowHeight = 480;
	if (g_settings.nWindowLeft + cxMin >= xVirtualScreen + cxVirtualScreen)
		g_settings.nWindowLeft = CW_USEDEFAULT;
	if (g_settings.nWindowTop + cyMin >= yVirtualScreen + cyVirtualScreen)
		g_settings.nWindowTop = CW_USEDEFAULT;
	if (g_settings.nWindowTop == CW_USEDEFAULT)
		g_settings.nWindowLeft = CW_USEDEFAULT;

	if (g_settings.nIDListLeft < xVirtualScreen)
		g_settings.nIDListLeft = CW_USEDEFAULT;
	if (g_settings.nIDListTop < yVirtualScreen)
		g_settings.nIDListTop = CW_USEDEFAULT;
	if (g_settings.nIDListWidth <= cxMin)
		g_settings.nIDListWidth = 366;
	if (g_settings.nIDListHeight <= cyMin)
		g_settings.nIDListHeight = 490;
	if (g_settings.nIDListLeft + cxMin >= xVirtualScreen + cxVirtualScreen)
		g_settings.nIDListLeft = CW_USEDEFAULT;
	if (g_settings.nIDListTop + cyMin >= yVirtualScreen + cyVirtualScreen)
		g_settings.nIDListTop = CW_USEDEFAULT;
	if (g_settings.nIDListTop == CW_USEDEFAULT)
		g_settings.nIDListLeft = CW_USEDEFAULT;

	if (g_settings.nRadLeft < xVirtualScreen)
		g_settings.nRadLeft = CW_USEDEFAULT;
	if (g_settings.nRadTop < yVirtualScreen)
		g_settings.nRadTop = CW_USEDEFAULT;
	if (g_settings.nRadLeft + cxMin >= xVirtualScreen + cxVirtualScreen)
		g_settings.nRadLeft = CW_USEDEFAULT;
	if (g_settings.nRadTop + cyMin >= yVirtualScreen + cyVirtualScreen)
		g_settings.nRadTop = CW_USEDEFAULT;
	if (g_settings.nRadTop == CW_USEDEFAULT)
		g_settings.nRadLeft = CW_USEDEFAULT;

	DWORD i, dwCount;
	TCHAR szFormat[32], szFile[MAX_PATH];

	// load the recently used files
	keyRisoh.QueryDword(TEXT("FileCount"), dwCount);
	if (dwCount > MAX_MRU)
		dwCount = MAX_MRU;
	for (i = 0; i < dwCount; ++i)
	{
		StringCchPrintf(szFormat, _countof(szFormat), TEXT("File%lu"), i);
		if (keyRisoh.QuerySz(szFormat, szFile, _countof(szFile)) == ERROR_SUCCESS)
		{
			if (PathFileExistsW(szFile))
			{
				g_settings.vecRecentlyUsed.push_back(szFile);
			}
		}
	}

	if (keyRisoh.QuerySz(TEXT("strWindResExe"), szText, _countof(szText)) == ERROR_SUCCESS)
	{
		if (PathFileExistsW(szText))
			g_settings.strWindResExe = szText;
	}

	if (keyRisoh.QuerySz(TEXT("strCppExe"), szText, _countof(szText)) == ERROR_SUCCESS)
	{
		if (PathFileExistsW(szText))
			g_settings.strCppExe = szText;
	}

	if (keyRisoh.QuerySz(TEXT("strPrevVersion"), szText, _countof(szText)) == ERROR_SUCCESS)
	{
		g_settings.strPrevVersion = szText;
	}

	// update association if version > 3.8
	if (!g_settings.strPrevVersion.empty() && g_settings.strPrevVersion > L"3.8")
	{
		TCHAR szName[MAX_PATH];
		for (auto& pair : g_settings.assoc_map)
		{
			if (keyRisoh.QuerySz(pair.first.c_str(), szName, _countof(szName)) == ERROR_SUCCESS)
			{
				pair.second = szName;
			}
		}
		UpdatePrefixDB(hwnd);
	}

	keyRisoh.QueryDword(TEXT("bSepFilesByLang"), (DWORD&)g_settings.bSepFilesByLang);
	g_settings.bStoreToResFolder = TRUE;
	keyRisoh.QueryDword(TEXT("bSelectableByMacro"), (DWORD&)g_settings.bSelectableByMacro);

	// load the captions
	DWORD dwNumCaptions = 0;
	keyRisoh.QueryDword(TEXT("dwNumCaptions"), (DWORD&)dwNumCaptions);
	if (dwNumCaptions > s_nMaxCaptions)
		dwNumCaptions = s_nMaxCaptions;

	for (DWORD i = 0; i < dwNumCaptions; ++i)
	{
		StringCchPrintf(szValueName, _countof(szValueName), TEXT("Caption%lu"), i);
		if (keyRisoh.QuerySz(szValueName, szText, _countof(szText)) == ERROR_SUCCESS)
		{
			g_settings.captions.push_back(szText);
		}
	}

	keyRisoh.QueryDword(TEXT("bShowToolBar"), (DWORD&)g_settings.bShowToolBar);

	if (keyRisoh.QuerySz(L"strAtlAxWin", szText, _countof(szText)) == ERROR_SUCCESS)
	{
		g_settings.strAtlAxWin = szText;
	}

	keyRisoh.QueryDword(TEXT("nSaveFilterIndex"), (DWORD&)g_settings.nSaveFilterIndex);
	keyRisoh.QueryDword(TEXT("bWordWrap"), (DWORD&)g_settings.bWordWrap);
	keyRisoh.QueryDword(TEXT("RCFileUTF16"), (DWORD&)g_settings.bRCFileUTF16);

	keyRisoh.QueryDword(TEXT("bBackup"), (DWORD&)g_settings.bBackup);

	if (keyRisoh.QuerySz(L"strBackupSuffix", szText, _countof(szText)) == ERROR_SUCCESS)
	{
		g_settings.strBackupSuffix = szText;
	}

	keyRisoh.QueryDword(TEXT("bRedundantComments"), (DWORD&)g_settings.bRedundantComments);
	keyRisoh.QueryDword(TEXT("bWrapManifest"), (DWORD&)g_settings.bWrapManifest);

	DWORD encoding_count = 0;
	if (keyRisoh.QueryDword(TEXT("encoding_count"), (DWORD&)encoding_count) == ERROR_SUCCESS)
	{
		g_settings.encoding_map.clear();
		for (DWORD i = 0; i < encoding_count; ++i)
		{
			WCHAR szName[32];
			StringCchPrintfW(szName, _countof(szName), L"encoding%lu", i);
			if (keyRisoh.QuerySz(szName, szText, _countof(szText)) == ERROR_SUCCESS)
			{
				if (LPWSTR pch = wcschr(szText, L':'))
				{
					*pch = 0;
					++pch;
					::CharLowerW(pch);
					if (lstrcmpW(pch, L"ansi") == 0 ||
						lstrcmpW(pch, L"wide") == 0 ||
						lstrcmpW(pch, L"utf8") == 0 ||
						lstrcmpW(pch, L"utf8n") == 0 ||
						lstrcmpW(pch, L"sjis") == 0)
					{
						g_settings.encoding_map[szText] = pch;
					}
				}
			}
		}
	}

	std::wstring strFrom;

	if (keyRisoh.QuerySz(TEXT("FontReplaceFrom1"), szText, _countof(szText)) == ERROR_SUCCESS)
	{
		strFrom = szText;
		if (keyRisoh.QuerySz(TEXT("FontReplaceTo1"), szText, _countof(szText)) == ERROR_SUCCESS)
		{
			g_settings.strFontReplaceFrom1 = strFrom;
			g_settings.strFontReplaceTo1 = szText;
		}
	}

	if (keyRisoh.QuerySz(TEXT("FontReplaceFrom2"), szText, _countof(szText)) == ERROR_SUCCESS)
	{
		strFrom = szText;
		if (keyRisoh.QuerySz(TEXT("FontReplaceTo2"), szText, _countof(szText)) == ERROR_SUCCESS)
		{
			g_settings.strFontReplaceFrom2 = strFrom;
			g_settings.strFontReplaceTo2 = szText;
		}
	}

	if (keyRisoh.QuerySz(TEXT("FontReplaceFrom3"), szText, _countof(szText)) == ERROR_SUCCESS)
	{
		strFrom = szText;
		if (keyRisoh.QuerySz(TEXT("FontReplaceTo3"), szText, _countof(szText)) == ERROR_SUCCESS)
		{
			g_settings.strFontReplaceFrom3 = std::move(strFrom);
			g_settings.strFontReplaceTo3 = szText;
		}
	}

	// update the menu
	UpdateMenu();

	return TRUE;
}

// save the settings
BOOL MMainWnd::SaveSettings(HWND hwnd)
{
#ifdef PORTABLE
	#ifdef _WIN64
		MRegKeyPortable keyRisoh(TEXT("RisohEditor64"), NULL);
	#else
		MRegKeyPortable keyRisoh(TEXT("RisohEditor"), NULL);
	#endif
#else
	#ifdef _WIN64
		MRegKey keyRisoh(HKCU, TEXT("Software\\Katayama Hirofumi MZ\\RisohEditor64"), TRUE);
	#else
		MRegKey keyRisoh(HKCU, TEXT("Software\\Katayama Hirofumi MZ\\RisohEditor"), TRUE);
	#endif
	if (!keyRisoh)
		return FALSE;
#endif

	// update pane extent settings
	if (m_splitter2.GetPaneCount() >= 2)
		g_settings.nBmpViewWidth = m_splitter2.GetPaneExtent(1);
	if (m_splitter1.GetPaneCount() >= 2)
		g_settings.nTreeViewWidth = m_splitter1.GetPaneExtent(0);

	keyRisoh.SetDword(TEXT("UILanguage"), g_settings.ui_lang);
	keyRisoh.SetDword(TEXT("HIDE.ID"), g_settings.bHideID);
	keyRisoh.SetDword(TEXT("bUseIDC_STATIC"), g_settings.bUseIDC_STATIC);
	keyRisoh.SetDword(TEXT("ShowStatusBar"), g_settings.bShowStatusBar);
	keyRisoh.SetDword(TEXT("AlwaysControl"), g_settings.bAlwaysControl);
	keyRisoh.SetDword(TEXT("TreeViewWidth"), g_settings.nTreeViewWidth);
	keyRisoh.SetDword(TEXT("BmpViewWidth"), g_settings.nBmpViewWidth);
	keyRisoh.SetDword(TEXT("bGuiByDblClick"), g_settings.bGuiByDblClick);
	keyRisoh.SetDword(TEXT("bResumeWindowPos"), g_settings.bResumeWindowPos);
	keyRisoh.SetDword(TEXT("bAutoLoadNearbyResH"), g_settings.bAutoLoadNearbyResH);
	keyRisoh.SetDword(TEXT("bAutoShowIDList"), g_settings.bAutoShowIDList);
	keyRisoh.SetDword(TEXT("bShowDotsOnDialog"), g_settings.bShowDotsOnDialog);
	keyRisoh.SetDword(TEXT("nComboHeight"), g_settings.nComboHeight);
	keyRisoh.SetDword(TEXT("nWindowLeft"), g_settings.nWindowLeft);
	keyRisoh.SetDword(TEXT("nWindowTop"), g_settings.nWindowTop);
	keyRisoh.SetDword(TEXT("nWindowWidth"), g_settings.nWindowWidth);
	keyRisoh.SetDword(TEXT("nWindowHeight"), g_settings.nWindowHeight);
	keyRisoh.SetDword(TEXT("bMaximized"), g_settings.bMaximized);
	keyRisoh.SetDword(TEXT("nIDListLeft"), g_settings.nIDListLeft);
	keyRisoh.SetDword(TEXT("nIDListTop"), g_settings.nIDListTop);
	keyRisoh.SetDword(TEXT("nIDListWidth"), g_settings.nIDListWidth);
	keyRisoh.SetDword(TEXT("nIDListHeight"), g_settings.nIDListHeight);
	keyRisoh.SetDword(TEXT("nRadLeft"), g_settings.nRadLeft);
	keyRisoh.SetDword(TEXT("nRadTop"), g_settings.nRadTop);
	keyRisoh.SetDword(TEXT("bAskUpdateResH"), g_settings.bAskUpdateResH);
	keyRisoh.SetDword(TEXT("bCompressByUPX"), g_settings.bCompressByUPX);
	keyRisoh.SetDword(TEXT("bAddBomToRC"), g_settings.bAddBomToRC);
	keyRisoh.SetDword(TEXT("bUseBeginEnd"), g_settings.bUseBeginEnd);
	keyRisoh.SetDword(TEXT("bShowFullPath"), g_settings.bShowFullPath);
	keyRisoh.SetDword(TEXT("nDfmCodePage"), g_settings.nDfmCodePage);
	keyRisoh.SetDword(TEXT("bDfmRawTextComments"), g_settings.bDfmRawTextComments);
	keyRisoh.SetDword(TEXT("bDfmNoUnicode"), g_settings.bDfmNoUnicode);
	keyRisoh.SetDword(TEXT("bUseMSMSGTABLE"), g_settings.bUseMSMSGTABLE);
	keyRisoh.SetDword(TEXT("nEgaX"), g_settings.nEgaX);
	keyRisoh.SetDword(TEXT("nEgaY"), g_settings.nEgaY);
	keyRisoh.SetDword(TEXT("nEgaWidth"), g_settings.nEgaWidth);
	keyRisoh.SetDword(TEXT("nEgaHeight"), g_settings.nEgaHeight);
	keyRisoh.SetDword(TEXT("nDefResLangID"), g_settings.nDefResLangID);
	keyRisoh.SetSz(TEXT("strSrcFont"), g_settings.strSrcFont.c_str());
	keyRisoh.SetDword(TEXT("nSrcFontSize"), g_settings.nSrcFontSize);
	keyRisoh.SetSz(TEXT("strBinFont"), g_settings.strBinFont.c_str());
	keyRisoh.SetDword(TEXT("nBinFontSize"), g_settings.nBinFontSize);

	DWORD i, dwCount;

	// save the recently used files
	dwCount = (DWORD)g_settings.vecRecentlyUsed.size();
	if (dwCount > MAX_MRU)
		dwCount = MAX_MRU;
	keyRisoh.SetDword(TEXT("FileCount"), dwCount);
	TCHAR szValueName[128];
	for (i = 0; i < dwCount; ++i)
	{
		StringCchPrintf(szValueName, _countof(szValueName), TEXT("File%lu"), i);
		keyRisoh.SetSz(szValueName, g_settings.vecRecentlyUsed[i].c_str());
	}

	// save the ID association mapping
	for (auto& pair : g_settings.assoc_map)
	{
		keyRisoh.SetSz(pair.first.c_str(), pair.second.c_str());
		//MessageBoxW(NULL, pair.first.c_str(), pair.second.c_str(), 0);
	}

	// save the macros
	DWORD dwMacroCount = DWORD(g_settings.macros.size());
	keyRisoh.SetDword(TEXT("dwMacroCount"), dwMacroCount);
	i = 0;
	for (auto& macro : g_settings.macros)
	{
		StringCchPrintf(szValueName, _countof(szValueName), TEXT("MacroName%lu"), i);
		keyRisoh.SetSz(szValueName, macro.first.c_str());

		StringCchPrintf(szValueName, _countof(szValueName), TEXT("MacroValue%lu"), i);
		keyRisoh.SetSz(szValueName, macro.second.c_str());
		++i;
	}

	// save the includes
	DWORD dwNumIncludes = DWORD(g_settings.includes.size());
	keyRisoh.SetDword(TEXT("dwNumIncludes"), dwNumIncludes);
	i = 0;
	for (auto& strInclude : g_settings.includes)
	{
		StringCchPrintf(szValueName, _countof(szValueName), TEXT("Include%lu"), i);
		keyRisoh.SetSz(szValueName, strInclude.c_str());
		++i;
	}

	keyRisoh.SetSz(TEXT("strWindResExe"), g_settings.strWindResExe.c_str());
	keyRisoh.SetSz(TEXT("strCppExe"), g_settings.strCppExe.c_str());

	// always use old style
	keyRisoh.SetDword(TEXT("bOldStyle"), TRUE);

	auto version = GetRisohEditorVersion();
	if (version.size())
		keyRisoh.SetSz(TEXT("strPrevVersion"), version.c_str());

	keyRisoh.SetDword(TEXT("bSepFilesByLang"), g_settings.bSepFilesByLang);
	keyRisoh.SetDword(TEXT("bStoreToResFolder"), TRUE);
	keyRisoh.SetDword(TEXT("bSelectableByMacro"), g_settings.bSelectableByMacro);

	// save the captions
	DWORD dwNumCaptions = DWORD(g_settings.captions.size());
	if (dwNumCaptions > s_nMaxCaptions)
		dwNumCaptions = s_nMaxCaptions;
	keyRisoh.SetDword(TEXT("dwNumCaptions"), dwNumCaptions);
	for (DWORD i = 0; i < dwNumCaptions; ++i)
	{
		StringCchPrintf(szValueName, _countof(szValueName), TEXT("Caption%lu"), i);
		keyRisoh.SetSz(szValueName, g_settings.captions[i].c_str());
	}

	keyRisoh.SetDword(TEXT("bShowToolBar"), g_settings.bShowToolBar);
	keyRisoh.SetSz(L"strAtlAxWin", g_settings.strAtlAxWin.c_str());
	keyRisoh.SetDword(TEXT("nSaveFilterIndex"), g_settings.nSaveFilterIndex);
	keyRisoh.SetDword(TEXT("bWordWrap"), g_settings.bWordWrap);
	keyRisoh.SetDword(TEXT("RCFileUTF16"), g_settings.bRCFileUTF16);

	keyRisoh.SetDword(TEXT("bBackup"), g_settings.bBackup);

	keyRisoh.SetSz(TEXT("strBackupSuffix"), GetRisohEditorVersion().c_str());
	keyRisoh.SetSz(L"strBackupSuffix", g_settings.strBackupSuffix.c_str());

	keyRisoh.SetDword(TEXT("bRedundantComments"), g_settings.bRedundantComments);
	keyRisoh.SetDword(TEXT("bWrapManifest"), g_settings.bWrapManifest);

	DWORD encoding_count = DWORD(g_settings.encoding_map.size());
	keyRisoh.SetDword(TEXT("encoding_count"), encoding_count);
	{
		DWORD i = 0;
		for (auto pair : g_settings.encoding_map)
		{
			WCHAR szName[32];
			StringCchPrintfW(szName, _countof(szName), L"encoding%lu", i);

			WCHAR szText[64];
			StringCchPrintfW(szText, _countof(szText), L"%s:%s", pair.first.c_str(), pair.second.c_str());

			keyRisoh.SetSz(szName, szText);

			++i;
		}
	}

	keyRisoh.SetSz(TEXT("FontReplaceFrom1"), g_settings.strFontReplaceFrom1.c_str());
	keyRisoh.SetSz(TEXT("FontReplaceTo1"), g_settings.strFontReplaceTo1.c_str());

	keyRisoh.SetSz(TEXT("FontReplaceFrom2"), g_settings.strFontReplaceFrom2.c_str());
	keyRisoh.SetSz(TEXT("FontReplaceTo2"), g_settings.strFontReplaceTo2.c_str());

	keyRisoh.SetSz(TEXT("FontReplaceFrom3"), g_settings.strFontReplaceFrom3.c_str());
	keyRisoh.SetSz(TEXT("FontReplaceTo3"), g_settings.strFontReplaceTo3.c_str());

	return TRUE;
}

LANGID GetUILang(void)
{
	DWORD langid = ::GetThreadUILanguage();

#ifdef PORTABLE
	#ifdef _WIN64
		MRegKeyPortable keyRisoh(TEXT("RisohEditor64"), NULL);
	#else
		MRegKeyPortable keyRisoh(TEXT("RisohEditor"), NULL);
	#endif
#else
	#ifdef _WIN64
		MRegKey keyRisoh(HKCU, TEXT("Software\\Katayama Hirofumi MZ\\RisohEditor64"));
	#else
		MRegKey keyRisoh(HKCU, TEXT("Software\\Katayama Hirofumi MZ\\RisohEditor"));
	#endif
	if (!keyRisoh)
		return LANGID(langid);
#endif
	keyRisoh.QueryDword(TEXT("UILanguage"), (DWORD&)langid);
	if (langid == 0)
		langid = GetThreadUILanguage();
	return LANGID(langid);
}
