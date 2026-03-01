// MTestDialog --- Test Dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "MWindowBase.hpp"
#include "MenuRes.hpp"
#include "DialogRes.hpp"
#include "DlgInit.h"
#include "Res.hpp"
#include <map>
#include "MOleHost.hpp"

//////////////////////////////////////////////////////////////////////////////

#ifdef ATL_SUPPORT
template <class StringType, class Helper, typename Helper::ReturnType(WINAPI* pFunc)(HINSTANCE, LPCDLGTEMPLATE, HWND, DLGPROC, LPARAM)>
typename Helper::ReturnType AtlAxDialogCreateIndirectT(
	HINSTANCE hInstance,
	const VOID *pTemplate,
	HWND hWndParent,
	DLGPROC lpDialogProc,
	LPARAM dwInitParam)
{
	AtlAxWinInit();
	typename Helper::ReturnType nRet = Helper::GetInvalidValue();

	DLGTEMPLATE* pDlg = (DLGTEMPLATE*)pTemplate;
	if (pDlg != NULL)
	{
		LPDLGTEMPLATE lpDialogTemplate;
		lpDialogTemplate = _DialogSplitHelper::SplitDialogTemplate(pDlg, NULL);
		nRet = (*pFunc)(hInstance, lpDialogTemplate, hWndParent, lpDialogProc, dwInitParam);
		if ((lpDialogTemplate != pDlg) && (lpDialogTemplate != NULL))
			GlobalFree(GlobalHandle(lpDialogTemplate));
	}
	return nRet;
}
#endif

class MTestDialog : public MDialogBase
{
public:
	DialogRes& m_dialog_res;
	MIdOrString m_menu;
	WORD m_lang;
	HMENU m_hMenu;
	std::vector<BYTE> m_dlginit_data;
	MTitleToBitmap  m_title_to_bitmap;
	MTitleToIcon    m_title_to_icon;
	INT             m_xDialogBaseUnit;
	INT             m_yDialogBaseUnit;
	MOleHost *m_pOleHost;

	MTestDialog(DialogRes& dialog_res, MIdOrString menu, WORD lang, const std::vector<BYTE>& dlginit_data)
		: m_dialog_res(dialog_res), m_menu(menu),
		  m_lang(lang), m_hMenu(NULL), m_dlginit_data(dlginit_data),
		  m_pOleHost(NULL)
	{
		m_xDialogBaseUnit = LOWORD(GetDialogBaseUnits());
		m_yDialogBaseUnit = HIWORD(GetDialogBaseUnits());
	}

	BOOL GetBaseUnits(INT& xDialogBaseUnit, INT& yDialogBaseUnit)
	{
		m_xDialogBaseUnit = m_dialog_res.GetBaseUnits(m_yDialogBaseUnit);
		if (m_xDialogBaseUnit == 0)
		{
			return FALSE;
		}

		xDialogBaseUnit = m_xDialogBaseUnit;
		yDialogBaseUnit = m_yDialogBaseUnit;

		return TRUE;
	}

	virtual ~MTestDialog()
	{
		if (m_hMenu)
		{
			DestroyMenu(m_hMenu);
			m_hMenu = NULL;
		}
		clear_maps();
	}

	void clear_maps()
	{
		for (auto& pair : m_title_to_bitmap)
		{
			DeleteObject(pair.second);
		}
		m_title_to_bitmap.clear();

		for (auto& pair : m_title_to_icon)
		{
			DestroyIcon(pair.second);
		}
		m_title_to_icon.clear();
	}

	void create_maps(WORD lang)
	{
		for (size_t i = 0; i < m_dialog_res.size(); ++i)
		{
			DialogItem& item = m_dialog_res[i];
			if (item.m_class == 0x0082 ||
				lstrcmpiW(item.m_class.c_str(), L"STATIC") == 0)
			{
				// static
				if ((item.m_style & SS_TYPEMASK) == SS_ICON)
				{
					g_res.do_icon(m_title_to_icon, item, lang);
				}
				else if ((item.m_style & SS_TYPEMASK) == SS_BITMAP)
				{
					g_res.do_bitmap(m_title_to_bitmap, item, lang);
				}
			}
		}
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		if (m_pOleHost)
			m_pOleHost->DoEnableMenu(TRUE);

		if (m_hMenu)
		{
			SetMenu(hwnd, NULL);
			DestroyMenu(m_hMenu);
			m_hMenu = NULL;
		}
		if (!m_menu.empty())
		{
			if (auto entry = g_res.find(ET_LANG, RT_MENU, m_menu, m_lang))
			{
				m_hMenu = LoadMenuIndirect(&(*entry)[0]);
				SetMenu(hwnd, m_hMenu);

				INT cyMenu = GetSystemMetrics(SM_CYMENU);
				RECT rc;
				GetWindowRect(hwnd, &rc);
				rc.bottom += cyMenu;
				SIZE siz = SizeFromRectDx(&rc);
				SetWindowPosDx(NULL, &siz);
			}
		}

		create_maps(0xFFFF);

		GetBaseUnits(m_xDialogBaseUnit, m_yDialogBaseUnit);

		INT i = 0;
		for (HWND hCtrl = GetTopWindow(hwnd);
			 hCtrl; hCtrl = GetNextWindow(hCtrl, GW_HWNDNEXT))
		{
			DWORD style = GetWindowStyle(hCtrl);
			SIZE siz = { 0, 0 };
			GetWindowPosDx(hCtrl, NULL, &siz);

			WCHAR szClass[MAX_PATH];
			GetClassNameW(hCtrl, szClass, _countof(szClass));
			if (lstrcmpiW(szClass, L"STATIC") == 0)
			{
				MIdOrString title = m_dialog_res[i].m_title;
				if ((style & SS_TYPEMASK) == SS_ICON)
				{
					HICON hIcon = m_title_to_icon[title];
					SendMessage(hCtrl, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
					if (style & SS_REALSIZEIMAGE)
					{
						ICONINFO info;
						GetIconInfo(hIcon, &info);
						BITMAP bm;
						GetObject(info.hbmColor, sizeof(BITMAP), &bm);
						siz.cx = bm.bmWidth;
						siz.cy = bm.bmHeight;
					}
					else if (style & SS_REALSIZECONTROL)
					{
						siz.cx = m_dialog_res[i].m_siz.cx * m_xDialogBaseUnit / 4;
						siz.cy = m_dialog_res[i].m_siz.cy * m_yDialogBaseUnit / 8;
					}
					SetWindowPosDx(hCtrl, NULL, &siz);
				}
				else if ((style & SS_TYPEMASK) == SS_BITMAP)
				{
					HBITMAP hbm = m_title_to_bitmap[title];
					SendMessage(hCtrl, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
					if (style & SS_REALSIZECONTROL)
					{
						siz.cx = m_dialog_res[i].m_siz.cx * m_xDialogBaseUnit / 4;
						siz.cy = m_dialog_res[i].m_siz.cy * m_yDialogBaseUnit / 8;
					}
					else
					{
						BITMAP bm;
						GetObject(hbm, sizeof(BITMAP), &bm);
						siz.cx = bm.bmWidth;
						siz.cy = bm.bmHeight;
					}
					SetWindowPosDx(hCtrl, NULL, &siz);
				}
			}

			++i;
		}

		if (m_bModal)
			CenterWindowDx();

		if (m_dlginit_data.size())
		{
			SIZE_T siz = m_dlginit_data.size();
			ExecuteDlgInitDataDx(hwnd, &m_dlginit_data[0], siz);
		}
		return TRUE;
	}

	void Destroy(HWND hwnd)
	{
		PostMessage(GetParent(hwnd), WM_COMMAND, ID_CHILDDESTROYED, 0);
		if (m_bModal)
			EndDialog(IDOK);
		else
			DestroyWindow(*this);

		if (m_pOleHost)
		{
			delete m_pOleHost;
			m_pOleHost = NULL;
		}
	}

	virtual INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		DWORD dwStyle;
		switch (uMsg)
		{
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
		case WM_LBUTTONDOWN:
			dwStyle = GetWindowStyle(hwnd);
			if (!(dwStyle & WS_SYSMENU))
			{
				Destroy(hwnd);
			}
			break;
		case WM_LBUTTONDBLCLK:
			Destroy(hwnd);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDOK: case IDCANCEL:
				Destroy(hwnd);
				break;
			}
			break;
		}
		return DefaultProcDx();
	}

	INT_PTR DialogBoxIndirectDx(HWND hwndOwner, const VOID* ptr)
	{
		if (hwndOwner)
		{
			m_hwndOwner = hwndOwner;
		}

		if (m_pOleHost)
		{
			delete m_pOleHost;
			m_pOleHost = NULL;
		}
		m_pOleHost = new MOleHost();
		DoSetActiveOleHost(m_pOleHost);

		m_bModal = TRUE;
#ifdef ATL_SUPPORT
		INT_PTR nID = AtlAxDialogCreateIndirectT<LPCWSTR, _AtlDialogBoxIndirectParamHelper, ::DialogBoxIndirectParamW>(
			::GetModuleHandle(NULL), ptr, m_hwndOwner, MDialogBase::DialogProc, reinterpret_cast<LPARAM>(this));
#else
		INT_PTR nID = ::DialogBoxIndirectParam(::GetModuleHandle(NULL),
			(const DLGTEMPLATE*)ptr,
			m_hwndOwner,
			MDialogBase::DialogProc,
			reinterpret_cast<LPARAM>(this));
#endif
		if (m_pOleHost)
		{
			delete m_pOleHost;
			m_pOleHost = NULL;
		}
		return nID;
	}

	inline BOOL CreateDialogIndirectDx(HWND hwndOwner, const VOID *ptr)
	{
		if (hwndOwner)
		{
			m_hwndOwner = hwndOwner;
		}

		if (m_pOleHost)
		{
			delete m_pOleHost;
			m_pOleHost = NULL;
		}
		m_pOleHost = new MOleHost();
		DoSetActiveOleHost(m_pOleHost);

		m_bModal = FALSE;
#ifdef ATL_SUPPORT
		HWND hwnd;
		hwnd = AtlAxDialogCreateIndirectT<LPCWSTR, _AtlCreateDialogIndirectParamHelper, ::CreateDialogIndirectParamW>(
			::GetModuleHandle(NULL), ptr, m_hwndOwner, MDialogBase::DialogProc, reinterpret_cast<LPARAM>(this));
#else
		HWND hwnd = ::CreateDialogIndirectParam(::GetModuleHandle(NULL),
			reinterpret_cast<const DLGTEMPLATE *>(ptr),
			m_hwndOwner, MDialogBase::DialogProc,
			reinterpret_cast<LPARAM>(this));
#endif
		if (hwnd == NULL)
		{
			Detach();
		}
		return hwnd != NULL;
	}
};
