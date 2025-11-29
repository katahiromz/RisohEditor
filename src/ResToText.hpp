// ResToText.hpp --- Dumping Resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "MWindowBase.hpp"
#include "MString.hpp"
#include "Res.hpp"

#define MYWM_DELPHI_DFM_B2T (WM_USER + 300)
#define MYWM_TLB_B2T (WM_USER + 301)

HBITMAP CreateBitmapFromIconDx(HICON hIcon, INT width, INT height, BOOL bCursor);
MStringW DumpIconInfo(const BITMAP& bm, BOOL bIcon = TRUE);
MStringW DumpBitmapInfo(HBITMAP hbm);
MStringW DumpBitmapInfo(const BITMAP& bm);
MStringW DumpGroupIconInfo(const std::vector<BYTE>& data);
MStringW DumpGroupCursorInfo(const std::vector<BYTE>& data);

HBITMAP
CreateBitmapFromIconOrPngDx(HWND hwnd, const EntryBase& entry, BITMAP& bm);

HBITMAP
CreateBitmapFromIconsDx(HWND hwnd, const EntryBase& entry);

HBITMAP
CreateBitmapFromCursorDx(HWND hwnd, const EntryBase& entry, BITMAP& bm);

HBITMAP
CreateBitmapFromCursorsDx(HWND hwnd, const EntryBase& entry);

MString GetLanguageStatement(WORD langid, BOOL bOldStyle);

MStringW DumpTextInclude(const EntryBase& entry);

//////////////////////////////////////////////////////////////////////////////

class ResToText
{
public:
	typedef std::vector<BYTE> data_type;

	ResToText() : m_hwnd(NULL), m_hwndDialog(NULL),
				  m_bHumanReadable(TRUE), m_bNoLanguage(FALSE)
	{
	}

	MString DumpEntry(const EntryBase& entry);
	MStringW GetResTypeName(const MIdOrString& type) const;

public:
	HWND m_hwnd;
	HWND m_hwndDialog;
	BOOL m_bHumanReadable;
	BOOL m_bNoLanguage;
	MString m_strFilePrefix;

	MString GetLanguageStatement(WORD langid)
	{
		if (!m_bNoLanguage)
			return ::GetLanguageStatement(langid, TRUE) + TEXT("\r\n");
		else
			return TEXT("");
	}

	MString DoCursor(const EntryBase& entry);
	MString DoBitmap(const EntryBase& entry);
	MString DoIcon(const EntryBase& entry);
	MString DoMenu(const EntryBase& entry);
	MString DoToolbar(const EntryBase& entry);
	MString DoDialog(const EntryBase& entry);
	MString DoString(const EntryBase& entry);
	MString DoAccel(const EntryBase& entry);
	MString DoGroupCursor(const EntryBase& entry);
	MString DoGroupIcon(const EntryBase& entry);
	MString DoVersion(const EntryBase& entry);
	MString DoAniCursor(const EntryBase& entry);
	MString DoAniIcon(const EntryBase& entry);
	MString DoText(const EntryBase& entry);
	MString DoManifest(const EntryBase& entry);
	MString DoImage(const EntryBase& entry);
	MString DoMessage(const EntryBase& entry);
	MString DoWave(const EntryBase& entry);
	MString DoMP3(const EntryBase& entry);
	MString DoAVI(const EntryBase& entry);
	MString DoTypeLib(const EntryBase& entry);
	MString DoDlgInit(const EntryBase& entry);
	MString DoRCData(const EntryBase& entry);
	MString DoTextInclude(const EntryBase& entry);
	MString DoUnknown(const EntryBase& entry);
	MString DoFont(const EntryBase& entry);
	MString DoFontDir(const EntryBase& entry);
	MString DoEncodedText(const EntryBase& entry, const MStringW& enc);

	MString DumpName(const MIdOrString& type, const MIdOrString& name);
	MString DumpEscapedName(const MIdOrString& name);

	MString GetEntryFileName(const EntryBase& entry);
};
