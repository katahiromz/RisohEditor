// MMainWnd_Toolbar.cpp --- RisohEditor toolbar
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "MMainWnd.hpp"
#include "Utils.h"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////////

// buttons info #0
static TBBUTTON g_buttons0[] =
{
	{ 11, ID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_GUIEDIT },
	{ 12, ID_TEST, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TEST },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
	{ 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
	{ 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
	{ 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
	{ 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
	{ 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
	{ 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
	{ 14, ID_EXPORTRES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPORT },
};

// buttons info #1
static TBBUTTON g_buttons1[] =
{
	{ 12, ID_TEST, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TEST },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
	{ 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
	{ 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
	{ 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
	{ 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
	{ 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
	{ 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
	{ 14, ID_EXPORTRES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPORT },
};

// buttons info #2
static TBBUTTON g_buttons2[] =
{
	{ 9, ID_COMPILE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_RECOMPILE },
	{ 10, ID_CANCELEDIT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CANCELEDIT },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
	{ 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
	{ 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
	{ 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
	{ 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
	{ 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
	{ 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
	{ 14, ID_EXPORTRES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPORT },
};

// buttons info #3
static TBBUTTON g_buttons3[] =
{
	{ 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
	{ 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
	{ 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
	{ 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
	{ 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
	{ 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
	{ 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
	{ 14, ID_EXPORTRES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPORT },
};

// buttons info #4
static TBBUTTON g_buttons4[] =
{
	{ 11, ID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_GUIEDIT },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_NEW },
	{ 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_OPEN },
	{ 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_SAVE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 3, ID_EXPAND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPAND },
	{ 4, ID_COLLAPSE_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_COLLAPSE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 5, ID_ADDBANG, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN, {0}, 0, IDS_TOOL_PLUS },
	{ 6, ID_DELETERES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_MINUS },
	{ 7, ID_EDITLABEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CHANGE },
	{ 8, ID_CLONE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_CLONE },
	{ -1, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0 },
	{ 13, ID_IMPORT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_IMPORT },
	{ 14, ID_EXPORTRES, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, IDS_TOOL_EXPORT },
};

//////////////////////////////////////////////////////////////////////////////

// create the toolbar
BOOL MMainWnd::CreateOurToolBar(HWND hwndParent, HIMAGELIST himlTools)
{
	// create a toolbar
	HWND hwndTB = CreateWindowW(TOOLBARCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | CCS_TOP | TBSTYLE_LIST | TBSTYLE_TOOLTIPS,
		0, 0, 0, 0, hwndParent, (HMENU)1, GetWindowInstance(hwndParent), NULL);
	if (hwndTB == NULL)
		return FALSE;

	// initialize
	SendMessageW(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessageW(hwndTB, TB_SETBITMAPSIZE, 0, MAKELPARAM(0, 0));
	SendMessageW(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)himlTools);
	SendMessageW(hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);

	ToolBar_StoreStrings(hwndTB, _countof(g_buttons0), g_buttons0);
	ToolBar_StoreStrings(hwndTB, _countof(g_buttons1), g_buttons1);
	ToolBar_StoreStrings(hwndTB, _countof(g_buttons2), g_buttons2);
	ToolBar_StoreStrings(hwndTB, _countof(g_buttons3), g_buttons3);
	ToolBar_StoreStrings(hwndTB, _countof(g_buttons4), g_buttons4);

	m_hToolBar = hwndTB;
	UpdateOurToolBarButtons(3);

	return TRUE;
}

void MMainWnd::UpdateToolBarStatus()
{
	if (TreeView_GetCount(m_hwndTV) == 0)
	{
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_EXPAND_ALL, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_COLLAPSE_ALL, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_EXPORTRES, 0);
	}
	else
	{
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_EXPAND_ALL, TBSTATE_ENABLED);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_COLLAPSE_ALL, TBSTATE_ENABLED);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_EXPORTRES, TBSTATE_ENABLED);
	}

	BOOL bCanEditLabel = TRUE;

	// get the selected entry
	auto entry = g_res.get_entry();
	if (entry && !entry->valid())
		entry = NULL;

    if (bCanEditLabel)
	{
		if (entry)
		{
			if (entry->m_et == ET_NAME || entry->m_et == ET_LANG)
			{
				if (entry->m_type == RT_STRING)
				{
					bCanEditLabel = FALSE;
				}
			}
		}
		else
		{
			bCanEditLabel = FALSE;
		}
	}

	SendMessageW(m_hToolBar, TB_SETSTATE, ID_EDITLABEL, (bCanEditLabel ? TBSTATE_ENABLED : 0));

	if (!entry || !entry->m_hItem)
	{
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_GUIEDIT, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, 0);
		return;
	}

	BOOL bEditable = entry && entry->is_editable(m_szVCBat);
	if (bEditable)
	{
		if (entry->can_gui_edit())
		{
			SendMessageW(m_hToolBar, TB_SETSTATE, ID_GUIEDIT, TBSTATE_ENABLED);
		}
		else
		{
			SendMessageW(m_hToolBar, TB_SETSTATE, ID_GUIEDIT, 0);
		}
	}
	else
	{
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_GUIEDIT, 0);
	}

	switch (entry ? entry->m_et : ET_ANY)
	{
	case ET_TYPE:
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, TBSTATE_ENABLED);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, TBSTATE_ENABLED);
		break;

	case ET_NAME:
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, TBSTATE_ENABLED);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, TBSTATE_ENABLED);
		break;

	case ET_LANG:
		if (entry->is_testable())
		{
			SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, TBSTATE_ENABLED);
		}
		else
		{
			SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, 0);
		}

		SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, TBSTATE_ENABLED);
		if (entry->m_type == RT_STRING)
			SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, 0);
		else
			SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, TBSTATE_ENABLED);
		break;

	case ET_STRING:
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, TBSTATE_ENABLED);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_TEST, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, TBSTATE_ENABLED);
		break;

	default:
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_DELETERES, 0);
		SendMessageW(m_hToolBar, TB_SETSTATE, ID_CLONE, 0);
		break;
	}
}

void MMainWnd::OnAddBang(HWND hwnd, NMTOOLBAR *pToolBar)
{
	if (IsWindow(m_splash))
		SendMessageW(m_splash, WM_CLOSE, 0, 0);

	// TODO: If you edited "Edit" menu, then you may have to update the below codes
	HMENU hMenu = GetMenu(hwnd);
	HMENU hEditMenu = GetSubMenu(hMenu, 1);
	HMENU hAddMenu = GetSubMenu(hEditMenu, 2);

	// the button rectangle
	RECT rcItem = pToolBar->rcButton;

	// get the hot point
	POINT pt;
	pt.x = rcItem.left;
	pt.y = rcItem.bottom;
	ClientToScreen(m_hToolBar, &pt);

	// get the monitor info
	HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	ZeroMemory(&mi, sizeof(mi));
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMon, &mi);

	// by the hot point, change the menu alignment
	UINT uFlags = TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL;
	if (pt.y >= (mi.rcWork.top + mi.rcWork.bottom) / 2)
	{
		uFlags |= TPM_BOTTOMALIGN;
		pt.x = rcItem.left;
		pt.y = rcItem.top;
		ClientToScreen(m_hToolBar, &pt);
	}
	else
	{
		uFlags |= TPM_TOPALIGN;
	}

	// See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648002.aspx
	SetForegroundWindow(m_hwnd);

	// show the popup menu
	TPMPARAMS params;
	ZeroMemory(&params, sizeof(params));
	params.cbSize = sizeof(params);
	params.rcExclude = rcItem;
	TrackPopupMenuEx(hAddMenu, uFlags, pt.x, pt.y, m_hwnd, &params);

	// See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648002.aspx
	SendMessageDx(WM_NULL);
}

// update the toolbar buttons
void MMainWnd::UpdateOurToolBarButtons(INT iType)
{
	// Suppress redraw while replacing buttons to avoid flicker
	SendMessageW(m_hToolBar, WM_SETREDRAW, FALSE, 0);

	if (m_iToolBarType != iType)
	{
		// delete all the buttons of toolbar
		while (SendMessageW(m_hToolBar, TB_DELETEBUTTON, 0, 0))
			;

		// add the buttons
		switch (iType)
		{
		case 0:
			SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons0), (LPARAM)g_buttons0);
			break;
		case 1:
			SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons1), (LPARAM)g_buttons1);
			break;
		case 2:
			SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons2), (LPARAM)g_buttons2);
			break;
		case 3:
			SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons3), (LPARAM)g_buttons3);
			break;
		case 4:
			SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons4), (LPARAM)g_buttons4);
			break;
		}
	}
	m_iToolBarType = iType;

	UpdateToolBarStatus();

	// Re-enable redraw and force a single paint
	SendMessageW(m_hToolBar, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(m_hToolBar, nullptr, FALSE);

	// show/hide the toolbar by settings
	if (g_settings.bShowToolBar)
		ShowWindow(m_hToolBar, SW_SHOWNOACTIVATE);
	else
		ShowWindow(m_hToolBar, SW_HIDE);
}
