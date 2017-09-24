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

#include <initguid.h>
#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <dlgs.h>
#include <tchar.h>
#include <commctrl.h>
#include <commdlg.h>
#include <mbstring.h>
#include <mmsystem.h>

#include <algorithm>    // for std::sort
#include <string>       // for std::string, std::wstring
#include <cassert>      // for assert macro
#include <vector>       // for std::vector
#include <map>          // for std::map
#include <cstdio>

////////////////////////////////////////////////////////////////////////////

#include "MWindowBase.hpp"
#include "MEditCtrl.hpp"
#include "MSplitterWnd.hpp"
#include "MRegKey.hpp"
#include "MBitmapDx.hpp"
#include "Res.hpp"
#include "ConstantsDB.hpp"
#include "MacroParser.hpp"
#include "MWaitCursor.hpp"
#include "resource.h"

// RisohEditor.cpp
LPWSTR MakeFilterDx(LPWSTR psz);
BOOL GetPathOfShortcutDx(HWND hwnd, LPCWSTR pszLnkFile, LPWSTR pszPath);
HBITMAP Create24BppBitmapDx(INT width, INT height);
BOOL DumpBinaryFileDx(const WCHAR *filename, LPCVOID pv, DWORD size);
LPWSTR GetTempFileNameDx(LPCWSTR pszPrefix3Chars);
HBITMAP CreateBitmapFromIconDx(HICON hIcon, INT width, INT height, BOOL bCursor);

HBITMAP
CreateBitmapFromIconOrPngDx(HWND hwnd, const ResEntry& Entry, BITMAP& bm);

HBITMAP
CreateBitmapFromIconsDx(HWND hwnd, ResEntries& Entries, const ResEntry& Entry);

HBITMAP
CreateBitmapFromCursorDx(HWND hwnd, const ResEntry& Entry, BITMAP& bm);

HBITMAP
CreateBitmapFromCursorsDx(HWND hwnd, ResEntries& Entries, const ResEntry& Entry);

////////////////////////////////////////////////////////////////////////////

#define MAX_MRU         5       // the number of most recently used files

typedef std::vector<MString>            mru_type;
typedef std::map<MString, MString>      assoc_map_type;
typedef std::map<MStringA, MStringA>    id_map_type;

struct RisohSettings
{
    BOOL        bShowBinEdit;
    BOOL        bAlwaysControl;
    BOOL        bShowStatusBar;
    INT         nTreeViewWidth;
    INT         nBmpViewWidth;
    INT         nBinEditHeight;
    BOOL        bGuiByDblClick;
    BOOL        bResumeWindowPos;
    BOOL        bAutoLoadNearbyResH;
    BOOL        bAutoShowIDList;
    BOOL        bHideID;
    BOOL        bShowDotsOnDialog;
    INT         nComboHeight;
    mru_type    vecRecentlyUsed;
    INT         nWindowLeft;
    INT         nWindowTop;
    INT         nWindowWidth;
    INT         nWindowHeight;
    BOOL        bMaximized;
    INT         nIDListLeft;
    INT         nIDListTop;
    INT         nIDListWidth;
    INT         nIDListHeight;
    INT         nRadLeft;
    INT         nRadTop;
    assoc_map_type      assoc_map;
    id_map_type         id_map;
    id_map_type         added_ids;
    id_map_type         removed_ids;

    RisohSettings()
    {
    }

    void AddFile(LPCTSTR pszFile)
    {
        for (size_t i = 0; i < vecRecentlyUsed.size(); ++i)
        {
            if (vecRecentlyUsed[i] == pszFile)
            {
                vecRecentlyUsed.erase(vecRecentlyUsed.begin() + i);
                break;
            }
        }
        vecRecentlyUsed.insert(vecRecentlyUsed.begin(), pszFile);
        if (vecRecentlyUsed.size() > MAX_MRU)
            vecRecentlyUsed.resize(MAX_MRU);
    }
};

////////////////////////////////////////////////////////////////////////////

#include "MRadWindow.hpp"
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
#include "MIdAssocDlg.hpp"
#include "MBmpView.hpp"
#include "MIDListDlg.hpp"
#include "MConfigDlg.hpp"
#include "MAdviceResHDlg.hpp"

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
#include "Res.hpp"

#include "MFile.hpp"
#include "MProcessMaker.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")

////////////////////////////////////////////////////////////////////////////

#endif  // ndef RISOHEDITOR_HPP_
