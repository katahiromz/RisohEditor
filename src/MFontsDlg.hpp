// MFontsDlg.hpp --- font settings dialog
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "resource.h"
#include "MWindowBase.hpp"
#include "MPropSheet.hpp"
#include "MHyperLinkCtrl.hpp"
#include "MString.hpp"
#include <commdlg.h>

class MFontsDlg;

//////////////////////////////////////////////////////////////////////////////

class MFontsDlg : public MPropSheetPage
{
public:
	HFONT m_hSrcFont;
	HFONT m_hBinFont;

	MFontsDlg()
		: MPropSheetPage(IDD_FONTS, LoadStringDx(IDS_FONTS))
		, m_hSrcFont(NULL)
		, m_hBinFont(NULL)
	{
	}

	virtual ~MFontsDlg()
	{
		DestroySrcFont();
		DestroyBinFont();
	}

	HFONT DetachSrcFont()
	{
		HFONT hFont = m_hSrcFont;
		m_hSrcFont = NULL;
		return hFont;
	}
	HFONT DetachBinFont()
	{
		HFONT hFont = m_hBinFont;
		m_hBinFont = NULL;
		return hFont;
	}

	void DestroySrcFont()
	{
		if (m_hSrcFont)
		{
			DeleteObject(m_hSrcFont);
			m_hSrcFont = NULL;
		}
	}
	void DestroyBinFont()
	{
		if (m_hBinFont)
		{
			DeleteObject(m_hBinFont);
			m_hBinFont = NULL;
		}
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
	{
		MString str;

		str = g_settings.strSrcFont;
		str += L", ";
		str += mstr_dec_short(g_settings.nSrcFontSize);
		str += L"pt";
		SetDlgItemText(hwnd, edt1, str.c_str());
		m_hSrcFont = CreateFontFromNameAndSize(g_settings.strSrcFont.c_str(), g_settings.nSrcFontSize);
		SetWindowFont(GetDlgItem(hwnd, stc1), m_hSrcFont, TRUE);

		str = g_settings.strBinFont;
		str += L", ";
		str += mstr_dec_short(g_settings.nBinFontSize);
		str += L"pt";
		SetDlgItemText(hwnd, edt2, str.c_str());
		m_hBinFont = CreateFontFromNameAndSize(g_settings.strBinFont.c_str(), g_settings.nBinFontSize);
		SetWindowFont(GetDlgItem(hwnd, stc2), m_hBinFont, TRUE);

		CenterWindowDx();
		return TRUE;
	}

	BOOL ApplyFonts(HWND hwnd)
	{
		MString str1 = GetDlgItemText(edt1);
		MString str2 = GetDlgItemText(edt2);
		mstr_trim(str1);
		mstr_trim(str2);

		size_t k1 = str1.find(L", ");
		size_t k2 = str2.find(L", ");
		if (k1 == MString::npos || k2 == MString::npos)
		{
			return FALSE;
		}

		g_settings.strSrcFont = str1.substr(0, k1);
		g_settings.nSrcFontSize = mstr_parse_int(str1.substr(k1 + 2).c_str());
		DestroySrcFont();
		m_hSrcFont = CreateFontFromNameAndSize(g_settings.strSrcFont.c_str(), g_settings.nSrcFontSize);

		g_settings.strBinFont = str2.substr(0, k2);
		g_settings.nBinFontSize = mstr_parse_int(str2.substr(k2 + 2).c_str());
		DestroyBinFont();
		m_hBinFont = CreateFontFromNameAndSize(g_settings.strBinFont.c_str(), g_settings.nBinFontSize);

		s_pMainWnd->RefreshFonts();
		return TRUE;
	}

	// PSN_APPLY: called by the sheet frame when the user presses OK/Apply.
	BOOL OnApply(HWND hwnd, BOOL bAllPages) override
	{
		return ApplyFonts(hwnd);
	}

	void OnPsh1(HWND hwnd)
	{
		MString str1 = GetDlgItemText(edt1);
		mstr_trim(str1);
		size_t k1 = str1.find(L", ");

		LOGFONT lf;
		ZeroMemory(&lf, sizeof(lf));

		if (k1 != MString::npos)
		{
			StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), str1.substr(0, k1).c_str());
			INT nPointSize = mstr_parse_int(str1.substr(k1 + 2).c_str());

			HDC hDC = CreateCompatibleDC(NULL);
			lf.lfHeight = -MulDiv(nPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
			DeleteDC(hDC);
		}

		CHOOSEFONT cf;
		ZeroMemory(&cf, sizeof(cf));
		cf.lStructSize = sizeof(cf);
		cf.hwndOwner = hwnd;
		cf.lpLogFont = &lf;
		cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL | CF_NOVERTFONTS | CF_SCREENFONTS;
		if (ChooseFont(&cf))
		{
			INT nPointSize = (cf.iPointSize + 5) / 10;

			str1 = lf.lfFaceName;
			str1 += L", ";
			str1 += mstr_dec_short(nPointSize);
			str1 += L"pt";
			SetDlgItemText(hwnd, edt1, str1.c_str());

			DestroySrcFont();
			m_hSrcFont = CreateFontFromNameAndSize(lf.lfFaceName, nPointSize);

			SetWindowFont(GetDlgItem(hwnd, stc1), m_hSrcFont, TRUE);
			SetModifiedDx();
		}
	}

	void OnPsh2(HWND hwnd)
	{
		MString str2 = GetDlgItemText(edt2);
		mstr_trim(str2);
		size_t k2 = str2.find(L", ");

		LOGFONT lf;
		ZeroMemory(&lf, sizeof(lf));

		if (k2 != MString::npos)
		{
			StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), str2.substr(0, k2).c_str());
			INT nPointSize = mstr_parse_int(str2.substr(k2 + 2).c_str());

			HDC hDC = CreateCompatibleDC(NULL);
			lf.lfHeight = -MulDiv(nPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
			DeleteDC(hDC);
		}

		CHOOSEFONT cf;
		ZeroMemory(&cf, sizeof(cf));
		cf.lStructSize = sizeof(cf);
		cf.hwndOwner = hwnd;
		cf.lpLogFont = &lf;
		cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL | CF_NOVERTFONTS | CF_SCREENFONTS;
		if (ChooseFont(&cf))
		{
			INT nPointSize = (cf.iPointSize + 5) / 10;

			str2 = lf.lfFaceName;
			str2 += L", ";
			str2 += mstr_dec_short(nPointSize);
			str2 += L"pt";
			SetDlgItemText(hwnd, edt2, str2.c_str());

			DestroyBinFont();
			m_hBinFont = CreateFontFromNameAndSize(lf.lfFaceName, nPointSize);

			SetWindowFont(GetDlgItem(hwnd, stc2), m_hBinFont, TRUE);
			SetModifiedDx();
		}
	}

	void OnPsh3(HWND hwnd)
	{
		MStringW strSrcFont, strBinFont;
		INT nSrcFontSize, nBinFontSize;
		GetDefaultMonoFonts(strSrcFont, nSrcFontSize,
		                    strBinFont, nBinFontSize);

		DestroySrcFont();
		DestroyBinFont();
		m_hSrcFont = CreateFontFromNameAndSize(strSrcFont.c_str(), nSrcFontSize);
		m_hBinFont = CreateFontFromNameAndSize(strBinFont.c_str(), nBinFontSize);

		MString str = strSrcFont + L", " + mstr_dec_short(nSrcFontSize) + L"pt";
		SetDlgItemText(hwnd, edt1, str.c_str());
		SetWindowFont(GetDlgItem(hwnd, stc1), m_hSrcFont, TRUE);

		str = strBinFont + L", " + mstr_dec_short(nBinFontSize) + L"pt";
		SetDlgItemText(hwnd, edt2, str.c_str());
		SetWindowFont(GetDlgItem(hwnd, stc2), m_hBinFont, TRUE);

		SetModifiedDx();
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case psh1: // Source font
			OnPsh1(hwnd);
			break;
		case psh2: // Binary font
			OnPsh2(hwnd);
			break;
		case psh3: // Reset
			OnPsh3(hwnd);
			break;
		}
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		switch (uMsg)
		{
		HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		case WM_NOTIFY:
			return OnNotify(hwnd, (INT)wParam, (LPNMHDR)lParam);
		default:
			return 0;
		}
	}

protected:
	MHyperLinkCtrl m_hyperlink;
};
