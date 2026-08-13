// Common.hpp --- The common code
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later
//////////////////////////////////////////////////////////////////////////////
#pragma once

#undef min
#undef max
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <climits>
#include "ConstantsDB.hpp"

enum LANG_TYPE
{
	LANG_TYPE_0,
	LANG_TYPE_1,
	LANG_TYPE_2
};

// structure for language information
struct LANG_ENTRY
{
	LANGID LangID;    // language ID
	MStringW str;   // string

	// for sorting
	bool operator<(const LANG_ENTRY& ent) const
	{
		return str < ent.str;
	}
};
extern std::vector<LANG_ENTRY> g_langs;

BOOL CheckCommand(MString strCommand);
BOOL CheckLangComboBox(HWND hCmb3, LANGID& lang);
BOOL CheckLangComboBox(HWND hCmb3, LANGID& lang, LANG_TYPE type);
BOOL CheckNameComboBox(HWND hCmb2, const MIdOrString& type, MIdOrString& name);
BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& type);
BOOL Cmb1_CheckKey(HWND hwnd, HWND hCmb1, BOOL bVirtKey, std::wstring& str);
BOOL Edt1_CheckFile(HWND hEdt1, std::wstring& file);
BOOL InitTypeListBox(HWND hwnd);
BOOL InitNameListBox(HWND hwnd);
BOOL InitLangListBox(HWND hwnd);
BOOL InitTypes(void);
BOOL InitNames(const MIdOrString& res_type = (WORD)0);
BOOL InitKeys(void);
BOOL InitCtrlIDs(void);
BOOL InitStringIDs(void);
BOOL ChooseTypeListBoxType(HWND hwnd, const MIdOrString& type);
BOOL ChooseNameListBoxName(HWND hwnd, const MIdOrString& type, const MIdOrString& name);
BOOL ChooseLangListBoxLang(HWND hwnd, LANGID wLangId);
BOOL IsThereWndClass(const WCHAR *pszName);
BYTE GetCharSetFromComboBox(HWND hCmb);
DWORD AnalyseStyleDiff(DWORD dwValue, ConstantsDB::TableType& table, std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel);
MString GetAssoc(const MString& name);
MString GetLanguageStatement(LANGID langid, BOOL bOldStyle);
MStringW GetRisohTemplate(const MIdOrString& type, const MIdOrString& name, LANGID wLang);
std::vector<INT> GetPrefixIndexes(const MString& prefix);
std::wstring GetKeyID(UINT wId);
void Cmb1_InitVirtualKeys(HWND hCmb1);
void GetStyleSelect(HWND hLst, std::vector<BYTE>& sel);
void GetStyleSelect(std::vector<BYTE>& sel, const ConstantsDB::TableType& table, DWORD dwValue);
void InitCaptionComboBox(HWND hCmb, LPCTSTR pszCaption);
void InitCharSetComboBox(HWND hCmb, BYTE CharSet);
void InitClassComboBox(HWND hCmb, LPCTSTR pszClass);
void InitComboBoxPlaceholder(HWND hCmb, UINT nStringID);
void InitConstantComboBox(HWND hCmb);
void InitCtrlIDComboBox(HWND hCmb);
void InitFontComboBox(HWND hCmb);
void InitLangComboBox(HWND hCmb3, LANGID langid);
void InitLangComboBox(HWND hCmb3, LANGID langid, BOOL bUILanguage);
void InitLangListView(HWND hLst1, LPCTSTR pszText);
void InitMessageComboBox(HWND hCmb, const MString& strString);
void InitResNameComboBox(HWND hCmb, const MIdOrString& id, IDTYPE_ nIDTYPE_);
void InitResNameComboBoxDword(HWND hCmb, const DWORD& id, IDTYPE_ nIDTYPE_);
void InitResNameComboBox(HWND hCmb, const MIdOrString& id, INT nIDTYPE_1, INT nIDTYPE_2);
void InitResTypeComboBox(HWND hCmb1, const MIdOrString& type);
void InitStringComboBox(HWND hCmb, const MString& strString);
void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table);
void InitWndClassComboBox(HWND hCmb, LPCTSTR pszWndClass);
void ReplaceFullWithHalf(LPWSTR pszText);
void ReplaceFullWithHalf(MStringW& strText);
LANGID GetDefaultResLanguage(VOID);
HRESULT FileSystemAutoComplete(HWND hwnd);
