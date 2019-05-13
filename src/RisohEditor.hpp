// RisohEditor.hpp --- RisohEditor header
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, 
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#ifndef RISOHEDITOR_HPP_
#define RISOHEDITOR_HPP_

#pragma once

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
#include <process.h>
#include <uxtheme.h>

#include <algorithm>    // for std::sort
#include <string>       // for std::string, std::wstring
#include <cassert>      // for assert macro
#include <vector>       // for std::vector
#include <map>          // for std::map
#include <cstdio>
#include <strsafe.h>

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
#include "RisohSettings.hpp"

// RisohEditor.cpp
BOOL GetPathOfShortcutDx(HWND hwnd, LPCWSTR pszLnkFile, LPWSTR pszPath);
BOOL DumpBinaryFileDx(const WCHAR *filename, LPCVOID pv, DWORD size);
HBITMAP CreateBitmapFromIconDx(HICON hIcon, INT width, INT height, BOOL bCursor);

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
#include "MMessagesDlg.hpp"
#include "MEditMenuDlg.hpp"
#include "MEditAccelDlg.hpp"
#include "MIdAssocDlg.hpp"
#include "MBmpView.hpp"
#include "MSrcEdit.hpp"
#include "MIDListDlg.hpp"
#include "MConfigDlg.hpp"
#include "MAdviceResHDlg.hpp"
#include "MCloneInNewNameDlg.hpp"
#include "MCloneInNewLangDlg.hpp"
#include "MItemSearchDlg.hpp"
#include "MVersionInfoDlg.hpp"
#include "MFontsDlg.hpp"
#include "MMacrosDlg.hpp"
#include "MPathsDlg.hpp"
#include "MExportOptionsDlg.hpp"
#include "MSaveOptionsDlg.hpp"
#include "MLangsDlg.hpp"
#include "MTestParentWnd.hpp"
#include "MOleCtrl.hpp"
#include "MDlgInitDlg.hpp"
#include "MEncodingDlg.hpp"

#include "MString.hpp"
#include "MByteStream.hpp"

#include "AccelRes.hpp"
#include "IconRes.hpp"
#include "MenuRes.hpp"
#include "MessageRes.hpp"
#include "StringRes.hpp"
#include "DialogRes.hpp"
#include "VersionRes.hpp"
#include "DlgInitRes.hpp"

#include "ConstantsDB.hpp"
#include "PackedDIB.hpp"
#include "Res.hpp"
#include "ResHeader.hpp"

#include "MFile.hpp"
#include "MProcessMaker.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")

#include "ResToText.hpp"

////////////////////////////////////////////////////////////////////////////

// TODO: Update the version number
#define RE_VERSION  "5.1.8"

////////////////////////////////////////////////////////////////////////////

#endif  // ndef RISOHEDITOR_HPP_
