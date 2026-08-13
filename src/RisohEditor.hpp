// RisohEditor.hpp --- RisohEditor header
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include <initguid.h>
#include <windows.h>
#include <windowsx.h>
#ifdef __GNUC__ // Workaround
	#define WINAPI_FAMILY_ONE_PARTITION(vset, v) ((WINAPI_FAMILY & vset) == v)
#endif
#include <shlobj.h>
#include <shlwapi.h>
#include <dlgs.h>
#include <tchar.h>
#include <commctrl.h>
#include <commdlg.h>
#include <mbstring.h>
#include <mmsystem.h>
#include <process.h>
#include <uxtheme.h>
#include <urlmon.h>
#include <wininet.h>
#ifdef ATL_SUPPORT
	#include <cguid.h>
	#include <atlbase.h>
	#include <atlhost.h>
#endif

#include <algorithm>    // for std::sort
#include <string>       // for std::string, std::wstring
#include <cassert>      // for assert macro
#include <vector>       // for std::vector
#include <map>          // for std::map
#include <unordered_map>
#include <cstdio>
#include <clocale>
#include <strsafe.h>

////////////////////////////////////////////////////////////////////////////

INT LogMessageBoxW(HWND hwnd, LPCWSTR text, LPCWSTR title, UINT uType);

#include "WonSetThreadUILanguage.h"

#include "MWindowBase.hpp"
#include "MHexEditCtrl.hpp"
#include "MSplitterWnd.hpp"
#include "MBitmapDx.hpp"
#include "Res.hpp"
#include "ConstantsDB.hpp"
#include "MacroParser.hpp"
#include "MWaitCursor.hpp"
#include "settings.h"

#define _CP_UTF16 1200

////////////////////////////////////////////////////////////////////////////

#include "MAddResDlg.hpp"
#include "MBmpView.hpp"
#include "MDropdownArrow.hpp"
#include "MEgaDlg.hpp"
#include "MIDListDlg.hpp"
#include "MItemSearchDlg.hpp"
#include "MRadWindow.hpp"
#include "MTabCtrl.hpp"

#include "MString.hpp"
#include "MByteStream.hpp"
#include "ConstantsDB.hpp"
#include "PackedDIB.hpp"
#include "Res.hpp"
#include "ResHeader.hpp"

#include "MFile.hpp"
#include "MProcessMaker.hpp"

#include "ResToText.hpp"

extern std::unordered_map<INT, MStringW> *g_pmapIDTypeToLocalized;
extern std::unordered_map<MStringW, INT> *g_pmapLocalizedToIDType;
MStringW MapIDType(IDTYPE_ nIDType);
IDTYPE_ UnMapIDType(const MStringW& str);
