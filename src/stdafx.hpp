#pragma once

#ifndef UNICODE
    #define UNICODE
#endif
#ifndef _UNICODE
    #define _UNICODE
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif

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

#ifndef _countof
    #define _countof(x)     (sizeof(x) / sizeof(x[0]))
#endif

#include "resource.h"
#include "WindowBase.hpp"

LPWSTR LoadStringDx(UINT id);
std::wstring str_vkey(WORD w);

#include "id_string.hpp"
#include "Text.hpp"
#include "ByteStream.hpp"

#include "Samples.hpp"

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

#include "File.hpp"
#include "ProcessMaker.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")
