// RisohEditor.hpp
////////////////////////////////////////////////////////////////////////////

#ifndef RISOHEDITOR_HPP_
#define RISOHEDITOR_HPP_

#pragma once

////////////////////////////////////////////////////////////////////////////

#ifndef UNICODE
    #define UNICODE
#endif
#ifndef _UNICODE
    #define _UNICODE
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif

////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <dlgs.h>
#include <tchar.h>
#include <commctrl.h>
#include <commdlg.h>
#include <mbstring.h>

#include <algorithm>    // for std::sort
#include <string>       // for std::string, std::wstring
#include <cassert>      // for assert macro
#include <cstdio>

////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"
#include "MRadWindow.hpp"
#include "Res.hpp"
#include "resource.h"

// RisohEditor.cpp
std::wstring str_vkey(WORD w);
LPWSTR MakeFilterDx(LPWSTR psz);
BOOL GetPathOfShortcutDx(HWND hwnd, LPCWSTR pszLnkFile, LPWSTR pszPath);
HBITMAP Create24BppBitmapDx(INT width, INT height);
BOOL DumpBinaryFileDx(const WCHAR *filename, LPCVOID pv, DWORD size);
LPWSTR GetTempFileNameDx(LPCWSTR pszPrefix3Chars);
std::wstring str_vkey(WORD w);
HBITMAP CreateBitmapFromIconDx(HICON hIcon, INT width, INT height, BOOL bCursor);

HBITMAP
CreateBitmapFromIconOrPngDx(HWND hwnd, const ResEntry& Entry, BITMAP& bm);

HBITMAP
CreateBitmapFromIconsDx(HWND hwnd, ResEntries& Entries, const ResEntry& Entry);

HBITMAP
CreateBitmapFromCursorDx(HWND hwnd, const ResEntry& Entry, BITMAP& bm);

HBITMAP
CreateBitmapFromCursorsDx(HWND hwnd, ResEntries& Entries, const ResEntry& Entry);

BOOL DoAddCursor(HWND hwnd,
                 ResEntries& Entries,
                 const ID_OR_STRING& Name,
                 WORD Lang,
                 const std::wstring& CurFile);
BOOL DoReplaceCursor(HWND hwnd,
                     ResEntries& Entries,
                     const ID_OR_STRING& Name,
                     WORD Lang,
                     const std::wstring& CurFile);
BOOL DoAddIcon(HWND hwnd,
               ResEntries& Entries,
               const ID_OR_STRING& Name,
               WORD Lang,
               const std::wstring& IconFile);
BOOL DoReplaceIcon(HWND hwnd,
                   ResEntries& Entries,
                   const ID_OR_STRING& Name,
                   WORD Lang,
                   const std::wstring& IconFile);
BOOL DoAddBin(HWND hwnd,
              ResEntries& Entries,
              const ID_OR_STRING& Type,
              const ID_OR_STRING& Name,
              WORD Lang,
              const std::wstring& File);
BOOL DoReplaceBin(HWND hwnd,
                  ResEntries& Entries,
                  const ID_OR_STRING& Type,
                  const ID_OR_STRING& Name,
                  WORD Lang,
                  const std::wstring& File);
BOOL DoAddBitmap(HWND hwnd,
                 ResEntries& Entries,
                 const ID_OR_STRING& Name,
                 WORD Lang,
                 const std::wstring& BitmapFile);
BOOL DoReplaceBitmap(HWND hwnd,
                     ResEntries& Entries,
                     const ID_OR_STRING& Name,
                     WORD Lang,
                     const std::wstring& BitmapFile);

////////////////////////////////////////////////////////////////////////////

#include "MReplaceBinDlg.hpp"
#include "MTestMenuDlg.hpp"
#include "MTestDialog.hpp"
#include "MAddIconDlg.hpp"
#include "MReplaceIconDlg.hpp"
#include "MReplaceCursorDlg.hpp"
#include "MAddBitmapDlg.hpp"
#include "MReplaceBitmapDlg.hpp"
#include "MAddCursorDlg.hpp"
#include "MAddResDlg.hpp"
#include "MStringsDlg.hpp"
#include "MEditMenuDlg.hpp"
#include "MEditAccelDlg.hpp"
#include "MBmpView.hpp"

#include "id_string.hpp"
#include "MString.hpp"
#include "MByteStream.hpp"

#include "AccelRes.hpp"
#include "IconRes.hpp"
#include "MenuRes.hpp"
#include "MessageRes.hpp"
#include "StringRes.hpp"
#include "DialogRes.hpp"
#include "VersionRes.hpp"

#include "ConstantsDB.hpp"
#include "PackedDIB.hpp"
#include "Png.hpp"
#include "Res.hpp"

#include "MFile.hpp"
#include "MProcessMaker.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")

////////////////////////////////////////////////////////////////////////////

#endif  // ndef RISOHEDITOR_HPP_

////////////////////////////////////////////////////////////////////////////
