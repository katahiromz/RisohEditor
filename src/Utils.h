// Utils.h --- RisohEditor
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once
// Small, focused utility header for common helpers extracted from RisohEditor.cpp

#include <string>
#include <vector>
#include <windows.h>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <climits>
#include "ConstantsDB.hpp"
#include "StringRes.hpp"
#include "MessageRes.hpp"

struct AutoDeleteFileW
{
	MStringW m_file;
	AutoDeleteFileW(const std::wstring& file) : m_file(file) { }
	~AutoDeleteFileW() { ::DeleteFileW(m_file.c_str()); }
};

enum LANG_TYPE
{
	LANG_TYPE_0,
	LANG_TYPE_1,
	LANG_TYPE_2
};

// window class libraries
typedef std::unordered_set<HMODULE> wclib_t;
extern wclib_t s_wclib;

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

extern TCHAR g_szMP3TempFile[MAX_PATH];

BOOL CheckCommandComboBox(HWND hCmb, MStringW& str);
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
BOOL InitHelpIDs(void);
BOOL InitFontNames(void);
BOOL ChooseTypeListBoxType(HWND hwnd, const MIdOrString& type);
BOOL ChooseNameListBoxName(HWND hwnd, const MIdOrString& type, const MIdOrString& name);
BOOL ChooseLangListBoxLang(HWND hwnd, LANGID wLangId);
DWORD AnalyseStyleDiff(DWORD dwValue, ConstantsDB::TableType& table, std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel);
MString GetAssoc(const MString& name);
MString GetLanguageStatement(LANGID langid, BOOL bOldStyle);
MStringW GetRisohTemplate(const MIdOrString& type, const MIdOrString& name, LANGID wLang);
std::vector<INT> GetPrefixIndexes(const MString& prefix);
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
void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table);
void InitWndClassComboBox(HWND hCmb, LPCTSTR pszWndClass);
void ReplaceFullWithHalf(LPWSTR pszText);
void ReplaceFullWithHalf(MStringW& strText);
LANGID GetDefaultResLanguage(VOID);
HRESULT FileSystemAutoComplete(HWND hwnd);
BOOL IsThereWndClass(PCWSTR pszName);
void FreeWCLib(void);
BOOL IsFileWritable(LPCWSTR pszFileName);
BOOL WaitForVirusScan(LPCWSTR pszFileName, DWORD dwTimeout = 15000);
bool create_directories_recursive_win32(const std::wstring& path);
MStringW DumpBinaryAsText(const std::vector<BYTE>& data);
BOOL WriteBinaryFileDx(const WCHAR *filename, LPCVOID pv, size_t size);
WORD GetMachineOfBinary(LPCWSTR pszExeFile);
BOOL IsFileLockedDx(LPCTSTR pszFileName);
BOOL DeleteDirectoryDx(LPCTSTR pszDir);
BOOL IsEmptyDirectoryDx(LPCTSTR pszPath);
BOOL GetPathOfShortcutDx(HWND hwnd, LPCWSTR pszLnkFile, LPWSTR pszPath);
INT LogMessageBoxW(HWND hwnd, LPCWSTR text, LPCWSTR title, UINT uType);
void MyChangeNotify(LPCWSTR pszFileName);
void GetStyleSelect(HWND hLst, std::vector<BYTE>& sel);
void GetStyleSelect(std::vector<BYTE>& sel, const ConstantsDB::TableType& table, DWORD dwValue);
BYTE GetCharSetFromComboBox(HWND hCmb);
void Res_ReplaceResTypeString(MString& str, bool bRevert = false);
MIdOrString ResourceTypeFromIDType(INT nIDTYPE_);
void InitStringComboBox(HWND hCmb, const MString& strString);
BOOL IsValidUILang(LANGID langid);
INT ParseType(const MStringW& input, MIdOrString& type);
INT ParseName(const MStringW& input, const MIdOrString& type, MIdOrString& name);
BOOL ParseLang(const MStringW& input, LANGID& lang);
MStringW GetKeyID(UINT wId);
void Cmb1_InitVirtualKeys(HWND hCmb1);
void DoSetFileModified(BOOL bModified);
VOID ToolBar_StoreStrings(HWND hwnd, INT nCount, TBBUTTON *pButtons);
BOOL StrDlg_GetEntry(HWND hwnd, STRING_ENTRY& entry);
void StrDlg_SetEntry(HWND hwnd, STRING_ENTRY& entry);
BOOL MsgDlg_GetEntry(HWND hwnd, MESSAGE_ENTRY& entry);
void MsgDlg_SetEntry(HWND hwnd, MESSAGE_ENTRY& entry);
BOOL PlayMP3(LPCVOID ptr, size_t size);
void StopMP3(void);
BOOL PlayAvi(HWND hwnd, LPCVOID ptr, size_t size);
void StopAvi(void);
BOOL CALLBACK EnumResLangProc(HMODULE hModule, LPCTSTR lpszType, LPCTSTR lpszName, WORD wIDLanguage, LPARAM lParam);
BOOL CALLBACK EnumLocalesProc(LPWSTR lpLocaleString);
BOOL CALLBACK EnumEngLocalesProc(LPWSTR lpLocaleString);
BOOL IsCodePageReallyUsable(UINT cp);
BOOL WritePayloadLoaderRC(PCWSTR resource_h, PCWSTR payload, PCWSTR payload_loader, LANGID langid);
BOOL CreateEmptyFile(PCWSTR filename);
MStringA GetCannotCreateTempFile(VOID);
MStringW GetResTypeEncoding(const MIdOrString& type);
bool IsValidHelpIDText(const WCHAR *str, DWORD *pValue = NULL);
MStringW GetComboBoxText(HWND hwndCombo);
MStringW GetComboBoxLBText(HWND hwndCombo, INT nIndex);
MStringW GetListBoxText(HWND hwndListBox, INT nIndex);
MStringW GetWindowTextW(HWND hwnd);
MStringW GetDlgItemTextW(HWND hwnd, INT nCtrlID);
MStringW GetListViewItemText(HWND hwndListView, INT iItem, INT iSubItem);
INT ErrorBoxDx(UINT nStringID, UINT uType = MB_ICONERROR);
INT MsgBoxDx(LPCTSTR pszString, LPCTSTR pszTitle, UINT uType = MB_ICONINFORMATION);
