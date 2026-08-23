// Toolbar.h --- TOOLBAR resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#ifndef _INC_WINDOWS
	#include <windows.h>
#endif
#ifndef _INC_COMMCTRL
	#include <commctrl.h>
#endif
#include <cassert>
#include <vector>

#ifndef RT_TOOLBAR
	#define RT_TOOLBAR  MAKEINTRESOURCE(241)
#endif

// for MSVC rc
typedef struct tagTOOLBARDATA
{
	WORD wVersion;                  // Always 1
	WORD wWidth;                    // icon width
	WORD wHeight;                   // icon height
	WORD wItemCount;                // the # of items
	WORD aItems[ANYSIZE_ARRAY];     // The command IDs (a separator is zero)
} TOOLBARDATA, *PTOOLBARDATA;

// for MinGW/clang windres (non-standard)
typedef struct tagTOOLBARDATAWINDRES
{
	union
	{
		WORD wVersion;              // >= 3
		DWORD wWidth;               // icon width
	};
	DWORD wHeight;                  // icon height
	DWORD wItemCount;               // the # of items
	DWORD aItems[ANYSIZE_ARRAY];    // The command IDs (a separator is zero)
} TOOLBARDATAWINDRES, *PTOOLBARDATAWINDRES;

typedef INT (CALLBACK *FN_INT2INT)(INT id);
typedef BOOL (CALLBACK *FN_INT2STR)(INT id, LPTSTR pszText, INT cchTextMax);

namespace ToolbarDetail
{
	template <typename T_ITEM>
	inline void AddToolbarButtons(HWND hwndTB, const T_ITEM *pItems, DWORD wItemCount,
								   FN_INT2INT fnCommandIdToImageIndex,
								   FN_INT2STR fnCommandIdToText)
	{
		std::vector<TBBUTTON> buttons;
		buttons.reserve(wItemCount);

		for (DWORD i = 0; i < wItemCount; ++i)
		{
			TBBUTTON button;
			ZeroMemory(&button, sizeof(button));

			INT idCommand = button.idCommand = (INT)pItems[i];
			button.fsState = TBSTATE_ENABLED;
			button.iBitmap = -1;
			button.iString = -1;
			if (idCommand)
			{
				button.iBitmap = fnCommandIdToImageIndex(idCommand);
				button.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;

				TCHAR szText[MAX_PATH];
				szText[0] = 0;
				if (fnCommandIdToText && fnCommandIdToText(idCommand, szText, _countof(szText)))
					button.iString = (INT)SendMessage(hwndTB, TB_ADDSTRING, 0, (LPARAM)szText);
			}
			else
			{
				button.fsStyle = BTNS_SEP;
			}

			buttons.push_back(button);
		}

		SendMessage(hwndTB, TB_ADDBUTTONS, WPARAM(buttons.size()), (LPARAM)buttons.data());
	}

	inline BOOL SetupToolbarImageList(HWND hwndTB, HINSTANCE hInst, LPCTSTR lpName,
									   DWORD wWidth, DWORD wHeight)
	{
		if (wWidth < 3 || wHeight < 3)
		{
			assert(0);
			return FALSE;
		}

		SendMessage(hwndTB, TB_SETBITMAPSIZE, 0, MAKELPARAM(wWidth, wHeight));

		HIMAGELIST himl = ImageList_LoadImage(hInst, lpName, (INT)wWidth, 0, RGB(255, 0, 255),
											  IMAGE_BITMAP, LR_CREATEDIBSECTION);
		if (himl == NULL)
		{
			assert(0);
			return FALSE;
		}

		HIMAGELIST himlOld = (HIMAGELIST)SendMessage(hwndTB, TB_GETIMAGELIST, 0, 0);
		SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)himl);
		if (himlOld)
			ImageList_Destroy(himlOld);

		return TRUE;
	}
}

// See: https://github.com/katahiromz/RisohEditor/blob/master/tests/ToolbarTest/ToolbarTest.cpp
inline BOOL
LoadToolbarResource(HWND hwndTB, HINSTANCE hInst, LPCTSTR lpName,
					FN_INT2INT fnCommandIdToImageIndex,
					FN_INT2STR fnCommandIdToText = NULL)
{
	assert(IsWindow(hwndTB));
	assert(lpName != NULL);
	assert(fnCommandIdToImageIndex != NULL);

	if (hInst == NULL)
		hInst = GetModuleHandle(NULL);

	// Set BUTTON struct size
	SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	// Load RT_TOOLBAR resource
	HRSRC hRsrc = FindResource(hInst, lpName, RT_TOOLBAR);
	if (!hRsrc)
	{
		assert(0);
		return FALSE;
	}
	DWORD cbReal = SizeofResource(hInst, hRsrc);
	if (cbReal < sizeof(TOOLBARDATA) - sizeof(WORD))
	{
		assert(0);
		return FALSE;
	}
	HGLOBAL hResData = LoadResource(hInst, hRsrc);
	LPVOID pvData = LockResource(hResData);
	if (pvData == NULL)
	{
		assert(0);
		return FALSE;
	}

	// Validate the data
	PTOOLBARDATA pData1 = (PTOOLBARDATA)pvData;
	if (pData1->wVersion == 1)
	{
		DWORD wWidth = pData1->wWidth, wHeight = pData1->wHeight;
		DWORD wItemCount = pData1->wItemCount;

		size_t cbExpect = (sizeof(TOOLBARDATA) - sizeof(WORD)) + wItemCount * sizeof(WORD);
		if (cbReal < cbExpect)
		{
			assert(0);
			return FALSE;
		}

		if (!ToolbarDetail::SetupToolbarImageList(hwndTB, hInst, lpName, wWidth, wHeight))
			return FALSE;

		ToolbarDetail::AddToolbarButtons(hwndTB, pData1->aItems, wItemCount,
										  fnCommandIdToImageIndex, fnCommandIdToText);
	}
#ifndef _MSC_VER // Not Visual C++
	else if (pData1->wVersion >= 3)
	{
		PTOOLBARDATAWINDRES pData2 = (PTOOLBARDATAWINDRES)pvData;
		DWORD wWidth = pData2->wWidth, wHeight = pData2->wHeight;
		DWORD wItemCount = pData2->wItemCount;

		if (wItemCount > (SIZE_MAX - (sizeof(TOOLBARDATAWINDRES) - sizeof(DWORD))) / sizeof(DWORD))
		{
			assert(0);
			return FALSE;
		}
		size_t cbExpect = (sizeof(TOOLBARDATAWINDRES) - sizeof(DWORD)) + wItemCount * sizeof(DWORD);
		if (cbReal < cbExpect)
		{
			assert(0);
			return FALSE;
		}

		if (!ToolbarDetail::SetupToolbarImageList(hwndTB, hInst, lpName, wWidth, wHeight))
			return FALSE;

		ToolbarDetail::AddToolbarButtons(hwndTB, pData2->aItems, wItemCount,
										  fnCommandIdToImageIndex, fnCommandIdToText);
	}
#endif // ndef _MSC_VER
	else
	{
		assert(0);
		return FALSE;
	}

	// Modify extended style
	DWORD extended = (DWORD)SendMessage(hwndTB, TB_GETEXTENDEDSTYLE, 0, 0);
	extended |= TBSTYLE_EX_DRAWDDARROWS; // BTNS_DROPDOWN and BTNS_WHOLEDROPDOWN will work
	//extended |= TBSTYLE_EX_MIXEDBUTTONS; // BTNS_SHOWTEXT works
	SendMessage(hwndTB, TB_SETEXTENDEDSTYLE, 0, extended);
	return TRUE;
}
