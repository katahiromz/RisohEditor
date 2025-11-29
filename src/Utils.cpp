// Utils.cpp --- RisohEditor
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "RisohEditor.hpp"
#include "MString.hpp"
#include "MIdOrString.hpp"
#include "ConstantsDB.hpp"
#include "Utils.h"
#include "Common.hpp"
#include "MString.hpp"
#include "resource.h"

extern INT g_bNoGuiMode; // No-GUI mode
extern LPWSTR g_pszLogFile;

INT LogMessageBoxW(HWND hwnd, LPCWSTR text, LPCWSTR title, UINT uType)
{
	if (g_bNoGuiMode)
	{
		if (g_pszLogFile)
		{
			if (FILE *fp = _wfopen(g_pszLogFile, L"a"))
			{
				fprintf(fp, "%ls\n", title);
				fclose(fp);
			}
		}
		return IDYES;
	}
	else
	{
		return ::MessageBoxW(hwnd, text, title, uType);
	}
}

// replace some fullwidth characters with halfwidth characters
void ReplaceFullWithHalf(LPWSTR pszText)
{
	MStringW strFullWidth = LoadStringDx(IDS_FULLWIDTH);
	MStringW strHalfWidth = LoadStringDx(IDS_HALFWIDTH);

	for (DWORD i = 0; pszText[i]; ++i)
	{
		size_t k = strFullWidth.find(pszText[i]);
		if (k != MStringW::npos)
		{
			pszText[i] = strHalfWidth[k];
		}
	}
}

// replace some fullwidth characters with halfwidth characters
void ReplaceFullWithHalf(MStringW& strText)
{
	ReplaceFullWithHalf(&strText[0]);
}

BOOL IsFileWritable(LPCWSTR pszFileName)
{
	HANDLE hFile = CreateFileW(pszFileName, GENERIC_WRITE, 0, NULL,
							   OPEN_EXISTING, 0, NULL);
	CloseHandle(hFile);
	return hFile != INVALID_HANDLE_VALUE;
}

// Wait before file operation for virus checker
BOOL WaitForVirusScan(LPCWSTR pszFileName, DWORD dwTimeout)
{
	::Sleep(300);

	const INT cRetry = 10;
	for (INT i = 0; i < cRetry; ++i)
	{
		if (IsFileWritable(pszFileName))
		{
			::Sleep(300);
			return TRUE;
		}

		::Sleep(dwTimeout / cRetry);
	}

	return FALSE;
}

bool create_directories_recursive_win32(const std::wstring& path) {
	DWORD attr = GetFileAttributesW(path.c_str());
	if (attr != INVALID_FILE_ATTRIBUTES) {
		if (attr & FILE_ATTRIBUTE_DIRECTORY) {
			return true;
		}
		SetLastError(ERROR_FILE_EXISTS);
		return false;
	}

	std::wstring parent_path = path;

	if (parent_path.back() == L'\\' || parent_path.back() == L'/') {
		parent_path.pop_back();
	}

	size_t last_separator = parent_path.find_last_of(L"\\/");
	if (last_separator != std::wstring::npos) {
		std::wstring parent_dir = parent_path.substr(0, last_separator);
		if (!create_directories_recursive_win32(parent_dir)) {
			return false;
		}
	}

	if (!CreateDirectoryW(path.c_str(), NULL)) {
		DWORD error = GetLastError();
		return (error == ERROR_ALREADY_EXISTS);
	}

	return true;
}

// dump data as a text
MStringW DumpBinaryAsText(const std::vector<BYTE>& data)
{
	MStringW ret;
	WCHAR sz[64];
	DWORD addr, size = DWORD(data.size());

	// is it empty?
	if (data.empty())
	{
		return ret;
	}

	ret.reserve(data.size() * 3);   // for speed

	// add the head
	ret +=
		L"+ADDRESS  +0 +1 +2 +3 +4 +5 +6 +7  +8 +9 +A +B +C +D +E +F  0123456789ABCDEF\r\n"
		L"--------  -----------------------  -----------------------  ----------------\r\n";

	// for all the addresses
	bool ending_flag = false;
	for (addr = 0; !ending_flag; ++addr)
	{
		if ((addr & 0xF) != 0)
			continue;

		// add the address
		StringCchPrintfW(sz, _countof(sz), L"%08lX  ", addr);
		ret += sz;

		ending_flag = false;

		// add the data
		for (DWORD i = 0; i < 16; ++i)
		{
			// add a space if the lowest digit was 8
			if (i == 8)
				ret += L' ';

			// add 3 characters
			DWORD offset = addr + i;    // the address to output
			if (offset < size)
			{
				StringCchPrintfW(sz, _countof(sz), L"%02X ", data[offset]);
				ret += sz;
			}
			else
			{
				ret += L"   ";
				ending_flag = true;
			}
		}

		// add the separation space
		ret += L' ';

		// add the characters
		for (DWORD i = 0; i < 16; ++i)
		{
			DWORD offset = addr + i;    // the address to output
			if (offset < size)
			{
				if (data[offset] == 0)
					ret += L' ';        // the NUL character
				else if (data[offset] < 0x20 || data[offset] > 0x7F)
					ret += L'.';        // invisible character
				else
					ret += WCHAR(data[offset]);     // otherwise
			}
			else
			{
				ret += L' ';            // out of range
				ending_flag = true;
			}
		}

		// add a newline
		ret += L"\r\n";
	}

	return ret;     // the result
}

// dump a file
BOOL DumpBinaryFileDx(const WCHAR *filename, LPCVOID pv, DWORD size)
{
	using namespace std;

	FILE *fp = _wfopen(filename, L"wb");        // open
	if (!fp)
		return FALSE;

	int n = (int)fwrite(pv, size, 1, fp);   // write
	fclose(fp);     // close the files

	return n == 1;  // success or not
}

WORD GetMachineOfBinary(LPCWSTR pszExeFile)
{
	WORD wMachine = IMAGE_FILE_MACHINE_UNKNOWN;
	if (FILE *fp = _wfopen(pszExeFile, L"rb"))
	{
		SIZE_T ib = 0;
		IMAGE_DOS_HEADER dos = { 0 };
		fread(&dos, sizeof(dos), 1, fp);
		if (dos.e_magic == IMAGE_DOS_SIGNATURE)
		{
			ib = dos.e_lfanew;
		}
#ifdef _WIN64
		_fseeki64(fp, ib, SEEK_SET);
#else
		fseek(fp, ib, SEEK_SET);
#endif
		IMAGE_NT_HEADERS nt = { 0 };
		fread(&nt, sizeof(nt), 1, fp);
		if (nt.Signature == IMAGE_NT_SIGNATURE)
		{
			auto& file = nt.FileHeader;
			wMachine = file.Machine;
		}
		fclose(fp);
	}
	return wMachine;
}

BOOL IsFileLockedDx(LPCTSTR pszFileName)
{
	if (!PathFileExistsW(pszFileName))
		return FALSE;

	HANDLE hFile;
	hFile = CreateFileW(pszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
						OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		return FALSE;
	}

	return TRUE;
}

// "." or ".."
#define IS_DOTS(psz) ((psz)[0] == '.' && ((psz)[1] == '\0' || ((psz)[1] == '.' && (psz)[2] == '\0')))

// delete a directory (a folder)
BOOL DeleteDirectoryDx(LPCTSTR pszDir)
{
	TCHAR szDirOld[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA find;

	GetCurrentDirectory(MAX_PATH, szDirOld);
	if (!SetCurrentDirectory(pszDir))
		return FALSE;

	hFind = FindFirstFile(TEXT("*"), &find);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!IS_DOTS(find.cFileName))
			{
				if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					DeleteDirectoryDx(find.cFileName);
				}
				else
				{
					SetFileAttributes(find.cFileName, FILE_ATTRIBUTE_NORMAL);
					DeleteFile(find.cFileName);
				}
			}
		} while(FindNextFile(hFind, &find));
		FindClose(hFind);
	}
	SetCurrentDirectory(szDirOld);

	SetFileAttributes(pszDir, FILE_ATTRIBUTE_DIRECTORY);
	return RemoveDirectory(pszDir);
}

// is the path an empty directory?
BOOL IsEmptyDirectoryDx(LPCTSTR pszPath)
{
	WCHAR sz[MAX_PATH];
	StringCchCopy(sz, _countof(sz), pszPath);
	StringCchCat(sz, _countof(sz), L"\\*");

	BOOL bFound = FALSE;
	WIN32_FIND_DATA find;
	HANDLE hFind = FindFirstFile(sz, &find);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			MString str = find.cFileName;
			if (str != L"." && str != L"..")
			{
				bFound = TRUE;
				break;
			}
		} while (FindNextFile(hFind, &find));

		FindClose(hFind);
	}

	return !bFound;
}

// get the path of a shortcut file
BOOL GetPathOfShortcutDx(HWND hwnd, LPCWSTR pszLnkFile, LPWSTR pszPath)
{
	BOOL                bRes = FALSE;
	WIN32_FIND_DATAW    find;
	IShellLinkW*        pShellLink;
	IPersistFile*       pPersistFile;
	HRESULT             hRes;

	// NOTE: CoInitialize/CoInitializeEx call required before this
	pszPath[0] = 0;
	hRes = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
							IID_IShellLinkW, (void **)&pShellLink);
	if (SUCCEEDED(hRes))
	{
		hRes = pShellLink->QueryInterface(IID_IPersistFile,
										  (void **)&pPersistFile);
		if (SUCCEEDED(hRes))
		{
			hRes = pPersistFile->Load(pszLnkFile, STGM_READ);
			if (SUCCEEDED(hRes))
			{
				pShellLink->Resolve(hwnd, SLR_NO_UI | SLR_UPDATE);

				hRes = pShellLink->GetPath(pszPath, MAX_PATH, &find, 0);
				if (SUCCEEDED(hRes) && 0 != pszPath[0])
				{
					bRes = TRUE;
				}
			}
			pPersistFile->Release();
		}
		pShellLink->Release();
	}
	return bRes;
}

HRESULT FileSystemAutoComplete(HWND hwnd)
{
	WCHAR szClass[64];
	if (!GetClassNameW(hwnd, szClass, _countof(szClass)))
		return E_FAIL;

	if (lstrcmpiW(szClass, L"COMBOBOX") == 0 ||
		lstrcmpiW(szClass, WC_COMBOBOXEX) == 0)
	{
		COMBOBOXINFO info = { sizeof(info) };
		if (GetComboBoxInfo(hwnd, &info))
			hwnd = info.hwndItem;
	}

	return SHAutoComplete(hwnd, SHACF_AUTOSUGGEST_FORCE_ON | SHACF_FILESYSTEM |
								SHACF_AUTOAPPEND_FORCE_ON | SHACF_FILESYS_ONLY);
}

void MyChangeNotify(LPCWSTR pszFileName)
{
	// Notify change of file icon
	SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW, pszFileName, NULL);
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

DWORD GetDefaultResLanguage(VOID)
{
	if (g_settings.nDefResLangID == BAD_LANG)
		return ::GetThreadUILanguage();
	return g_settings.nDefResLangID;
}

//////////////////////////////////////////////////////////////////////////////
// window styles

// store the window style info to a vector
void GetStyleSelect(HWND hLst, std::vector<BYTE>& sel)
{
	for (size_t i = 0; i < sel.size(); ++i)
	{
		sel[i] = (ListBox_GetSel(hLst, (DWORD)i) > 0);
	}
}

// store the window style info to a vector
void GetStyleSelect(std::vector<BYTE>& sel, const ConstantsDB::TableType& table, DWORD dwValue)
{
	sel.resize(table.size());
	for (size_t i = 0; i < table.size(); ++i)
	{
		if (table[i].name.find(L'|') != ConstantsDB::StringType::npos)
			continue;

		if ((dwValue & table[i].mask) == table[i].value)
			sel[i] = TRUE;
		else
			sel[i] = FALSE;
	}
}

// analyse the difference of two window styles
DWORD AnalyseStyleDiff(
	DWORD dwValue, ConstantsDB::TableType& table,
	std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel)
{
	assert(old_sel.size() == new_sel.size());
	for (size_t i = 0; i < old_sel.size(); ++i)
	{
		if (table[i].name.find(L'|') != ConstantsDB::StringType::npos)
			continue;

		if (old_sel[i] && !new_sel[i])
		{
			dwValue &= ~table[i].mask;
		}
		else if (!old_sel[i] && new_sel[i])
		{
			dwValue &= ~table[i].mask;
			dwValue |= table[i].value;
		}
	}
	return dwValue;
}

// initialize the style list box
void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table)
{
	// clear all the items of listbox
	ListBox_ResetContent(hLst);

	for (auto& table_entry : table)
	{
		if (table_entry.name.find(L'|') != ConstantsDB::StringType::npos)
			continue;

		ListBox_AddString(hLst, table_entry.name.c_str());
	}

	ListBox_SetHorizontalExtent(hLst, 300);
}

//////////////////////////////////////////////////////////////////////////////
// font names

// the callback for InitFontComboBox
static int CALLBACK
EnumFontFamProc(ENUMLOGFONT *lpelf,
				NEWTEXTMETRIC *lpntm,
				INT FontType,
				LPARAM lParam)
{
	HWND hCmb = HWND(lParam);

	// ignore vertical fonts
	if (lpelf->elfLogFont.lfFaceName[0] != TEXT('@'))
		ComboBox_AddString(hCmb, lpelf->elfLogFont.lfFaceName);

	return TRUE;    // continue
}

// initialize the font combobox
void InitFontComboBox(HWND hCmb)
{
	ComboBox_AddString(hCmb, L"MS Shell Dlg");
	ComboBox_AddString(hCmb, L"MS Shell Dlg 2");

	HDC hDC = CreateCompatibleDC(NULL);
	EnumFontFamilies(hDC, NULL, (FONTENUMPROC)EnumFontFamProc, (LPARAM)hCmb);
	DeleteDC(hDC);
}

// character set information
typedef struct CharSetData
{
	BYTE CharSet;
	LPCTSTR name;
} CharSetData;

static const CharSetData s_charset_entries[] =
{
	{ ANSI_CHARSET, TEXT("ANSI_CHARSET") },
	{ DEFAULT_CHARSET, TEXT("DEFAULT_CHARSET") },
	{ SYMBOL_CHARSET, TEXT("SYMBOL_CHARSET") },
	{ SHIFTJIS_CHARSET, TEXT("SHIFTJIS_CHARSET") },
	{ HANGEUL_CHARSET, TEXT("HANGEUL_CHARSET") },
	{ HANGUL_CHARSET, TEXT("HANGUL_CHARSET") },
	{ GB2312_CHARSET, TEXT("GB2312_CHARSET") },
	{ CHINESEBIG5_CHARSET, TEXT("CHINESEBIG5_CHARSET") },
	{ OEM_CHARSET, TEXT("OEM_CHARSET") },
	{ JOHAB_CHARSET, TEXT("JOHAB_CHARSET") },
	{ HEBREW_CHARSET, TEXT("HEBREW_CHARSET") },
	{ ARABIC_CHARSET, TEXT("ARABIC_CHARSET") },
	{ GREEK_CHARSET, TEXT("GREEK_CHARSET") },
	{ TURKISH_CHARSET, TEXT("TURKISH_CHARSET") },
	{ VIETNAMESE_CHARSET, TEXT("VIETNAMESE_CHARSET") },
	{ THAI_CHARSET, TEXT("THAI_CHARSET") },
	{ EASTEUROPE_CHARSET, TEXT("EASTEUROPE_CHARSET") },
	{ RUSSIAN_CHARSET, TEXT("RUSSIAN_CHARSET") },
	{ MAC_CHARSET, TEXT("MAC_CHARSET") },
	{ BALTIC_CHARSET, TEXT("BALTIC_CHARSET") },
};

// initialize the charset combobox
void InitCharSetComboBox(HWND hCmb, BYTE CharSet)
{
	// clear all the items of combobox
	ComboBox_ResetContent(hCmb);

	// add entries to combobox
	for (auto& entry : s_charset_entries)
	{
		ComboBox_AddString(hCmb, entry.name);
	}

	// set data (charset values)
	for (UINT i = 0; i < _countof(s_charset_entries); ++i)
	{
		ComboBox_SetItemData(hCmb, i, s_charset_entries[i].CharSet);
	}

	// select DEFAULT_CHARSET
	ComboBox_SetCurSel(hCmb, 1);

	for (UINT i = 0; i < _countof(s_charset_entries); ++i)
	{
		if (s_charset_entries[i].CharSet == CharSet)
		{
			// charset was matched
			ComboBox_SetCurSel(hCmb, i);
			break;
		}
	}
}

// get charset value from the charset combobox
BYTE GetCharSetFromComboBox(HWND hCmb)
{
	// get current selection of combobox
	INT i = ComboBox_GetCurSel(hCmb);
	if (i == CB_ERR)    // not selected
		return DEFAULT_CHARSET;     // return the default value

	if (i < INT(_countof(s_charset_entries)))
		return s_charset_entries[i].CharSet;    // return the charset value

	return DEFAULT_CHARSET;     // return the default value
}

//////////////////////////////////////////////////////////////////////////////
// misc.

// initialize the caption combobox
void InitCaptionComboBox(HWND hCmb, LPCTSTR pszCaption)
{
	// clear all the items of combobox
	ComboBox_ResetContent(hCmb);

	// add captions from settings
	for (auto& cap : g_settings.captions)
	{
		ComboBox_AddString(hCmb, cap.c_str());
	}

	// set the text
	ComboBox_SetText(hCmb, pszCaption);
}

// initialize the control class combobox
void InitClassComboBox(HWND hCmb, LPCTSTR pszClass)
{
	// clear all the items of combobox
	ComboBox_ResetContent(hCmb);

	auto table = g_db.GetTable(TEXT("CONTROL.CLASSES"));

	for (auto& table_entry : table)
	{
		INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
		if (table_entry.name == pszClass)
		{
			ComboBox_SetCurSel(hCmb, i);
		}
	}
}

// initialize the window class combobox
void InitWndClassComboBox(HWND hCmb, LPCTSTR pszWndClass)
{
	// clear all the items of combobox
	ComboBox_ResetContent(hCmb);

	// get the control classes
	auto table = g_db.GetTable(TEXT("CONTROL.CLASSES"));

	for (auto& table_entry : table)
	{
		if (table_entry.value > 2)
			continue;   // not a window class

		// add the window class
		INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
		if (table_entry.name == pszWndClass)
		{
			// matched. select
			ComboBox_SetCurSel(hCmb, i);
		}
	}
}

// initialize the control ID combobox
void InitCtrlIDComboBox(HWND hCmb)
{
	// add the control IDs
	auto table = g_db.GetTable(TEXT("CTRLID"));
	for (auto& table_entry : table)
	{
		ComboBox_AddString(hCmb, table_entry.name.c_str());
	}

	// get the prefix of Control.ID
	table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
	MStringW prefix = table[IDTYPE_CONTROL].name;
	if (prefix.size())
	{
		// get the resource IDs by the prefix
		table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
		for (auto& table_entry : table)
		{
			// add the resource IDs
			ComboBox_AddString(hCmb, table_entry.name.c_str());
		}
	}

	// get the prefix of Command.ID
	table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
	prefix = table[IDTYPE_COMMAND].name;
	if (prefix.size())
	{
		// get the resource IDs by the prefix
		table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
		for (auto& table_entry : table)
		{
			// add the resource IDs
			ComboBox_AddString(hCmb, table_entry.name.c_str());
		}
	}

	// get the prefix of New.Command.ID
	table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
	prefix = table[IDTYPE_NEWCOMMAND].name;
	if (prefix.size())
	{
		// get the resource IDs by the prefix
		table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
		for (auto& table_entry : table)
		{
			// add the resource IDs
			ComboBox_AddString(hCmb, table_entry.name.c_str());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// resources

// switch between a resource ID and an IDTYPE_
void Res_ReplaceResTypeString(MString& str, bool bRevert)
{
	if (bRevert)
	{
		// revert
		if (str == L"Icon.ID")
			str = L"RT_GROUP_ICON";
		else if (str == L"Cursor.ID")
			str = L"RT_GROUP_CURSOR";
		else if (str == L"Accel.ID")
			str = L"RT_ACCELERATOR";
		else if (str == L"AniCursor.ID")
			str = L"RT_ANICURSOR";
		else if (str == L"AniIcon.ID")
			str = L"RT_ANIICON";
		else if (str == L"Dialog.ID")
			str = L"RT_DIALOG";
		else if (str == L"Menu.ID")
			str = L"RT_MENU";
		else if (str == L"Bitmap.ID")
			str = L"RT_BITMAP";
		else if (str == L"RCData.ID")
			str = L"RT_RCDATA";
		else if (str == L"Html.ID")
			str = L"RT_HTML";
	}
	else
	{
		// convert
		if (str == L"RT_GROUP_CURSOR")
			str = L"Cursor.ID";
		else if (str == L"RT_GROUP_ICON")
			str = L"Icon.ID";
		else if (str == L"RT_ACCELERATOR")
			str = L"Accel.ID";
		else if (str == L"RT_ANICURSOR")
			str = L"AniCursor.ID";
		else if (str == L"RT_ANIICON")
			str = L"AniIcon.ID";
		else if (str == L"RT_DIALOG")
			str = L"Dialog.ID";
		else if (str == L"RT_MENU")
			str = L"Menu.ID";
		else if (str == L"RT_BITMAP")
			str = L"Bitmap.ID";
		else if (str == L"RT_RCDATA")
			str = L"RCData.ID";
		else if (str == L"RT_HTML")
			str = L"Html.ID";
	}
}

MIdOrString ResourceTypeFromIDType(INT nIDTYPE_)
{
	MIdOrString type;
	switch (nIDTYPE_)
	{
	case IDTYPE_CURSOR:     type = RT_GROUP_CURSOR; break;
	case IDTYPE_BITMAP:     type = RT_BITMAP; break;
	case IDTYPE_MENU:       type = RT_MENU; break;
	case IDTYPE_DIALOG:     type = RT_DIALOG; break;
	case IDTYPE_ACCEL:      type = RT_ACCELERATOR; break;
	case IDTYPE_ICON:       type = RT_GROUP_ICON; break;
	case IDTYPE_ANICURSOR:  type = RT_ANICURSOR; break;
	case IDTYPE_ANIICON:    type = RT_ANIICON; break;
	case IDTYPE_HTML:       type = RT_HTML; break;
	case IDTYPE_RESOURCE:   type.clear(); break;
	case IDTYPE_RCDATA:     type = RT_RCDATA; break;
	default: break;
	}
	return type;
}

MString GetAssoc(const MString& name)
{
	if (name == L"IDC_STATIC")
		return L"Control.ID";

	MString ret;
	MString prefix = name.substr(0, name.find(L'_') + 1);
	if (prefix.empty())
		return L"";

	MIdOrString type;

	std::vector<INT> indexes = GetPrefixIndexes(prefix);
	for (size_t i = 0; i < indexes.size(); ++i)
	{
		auto nIDTYPE_ = IDTYPE_(indexes[i]);
		if (nIDTYPE_ == IDTYPE_UNKNOWN)
			continue;

		auto type = ResourceTypeFromIDType(nIDTYPE_);

		MIdOrString name_or_id;
		WORD wName = (WORD)g_db.GetValue(L"RESOURCE.ID", name);
		if (wName)
			name_or_id = wName;
		else
			name_or_id.m_str = name;

		EntrySet found;
		g_res.search(found, ET_LANG, type, name_or_id);

		if (found.size() && g_db.IsEntityIDType(nIDTYPE_))
		{
			for (auto e : found)    // enumerate the found entries
			{
				MString res_type;
				if (e->m_type.is_int()) // it's an integral name
				{
					// get resource type name
					res_type = g_db.GetName(L"RESOURCE", e->m_type.m_id);
					if (res_type.empty())   // no name
					{
						res_type = mstr_dec(e->m_type.m_id);    // store numeric
					}

					// convert the resource type
					Res_ReplaceResTypeString(res_type, false);
				}
				else
				{
					res_type = e->m_type.str();
				}

				if (res_type.size())
				{
					// add a resource type tag
					if (ret.find(L"[" + res_type + L"]") == MString::npos)
					{
						ret += L"[";
						ret += res_type;
						ret += L"]";
					}
				}
			}
		}
	}

	if (ret.empty())
	{
		for (size_t i = 0; i < indexes.size(); ++i)
		{
			auto nIDTYPE_ = IDTYPE_(indexes[i]);
			if (nIDTYPE_ == IDTYPE_UNKNOWN)
				continue;

			if (ret.empty())
			{
				ret = g_db.GetName(L"RESOURCE.ID.TYPE", nIDTYPE_);
			}
			else
			{
				ret += TEXT("/");
				ret += g_db.GetName(L"RESOURCE.ID.TYPE", nIDTYPE_);
			}
		}
	}

	// convert tags with slashes
	mstr_replace_all(ret, L"][", L"/");
	mstr_replace_all(ret, L"[", L"");
	mstr_replace_all(ret, L"]", L"");

	return ret;
}

void InitComboBoxPlaceholder(HWND hCmb, UINT nStringID)
{
	HWND hEdit = (HWND)SendMessage(hCmb, CBEM_GETEDITCONTROL, 0, 0);
	if (!hEdit)
	{
		hEdit = FindWindowEx(hCmb, NULL, TEXT("EDIT"), NULL);
		if (!hEdit)
		{
			hCmb = FindWindowEx(hCmb, NULL, TEXT("COMBOBOX"), NULL);
			hEdit = FindWindowEx(hCmb, NULL, TEXT("EDIT"), NULL);
		}
	}

	if (hEdit)
	{
		SendMessage(hEdit, EM_SETCUEBANNER, 0, (LPARAM)LoadStringDx(nStringID));
	}
}

void InitResTypeComboBox(HWND hCmb1, const MIdOrString& type)
{
	InitComboBoxPlaceholder(hCmb1, IDS_INTEGERORIDENTIFIER);

	auto table = g_db.GetTable(L"RESOURCE");
	for (auto& table_entry : table)
	{
		WCHAR sz[MAX_PATH];
		StringCchPrintfW(sz, _countof(sz), L"%s (%lu)",
						 table_entry.name.c_str(), table_entry.value);
		INT k = ComboBox_AddString(hCmb1, sz);
		if (type == WORD(table_entry.value))
		{
			ComboBox_SetCurSel(hCmb1, k);
		}
	}

	table = g_db.GetTable(L"RESOURCE.STRING.TYPE");
	for (auto& table_entry : table)
	{
		INT k = ComboBox_AddString(hCmb1, table_entry.name.c_str());
		if (type == table_entry.name.c_str())
		{
			ComboBox_SetCurSel(hCmb1, k);
		}
	}
}

// initialize the resource name combobox
void InitResNameComboBox(HWND hCmb, const MIdOrString& id, IDTYPE_ nIDTYPE_)
{
	MIdOrString id2 = id;
	if (id2 == BAD_NAME) id2.clear();

	InitComboBoxPlaceholder(hCmb, IDS_INTEGERORIDENTIFIER);

	// set the text of the ID
	SetWindowTextW(hCmb, id2.c_str());

	if (g_settings.bHideID)
		return;     // don't use macro IDs

	INT k = -1;     // not matched yet
	MStringW prefix;
	if (nIDTYPE_ != IDTYPE_UNKNOWN)
	{
		// get the prefix from an IDTYPE_ value
		auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
		prefix = table[nIDTYPE_].name;
		if (prefix.empty())
			return;     // unable to get

		// get the resource IDs by the prefix
		table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
		for (auto& table_entry : table)
		{
			// add a resource ID to combobox
			INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
			if (table_entry.value == id2.m_id)   // matched
			{
				k = i;  // matched index is k
				ComboBox_SetCurSel(hCmb, i);    // select its
				SetWindowTextW(hCmb, table_entry.name.c_str()); // set the text
			}
		}
	}

	if (k == -1 &&
		nIDTYPE_ != IDTYPE_RESOURCE && g_db.IsEntityIDType(nIDTYPE_))
	{
		// not found

		// get the prefix of Resource.ID
		auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
		prefix = table[IDTYPE_RESOURCE].name;

		// get the resource IDs by the prefix
		table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
		for (auto& table_entry : table)
		{
			// add the resource name to combobox
			INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
			if (table_entry.value == id2.m_id)   // matched
			{
				ComboBox_SetCurSel(hCmb, i);    // selected
				SetWindowTextW(hCmb, table_entry.name.c_str());  // set the text
			}
		}
	}
}

// initialize the resource name combobox
void InitResNameComboBox(HWND hCmb, const MIdOrString& id, IDTYPE_ nIDTYPE_1, IDTYPE_ nIDTYPE_2)
{
	MIdOrString id2 = id;
	if (id2 == BAD_NAME) id2.clear();

	InitComboBoxPlaceholder(hCmb, IDS_INTEGERORIDENTIFIER);

	// set the ID text to combobox
	SetWindowTextW(hCmb, id2.c_str());

	if (g_settings.bHideID)
		return;     // don't use the macro IDs

	INT k = -1; // not found yet
	MStringW prefix;
	if (nIDTYPE_1 != IDTYPE_UNKNOWN)
	{
		// get the prefix from nIDTYPE_1
		auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
		prefix = table[nIDTYPE_1].name;
		if (prefix.empty())
			return;

		// get the resource IDs from the prefix
		table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);

		// add the resource IDs
		for (auto& table_entry : table)
		{
			// add an item to combobox
			INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
			if (table_entry.value == id2.m_id)   // matched
			{
				k = i;  // found

				// select it in combobox
				ComboBox_SetCurSel(hCmb, i);

				// set its text to combobox
				SetWindowTextW(hCmb, table_entry.name.c_str());
			}
		}
	}
	if (nIDTYPE_2 != IDTYPE_UNKNOWN)
	{
		// get the prefix from nIDTYPE_2
		auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
		prefix = table[nIDTYPE_2].name;
		if (prefix.empty())
			return;

		// get the resource IDs from the prefix
		table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
		for (auto& table_entry : table)
		{
			// add an item to combobox
			INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
			if (table_entry.value == id2.m_id)   // matched
			{
				k = i;  // found

				// select it in combobox
				ComboBox_SetCurSel(hCmb, i);

				// set its text to combobox
				SetWindowTextW(hCmb, table_entry.name.c_str());
			}
		}
	}

	if (k == -1 &&
		nIDTYPE_1 != IDTYPE_RESOURCE && g_db.IsEntityIDType(nIDTYPE_1))
	{
		// not found

		// get the prefix from IDTYPE_RESOURCE
		auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
		prefix = table[IDTYPE_RESOURCE].name;

		// get the resource IDs from the prefix
		table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);
		for (auto& table_entry : table)
		{
			// add an item to combobox
			INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
			if (table_entry.value == id2.m_id)   // matched
			{
				// select it in combobox
				ComboBox_SetCurSel(hCmb, i);

				// set its text to combobox
				SetWindowTextW(hCmb, table_entry.name.c_str());
			}
		}
	}
}

// check the command ID text
BOOL CheckCommand(MString strCommand)
{
	// trim the string
	mstr_trim(strCommand);

	if (('0' <= strCommand[0] && strCommand[0] <= '9') ||
		strCommand[0] == '-' || strCommand[0] == '+')
	{
		// a numeric command ID
		return TRUE;    // OK
	}

	return g_db.HasResID(strCommand);   // is it resource ID name?
}

void InitConstantComboBox(HWND hCmb)
{
	auto table = g_db.GetWholeTable();

	// add the resource IDs
	for (auto& table_entry : table)
	{
		ComboBox_AddString(hCmb, table_entry.name.c_str());
	}

	for (auto& pair : g_settings.id_map)
	{
		MAnsiToWide wide(CP_ACP, pair.first);

		if (ComboBox_FindStringExact(hCmb, -1, wide.c_str()) != CB_ERR)
			continue;

		ComboBox_AddString(hCmb, wide.c_str());
	}
}

// initialize the resource string ID combobox
void InitStringComboBox(HWND hCmb, const MString& strString)
{
	// set the text to combobox
	SetWindowText(hCmb, strString.c_str());

	if (g_settings.bHideID)
		return;     // don't use macro IDs

	// get the prefix from IDTYPE_STRING
	auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
	MStringW prefix = table[IDTYPE_STRING].name;
	if (prefix.empty())
		return;

	// get the resource IDs from the prefix
	table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);

	// add the resource IDs
	for (auto& table_entry : table)
	{
		// add an item to combobox
		INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
		if (table_entry.name == strString)  // matched
		{
			// select it
			ComboBox_SetCurSel(hCmb, i);
		}
	}

	// get the prefix from IDTYPE_PROMPT
	table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
	prefix = table[IDTYPE_PROMPT].name;

	// get the resource IDs from the prefix
	table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);

	// add the resource IDs
	for (auto& table_entry : table)
	{
		// add an item to combobox
		INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
		if (table_entry.name == strString)  // matched
		{
			// select it
			ComboBox_SetCurSel(hCmb, i);
		}
	}
}

// initialize the resource message ID combobox
void InitMessageComboBox(HWND hCmb, const MString& strString)
{
	// set the text to combobox
	SetWindowText(hCmb, strString.c_str());

	if (g_settings.bHideID)
		return;     // don't use macro IDs

	// get the prefix from IDTYPE_MESSAGE
	auto table = g_db.GetTable(L"RESOURCE.ID.PREFIX");
	MStringW prefix = table[IDTYPE_MESSAGE].name;
	if (prefix.empty())
		return;

	// get the resource IDs from the prefix
	table = g_db.GetTableByPrefix(L"RESOURCE.ID", prefix);

	// add the resource IDs
	for (auto& table_entry : table)
	{
		// add an item to combobox
		INT i = ComboBox_AddString(hCmb, table_entry.name.c_str());
		if (table_entry.name == strString)  // matched
		{
			ComboBox_SetCurSel(hCmb, i);    // select it
		}
	}
}
