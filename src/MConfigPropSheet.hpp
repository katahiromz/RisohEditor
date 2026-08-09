// MConfigPropSheet.hpp --- "Configuration" property sheet (General + Macros)
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "MPropSheet.hpp"
#include "MConfigDlg.hpp"
#include "MMacrosDlg.hpp"
#include "MPathsDlg.hpp"
#include "MFontsDlg.hpp"
#include "MDfmSettingsDlg.hpp"
#include "MIdAssocDlg.hpp"

//////////////////////////////////////////////////////////////////////////////

enum {
	PAGE_CONFIG,
	PAGE_MACROS,
	PAGE_PATHS,
	PAGE_FONTS,
	PAGE_DFMSETTINGS,
	PAGE_IDASSOC,
};

// Replaces the old standalone "MConfigDlg dialog; dialog.DialogBoxDx(hwnd);"
// call. MConfigDlg's "Edit Macros..." button used to launch MMacrosDlg as a
// second, nested modal dialog; now both live as tabs of one sheet, and OK
// (or Apply) commits both pages via PSN_APPLY -> OnApply().
class MConfigPropSheet : public MPropSheet
{
public:
	MConfigDlg  m_pageConfig;
	MMacrosDlg  m_pageMacros;
	MPathsDlg   m_pagePaths;
	MFontsDlg   m_pageFonts;
	MDfmSettingsDlg m_pageDfmSettings;
	MIdAssocDlg m_pageIdAssoc;

	MConfigPropSheet()
		: MPropSheet(NULL, LoadStringDx(IDS_CONFIG))
	{
		AddPage(m_pageConfig);
		AddPage(m_pageMacros);
		AddPage(m_pagePaths);
		AddPage(m_pageFonts);
		AddPage(m_pageDfmSettings);
		AddPage(m_pageIdAssoc);
	}

	// Mirrors MDialogBase::DialogBoxDx(hwndOwner)'s call shape so callers
	// only need to change the class name at the call site, e.g.:
	//     MConfigPropSheet dialog;
	//     if (dialog.DoModalDx(hwnd) == IDOK) { ... }
	INT_PTR DoModalDx(HWND hwndOwner, INT iPage = PAGE_CONFIG)
	{
		m_psh.hwndParent = hwndOwner;
		SetCurSelDx(iPage);
		return DoSheetDx();
	}
};

//////////////////////////////////////////////////////////////////////////////
