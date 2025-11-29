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

//////////////////////////////////////////////////////////////////////////////
// languages

std::vector<LANG_ENTRY> g_langs;

BOOL CALLBACK
EnumResLangProc(HMODULE hModule, LPCTSTR lpszType, LPCTSTR lpszName, WORD wIDLanguage,
				LPARAM lParam)
{
	LPDWORD pvalue = (LPDWORD)lParam;
	if (wIDLanguage == *pvalue)
	{
		*pvalue = 0;
		return FALSE;
	}
	return TRUE;
}

BOOL IsValidUILang(LANGID langid)
{
	DWORD value = langid;
	LPARAM lParam = (LPARAM)&value;
	EnumResourceLanguagesW(NULL, RT_MENU, MAKEINTRESOURCEW(IDR_MAINMENU), EnumResLangProc, lParam);
	return value != langid;
}

MLangAutoComplete::MLangAutoComplete(BOOL bUILanguage)
{
	m_nCurrentElement = 0;
	m_nRefCount = 1;
	m_fBound = FALSE;
	m_pAC = NULL;

	for (auto& lang : g_langs)
	{
		if (bUILanguage && !IsValidUILang(lang.LangID))
			continue;

		push_back(lang.str);
	}
}

// initialize the language combobox
void InitLangComboBox(HWND hCmb3, DWORD langid, BOOL bUILanguage)
{
	InitComboBoxPlaceholder(hCmb3, IDS_INTEGERORIDENTIFIER);

	// for all the elements of g_langs
	for (auto& entry : g_langs)
	{
		if (bUILanguage && !IsValidUILang(entry.LangID))
			continue;

		// build the text
		WCHAR sz[MAX_PATH];
		StringCchPrintfW(sz, _countof(sz), L"%s (%u)", entry.str.c_str(), entry.LangID);

		// search the text
		if (ComboBox_FindStringExact(hCmb3, -1, sz) != CB_ERR)
			continue;   // found

		// add the text as a new item to combobox
		INT k = ComboBox_AddString(hCmb3, sz);
		if (langid == entry.LangID)     // matched
		{
			ComboBox_SetCurSel(hCmb3, k);   // select it
		}
	}

	auto table = g_db.GetTable(L"LANGUAGES");
	for (auto& table_entry : table)
	{
		if (bUILanguage && !IsValidUILang(LANGID(table_entry.value)))
			continue;

		// build the text
		WCHAR sz[MAX_PATH];
		StringCchPrintfW(sz, _countof(sz), L"%s (%lu)", table_entry.name.c_str(), table_entry.value);

		// search the text
		if (ComboBox_FindStringExact(hCmb3, -1, sz) != CB_ERR)
			continue;   // found

		// add the text as a new item to combobox
		ComboBox_AddString(hCmb3, sz);
	}

	if (langid == (DWORD)-1)
		ComboBox_SetText(hCmb3, L"");
}

// initialize the language combobox
void InitLangComboBox(HWND hCmb3, DWORD langid)
{
	InitLangComboBox(hCmb3, langid, FALSE);
}

// initialize the language listview
void InitLangListView(HWND hLst1, LPCTSTR pszText)
{
	// delete all the items of listview
	ListView_DeleteAllItems(hLst1);

	WCHAR szText[128];
	if (pszText)
	{
		StringCbCopyW(szText, sizeof(szText), pszText);
		CharUpperW(szText);
	}

	WCHAR sz1[64], sz2[64];
	LV_ITEM item;
	INT iItem = 0;
	for (auto& entry : g_langs)     // for all the items of g_langs
	{
		// build two texts of an entry
		StringCchPrintfW(sz1, _countof(sz1), L"%s", entry.str.c_str());
		StringCchPrintfW(sz2, _countof(sz2), L"%u", entry.LangID);

		if (pszText)
		{
			// filtering by pszText
			MString str = sz1;
			CharUpperW(&str[0]);
			if (str.find(szText) == MString::npos)
			{
				str = sz2;
				CharUpperW(&str[0]);
				if (str.find(szText) == MString::npos)
					continue;
			}
		}

		// if it exists, don't add it
		LV_FINDINFO find = { LVFI_STRING, sz1 };
		INT iFound = ListView_FindItem(hLst1, -1, &find);
		if (iFound != -1)
			continue;

		// add it
		ZeroMemory(&item, sizeof(item));
		item.iItem = iItem;
		item.mask = LVIF_TEXT;
		item.iSubItem = 0;
		item.pszText = sz1;
		ListView_InsertItem(hLst1, &item);

		item.iSubItem = 1;
		item.pszText = sz2;
		ListView_SetItem(hLst1, &item);

		++iItem;    // next item index
	}

	auto table = g_db.GetTable(L"LANGUAGES");
	for (auto& table_entry : table)
	{
		// build two texts of an entry
		StringCchPrintfW(sz1, _countof(sz1), L"%s", table_entry.name.c_str());
		StringCchPrintfW(sz2, _countof(sz2), L"%lu", table_entry.value);

		if (pszText)
		{
			// filtering by pszText
			MString str = sz1;
			if (str.find(pszText) == MString::npos)
			{
				str = sz2;
				if (str.find(pszText) == MString::npos)
					continue;
			}
		}

		// if it exists, don't add it
		LV_FINDINFO find = { LVFI_STRING, sz1 };
		INT iFound = ListView_FindItem(hLst1, -1, &find);
		if (iFound != -1)
			continue;

		// add it
		ZeroMemory(&item, sizeof(item));
		item.iItem = iItem;
		item.mask = LVIF_TEXT;
		item.iSubItem = 0;
		item.pszText = sz1;
		ListView_InsertItem(hLst1, &item);

		item.iSubItem = 1;
		item.pszText = sz2;
		ListView_SetItem(hLst1, &item);

		++iItem;    // next item index
	}
}

// get the language ID from a text
WORD LangFromText(LPWSTR pszLang)
{
	WORD lang = BAD_LANG;     // not found yet

	// replace the fullwidth characters with halfwidth characters
	ReplaceFullWithHalf(pszLang);

	// trim and store to pszLang and strLang
	MStringW strLang = pszLang;
	mstr_trim(strLang);
	StringCchCopyW(pszLang, MAX_PATH, strLang.c_str());

	do
	{
		if (strLang[0] == 0)
			break;  // it's empty. invalid

		// is it American English?
		if (lstrcmpiW(pszLang, L"en") == 0 ||
			lstrcmpiW(pszLang, L"English") == 0 ||
			lstrcmpiW(pszLang, L"America") == 0 ||
			lstrcmpiW(pszLang, L"American") == 0 ||
			lstrcmpiW(pszLang, L"United States") == 0 ||
			lstrcmpiW(pszLang, L"USA") == 0 ||
			lstrcmpiW(pszLang, L"US") == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_AMERICA)) == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_ENGLISH)) == 0)
		{
			// American English
			lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
			break;
		}

		// is it Chinese?
		if (lstrcmpiW(pszLang, L"Chinese") == 0 ||
			lstrcmpiW(pszLang, L"PRC") == 0 ||
			lstrcmpiW(pszLang, L"CHN") == 0 ||
			lstrcmpiW(pszLang, L"CN") == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_CHINA)) == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_CHINESE)) == 0)
		{
			// Chinese
			lang = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
			break;
		}

		// is it Russian?
		if (lstrcmpiW(pszLang, L"Russia") == 0 ||
			lstrcmpiW(pszLang, L"Russian") == 0 ||
			lstrcmpiW(pszLang, L"RUS") == 0 ||
			lstrcmpiW(pszLang, L"RU") == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_RUSSIA)) == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_RUSSIAN)) == 0)
		{
			// Russian
			lang = MAKELANGID(LANG_RUSSIAN, SUBLANG_RUSSIAN_RUSSIA);
			break;
		}

		// is it British?
		if (lstrcmpiW(pszLang, L"United Kingdom") == 0 ||
			lstrcmpiW(pszLang, L"Great Britain") == 0 ||
			lstrcmpiW(pszLang, L"British") == 0 ||
			lstrcmpiW(pszLang, L"GBR") == 0 ||
			lstrcmpiW(pszLang, L"GB") == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_UNITEDKINGDOM)) == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_GREATBRITAIN)) == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_BRITISH)) == 0)
		{
			// Russian
			lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK);
			break;
		}

		// is it French?
		if (lstrcmpiW(pszLang, L"French") == 0 ||
			lstrcmpiW(pszLang, L"France") == 0 ||
			lstrcmpiW(pszLang, L"FRA") == 0 ||
			lstrcmpiW(pszLang, L"FR") == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_FRANCE)) == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_FRENCH)) == 0)
		{
			// French
			lang = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
			break;
		}

		// is it Germany?
		if (lstrcmpiW(pszLang, L"Germany") == 0 ||
			lstrcmpiW(pszLang, L"German") == 0 ||
			lstrcmpiW(pszLang, L"DEU") == 0 ||
			lstrcmpiW(pszLang, L"DE") == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_GERMANY)) == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_GERMAN)) == 0)
		{
			// Germany
			lang = MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN);
			break;
		}

		// is it Spanish?
		if (lstrcmpiW(pszLang, L"Spanish") == 0 ||
			lstrcmpiW(pszLang, L"Spain") == 0 ||
			lstrcmpiW(pszLang, L"ESP") == 0 ||
			lstrcmpiW(pszLang, L"ES") == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_SPANISH)) == 0 ||
			lstrcmpiW(pszLang, LoadStringDx(IDS_SPAIN)) == 0)
		{
			// Spanish
			lang = MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH);
			break;
		}

		// maybe en_US, or jp_JP etc.
		if (INT nValue = g_db.GetValueI(L"LANGUAGES", strLang))
		{
			lang = (WORD)nValue;    // found
			break;
		}

		// maybe en-US, or jp-JP etc.
		{
			MStringW str = strLang;

			// replace '-' with '_'
			auto i = str.find(L'-');
			if (i != MString::npos)
				str[i] = L'_';

			// maybe en_US, or jp_JP etc.
			if (INT nValue = g_db.GetValueI(L"LANGUAGES", str))
			{
				lang = (WORD)nValue;    // found
				break;
			}
		}

		// is it numeric?
		if (mchr_is_digit(strLang[0]))
		{
			// strLang is numeric
			int nValue = mstr_parse_int(pszLang);
			if (nValue < 0 || BAD_LANG <= nValue)
				break;  // invalid

			lang = WORD(nValue);
		}

		if (lang == BAD_LANG)     // not found yet?
		{
			// whole match
			for (auto& entry : g_langs)
			{
				if (lstrcmpiW(entry.str.c_str(), pszLang) == 0) // matched
				{
					lang = entry.LangID;
					break;
				}
			}
		}

		if (lang == BAD_LANG)     // not found yet?
		{
			// numeric after parenthesis
			if (WCHAR *pch = wcsrchr(pszLang, L'('))
			{
				++pch;
				if (mchr_is_digit(*pch))
				{
					int nValue = mstr_parse_int(pch);
					if (nValue < 0 || BAD_LANG <= nValue)
						break;  // invalid

					lang = WORD(nValue);
				}
			}
		}

		if (lang == BAD_LANG)     // not found yet?
		{
			// partial match
			for (auto& entry : g_langs)
			{
				if (wcsstr(entry.str.c_str(), pszLang) != NULL)
				{
					lang = entry.LangID;
					break;
				}
			}
		}

		if (lang == BAD_LANG)     // not found yet?
		{
			// ignore case, partial match
			CharUpperW(&strLang[0]);
			for (auto& entry : g_langs)
			{
				MStringW strEntry = entry.str;
				CharUpperW(&strEntry[0]);

				if (wcsstr(strEntry.c_str(), pszLang) != NULL)
				{
					lang = entry.LangID;
					break;
				}
			}
		}
	} while (0);

	return lang;
}

// verify the language combobox
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang, LANG_TYPE type)
{
	// get the text from combobox
	MStringW strLang = MWindowBase::GetWindowText(hCmb3);

	// get the language ID from texts
	lang = LangFromText(&strLang[0]);
	if ((type != LANG_TYPE_1 || IsValidUILang(lang)) && lang != BAD_LANG)
		return TRUE;    // success

	if (type != LANG_TYPE_2)
	{
		// error
		ComboBox_SetEditSel(hCmb3, 0, -1);
		SetFocus(hCmb3);
		LogMessageBoxW(GetParent(hCmb3), LoadStringDx(IDS_ENTERLANG),
					   NULL, MB_ICONERROR);
	}

	return FALSE;   // failure
}

// verify the language combobox
BOOL CheckLangComboBox(HWND hCmb3, WORD& lang)
{
	return CheckLangComboBox(hCmb3, lang, LANG_TYPE_0);
}

// callback function for MMainWnd::DoLoadLangInfo
BOOL CALLBACK EnumLocalesProc(LPWSTR lpLocaleString)
{
	// get the locale ID from string
	LCID lcid = mstr_parse_int(lpLocaleString, false, 16);

	LANG_ENTRY entry;
	entry.LangID = LANGIDFROMLCID(lcid);    // store the language ID

	// get the localized language and country
	WCHAR sz[128] = L"";
	if (lcid == 0)
		return TRUE;    // continue
	if (!GetLocaleInfoW(lcid, LOCALE_SLANGUAGE, sz, _countof(sz)))
		return TRUE;    // continue

	entry.str = sz;     // store the text

	// add it
	g_langs.push_back(entry);

	return TRUE;    // continue
}

// callback function for MMainWnd::DoLoadLangInfo
BOOL CALLBACK EnumEngLocalesProc(LPWSTR lpLocaleString)
{
	// get the locale ID from string
	LCID lcid = mstr_parse_int(lpLocaleString, false, 16);

	LANG_ENTRY entry;
	entry.LangID = LANGIDFROMLCID(lcid);    // store the language ID

	// get the language and country in English
	WCHAR sz1[128] = L"", sz2[128] = L"";
	if (lcid == 0)
		return TRUE;    // continue
	if (!GetLocaleInfoW(lcid, LOCALE_SENGLANGUAGE, sz1, _countof(sz1)))
		return TRUE;    // continue
	if (!GetLocaleInfoW(lcid, LOCALE_SENGCOUNTRY, sz2, _countof(sz2)))
		return TRUE;    // continue

	// join them and store it
	entry.str = sz1;
	entry.str += L" (";
	entry.str += sz2;
	entry.str += L")";

	// add it
	g_langs.push_back(entry);

	return TRUE;    // continue
}

// get the text from a language ID
MStringW TextFromLang(WORD lang)
{
	WCHAR sz[128], szLoc[128];

	// get the locale ID
	LCID lcid = MAKELCID(lang, SORT_DEFAULT);
	if (lcid == 0)
	{
		// neutral language
		StringCchPrintfW(sz, _countof(sz), L"%s (0)", LoadStringDx(IDS_NEUTRAL));
	}
	else
	{
		if (GetLocaleInfo(lcid, LOCALE_SLANGUAGE, szLoc, _countof(szLoc)))
		{
			// a valid language
			StringCchPrintfW(sz, _countof(sz), L"%s (%u)", szLoc, lang);
		}
		else
		{
			// invalid or unknown language. just store numeric
			StringCchPrintfW(sz, _countof(sz), L"%u", lang);
		}
	}

	return MStringW(sz);
}

//////////////////////////////////////////////////////////////////////////////

// verify the resource type combobox
BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& type)
{
	// get the combobox text
	MStringW str = MWindowBase::GetWindowText(hCmb1);

	// replace the fullwidth characters with halfwidth characters
	ReplaceFullWithHalf(str);

	// trim
	mstr_trim(str);

	if (str.empty())  // an empty string
	{
		ComboBox_SetEditSel(hCmb1, 0, -1);  // select all
		SetFocus(hCmb1);    // set focus
		// show error message
		LogMessageBoxW(GetParent(hCmb1), LoadStringDx(IDS_ENTERTYPE),
					   NULL, MB_ICONERROR);
		return FALSE;   // failure
	}
	else if (mchr_is_digit(str[0]) || str[0] == L'-' || str[0] == L'+')
	{
		// numeric type name
		type = WORD(mstr_parse_int(str.c_str()));
		if (type == (WORD)0)
		{
			ComboBox_SetEditSel(hCmb1, 0, -1);  // select all
			SetFocus(hCmb1);    // set focus
			// show error message
			LogMessageBoxW(GetParent(hCmb1), LoadStringDx(IDS_ENTERNONZEROTYPE),
						   NULL, MB_ICONERROR);
			return FALSE;   // failure
		}
	}
	else
	{
		size_t i = str.rfind(L'('); // ')'
		if (i != MStringW::npos && mchr_is_digit(str[i + 1]))
		{
			// numeric type name after the parenthesis
			type = WORD(mstr_parse_int(&str[i + 1]));
		}
		else
		{
			WORD nRT_ = (WORD)g_db.GetValue(L"RESOURCE", str);
			if (nRT_ != 0)
			{
				type = nRT_;
			}
			else
			{
				// a string type name
				type = str.c_str();
			}
		}
	}

	return TRUE;    // success
}

// verify the resource name combobox
BOOL CheckNameComboBox(HWND hCmb2, MIdOrString& name)
{
	// get the combobox text
	MStringW str = MWindowBase::GetWindowText(hCmb2);

	// replace the fullwidth characters with halfwidth characters
	ReplaceFullWithHalf(str);

	// trim
	mstr_trim(str);

	if (str.empty()) // an empty string
	{
		ComboBox_SetEditSel(hCmb2, 0, -1);  // select all
		SetFocus(hCmb2);    // set focus
		// show error message
		LogMessageBoxW(GetParent(hCmb2), LoadStringDx(IDS_ENTERNAME),
					   NULL, MB_ICONERROR);
		return FALSE;   // failure
	}
	else if (mchr_is_digit(str[0]) || str[0] == L'-' || str[0] == L'+')
	{
		// a numeric name
		name = WORD(mstr_parse_int(str.c_str()));
	}
	else
	{
		// a non-numeric name
		if (g_db.HasResID(str))
			name = (WORD)g_db.GetResIDValue(str);    // a valued name
		else
			name = str.c_str();  // a string name
	}

	return TRUE;    // success
}

// verify the file textbox
BOOL Edt1_CheckFile(HWND hEdt1, MStringW& file)
{
	// get the text from textbox
	MStringW str = MWindowBase::GetWindowText(hEdt1);

	// trim
	mstr_trim(str);

	if (!PathFileExistsW(str.c_str()))    // not exists
		return FALSE;   // failure

	// store
	file = str;

	return TRUE;    // success
}

// get the text from a command ID
MStringW GetKeyID(UINT wId)
{
	if (g_settings.bHideID) // don't use the macro IDs
		return mstr_dec_short((SHORT)wId);  // return the numeric ID string

	// convert the numeric ID value to the named ID name
	return g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, wId);
}

// initialize the virtual key combobox
void Cmb1_InitVirtualKeys(HWND hCmb1)
{
	// clear all the items of combobox
	ComboBox_ResetContent(hCmb1);

	// add items to combobox
	auto table = g_db.GetTable(L"VIRTUALKEYS");
	for (auto& table_entry : table)
	{
		// add an item to combobox
		ComboBox_AddString(hCmb1, table_entry.name.c_str());
	}
}

// verify the virtual key combobox
BOOL Cmb1_CheckKey(HWND hwnd, HWND hCmb1, BOOL bVirtKey, MStringW& str)
{
	BOOL bOK;
	if (bVirtKey)
	{
		// a virtual key
		INT i = ComboBox_FindStringExact(hCmb1, -1, str.c_str());
		if (i == CB_ERR)
		{
			// not a string. is it numeric?
			i = GetDlgItemInt(hwnd, cmb1, &bOK, TRUE);
			if (!bOK)
			{
				return FALSE;   // not a string nor a numeric. invalid
			}
			str = mstr_dec(i);
		}
	}
	else
	{
		// a non-virtual key
		INT i = GetDlgItemInt(hwnd, cmb1, &bOK, TRUE);
		if (bOK)
		{
			// a numeric
			str = mstr_dec(i);
		}
		else
		{
			// not a numeric. is it a string?
			LPCWSTR pch = str.c_str();
			MStringW str2;
			if (!guts_quote(str2, pch) || str2.size() != 1)
			{
				return FALSE;   // invalid
			}
			str = mstr_quote(str2);
		}
	}

	return TRUE;    // success
}
