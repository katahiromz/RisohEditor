// Utils.cpp --- RisohEditor
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "MString.hpp"
#include "MIdOrString.hpp"
#include "ConstantsDB.hpp"
#include "Utils.h"
#include "Common.hpp"
#include "MString.hpp"
#include "resource.h"

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
