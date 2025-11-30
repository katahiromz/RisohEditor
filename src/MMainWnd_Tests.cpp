// MMainWnd_Tests.cpp --- RisohEditor
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "MMainWnd.hpp"
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

LPVOID load_resource(HINSTANCE hinst, LPCTSTR type, LPCTSTR name, DWORD* pSize) {
	HRSRC hRsrc = FindResource(hinst, name, type);
	*pSize = SizeofResource(hinst, hRsrc);
	HGLOBAL hGlobal = LoadResource(hinst, hRsrc);
	return LockResource(hGlobal);
}

#ifndef NDEBUG
void TEST_MenuTest() {
	TCHAR szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, _countof(szPath));
	PathRemoveFileSpec(szPath);
	PathAppend(szPath, TEXT("MenuTest.exe"));

	HINSTANCE hinstMenuTest = LoadLibraryEx(szPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
	assert(hinstMenuTest);

	DWORD size;
	LPVOID ptr;
	
	auto name1 = MAKEINTRESOURCE(1);
	ptr = load_resource(hinstMenuTest, RT_MENU, name1, &size);
	assert(ptr);
	assert(size);
	if (ptr) {
		MByteStreamEx stream(ptr, size);
		MenuRes menu_res;
		assert(menu_res.LoadFromStream(stream));
		MString str = menu_res.Dump(name1);
		MTRACE(TEXT("%s\n"), str.c_str());
	}

	auto name2 = MAKEINTRESOURCE(2);
	ptr = load_resource(hinstMenuTest, RT_MENU, name2, &size);
	assert(ptr);
	assert(size);
	if (ptr) {
		MByteStreamEx stream(ptr, size);
		MenuRes menu_res;
		assert(menu_res.LoadFromStream(stream));
		MString str = menu_res.DumpEx(name2);
		MTRACE(TEXT("%s\n"), str.c_str());
	}

	FreeLibrary(hinstMenuTest);
}
#endif

void MMainWnd::OnInternalTest(HWND hwnd) {
#ifndef NDEBUG
	TEST_MenuTest();
#endif
}
