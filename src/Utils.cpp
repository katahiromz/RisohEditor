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

// store the toolbar strings
VOID ToolBar_StoreStrings(HWND hwnd, INT nCount, TBBUTTON *pButtons)
{
	for (INT i = 0; i < nCount; ++i)
	{
		if (pButtons[i].idCommand == 0 || (pButtons[i].fsStyle & BTNS_SEP))
			continue;   // ignore separators

		// replace the resource string ID with a toolbar string ID
		INT_PTR id = pButtons[i].iString;
		LPWSTR psz = LoadStringDx(INT(id));
		id = SendMessageW(hwnd, TB_ADDSTRING, 0, (LPARAM)psz);
		pButtons[i].iString = id;
	}
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

// get the LANGUAGE statement
MString GetLanguageStatement(WORD langid, BOOL bOldStyle)
{
	MString strPrim, strSub;

#define SWITCH_SUBLANG() switch (SUBLANGID(langid))

	// try to get the primary language name and the sub-language name
	switch (PRIMARYLANGID(langid))
	{
	case LANG_NEUTRAL: strPrim = TEXT("LANG_NEUTRAL");
		break;
	case LANG_INVARIANT: strPrim = TEXT("LANG_INVARIANT");
		break;
	case LANG_AFRIKAANS: strPrim = TEXT("LANG_AFRIKAANS");
		SWITCH_SUBLANG()
		{
		case SUBLANG_AFRIKAANS_SOUTH_AFRICA: strSub = TEXT("SUBLANG_AFRIKAANS_SOUTH_AFRICA"); break;
		}
		break;
	case LANG_ALBANIAN: strPrim = TEXT("LANG_ALBANIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ALBANIAN_ALBANIA: strSub = TEXT("SUBLANG_ALBANIAN_ALBANIA"); break;
		}
		break;
	case LANG_ALSATIAN: strPrim = TEXT("LANG_ALSATIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ALSATIAN_FRANCE: strSub = TEXT("SUBLANG_ALSATIAN_FRANCE"); break;
		}
		break;
	case LANG_AMHARIC: strPrim = TEXT("LANG_AMHARIC");
		SWITCH_SUBLANG()
		{
		case SUBLANG_AMHARIC_ETHIOPIA: strSub = TEXT("SUBLANG_AMHARIC_ETHIOPIA"); break;
		}
		break;
	case LANG_ARABIC: strPrim = TEXT("LANG_ARABIC");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ARABIC_SAUDI_ARABIA: strSub = TEXT("SUBLANG_ARABIC_SAUDI_ARABIA"); break;
		case SUBLANG_ARABIC_IRAQ: strSub = TEXT("SUBLANG_ARABIC_IRAQ"); break;
		case SUBLANG_ARABIC_EGYPT: strSub = TEXT("SUBLANG_ARABIC_EGYPT"); break;
		case SUBLANG_ARABIC_LIBYA: strSub = TEXT("SUBLANG_ARABIC_LIBYA"); break;
		case SUBLANG_ARABIC_ALGERIA: strSub = TEXT("SUBLANG_ARABIC_ALGERIA"); break;
		case SUBLANG_ARABIC_MOROCCO: strSub = TEXT("SUBLANG_ARABIC_MOROCCO"); break;
		case SUBLANG_ARABIC_TUNISIA: strSub = TEXT("SUBLANG_ARABIC_TUNISIA"); break;
		case SUBLANG_ARABIC_OMAN: strSub = TEXT("SUBLANG_ARABIC_OMAN"); break;
		case SUBLANG_ARABIC_YEMEN: strSub = TEXT("SUBLANG_ARABIC_YEMEN"); break;
		case SUBLANG_ARABIC_SYRIA: strSub = TEXT("SUBLANG_ARABIC_SYRIA"); break;
		case SUBLANG_ARABIC_JORDAN: strSub = TEXT("SUBLANG_ARABIC_JORDAN"); break;
		case SUBLANG_ARABIC_LEBANON: strSub = TEXT("SUBLANG_ARABIC_LEBANON"); break;
		case SUBLANG_ARABIC_KUWAIT: strSub = TEXT("SUBLANG_ARABIC_KUWAIT"); break;
		case SUBLANG_ARABIC_UAE: strSub = TEXT("SUBLANG_ARABIC_UAE"); break;
		case SUBLANG_ARABIC_BAHRAIN: strSub = TEXT("SUBLANG_ARABIC_BAHRAIN"); break;
		case SUBLANG_ARABIC_QATAR: strSub = TEXT("SUBLANG_ARABIC_QATAR"); break;
		}
		break;
	case LANG_ARMENIAN: strPrim = TEXT("LANG_ARMENIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ARMENIAN_ARMENIA: strSub = TEXT("SUBLANG_ARMENIAN_ARMENIA"); break;
		}
		break;
	case LANG_ASSAMESE: strPrim = TEXT("LANG_ASSAMESE");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ASSAMESE_INDIA: strSub = TEXT("SUBLANG_ASSAMESE_INDIA"); break;
		}
		break;
	case LANG_AZERI: strPrim = TEXT("LANG_AZERI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_AZERI_LATIN: strSub = TEXT("SUBLANG_AZERI_LATIN"); break;
		case SUBLANG_AZERI_CYRILLIC: strSub = TEXT("SUBLANG_AZERI_CYRILLIC"); break;
		}
		break;
	//case LANG_AZERBAIJANI: strPrim = TEXT("LANG_AZERBAIJANI"); // same as LANG_AZERI
	//    SWITCH_SUBLANG()
	//    {
	//        case SUBLANG_AZERBAIJANI_AZERBAIJAN_LATIN: strSub = TEXT("SUBLANG_AZERBAIJANI_AZERBAIJAN_LATIN"); break;
	//        case SUBLANG_AZERBAIJANI_AZERBAIJAN_CYRILLIC: strSub = TEXT("SUBLANG_AZERBAIJANI_AZERBAIJAN_CYRILLIC"); break;
	//    }
	//    break;
	//case LANG_BANGLA: strPrim = TEXT("LANG_BANGLA"); // same as LANG_BENGALI
	//    SWITCH_SUBLANG()
	//    {
	//    case SUBLANG_BANGLA_INDIA: strSub = TEXT("SUBLANG_BANGLA_INDIA"); break;
	//    case SUBLANG_BANGLA_BANGLADESH: strSub = TEXT("SUBLANG_BANGLA_BANGLADESH"); break;
	//    }
	//    break;
	case LANG_BASHKIR: strPrim = TEXT("LANG_BASHKIR");
		SWITCH_SUBLANG()
		{
		case SUBLANG_BASHKIR_RUSSIA: strSub = TEXT("SUBLANG_BASHKIR_RUSSIA"); break;
		}
		break;
	case LANG_BASQUE: strPrim = TEXT("LANG_BASQUE");
		SWITCH_SUBLANG()
		{
		case SUBLANG_BASQUE_BASQUE: strSub = TEXT("SUBLANG_BASQUE_BASQUE"); break;
		}
		break;
	case LANG_BELARUSIAN: strPrim = TEXT("LANG_BELARUSIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_BELARUSIAN_BELARUS: strSub = TEXT("SUBLANG_BELARUSIAN_BELARUS"); break;
		}
		break;
	case LANG_BENGALI: strPrim = TEXT("LANG_BENGALI");
		SWITCH_SUBLANG()
		{
			case SUBLANG_BENGALI_INDIA: strSub = TEXT("SUBLANG_BENGALI_INDIA"); break;
			case SUBLANG_BENGALI_BANGLADESH: strSub = TEXT("SUBLANG_BENGALI_BANGLADESH"); break;
		}
		break;
	case LANG_BRETON: strPrim = TEXT("LANG_BRETON");
		SWITCH_SUBLANG()
		{
		case SUBLANG_BRETON_FRANCE: strSub = TEXT("SUBLANG_BRETON_FRANCE"); break;
		}
		break;
	case LANG_BOSNIAN: strPrim = TEXT("LANG_BOSNIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
		case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC: strSub = TEXT("SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC"); break;
		}
		break;
	case LANG_BOSNIAN_NEUTRAL: strPrim = TEXT("LANG_BOSNIAN_NEUTRAL");
		SWITCH_SUBLANG()
		{
		case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
		case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC: strSub = TEXT("SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC"); break;
		}
		break;
	case LANG_BULGARIAN: strPrim = TEXT("LANG_BULGARIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_BULGARIAN_BULGARIA: strSub = TEXT("SUBLANG_BULGARIAN_BULGARIA"); break;
		}
		break;
	case LANG_CATALAN: strPrim = TEXT("LANG_CATALAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_CATALAN_CATALAN: strSub = TEXT("SUBLANG_CATALAN_CATALAN"); break;
		}
		break;
#ifdef ENABLE_NEW_LANGS
	case LANG_CENTRAL_KURDISH: strPrim = TEXT("LANG_CENTRAL_KURDISH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_CENTRAL_KURDISH_IRAQ: strSub = TEXT("SUBLANG_CENTRAL_KURDISH_IRAQ"); break;
		}
		break;
#endif
#ifdef ENABLE_NEW_LANGS
	case LANG_CHEROKEE: strPrim = TEXT("LANG_CHEROKEE");
		SWITCH_SUBLANG()
		{
		case SUBLANG_CHEROKEE_CHEROKEE: strSub = TEXT("SUBLANG_CHEROKEE_CHEROKEE"); break;
		}
		break;
#endif
	case LANG_CHINESE: strPrim = TEXT("LANG_CHINESE");
		SWITCH_SUBLANG()
		{
		case SUBLANG_CHINESE_TRADITIONAL: strSub = TEXT("SUBLANG_CHINESE_TRADITIONAL"); break;
		case SUBLANG_CHINESE_SIMPLIFIED: strSub = TEXT("SUBLANG_CHINESE_SIMPLIFIED"); break;
		case SUBLANG_CHINESE_HONGKONG: strSub = TEXT("SUBLANG_CHINESE_HONGKONG"); break;
		case SUBLANG_CHINESE_SINGAPORE: strSub = TEXT("SUBLANG_CHINESE_SINGAPORE"); break;
		case SUBLANG_CHINESE_MACAU: strSub = TEXT("SUBLANG_CHINESE_MACAU"); break;
		}
		break;
	//case LANG_CHINESE_SIMPLIFIED: strPrim = TEXT("LANG_CHINESE_SIMPLIFIED"); // same as LANG_CHINESE
	//    SWITCH_SUBLANG()
	//    {
	//        case SUBLANG_CHINESE_TRADITIONAL: strSub = TEXT("SUBLANG_CHINESE_TRADITIONAL"); break;
	//        case SUBLANG_CHINESE_SIMPLIFIED: strSub = TEXT("SUBLANG_CHINESE_SIMPLIFIED"); break;
	//        case SUBLANG_CHINESE_HONGKONG: strSub = TEXT("SUBLANG_CHINESE_HONGKONG"); break;
	//        case SUBLANG_CHINESE_SINGAPORE: strSub = TEXT("SUBLANG_CHINESE_SINGAPORE"); break;
	//        case SUBLANG_CHINESE_MACAU: strSub = TEXT("SUBLANG_CHINESE_MACAU"); break;
	//    }
	//    break;
	case LANG_CHINESE_TRADITIONAL: strPrim = TEXT("LANG_CHINESE_TRADITIONAL");
		SWITCH_SUBLANG()
		{
		case SUBLANG_CHINESE_TRADITIONAL: strSub = TEXT("SUBLANG_CHINESE_TRADITIONAL"); break;
		case SUBLANG_CHINESE_SIMPLIFIED: strSub = TEXT("SUBLANG_CHINESE_SIMPLIFIED"); break;
		case SUBLANG_CHINESE_HONGKONG: strSub = TEXT("SUBLANG_CHINESE_HONGKONG"); break;
		case SUBLANG_CHINESE_SINGAPORE: strSub = TEXT("SUBLANG_CHINESE_SINGAPORE"); break;
		case SUBLANG_CHINESE_MACAU: strSub = TEXT("SUBLANG_CHINESE_MACAU"); break;
		}
		break;
	case LANG_CORSICAN: strPrim = TEXT("LANG_CORSICAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_CORSICAN_FRANCE: strSub = TEXT("SUBLANG_CORSICAN_FRANCE"); break;
		}
		break;
	//case LANG_CROATIAN: strPrim = TEXT("LANG_CROATIAN"); // same as LANG_BOSNIAN
	//    SWITCH_SUBLANG()
	//    {
	//        case SUBLANG_CROATIAN_CROATIA: strSub = TEXT("SUBLANG_CROATIAN_CROATIA"); break;
	//        case SUBLANG_CROATIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_CROATIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
	//    }
	//    break;
	case LANG_CZECH: strPrim = TEXT("LANG_CZECH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_CZECH_CZECH_REPUBLIC: strSub = TEXT("SUBLANG_CZECH_CZECH_REPUBLIC"); break;
		}
		break;
	case LANG_DANISH: strPrim = TEXT("LANG_DANISH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_DANISH_DENMARK: strSub = TEXT("SUBLANG_DANISH_DENMARK"); break;
		}
		break;
	case LANG_DARI: strPrim = TEXT("LANG_DARI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_DARI_AFGHANISTAN: strSub = TEXT("SUBLANG_DARI_AFGHANISTAN"); break;
		}
		break;
	case LANG_DIVEHI: strPrim = TEXT("LANG_DIVEHI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_DIVEHI_MALDIVES: strSub = TEXT("SUBLANG_DIVEHI_MALDIVES"); break;
		}
		break;
	case LANG_DUTCH: strPrim = TEXT("LANG_DUTCH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_DUTCH: strSub = TEXT("SUBLANG_DUTCH"); break;
		case SUBLANG_DUTCH_BELGIAN: strSub = TEXT("SUBLANG_DUTCH_BELGIAN"); break;
		}
		break;
	case LANG_ENGLISH: strPrim = TEXT("LANG_ENGLISH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ENGLISH_US: strSub = TEXT("SUBLANG_ENGLISH_US"); break;
		case SUBLANG_ENGLISH_UK: strSub = TEXT("SUBLANG_ENGLISH_UK"); break;
		case SUBLANG_ENGLISH_AUS: strSub = TEXT("SUBLANG_ENGLISH_AUS"); break;
		case SUBLANG_ENGLISH_CAN: strSub = TEXT("SUBLANG_ENGLISH_CAN"); break;
		case SUBLANG_ENGLISH_NZ: strSub = TEXT("SUBLANG_ENGLISH_NZ"); break;
		//case SUBLANG_ENGLISH_IRELAND: strSub = TEXT("SUBLANG_ENGLISH_IRELAND"); break; // same as SUBLANG_ENGLISH_EIRE
		case SUBLANG_ENGLISH_EIRE: strSub = TEXT("SUBLANG_ENGLISH_EIRE"); break;
		case SUBLANG_ENGLISH_SOUTH_AFRICA: strSub = TEXT("SUBLANG_ENGLISH_SOUTH_AFRICA"); break;
		case SUBLANG_ENGLISH_JAMAICA: strSub = TEXT("SUBLANG_ENGLISH_JAMAICA"); break;
		case SUBLANG_ENGLISH_CARIBBEAN: strSub = TEXT("SUBLANG_ENGLISH_CARIBBEAN"); break;
		case SUBLANG_ENGLISH_BELIZE: strSub = TEXT("SUBLANG_ENGLISH_BELIZE"); break;
		case SUBLANG_ENGLISH_TRINIDAD: strSub = TEXT("SUBLANG_ENGLISH_TRINIDAD"); break;
		case SUBLANG_ENGLISH_ZIMBABWE: strSub = TEXT("SUBLANG_ENGLISH_ZIMBABWE"); break;
		case SUBLANG_ENGLISH_PHILIPPINES: strSub = TEXT("SUBLANG_ENGLISH_PHILIPPINES"); break;
		case SUBLANG_ENGLISH_INDIA: strSub = TEXT("SUBLANG_ENGLISH_INDIA"); break;
		case SUBLANG_ENGLISH_MALAYSIA: strSub = TEXT("SUBLANG_ENGLISH_MALAYSIA"); break;
		case SUBLANG_ENGLISH_SINGAPORE: strSub = TEXT("SUBLANG_ENGLISH_SINGAPORE"); break;
		}
		break;
	case LANG_ESTONIAN: strPrim = TEXT("LANG_ESTONIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ESTONIAN_ESTONIA: strSub = TEXT("SUBLANG_ESTONIAN_ESTONIA"); break;
		}
		break;
	case LANG_FAEROESE: strPrim = TEXT("LANG_FAEROESE");
		SWITCH_SUBLANG()
		{
		case SUBLANG_FAEROESE_FAROE_ISLANDS: strSub = TEXT("SUBLANG_FAEROESE_FAROE_ISLANDS"); break;
		}
		break;
	//case LANG_FARSI: strPrim = TEXT("LANG_FARSI"); // same as LANG_PERSIAN
	//    break;
	case LANG_FILIPINO: strPrim = TEXT("LANG_FILIPINO");
		SWITCH_SUBLANG()
		{
		case SUBLANG_FILIPINO_PHILIPPINES: strSub = TEXT("SUBLANG_FILIPINO_PHILIPPINES"); break;
		}
		break;
	case LANG_FINNISH: strPrim = TEXT("LANG_FINNISH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_FINNISH_FINLAND: strSub = TEXT("SUBLANG_FINNISH_FINLAND"); break;
		}
		break;
	case LANG_FRENCH: strPrim = TEXT("LANG_FRENCH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_FRENCH: strSub = TEXT("SUBLANG_FRENCH"); break;
		case SUBLANG_FRENCH_BELGIAN: strSub = TEXT("SUBLANG_FRENCH_BELGIAN"); break;
		case SUBLANG_FRENCH_CANADIAN: strSub = TEXT("SUBLANG_FRENCH_CANADIAN"); break;
		case SUBLANG_FRENCH_SWISS: strSub = TEXT("SUBLANG_FRENCH_SWISS"); break;
		case SUBLANG_FRENCH_LUXEMBOURG: strSub = TEXT("SUBLANG_FRENCH_LUXEMBOURG"); break;
		case SUBLANG_FRENCH_MONACO: strSub = TEXT("SUBLANG_FRENCH_MONACO"); break;
		}
		break;
	case LANG_FRISIAN: strPrim = TEXT("LANG_FRISIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_FRISIAN_NETHERLANDS: strSub = TEXT("SUBLANG_FRISIAN_NETHERLANDS"); break;
		}
		break;
	//case LANG_FULAH: strPrim = TEXT("LANG_FULAH"); // same as LANG_PULAR
	//    SWITCH_SUBLANG()
	//    {
	//    case SUBLANG_FULAH_SENEGAL: strSub = TEXT("SUBLANG_FULAH_SENEGAL"); break;
	//    }
	//    break;
	case LANG_GALICIAN: strPrim = TEXT("LANG_GALICIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_GALICIAN_GALICIAN: strSub = TEXT("SUBLANG_GALICIAN_GALICIAN"); break;
		}
		break;
	case LANG_GEORGIAN: strPrim = TEXT("LANG_GEORGIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_GEORGIAN_GEORGIA: strSub = TEXT("SUBLANG_GEORGIAN_GEORGIA"); break;
		}
		break;
	case LANG_GERMAN: strPrim = TEXT("LANG_GERMAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_GERMAN: strSub = TEXT("SUBLANG_GERMAN"); break;
		case SUBLANG_GERMAN_SWISS: strSub = TEXT("SUBLANG_GERMAN_SWISS"); break;
		case SUBLANG_GERMAN_AUSTRIAN: strSub = TEXT("SUBLANG_GERMAN_AUSTRIAN"); break;
		case SUBLANG_GERMAN_LUXEMBOURG: strSub = TEXT("SUBLANG_GERMAN_LUXEMBOURG"); break;
		case SUBLANG_GERMAN_LIECHTENSTEIN: strSub = TEXT("SUBLANG_GERMAN_LIECHTENSTEIN"); break;
		}
		break;
	case LANG_GREEK: strPrim = TEXT("LANG_GREEK");
		SWITCH_SUBLANG()
		{
		case SUBLANG_GREEK_GREECE: strSub = TEXT("SUBLANG_GREEK_GREECE"); break;
		}
		break;
	case LANG_GREENLANDIC: strPrim = TEXT("LANG_GREENLANDIC");
		SWITCH_SUBLANG()
		{
		case SUBLANG_GREENLANDIC_GREENLAND: strSub = TEXT("SUBLANG_GREENLANDIC_GREENLAND"); break;
		}
		break;
	case LANG_GUJARATI: strPrim = TEXT("LANG_GUJARATI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_GUJARATI_INDIA: strSub = TEXT("SUBLANG_GUJARATI_INDIA"); break;
		}
		break;
	case LANG_HAUSA: strPrim = TEXT("LANG_HAUSA");
		SWITCH_SUBLANG()
		{
		case SUBLANG_HAUSA_NIGERIA_LATIN: strSub = TEXT("SUBLANG_HAUSA_NIGERIA_LATIN"); break;
		}
		break;
	case LANG_HEBREW: strPrim = TEXT("LANG_HEBREW");
		SWITCH_SUBLANG()
		{
		case SUBLANG_HEBREW_ISRAEL: strSub = TEXT("SUBLANG_HEBREW_ISRAEL"); break;
		}
		break;
	case LANG_HINDI: strPrim = TEXT("LANG_HINDI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_HINDI_INDIA: strSub = TEXT("SUBLANG_HINDI_INDIA"); break;
		}
		break;
	case LANG_HUNGARIAN: strPrim = TEXT("LANG_HUNGARIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_HUNGARIAN_HUNGARY: strSub = TEXT("SUBLANG_HUNGARIAN_HUNGARY"); break;
		}
		break;
	case LANG_ICELANDIC: strPrim = TEXT("LANG_ICELANDIC");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ICELANDIC_ICELAND: strSub = TEXT("SUBLANG_ICELANDIC_ICELAND"); break;
		}
		break;
	case LANG_IGBO: strPrim = TEXT("LANG_IGBO");
		SWITCH_SUBLANG()
		{
		case SUBLANG_IGBO_NIGERIA: strSub = TEXT("SUBLANG_IGBO_NIGERIA"); break;
		}
		break;
	case LANG_INDONESIAN: strPrim = TEXT("LANG_INDONESIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_INDONESIAN_INDONESIA: strSub = TEXT("SUBLANG_INDONESIAN_INDONESIA"); break;
		}
		break;
	case LANG_INUKTITUT: strPrim = TEXT("LANG_INUKTITUT");
		SWITCH_SUBLANG()
		{
		case SUBLANG_INUKTITUT_CANADA: strSub = TEXT("SUBLANG_INUKTITUT_CANADA"); break;
		case SUBLANG_INUKTITUT_CANADA_LATIN: strSub = TEXT("SUBLANG_INUKTITUT_CANADA_LATIN"); break;
		}
		break;
	case LANG_IRISH: strPrim = TEXT("LANG_IRISH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_IRISH_IRELAND: strSub = TEXT("SUBLANG_IRISH_IRELAND"); break;
		}
		break;
	case LANG_ITALIAN: strPrim = TEXT("LANG_ITALIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ITALIAN: strSub = TEXT("SUBLANG_ITALIAN"); break;
		case SUBLANG_ITALIAN_SWISS: strSub = TEXT("SUBLANG_ITALIAN_SWISS"); break;
		}
		break;
	case LANG_JAPANESE: strPrim = TEXT("LANG_JAPANESE");
		SWITCH_SUBLANG()
		{
		case SUBLANG_JAPANESE_JAPAN: strSub = TEXT("SUBLANG_JAPANESE_JAPAN"); break;
		}
		break;
	case LANG_KANNADA: strPrim = TEXT("LANG_KANNADA");
		SWITCH_SUBLANG()
		{
		case SUBLANG_KANNADA_INDIA: strSub = TEXT("SUBLANG_KANNADA_INDIA"); break;
		}
		break;
	case LANG_KASHMIRI: strPrim = TEXT("LANG_KASHMIRI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_KASHMIRI_INDIA: strSub = TEXT("SUBLANG_KASHMIRI_INDIA"); break;
		//case SUBLANG_KASHMIRI_SASIA: strSub = TEXT("SUBLANG_KASHMIRI_SASIA"); break; // same as SUBLANG_KASHMIRI_INDIA
		}
		break;
	case LANG_KAZAK: strPrim = TEXT("LANG_KAZAK");
		SWITCH_SUBLANG()
		{
		case SUBLANG_KAZAK_KAZAKHSTAN: strSub = TEXT("SUBLANG_KAZAK_KAZAKHSTAN"); break;
		}
		break;
	case LANG_KHMER: strPrim = TEXT("LANG_KHMER");
		SWITCH_SUBLANG()
		{
		case SUBLANG_KHMER_CAMBODIA: strSub = TEXT("SUBLANG_KHMER_CAMBODIA"); break;
		}
		break;
	case LANG_KICHE: strPrim = TEXT("LANG_KICHE");
		SWITCH_SUBLANG()
		{
		case SUBLANG_KICHE_GUATEMALA: strSub = TEXT("SUBLANG_KICHE_GUATEMALA"); break;
		}
		break;
	case LANG_KINYARWANDA: strPrim = TEXT("LANG_KINYARWANDA");
		SWITCH_SUBLANG()
		{
		case SUBLANG_KINYARWANDA_RWANDA: strSub = TEXT("SUBLANG_KINYARWANDA_RWANDA"); break;
		}
		break;
	case LANG_KONKANI: strPrim = TEXT("LANG_KONKANI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_KONKANI_INDIA: strSub = TEXT("SUBLANG_KONKANI_INDIA"); break;
		}
		break;
	case LANG_KOREAN: strPrim = TEXT("LANG_KOREAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_KOREAN: strSub = TEXT("SUBLANG_KOREAN"); break;
		}
		break;
	case LANG_KYRGYZ: strPrim = TEXT("LANG_KYRGYZ");
		SWITCH_SUBLANG()
		{
		case SUBLANG_KYRGYZ_KYRGYZSTAN: strSub = TEXT("SUBLANG_KYRGYZ_KYRGYZSTAN"); break;
		}
		break;
	case LANG_LAO: strPrim = TEXT("LANG_LAO");
		SWITCH_SUBLANG()
		{
		case SUBLANG_LAO_LAO: strSub = TEXT("SUBLANG_LAO_LAO"); break;
		}
		break;
	case LANG_LATVIAN: strPrim = TEXT("LANG_LATVIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_LATVIAN_LATVIA: strSub = TEXT("SUBLANG_LATVIAN_LATVIA"); break;
		}
		break;
	case LANG_LITHUANIAN: strPrim = TEXT("LANG_LITHUANIAN");
#if (WINVER >= 0x0600) && defined(ENABLE_NEW_LANGS)
		SWITCH_SUBLANG()
		{
		case SUBLANG_LITHUANIAN_LITHUANIA: strSub = TEXT("SUBLANG_LITHUANIAN_LITHUANIA"); break;
		}
#endif
		break;
	//case LANG_LOWER_SORBIAN: strPrim = TEXT("LANG_LOWER_SORBIAN"); // same as LANG_UPPER_SORBIAN
	//    SWITCH_SUBLANG()
	//    {
	//    case SUBLANG_LOWER_SORBIAN_GERMANY: strSub = TEXT("SUBLANG_LOWER_SORBIAN_GERMANY"); break;
	//    }
	//    break;
	case LANG_LUXEMBOURGISH: strPrim = TEXT("LANG_LUXEMBOURGISH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_LUXEMBOURGISH_LUXEMBOURG: strSub = TEXT("SUBLANG_LUXEMBOURGISH_LUXEMBOURG"); break;
		}
		break;
	case LANG_MACEDONIAN: strPrim = TEXT("LANG_MACEDONIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_MACEDONIAN_MACEDONIA: strSub = TEXT("SUBLANG_MACEDONIAN_MACEDONIA"); break;
		}
		break;
	case LANG_MALAY: strPrim = TEXT("LANG_MALAY");
		SWITCH_SUBLANG()
		{
		case SUBLANG_MALAY_MALAYSIA: strSub = TEXT("SUBLANG_MALAY_MALAYSIA"); break;
		case SUBLANG_MALAY_BRUNEI_DARUSSALAM: strSub = TEXT("SUBLANG_MALAY_BRUNEI_DARUSSALAM"); break;
		}
		break;
	case LANG_MALAYALAM: strPrim = TEXT("LANG_MALAYALAM");
		SWITCH_SUBLANG()
		{
		case SUBLANG_MALAYALAM_INDIA: strSub = TEXT("SUBLANG_MALAYALAM_INDIA"); break;
		}
		break;
	case LANG_MALTESE: strPrim = TEXT("LANG_MALTESE");
		SWITCH_SUBLANG()
		{
		case SUBLANG_MALTESE_MALTA: strSub = TEXT("SUBLANG_MALTESE_MALTA"); break;
		}
		break;
	case LANG_MANIPURI: strPrim = TEXT("LANG_MANIPURI");
		break;
	case LANG_MAORI: strPrim = TEXT("LANG_MAORI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_MAORI_NEW_ZEALAND: strSub = TEXT("SUBLANG_MAORI_NEW_ZEALAND"); break;
		}
		break;
	case LANG_MAPUDUNGUN: strPrim = TEXT("LANG_MAPUDUNGUN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_MAPUDUNGUN_CHILE: strSub = TEXT("SUBLANG_MAPUDUNGUN_CHILE"); break;
		}
		break;
	case LANG_MARATHI: strPrim = TEXT("LANG_MARATHI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_MARATHI_INDIA: strSub = TEXT("SUBLANG_MARATHI_INDIA"); break;
		}
		break;
	case LANG_MOHAWK: strPrim = TEXT("LANG_MOHAWK");
		SWITCH_SUBLANG()
		{
		case SUBLANG_MOHAWK_MOHAWK: strSub = TEXT("SUBLANG_MOHAWK_MOHAWK"); break;
		}
		break;
	case LANG_MONGOLIAN: strPrim = TEXT("LANG_MONGOLIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA: strSub = TEXT("SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA"); break;
		case SUBLANG_MONGOLIAN_PRC: strSub = TEXT("SUBLANG_MONGOLIAN_PRC"); break;
		}
		break;
	case LANG_NEPALI: strPrim = TEXT("LANG_NEPALI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_NEPALI_NEPAL: strSub = TEXT("SUBLANG_NEPALI_NEPAL"); break;
		case SUBLANG_NEPALI_INDIA: strSub = TEXT("SUBLANG_NEPALI_INDIA"); break;
		}
		break;
	case LANG_NORWEGIAN: strPrim = TEXT("LANG_NORWEGIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_NORWEGIAN_BOKMAL: strSub = TEXT("SUBLANG_NORWEGIAN_BOKMAL"); break;
		case SUBLANG_NORWEGIAN_NYNORSK: strSub = TEXT("SUBLANG_NORWEGIAN_NYNORSK"); break;
		}
		break;
	case LANG_OCCITAN: strPrim = TEXT("LANG_OCCITAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_OCCITAN_FRANCE: strSub = TEXT("SUBLANG_OCCITAN_FRANCE"); break;
		}
		break;
	//case LANG_ODIA: strPrim = TEXT("LANG_ODIA");  // same as LANG_ORIYA
	//    break;
	case LANG_ORIYA: strPrim = TEXT("LANG_ORIYA");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ORIYA_INDIA: strSub = TEXT("SUBLANG_ORIYA_INDIA"); break;
		}
		break;
	case LANG_PASHTO: strPrim = TEXT("LANG_PASHTO");
		SWITCH_SUBLANG()
		{
		case SUBLANG_PASHTO_AFGHANISTAN: strSub = TEXT("SUBLANG_PASHTO_AFGHANISTAN"); break;
		}
		break;
	case LANG_PERSIAN: strPrim = TEXT("LANG_PERSIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_PERSIAN_IRAN: strSub = TEXT("SUBLANG_PERSIAN_IRAN"); break;
		}
		break;
	case LANG_POLISH: strPrim = TEXT("LANG_POLISH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_POLISH_POLAND: strSub = TEXT("SUBLANG_POLISH_POLAND"); break;
		}
		break;
	case LANG_PORTUGUESE: strPrim = TEXT("LANG_PORTUGUESE");
		SWITCH_SUBLANG()
		{
		case SUBLANG_PORTUGUESE: strSub = TEXT("SUBLANG_PORTUGUESE"); break;
		case SUBLANG_PORTUGUESE_BRAZILIAN: strSub = TEXT("SUBLANG_PORTUGUESE_BRAZILIAN"); break;
		}
		break;
#ifdef ENABLE_NEW_LANGS
	case LANG_PULAR: strPrim = TEXT("LANG_PULAR"); // same as LANG_FULAH
		SWITCH_SUBLANG()
		{
		case SUBLANG_PULAR_SENEGAL: strSub = TEXT("SUBLANG_PULAR_SENEGAL"); break;
		default: break;
		}
#endif
		break;
	case LANG_PUNJABI: strPrim = TEXT("LANG_PUNJABI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_PUNJABI_INDIA: strSub = TEXT("SUBLANG_PUNJABI_INDIA"); break;
#ifdef ENABLE_NEW_LANGS
		case SUBLANG_PUNJABI_PAKISTAN: strSub = TEXT("SUBLANG_PUNJABI_PAKISTAN"); break;
#endif
		default: break;
		}
		break;
	case LANG_QUECHUA: strPrim = TEXT("LANG_QUECHUA");
		SWITCH_SUBLANG()
		{
		case SUBLANG_QUECHUA_BOLIVIA: strSub = TEXT("SUBLANG_QUECHUA_BOLIVIA"); break;
		case SUBLANG_QUECHUA_ECUADOR: strSub = TEXT("SUBLANG_QUECHUA_ECUADOR"); break;
		case SUBLANG_QUECHUA_PERU: strSub = TEXT("SUBLANG_QUECHUA_PERU"); break;
		}
		break;
	case LANG_ROMANIAN: strPrim = TEXT("LANG_ROMANIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ROMANIAN_ROMANIA: strSub = TEXT("SUBLANG_ROMANIAN_ROMANIA"); break;
		}
		break;
	case LANG_ROMANSH: strPrim = TEXT("LANG_ROMANSH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ROMANSH_SWITZERLAND: strSub = TEXT("SUBLANG_ROMANSH_SWITZERLAND"); break;
		}
		break;
	case LANG_RUSSIAN: strPrim = TEXT("LANG_RUSSIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_RUSSIAN_RUSSIA: strSub = TEXT("SUBLANG_RUSSIAN_RUSSIA"); break;
		}
		break;
#ifdef ENABLE_NEW_LANGS
	case LANG_SAKHA: strPrim = TEXT("LANG_SAKHA");
		SWITCH_SUBLANG()
		{
		case SUBLANG_SAKHA_RUSSIA: strSub = TEXT("SUBLANG_SAKHA_RUSSIA"); break;
		default: break;
		}
		break;
#endif
	case LANG_SAMI: strPrim = TEXT("LANG_SAMI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_SAMI_NORTHERN_NORWAY: strSub = TEXT("SUBLANG_SAMI_NORTHERN_NORWAY"); break;
		case SUBLANG_SAMI_NORTHERN_SWEDEN: strSub = TEXT("SUBLANG_SAMI_NORTHERN_SWEDEN"); break;
		case SUBLANG_SAMI_NORTHERN_FINLAND: strSub = TEXT("SUBLANG_SAMI_NORTHERN_FINLAND"); break;
		case SUBLANG_SAMI_LULE_NORWAY: strSub = TEXT("SUBLANG_SAMI_LULE_NORWAY"); break;
		case SUBLANG_SAMI_LULE_SWEDEN: strSub = TEXT("SUBLANG_SAMI_LULE_SWEDEN"); break;
		case SUBLANG_SAMI_SOUTHERN_NORWAY: strSub = TEXT("SUBLANG_SAMI_SOUTHERN_NORWAY"); break;
		case SUBLANG_SAMI_SOUTHERN_SWEDEN: strSub = TEXT("SUBLANG_SAMI_SOUTHERN_SWEDEN"); break;
		case SUBLANG_SAMI_SKOLT_FINLAND: strSub = TEXT("SUBLANG_SAMI_SKOLT_FINLAND"); break;
		case SUBLANG_SAMI_INARI_FINLAND: strSub = TEXT("SUBLANG_SAMI_INARI_FINLAND"); break;
		}
		break;
	case LANG_SANSKRIT: strPrim = TEXT("LANG_SANSKRIT");
		SWITCH_SUBLANG()
		{
		case SUBLANG_SANSKRIT_INDIA: strSub = TEXT("SUBLANG_SANSKRIT_INDIA"); break;
		}
		break;
#ifdef ENABLE_NEW_LANGS
	case LANG_SCOTTISH_GAELIC: strPrim = TEXT("LANG_SCOTTISH_GAELIC");
		SWITCH_SUBLANG()
		{
		case SUBLANG_SCOTTISH_GAELIC: strSub = TEXT("SUBLANG_SCOTTISH_GAELIC"); break;
		default: break;
		}
		break;
#endif
	//case LANG_SERBIAN: strPrim = TEXT("LANG_SERBIAN"); // same as LANG_BOSNIAN
	//    SWITCH_SUBLANG()
	//    {
	//    case SUBLANG_SERBIAN_LATIN: strSub = TEXT("SUBLANG_SERBIAN_LATIN"); break;
	//    case SUBLANG_SERBIAN_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_CYRILLIC"); break;
	//    case SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
	//    case SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC"); break;
	//    case SUBLANG_SERBIAN_MONTENEGRO_LATIN: strSub = TEXT("SUBLANG_SERBIAN_MONTENEGRO_LATIN"); break;
	//    case SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC"); break;
	//    case SUBLANG_SERBIAN_SERBIA_LATIN: strSub = TEXT("SUBLANG_SERBIAN_SERBIA_LATIN"); break;
	//    case SUBLANG_SERBIAN_SERBIA_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_SERBIA_CYRILLIC"); break;
	//    }
	//    break;
	case LANG_SERBIAN_NEUTRAL: strPrim = TEXT("LANG_SERBIAN_NEUTRAL");
		SWITCH_SUBLANG()
		{
		case SUBLANG_SERBIAN_LATIN: strSub = TEXT("SUBLANG_SERBIAN_LATIN"); break;
		case SUBLANG_SERBIAN_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_CYRILLIC"); break;
		case SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
		case SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC"); break;
#ifdef ENABLE_NEW_LANGS
		case SUBLANG_SERBIAN_MONTENEGRO_LATIN: strSub = TEXT("SUBLANG_SERBIAN_MONTENEGRO_LATIN"); break;
		case SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC"); break;
		case SUBLANG_SERBIAN_SERBIA_LATIN: strSub = TEXT("SUBLANG_SERBIAN_SERBIA_LATIN"); break;
		case SUBLANG_SERBIAN_SERBIA_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_SERBIA_CYRILLIC"); break;
#endif
		}
		break;
	case LANG_SINDHI: strPrim = TEXT("LANG_SINDHI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_SINDHI_INDIA: strSub = TEXT("SUBLANG_SINDHI_INDIA"); break;
		case SUBLANG_SINDHI_AFGHANISTAN: strSub = TEXT("SUBLANG_SINDHI_AFGHANISTAN"); break;
		//case SUBLANG_SINDHI_PAKISTAN: strSub = TEXT("SUBLANG_SINDHI_PAKISTAN"); break; // same as SUBLANG_SINDHI_AFGHANISTAN
		}
		break;
	case LANG_SINHALESE: strPrim = TEXT("LANG_SINHALESE");
		SWITCH_SUBLANG()
		{
		case SUBLANG_SINHALESE_SRI_LANKA: strSub = TEXT("SUBLANG_SINHALESE_SRI_LANKA"); break;
		}
		break;
	case LANG_SLOVAK: strPrim = TEXT("LANG_SLOVAK");
		SWITCH_SUBLANG()
		{
		case SUBLANG_SLOVAK_SLOVAKIA: strSub = TEXT("SUBLANG_SLOVAK_SLOVAKIA"); break;
		}
		break;
	case LANG_SLOVENIAN: strPrim = TEXT("LANG_SLOVENIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_SLOVENIAN_SLOVENIA: strSub = TEXT("SUBLANG_SLOVENIAN_SLOVENIA"); break;
		}
		break;
	case LANG_SOTHO: strPrim = TEXT("LANG_SOTHO");
		SWITCH_SUBLANG()
		{
		case SUBLANG_SOTHO_NORTHERN_SOUTH_AFRICA: strSub = TEXT("SUBLANG_SOTHO_NORTHERN_SOUTH_AFRICA"); break;
		}
		break;
	case LANG_SPANISH: strPrim = TEXT("LANG_SPANISH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_SPANISH: strSub = TEXT("SUBLANG_SPANISH"); break;
		case SUBLANG_SPANISH_MEXICAN: strSub = TEXT("SUBLANG_SPANISH_MEXICAN"); break;
		case SUBLANG_SPANISH_MODERN: strSub = TEXT("SUBLANG_SPANISH_MODERN"); break;
		case SUBLANG_SPANISH_GUATEMALA: strSub = TEXT("SUBLANG_SPANISH_GUATEMALA"); break;
		case SUBLANG_SPANISH_COSTA_RICA: strSub = TEXT("SUBLANG_SPANISH_COSTA_RICA"); break;
		case SUBLANG_SPANISH_PANAMA: strSub = TEXT("SUBLANG_SPANISH_PANAMA"); break;
		case SUBLANG_SPANISH_DOMINICAN_REPUBLIC: strSub = TEXT("SUBLANG_SPANISH_DOMINICAN_REPUBLIC"); break;
		case SUBLANG_SPANISH_VENEZUELA: strSub = TEXT("SUBLANG_SPANISH_VENEZUELA"); break;
		case SUBLANG_SPANISH_COLOMBIA: strSub = TEXT("SUBLANG_SPANISH_COLOMBIA"); break;
		case SUBLANG_SPANISH_PERU: strSub = TEXT("SUBLANG_SPANISH_PERU"); break;
		case SUBLANG_SPANISH_ARGENTINA: strSub = TEXT("SUBLANG_SPANISH_ARGENTINA"); break;
		case SUBLANG_SPANISH_ECUADOR: strSub = TEXT("SUBLANG_SPANISH_ECUADOR"); break;
		case SUBLANG_SPANISH_CHILE: strSub = TEXT("SUBLANG_SPANISH_CHILE"); break;
		case SUBLANG_SPANISH_URUGUAY: strSub = TEXT("SUBLANG_SPANISH_URUGUAY"); break;
		case SUBLANG_SPANISH_PARAGUAY: strSub = TEXT("SUBLANG_SPANISH_PARAGUAY"); break;
		case SUBLANG_SPANISH_BOLIVIA: strSub = TEXT("SUBLANG_SPANISH_BOLIVIA"); break;
		case SUBLANG_SPANISH_EL_SALVADOR: strSub = TEXT("SUBLANG_SPANISH_EL_SALVADOR"); break;
		case SUBLANG_SPANISH_HONDURAS: strSub = TEXT("SUBLANG_SPANISH_HONDURAS"); break;
		case SUBLANG_SPANISH_NICARAGUA: strSub = TEXT("SUBLANG_SPANISH_NICARAGUA"); break;
		case SUBLANG_SPANISH_PUERTO_RICO: strSub = TEXT("SUBLANG_SPANISH_PUERTO_RICO"); break;
		case SUBLANG_SPANISH_US: strSub = TEXT("SUBLANG_SPANISH_US"); break;
		}
		break;
	case LANG_SWAHILI: strPrim = TEXT("LANG_SWAHILI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_SWAHILI_KENYA: strSub = TEXT("SUBLANG_SWAHILI_KENYA"); break;
		}
		break;
	case LANG_SWEDISH: strPrim = TEXT("LANG_SWEDISH");
		SWITCH_SUBLANG()
		{
		//case SUBLANG_SWEDISH_SWEDEN: strSub = TEXT("SUBLANG_SWEDISH_SWEDEN"); break; // same as SUBLANG_SWEDISH
		case SUBLANG_SWEDISH: strSub = TEXT("SUBLANG_SWEDISH"); break;
		case SUBLANG_SWEDISH_FINLAND: strSub = TEXT("SUBLANG_SWEDISH_FINLAND"); break;
		}
		break;
	case LANG_SYRIAC: strPrim = TEXT("LANG_SYRIAC");
#if defined(ENABLE_NEW_LANGS)
		SWITCH_SUBLANG()
		{
		case SUBLANG_SYRIAC: strSub = TEXT("SUBLANG_SYRIAC"); break;
		}
#endif
		break;
	case LANG_TAJIK: strPrim = TEXT("LANG_TAJIK");
		SWITCH_SUBLANG()
		{
		case SUBLANG_TAJIK_TAJIKISTAN: strSub = TEXT("SUBLANG_TAJIK_TAJIKISTAN"); break;
		}
		break;
	case LANG_TAMAZIGHT: strPrim = TEXT("LANG_TAMAZIGHT");
		SWITCH_SUBLANG()
		{
		case SUBLANG_TAMAZIGHT_ALGERIA_LATIN   : strSub = TEXT("SUBLANG_TAMAZIGHT_ALGERIA_LATIN   "); break;
#ifdef ENABLE_NEW_LANGS
		case SUBLANG_TAMAZIGHT_MOROCCO_TIFINAGH: strSub = TEXT("SUBLANG_TAMAZIGHT_MOROCCO_TIFINAGH"); break;
#endif
		}
		break;
	case LANG_TAMIL: strPrim = TEXT("LANG_TAMIL");
		SWITCH_SUBLANG()
		{
		case SUBLANG_TAMIL_INDIA: strSub = TEXT("SUBLANG_TAMIL_INDIA"); break;
#ifdef ENABLE_NEW_LANGS
		case SUBLANG_TAMIL_SRI_LANKA: strSub = TEXT("SUBLANG_TAMIL_SRI_LANKA"); break;
#endif
		default: break;
		}
		break;
	case LANG_TATAR: strPrim = TEXT("LANG_TATAR");
		SWITCH_SUBLANG()
		{
		case SUBLANG_TATAR_RUSSIA: strSub = TEXT("SUBLANG_TATAR_RUSSIA"); break;
		}
		break;
	case LANG_TELUGU: strPrim = TEXT("LANG_TELUGU");
		SWITCH_SUBLANG()
		{
		case SUBLANG_TELUGU_INDIA: strSub = TEXT("SUBLANG_TELUGU_INDIA"); break;
		}
		break;
	case LANG_THAI: strPrim = TEXT("LANG_THAI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_THAI_THAILAND: strSub = TEXT("SUBLANG_THAI_THAILAND"); break;
		}
		break;
	case LANG_TIBETAN: strPrim = TEXT("LANG_TIBETAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_TIBETAN_PRC: strSub = TEXT("SUBLANG_TIBETAN_PRC"); break;
#if defined(ENABLE_NEW_LANGS)
		case SUBLANG_TIBETAN_BHUTAN: strSub = TEXT("SUBLANG_TIBETAN_BHUTAN"); break;
#endif
		}
		break;
	case LANG_TIGRIGNA: strPrim = TEXT("LANG_TIGRIGNA"); // same as LANG_TIGRINYA
		SWITCH_SUBLANG()
		{
		case SUBLANG_TIGRIGNA_ERITREA: strSub = TEXT("SUBLANG_TIGRIGNA_ERITREA"); break;
		default: break;
		}
		break;
	//case LANG_TIGRINYA: strPrim = TEXT("LANG_TIGRINYA"); // same as LANG_TIGRIGNA
	//    SWITCH_SUBLANG()
	//    {
	//    case SUBLANG_TIGRINYA_ERITREA: strSub = TEXT("SUBLANG_TIGRINYA_ERITREA"); break;
	//    case SUBLANG_TIGRINYA_ETHIOPIA: strSub = TEXT("SUBLANG_TIGRINYA_ETHIOPIA"); break;
	//    }
	//    break;
	case LANG_TSWANA: strPrim = TEXT("LANG_TSWANA");
		SWITCH_SUBLANG()
		{
#ifdef ENABLE_NEW_LANGS
		case SUBLANG_TSWANA_BOTSWANA: strSub = TEXT("SUBLANG_TSWANA_BOTSWANA"); break;
#endif
		case SUBLANG_TSWANA_SOUTH_AFRICA: strSub = TEXT("SUBLANG_TSWANA_SOUTH_AFRICA"); break;
		}
		break;
	case LANG_TURKISH: strPrim = TEXT("LANG_TURKISH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_TURKISH_TURKEY: strSub = TEXT("SUBLANG_TURKISH_TURKEY"); break;
		}
		break;
	case LANG_TURKMEN: strPrim = TEXT("LANG_TURKMEN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_TURKMEN_TURKMENISTAN: strSub = TEXT("SUBLANG_TURKMEN_TURKMENISTAN"); break;
		}
		break;
	case LANG_UIGHUR: strPrim = TEXT("LANG_UIGHUR");
		SWITCH_SUBLANG()
		{
		case SUBLANG_UIGHUR_PRC: strSub = TEXT("SUBLANG_UIGHUR_PRC"); break;
		}
		break;
	case LANG_UKRAINIAN: strPrim = TEXT("LANG_UKRAINIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_UKRAINIAN_UKRAINE: strSub = TEXT("SUBLANG_UKRAINIAN_UKRAINE"); break;
		}
		break;
	case LANG_UPPER_SORBIAN: strPrim = TEXT("LANG_UPPER_SORBIAN");
		SWITCH_SUBLANG()
		{
		case SUBLANG_UPPER_SORBIAN_GERMANY: strSub = TEXT("SUBLANG_UPPER_SORBIAN_GERMANY"); break;
		}
		break;
	case LANG_URDU: strPrim = TEXT("LANG_URDU");
		SWITCH_SUBLANG()
		{
		case SUBLANG_URDU_PAKISTAN: strSub = TEXT("SUBLANG_URDU_PAKISTAN"); break;
		case SUBLANG_URDU_INDIA: strSub = TEXT("SUBLANG_URDU_INDIA"); break;
		}
		break;
	case LANG_UZBEK: strPrim = TEXT("LANG_UZBEK");
		SWITCH_SUBLANG()
		{
		case SUBLANG_UZBEK_LATIN: strSub = TEXT("SUBLANG_UZBEK_LATIN"); break;
		case SUBLANG_UZBEK_CYRILLIC: strSub = TEXT("SUBLANG_UZBEK_CYRILLIC"); break;
		}
		break;
	//case LANG_VALENCIAN: strPrim = TEXT("LANG_VALENCIAN"); // same as LANG_CATALAN
	//    SWITCH_SUBLANG()
	//    {
	//    case SUBLANG_VALENCIAN_VALENCIA: strSub = TEXT("SUBLANG_VALENCIAN_VALENCIA"); break;
	//    }
	//    break;
	case LANG_VIETNAMESE: strPrim = TEXT("LANG_VIETNAMESE");
		SWITCH_SUBLANG()
		{
		case SUBLANG_VIETNAMESE_VIETNAM: strSub = TEXT("SUBLANG_VIETNAMESE_VIETNAM"); break;
		}
		break;
	case LANG_WELSH: strPrim = TEXT("LANG_WELSH");
		SWITCH_SUBLANG()
		{
		case SUBLANG_WELSH_UNITED_KINGDOM: strSub = TEXT("SUBLANG_WELSH_UNITED_KINGDOM"); break;
		}
		break;
	case LANG_WOLOF: strPrim = TEXT("LANG_WOLOF");
		SWITCH_SUBLANG()
		{
		case SUBLANG_WOLOF_SENEGAL: strSub = TEXT("SUBLANG_WOLOF_SENEGAL"); break;
		}
		break;
	case LANG_XHOSA: strPrim = TEXT("LANG_XHOSA");
		SWITCH_SUBLANG()
		{
		case SUBLANG_XHOSA_SOUTH_AFRICA: strSub = TEXT("SUBLANG_XHOSA_SOUTH_AFRICA"); break;
		}
		break;
	//case LANG_YAKUT: strPrim = TEXT("LANG_YAKUT"); // same as LANG_SAKHA
	//    SWITCH_SUBLANG()
	//    {
	//    case SUBLANG_YAKUT_RUSSIA: strSub = TEXT("SUBLANG_YAKUT_RUSSIA"); break;
	//    }
	//    break;
	case LANG_YI: strPrim = TEXT("LANG_YI");
		SWITCH_SUBLANG()
		{
		case SUBLANG_YI_PRC: strSub = TEXT("SUBLANG_YI_PRC"); break;
		}
		break;
	case LANG_YORUBA: strPrim = TEXT("LANG_YORUBA");
		SWITCH_SUBLANG()
		{
		case SUBLANG_YORUBA_NIGERIA: strSub = TEXT("SUBLANG_YORUBA_NIGERIA"); break;
		}
		break;
	case LANG_ZULU: strPrim = TEXT("LANG_ZULU");
		SWITCH_SUBLANG()
		{
		case SUBLANG_ZULU_SOUTH_AFRICA: strSub = TEXT("SUBLANG_ZULU_SOUTH_AFRICA"); break;
		}
		break;
	default:
		break;
	}

	TCHAR szText[32];
	if (strPrim.empty())
	{
		StringCchPrintf(szText, _countof(szText), TEXT("0x%04X"), PRIMARYLANGID(langid));
		strPrim = szText;
	}

	if (bOldStyle)
		strSub.clear();

	// sub-language
	if (strSub.empty())
	{
		switch (SUBLANGID(langid))
		{
		case SUBLANG_NEUTRAL: strSub = TEXT("SUBLANG_NEUTRAL"); break;
		case SUBLANG_DEFAULT: strSub = TEXT("SUBLANG_DEFAULT"); break;
		case SUBLANG_SYS_DEFAULT: strSub = TEXT("SUBLANG_SYS_DEFAULT"); break;
		// NOTE: RosBE <winnt.rh> doesn't support the following names: SUBLANG_CUSTOM_DEFAULT, SUBLANG_CUSTOM_UNSPECIFIED, and SUBLANG_UI_CUSTOM_DEFAULT.
		//case SUBLANG_CUSTOM_DEFAULT: strSub = TEXT("SUBLANG_CUSTOM_DEFAULT"); break;
		//case SUBLANG_CUSTOM_UNSPECIFIED: strSub = TEXT("SUBLANG_CUSTOM_UNSPECIFIED"); break;
		//case SUBLANG_UI_CUSTOM_DEFAULT: strSub = TEXT("SUBLANG_UI_CUSTOM_DEFAULT"); break;
		default:
			break;
		}
	}

	// sub-language
	if (strSub.empty())
	{
		StringCchPrintf(szText, _countof(szText), TEXT("0x%04X"), SUBLANGID(langid));
		strSub = szText;
	}
#undef SWITCH_SUBLANG

	// output the LANGUAGE statement
	MString str = TEXT("LANGUAGE ");
	str += strPrim;
	str += TEXT(", ");
	str += strSub;
	str += TEXT("\r\n");

	return str;
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

//////////////////////////////////////////////////////////////////////////////
// STRING_ENTRY

// helper function for MAddStrDlg and MModifyStrDlg
BOOL StrDlg_GetEntry(HWND hwnd, STRING_ENTRY& entry)
{
	// get the text from combobox
	// replace the fullwidth characters with halfwidth characters
	MString str = MWindowBase::GetDlgItemText(hwnd, cmb1);
	ReplaceFullWithHalf(str);

	if (('0' <= str[0] && str[0] <= '9') || str[0] == '-' || str[0] == '+')
	{
		// numeric
		LONG n = mstr_parse_int(str.c_str());
		str = mstr_dec_word(WORD(n));
	}
	else if (!g_db.HasResID(str))
	{
		// non-numeric and not resource ID. invalid
		return FALSE;   // failure
	}

	// store the string to entry.StringID
	StringCchCopyW(entry.StringID, _countof(entry.StringID), str.c_str());

	// get the text from EDIT control
	str = MWindowBase::GetDlgItemText(hwnd, edt1);
	//mstr_trim(str);     // trim it

	// unquote if quoted
	if (str[0] == L'"')
	{
		mstr_unquote(str);
	}

	// store the text to entry.StringValue
	StringCchCopyW(entry.StringValue, _countof(entry.StringValue), str.c_str());

	return TRUE;    // success
}

// helper function for MAddStrDlg and MModifyStrDlg
void StrDlg_SetEntry(HWND hwnd, STRING_ENTRY& entry)
{
	// store entry.StringID to combobox
	SetDlgItemTextW(hwnd, cmb1, entry.StringID);

	// store the quoted entry.StringValue to the EDIT control
	MStringW str = entry.StringValue;
	str = mstr_quote(str);
	SetDlgItemTextW(hwnd, edt1, str.c_str());
}

//////////////////////////////////////////////////////////////////////////////
// MESSAGE_ENTRY

// helper function for MAddMsgDlg and MModifyMsgDlg
BOOL MsgDlg_GetEntry(HWND hwnd, MESSAGE_ENTRY& entry)
{
	// get the text from combobox
	// replace the fullwidth characters with halfwidth characters
	MString str = MWindowBase::GetDlgItemText(hwnd, cmb1);
	ReplaceFullWithHalf(str);

	if (('0' <= str[0] && str[0] <= '9') || str[0] == '-' || str[0] == '+')
	{
		// numeric
		LONG n = mstr_parse_int(str.c_str());
		str = mstr_hex(n);      // make it hexidemical
	}
	else if (!g_db.HasResID(str))
	{
		// non-numeric and not resource ID. invalid
		return FALSE;   // failure
	}

	// store the string to entry.MessageID
	StringCchCopyW(entry.MessageID, _countof(entry.MessageID), str.c_str());

	// get the text from EDIT control
	str = MWindowBase::GetDlgItemText(hwnd, edt1);

	// unquote if quoted
	if (str[0] == L'"')
	{
		mstr_unquote(str);
	}

	// store the text to entry.MessageValue
	StringCchCopyW(entry.MessageValue, _countof(entry.MessageValue), str.c_str());

	return TRUE;    // success
}

// helper function for MAddMsgDlg and MModifyMsgDlg
void MsgDlg_SetEntry(HWND hwnd, MESSAGE_ENTRY& entry)
{
	// get the text from combobox
	SetDlgItemTextW(hwnd, cmb1, entry.MessageID);

	// set the quoted entry.MessageValue to the EDIT control
	MStringW str = entry.MessageValue;
	str = mstr_quote(str);
	SetDlgItemTextW(hwnd, edt1, str.c_str());
}

//////////////////////////////////////////////////////////////////////////////

// get the RISOHTEMPLATE text
MStringW GetRisohTemplate(const MIdOrString& type, const MIdOrString& name, WORD wLang)
{
	// get this application module
	HINSTANCE hInst = GetModuleHandle(NULL);

	if (type.empty())
	{
		return L"";    // failure
	}

	MIdOrString name0 = type;
	if (type == L"TEXTINCLUDE")
		name0 = (L"TEXTINCLUDE" + name.str()).c_str();

	// try to find the RISOHTEMPLATE resource
	WORD LangID = PRIMARYLANGID(wLang);
	HRSRC hRsrc = NULL;
	if (hRsrc == NULL)
		hRsrc = FindResourceExW(hInst, L"RISOHTEMPLATE", name0.ptr(), wLang);
	if (hRsrc == NULL)
		hRsrc = FindResourceExW(hInst, L"RISOHTEMPLATE", name0.ptr(), MAKELANGID(LangID, SUBLANG_NEUTRAL));
	if (hRsrc == NULL)
		hRsrc = FindResourceExW(hInst, L"RISOHTEMPLATE", name0.ptr(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	if (hRsrc == NULL)        hRsrc = FindResourceExW(hInst, L"RISOHTEMPLATE", name0.ptr(), MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));
	if (hRsrc == NULL)
		hRsrc = FindResourceExW(hInst, L"RISOHTEMPLATE", name0.ptr(), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
	if (hRsrc == NULL)
		return L"";

	// get the pointer and byte size
	HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
	DWORD cb = SizeofResource(hInst, hRsrc);
	const BYTE *pb = (const BYTE *)LockResource(hGlobal);

	// ignore the BOM if any
	if (memcmp(pb, "\xEF\xBB\xBF", 3) == 0)
	{
		pb += 3;
		cb -= 3;
	}

	// convert the UTF-8 text to the UTF-16 text (wide)
	MStringA utf8((LPCSTR)(pb), (LPCSTR)(pb) + cb);
	MAnsiToWide wide(CP_UTF8, utf8);

	return wide.c_str();    // return the wide
}
