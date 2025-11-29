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
