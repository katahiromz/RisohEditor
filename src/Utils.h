// Utils.h --- RisohEditor
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once
// Small, focused utility header for common helpers extracted from RisohEditor.cpp

#include <string>
#include <vector>
#include <windows.h>

void ReplaceFullWithHalf(wchar_t* pszText);
void ReplaceFullWithHalf(std::wstring& strText);

BOOL IsFileWritable(LPCWSTR pszFileName);
BOOL WaitForVirusScan(LPCWSTR pszFileName, DWORD dwTimeout = 15000);

bool create_directories_recursive_win32(const std::wstring& path);

std::wstring DumpBinaryAsText(const std::vector<BYTE>& data);
BOOL DumpBinaryFileDx(const WCHAR *filename, LPCVOID pv, DWORD size);

struct AutoDeleteFileW
{
	std::wstring m_file;
	AutoDeleteFileW(const std::wstring& file) : m_file(file) { }
	~AutoDeleteFileW() { ::DeleteFileW(m_file.c_str()); }
};

WORD GetMachineOfBinary(LPCWSTR pszExeFile);
BOOL IsFileLockedDx(LPCTSTR pszFileName);
BOOL DeleteDirectoryDx(LPCTSTR pszDir);
BOOL IsEmptyDirectoryDx(LPCTSTR pszPath);
BOOL GetPathOfShortcutDx(HWND hwnd, LPCWSTR pszLnkFile, LPWSTR pszPath);
INT LogMessageBoxW(HWND hwnd, LPCWSTR text, LPCWSTR title, UINT uType);
HRESULT FileSystemAutoComplete(HWND hwnd);
void MyChangeNotify(LPCWSTR pszFileName);
DWORD GetDefaultResLanguage(VOID);
void GetStyleSelect(HWND hLst, std::vector<BYTE>& sel);
void GetStyleSelect(std::vector<BYTE>& sel, const ConstantsDB::TableType& table, DWORD dwValue);
DWORD AnalyseStyleDiff(
	DWORD dwValue, ConstantsDB::TableType& table,
	std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel);
void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table);
void InitFontComboBox(HWND hCmb);
void InitCharSetComboBox(HWND hCmb, BYTE CharSet);
BYTE GetCharSetFromComboBox(HWND hCmb);
void InitCaptionComboBox(HWND hCmb, LPCTSTR pszCaption);
void InitClassComboBox(HWND hCmb, LPCTSTR pszClass);
void InitWndClassComboBox(HWND hCmb, LPCTSTR pszWndClass);
void InitCtrlIDComboBox(HWND hCmb);
void Res_ReplaceResTypeString(MString& str, bool bRevert = false);
MIdOrString ResourceTypeFromIDType(INT nIDTYPE_);
MString GetAssoc(const MString& name);
void InitComboBoxPlaceholder(HWND hCmb, UINT nStringID);
void InitResTypeComboBox(HWND hCmb1, const MIdOrString& type);	
void InitResNameComboBox(HWND hCmb, const MIdOrString& id, IDTYPE_ nIDTYPE_);
void InitResNameComboBox(HWND hCmb, const MIdOrString& id, IDTYPE_ nIDTYPE_1, IDTYPE_ nIDTYPE_2);
BOOL CheckCommand(MString strCommand);
void InitConstantComboBox(HWND hCmb);
void InitStringComboBox(HWND hCmb, const MString& strString);
void InitMessageComboBox(HWND hCmb, const MString& strString);
BOOL IsValidUILang(LANGID langid);
void InitLangComboBox(HWND hCmb3, DWORD langid, BOOL bUILanguage);
void InitLangComboBox(HWND hCmb3, DWORD langid);
void InitLangListView(HWND hLst1, LPCTSTR pszText);
WORD LangFromText(LPWSTR pszLang);
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang, LANG_TYPE type);
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang);
MStringW TextFromLang(WORD lang);
BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& type);
BOOL CheckNameComboBox(HWND hCmb2, MIdOrString& name);
BOOL Edt1_CheckFile(HWND hEdt1, MStringW& file);
MStringW GetKeyID(UINT wId);
void Cmb1_InitVirtualKeys(HWND hCmb1);
BOOL Cmb1_CheckKey(HWND hwnd, HWND hCmb1, BOOL bVirtKey, MStringW& str);
MString GetLanguageStatement(WORD langid, BOOL bOldStyle);
void DoSetFileModified(BOOL bModified);

extern std::vector<LANG_ENTRY> g_langs;

BOOL CALLBACK
EnumResLangProc(HMODULE hModule, LPCTSTR lpszType, LPCTSTR lpszName, WORD wIDLanguage, LPARAM lParam);
BOOL CALLBACK EnumLocalesProc(LPWSTR lpLocaleString);
BOOL CALLBACK EnumEngLocalesProc(LPWSTR lpLocaleString);
