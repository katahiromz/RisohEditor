// stdafx.h --- the header file
// Author: katahiromz
// License: CC0

#if defined(_MSC_VER) && !defined(NDEBUG) && !defined(_CRTDBG_MAP_ALLOC)
	// for detecting memory leak (MSVC only)
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "resource.h"
#include "protection.h"
#if defined(NDEBUG) && defined(PROTECTION) && defined(RESOURCE_PROTECTION)
    #include "WinResToWonRes.h"
#endif
