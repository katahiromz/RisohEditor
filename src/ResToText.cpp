// ResToText.cpp --- Dumping Resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "resource.h"
#include "ResToText.hpp"
#include "MTextToText.hpp"
#include "ConstantsDB.hpp"
#include "settings.h"
#include "MStrBin.hpp"

#include "DialogRes.hpp"
#include "MenuRes.hpp"
#include "VersionRes.hpp"
#include "StringRes.hpp"
#include "AccelRes.hpp"
#include "DlgInitRes.hpp"
#include "MessageRes.hpp"
#include "ToolbarRes.hpp"

#define SUPPORT_OLD_ENVIRONMENTS TRUE // Borland C++ is still alive in some places

static inline MStringW generate_begin(void)
{
	return g_settings.bUseBeginEnd ? L"BEGIN" : L"{";
}
static inline MStringW generate_end(void)
{
	return g_settings.bUseBeginEnd ? L"END" : L"}";
}

BOOL ResToText::GetEntryFileNameEx(const EntryBase& entry, MStringW& str)
{
	BOOL ret = FALSE;
	if (entry.m_type.is_int())
	{
		WORD wType = entry.m_type.m_id;
		if (wType == (WORD)(UINT_PTR)RT_CURSOR)
		{
			// No output file
		}
		else if (wType == (WORD)(UINT_PTR)RT_BITMAP)
		{
			str += L"Bitmap_";
			str += DumpEscapedName(entry.m_name);
			str += L".bmp";
			ret = TRUE;
		}
		else if (wType == (WORD)(UINT_PTR)RT_ICON)
		{
			// No output file
		}
		else if (wType == (WORD)(UINT_PTR)RT_MENU)
		{
			str += L"Menu_";
			str += DumpEscapedName(entry.m_name);
			str += L".bin";
			ret = FALSE; // No output file
		}
		else if (wType == (WORD)(UINT_PTR)RT_TOOLBAR)
		{
			str += L"Toolbar_";
			str += DumpEscapedName(entry.m_name);
			str += L".bin";
			ret = FALSE; // No output file
		}
		else if (wType == (WORD)(UINT_PTR)RT_DIALOG)
		{
			str += L"Dialog_";
			str += DumpEscapedName(entry.m_name);
			str += L".bin";
			ret = FALSE; // No output file
		}
		else if (wType == (WORD)(UINT_PTR)RT_STRING)
		{
			// No output file
		}
		else if (wType == (WORD)(UINT_PTR)RT_FONTDIR)
		{
			// No output file
		}
		else if (wType == (WORD)(UINT_PTR)RT_FONT)
		{
			if (entry.m_data.size() < 4)
			{
				// No output file
			}
			else
			{
				if (memcmp(&entry.m_data[0], "OTTO", 4) == 0)
				{
					// OpenType
					str += L"Font_";
					str += DumpEscapedName(entry.m_name);
					str += L".otf";
					ret = TRUE;
				}
				else if (memcmp(&entry.m_data[0], "\x00\x01\x00\x00", 4) == 0)
				{
					// TrueType
					str += L"Font_";
					str += DumpEscapedName(entry.m_name);
					str += L".ttf";
					ret = TRUE;
				}
				else if (memcmp(&entry.m_data[0], "ttcf", 4) == 0)
				{
					// TrueType Collection
					str += L"Font_";
					str += DumpEscapedName(entry.m_name);
					str += L".ttc";
					ret = TRUE;
				}
				else
				{
					// otherwise
					str += L"Font_";
					str += DumpEscapedName(entry.m_name);
					str += L".fon";
					ret = TRUE;
				}
			}
		}
		else if (wType == (WORD)(UINT_PTR)RT_ACCELERATOR)
		{
			str += L"Accel_";
			str += DumpEscapedName(entry.m_name);
			str += L".bin";
			ret = FALSE; // No output file
		}
		else if (wType == (WORD)(UINT_PTR)RT_RCDATA)
		{
			if (entry.is_delphi_dfm())
			{
				str += L"Delphi_";
				str += DumpEscapedName(entry.m_name);
				str += L".dfm";
				ret = TRUE;
			}
			else
			{
				str += L"RCData_";
				str += DumpEscapedName(entry.m_name);
				str += L".bin";
				ret = TRUE;
			}
		}
		else if (wType == (WORD)(UINT_PTR)RT_MESSAGETABLE)
		{
			if (g_settings.bUseMSMSGTABLE)
			{
				str += L"MsgTable_";
				str += DumpEscapedName(entry.m_name);
				str += L".bin";
				ret = TRUE;
			}
			else
			{
				// No output file
			}
		}
		else if (wType == (WORD)(UINT_PTR)RT_GROUP_CURSOR)
		{
			str += L"Cursor_";
			str += DumpEscapedName(entry.m_name);
			str += L".cur";
			ret = TRUE;
		}
		else if (wType == (WORD)(UINT_PTR)RT_GROUP_ICON)
		{
			str += L"Icon_";
			str += DumpEscapedName(entry.m_name);
			str += L".ico";
			ret = TRUE;
		}
		else if (wType == (WORD)(UINT_PTR)RT_VERSION)
		{
			str += L"Version_";
			str += DumpEscapedName(entry.m_name);
			str += L".bin";
			ret = FALSE; // No output file
		}
		else if (wType == (WORD)(UINT_PTR)RT_DLGINCLUDE)
		{
			// TODO:
		}
		else if (wType == (WORD)(UINT_PTR)RT_PLUGPLAY)
		{
			// TODO:
		}
		else if (wType == (WORD)(UINT_PTR)RT_VXD)
		{
			// TODO:
		}
		else if (wType == (WORD)(UINT_PTR)RT_ANICURSOR)
		{
			str += L"AniCursor_";
			str += DumpEscapedName(entry.m_name);
			str += L".ani";
			ret = TRUE;
		}
		else if (wType == (WORD)(UINT_PTR)RT_ANIICON)
		{
			str += L"AniIcon_";
			str += DumpEscapedName(entry.m_name);
			str += L".ani";
			ret = TRUE;
		}
		else if (wType == (WORD)(UINT_PTR)RT_HTML)
		{
			str += L"Html_";
			str += DumpEscapedName(entry.m_name);
			str += L".html";
			ret = TRUE;
		}
		else if (wType == (WORD)(UINT_PTR)RT_MANIFEST)
		{
			str += L"Manifest_";
			str += DumpEscapedName(entry.m_name);
			str += L".manifest";
			ret = TRUE;
		}
		else
		{
			str += entry.m_type.str(true);
			str += L"_";
			str += DumpEscapedName(entry.m_name);
			str += L".bin";
			ret = TRUE;
		}
	}
	else
	{
		if (entry.m_type == L"AVI")
		{
			str += L"Avi_";
			str += DumpEscapedName(entry.m_name);
			str += L".avi";
			ret = TRUE;
		}
		else if (entry.m_type == L"PNG")
		{
			str += L"Png_";
			str += DumpEscapedName(entry.m_name);
			str += L".png";
			ret = TRUE;
		}
		else if (entry.m_type == L"GIF")
		{
			str += L"Gif_";
			str += DumpEscapedName(entry.m_name);
			str += L".gif";
			ret = TRUE;
		}
		else if (entry.m_type == L"JPEG")
		{
			str += L"Jpeg_";
			str += DumpEscapedName(entry.m_name);
			str += L".jpg";
			ret = TRUE;
		}
		else if (entry.m_type == L"JPG")
		{
			str += L"Jpg_";
			str += DumpEscapedName(entry.m_name);
			str += L".jpg";
			ret = TRUE;
		}
		else if (entry.m_type == L"TIFF")
		{
			str += L"Tiff_";
			str += DumpEscapedName(entry.m_name);
			str += L".tif";
			ret = TRUE;
		}
		else if (entry.m_type == L"TIF")
		{
			str += L"Tif_";
			str += DumpEscapedName(entry.m_name);
			str += L".tif";
			ret = TRUE;
		}
		else if (entry.m_type == L"EMF")
		{
			str += L"Emf_";
			str += DumpEscapedName(entry.m_name);
			str += L".emf";
			ret = TRUE;
		}
		else if (entry.m_type == L"ENHMETAFILE")
		{
			str += L"EnhMetaFile_";
			str += DumpEscapedName(entry.m_name);
			str += L".emf";
			ret = TRUE;
		}
		else if (entry.m_type == L"ENHMETAPICT")
		{
			str += L"EnhMetaPict_";
			str += DumpEscapedName(entry.m_name);
			str += L".emf";
			ret = TRUE;
		}
		else if (entry.m_type == L"WMF")
		{
			str += L"Wmf_";
			str += DumpEscapedName(entry.m_name);
			str += L".wmf";
			ret = TRUE;
		}
		else if (entry.m_type == L"WAVE")
		{
			str += L"Wave_";
			str += DumpEscapedName(entry.m_name);
			str += L".wav";
			ret = TRUE;
		}
		else if (entry.m_type == L"MP3")
		{
			str += L"MP3_";
			str += DumpEscapedName(entry.m_name);
			str += L".mp3";
			ret = TRUE;
		}
		else if (entry.m_type == L"TYPELIB")
		{
			str += L"TYPELIB_";
			str += DumpEscapedName(entry.m_name);
			str += L".tlb";
			ret = TRUE;
		}
		else if (entry.m_type == L"TEXTINCLUDE")
		{
			// No output file
		}
		else if (entry.m_type == L"IMAGE")
		{
			if (entry.m_et == ET_LANG)
			{
				if (entry.size() >= 4)
				{
					if (memcmp(&entry[0], "BM", 2) == 0)
					{
						str += L"Image_";
						str += DumpEscapedName(entry.m_name);
						str += L".bmp";
						ret = TRUE;
					}
					else if (memcmp(&entry[0], "GIF", 3) == 0)
					{
						str += L"Image_";
						str += DumpEscapedName(entry.m_name);
						str += L".gif";
						ret = TRUE;
					}
					else if (memcmp(&entry[0], "\x89\x50\x4E\x47", 4) == 0)
					{
						str += L"Image_";
						str += DumpEscapedName(entry.m_name);
						str += L".png";
						ret = TRUE;
					}
					else if (memcmp(&entry[0], "\xFF\xD8", 2) == 0)
					{
						str += L"Image_";
						str += DumpEscapedName(entry.m_name);
						str += L".jpg";
						ret = TRUE;
					}
					else if (memcmp(&entry[0], "\x4D\x4D", 2) == 0 ||
							 memcmp(&entry[0], "\x49\x49", 2) == 0)
					{
						str += L"Image_";
						str += DumpEscapedName(entry.m_name);
						str += L".tif";
						ret = TRUE;
					}
				}
			}
		}
		else
		{
			str += entry.m_type.str(true);
			str += L"_";
			str += DumpEscapedName(entry.m_name);
			str += L".bin";
			ret = TRUE;
		}
	}

	if (str.size())
	{
		if (entry.m_lang != 0 && entry.m_lang != BAD_LANG)
		{
			WCHAR sz[MAX_PATH];
			StringCchPrintfW(sz, _countof(sz), L"%u_", entry.m_lang);
			str = sz + str;
		}
		str = m_strFilePrefix + str;
	}

	return ret;
}

MStringW
ResToText::GetEntryFileName(const EntryBase& entry)
{
	MStringW strFileName;
	if (!GetEntryFileNameEx(entry, strFileName))
		return L"";
	return strFileName;
}

MString
ResToText::DoCursor(const EntryBase& entry)
{
	MString str;

	if (m_bHumanReadable)
	{
		BITMAP bm = {};
		HCURSOR hCursor = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, FALSE);
		HBITMAP hbm = CreateBitmapFromIconDx(hCursor, bm.bmWidth, bm.bmHeight, TRUE);
		str += DumpIconInfo(bm, FALSE);
		DestroyCursor(hCursor);
		DeleteObject(hbm);

		str += L"\r\n";
	}

	return str;
}

MString
ResToText::DoBitmap(const EntryBase& entry)
{
	BITMAP bm;
	PackedDIB_GetInfo(&entry[0], entry.size(), bm);
	MString str;

	if (m_bHumanReadable)
	{
		str += DumpBitmapInfo(bm);
		str += L"\r\n";
	}

	// LANGUAGE ..., ...
	str += GetLanguageStatement(entry.m_lang);

	str += DumpName(entry.m_type, entry.m_name);
	str += L" BITMAP \"";
	str += GetEntryFileName(entry);
	str += L"\"\r\n\r\n";

	return str;
}

MString
ResToText::DoIcon(const EntryBase& entry)
{
	MString str;

	if (m_bHumanReadable)
	{
		BITMAP bm = {};
		HBITMAP hbm = CreateBitmapFromIconOrPngDx(m_hwnd, entry, bm);

		HICON hIcon = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, TRUE);
		if (hIcon)
		{
			str += DumpIconInfo(bm, TRUE);
		}
		else
		{
			str += DumpBitmapInfo(hbm);
		}
		DestroyIcon(hIcon);
		DeleteObject(hbm);

		str += L"\r\n";
	}

	return str;
}

MString
ResToText::DoMenu(const EntryBase& entry)
{
	MByteStreamEx stream(entry.m_data);
	MenuRes menu_res;
	if (menu_res.LoadFromStream(stream))
	{
		MString str = GetLanguageStatement(entry.m_lang);
		str += menu_res.Dump(entry.m_name);
		str += L"\r\n";
		return str;
	}
	return L"";
}

MString
ResToText::DoToolbar(const EntryBase& entry)
{
	MByteStreamEx stream(entry.m_data);
	ToolbarRes toolbar_res;
	if (toolbar_res.LoadFromStream(stream))
	{
		MString str = GetLanguageStatement(entry.m_lang);
		str += toolbar_res.Dump(entry.m_name);
		str += L"\r\n";
		return str;
	}
	return L"";
}

MString
ResToText::DoDialog(const EntryBase& entry)
{
	MByteStreamEx stream(entry.m_data);
	DialogRes dialog_res;
	if (dialog_res.LoadFromStream(stream))
	{
		MString str = GetLanguageStatement(entry.m_lang);
		str += dialog_res.Dump(entry.m_name, !!g_settings.bAlwaysControl);
		str += L"\r\n";
		return str;
	}
	return L"";
}

MString
ResToText::DoString(const EntryBase& entry)
{
	EntrySet found;
	g_res.search(found, ET_LANG, RT_STRING, entry.m_name, entry.m_lang);

	StringRes str_res;
	for (auto e : found)
	{
		MByteStreamEx stream(e->m_data);
		if (!str_res.LoadFromStream(stream, e->m_name.m_id))
			return L"";
	}

	MString str;
	if (entry.m_name.empty())
		str += GetLanguageStatement(entry.m_lang);
	str += str_res.Dump();
	str += L"\r\n\r\n";
	return str;
}

MString
ResToText::DoMessage(const EntryBase& entry)
{
	EntrySet found;
	g_res.search(found, ET_LANG, RT_MESSAGETABLE, entry.m_name, entry.m_lang);

	MIdOrString name = entry.m_name;
	MessageRes msg_res;
	for (auto e : found)
	{
		MByteStreamEx stream(e->m_data);
		if (!msg_res.LoadFromStream(stream))
			return L"";
		name = e->m_name;
		break;
	}

	MString str;

	// LANGUAGE ..., ...
	str += GetLanguageStatement(entry.m_lang);

	if (g_settings.bUseMSMSGTABLE)
	{
		str += DumpName(entry.m_type, name);
		str += L" MESSAGETABLE \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	else
	{
		str += L"#ifdef MCDX_INVOKED\r\n";
		str += msg_res.Dump(entry.m_name);
		str += L"#endif\r\n\r\n";
	}

	return str;
}

MString
ResToText::DoAccel(const EntryBase& entry)
{
	MByteStreamEx stream(entry.m_data);
	AccelRes accel;
	if (accel.LoadFromStream(stream))
	{
		MString str = GetLanguageStatement(entry.m_lang);
		str += accel.Dump(entry.m_name);
		str += L"\r\n\r\n";
		return str;
	}
	return L"";
}

MString
ResToText::DoGroupCursor(const EntryBase& entry)
{
	MStringW str;

	if (m_bHumanReadable)
	{
		str += DumpGroupCursorInfo(entry.m_data);
		str += L"\r\n";
	}

	// LANGUAGE ..., ...
	str += GetLanguageStatement(entry.m_lang);

	str += DumpName(entry.m_type, entry.m_name);
	str += L" CURSOR \"";
	str += GetEntryFileName(entry);
	str += L"\"\r\n\r\n";

	return str;
}

MString
ResToText::DoGroupIcon(const EntryBase& entry)
{
	MStringW str;

	if (m_bHumanReadable)
	{
		str += DumpGroupIconInfo(entry.m_data);
		str += L"\r\n";
	}

	// LANGUAGE ..., ...
	str += GetLanguageStatement(entry.m_lang);

	str += DumpName(entry.m_type, entry.m_name);
	str += L" ICON \"";
	str += GetEntryFileName(entry);
	str += L"\"\r\n\r\n";

	return str;
}

MString
ResToText::DoVersion(const EntryBase& entry)
{
	VersionRes ver_res;
	if (ver_res.LoadFromData(entry.m_data))
	{
		MString str = GetLanguageStatement(entry.m_lang);
		str += ver_res.Dump(entry.m_name);
		str += L"\r\n";
		return str;
	}
	return L"";
}

MString
ResToText::DoAniCursor(const EntryBase& entry)
{
	MString str;

	if (m_bHumanReadable)
	{
		str += LoadStringDx(IDS_ANICURSOR);
		str += L"\r\n";
	}

	// LANGUAGE ..., ...
	str += GetLanguageStatement(entry.m_lang);

	str += DumpName(entry.m_type, entry.m_name);
	str += L' ';
	str += (SUPPORT_OLD_ENVIRONMENTS ? L"21" : L"ANICURSOR");
	//static_assert((INT_PTR)RT_ANICURSOR == 21, "");
	str += L" \"";
	str += GetEntryFileName(entry);
	str += L"\"\r\n\r\n";

	return str;
}

MString
ResToText::DoAniIcon(const EntryBase& entry)
{
	MString str;

	if (m_bHumanReadable)
	{
		str += LoadStringDx(IDS_ANIICON);
		str += L"\r\n";
	}

	// LANGUAGE ..., ...
	str += GetLanguageStatement(entry.m_lang);

	str += DumpName(entry.m_type, entry.m_name);
	str += L' ';
	str += (SUPPORT_OLD_ENVIRONMENTS ? L"22" : L"ANIICON");
	//static_assert((INT_PTR)RT_ANIICON == 22, "");
	str += L" \"";
	str += GetEntryFileName(entry);
	str += L"\"\r\n\r\n";

	return str;
}

MString
ResToText::DoText(const EntryBase& entry)
{
	MString str;
	if (m_bHumanReadable)
	{
		MTextType type;
		type.nNewLine = MNEWLINE_CRLF;
		if (entry.size())
		{
			str = mstr_from_bin(&entry.m_data[0], entry.m_data.size(), &type);
		}
	}
	else
	{
		str += GetLanguageStatement(entry.m_lang);
		str += DumpName(entry.m_type, entry.m_name);
		str += L" ";
		str += DumpEscapedName(entry.m_type);
		str += L" \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	return str;
}

MString
ResToText::DoManifest(const EntryBase& entry)
{
	MString str;
	if (m_bHumanReadable)
	{
		MTextType type;
		type.nNewLine = MNEWLINE_CRLF;
		if (entry.size())
		{
			str = mstr_from_bin(&entry.m_data[0], entry.m_data.size(), &type);
		}
	}
	else
	{
		if (g_settings.bWrapManifest)
		{
			str += L"#ifndef MSVC\r\n";
		}
		str += GetLanguageStatement(entry.m_lang);
		str += DumpName(entry.m_type, entry.m_name);
		str += L' ';
		str += (SUPPORT_OLD_ENVIRONMENTS ? L"24" : L"MANIFEST");
		//static_assert((INT_PTR)RT_MANIFEST == 24, "");
		str += L" \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n";
		if (g_settings.bWrapManifest)
		{
			str += L"#endif\r\n";
		}
		str += L"\r\n";
	}
	return str;
}

MString
ResToText::DoImage(const EntryBase& entry)
{
	MString str;

	if (m_bHumanReadable)
	{
		HBITMAP hbm = nullptr;

		MBitmapDx bitmap;
		if (bitmap.CreateFromMemory(&entry[0], entry.size()))
		{
			LONG cx, cy;
			hbm = bitmap.GetHBITMAP(cx, cy);
		}

		str += DumpBitmapInfo(hbm);
		DeleteObject(hbm);

		str += L"\r\n";
	}

	// LANGUAGE ..., ...
	str += GetLanguageStatement(entry.m_lang);

	if (entry.m_type == L"PNG")
	{
		str += DumpName(entry.m_type, entry.m_name);
		str += L" PNG \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	else if (entry.m_type == L"GIF")
	{
		str += DumpName(entry.m_type, entry.m_name);
		str += L" GIF \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	else if (entry.m_type == L"JPEG")
	{
		str += DumpName(entry.m_type, entry.m_name);
		str += L" JPEG \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	else if (entry.m_type == L"JPG")
	{
		str += DumpName(entry.m_type, entry.m_name);
		str += L" JPG \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	else if (entry.m_type == L"TIFF")
	{
		str += DumpName(entry.m_type, entry.m_name);
		str += L" TIFF \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	else if (entry.m_type == L"TIF")
	{
		str += DumpName(entry.m_type, entry.m_name);
		str += L" TIF \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	else if (entry.m_type == L"EMF")
	{
		str += DumpName(entry.m_type, entry.m_name);
		str += L" EMF \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	else if (entry.m_type == L"ENHMETAFILE")
	{
		str += DumpName(entry.m_type, entry.m_name);
		str += L" ENHMETAFILE \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	else if (entry.m_type == L"ENHMETAPICT")
	{
		str += DumpName(entry.m_type, entry.m_name);
		str += L" ENHMETAPICT \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	else if (entry.m_type == L"WMF")
	{
		str += DumpName(entry.m_type, entry.m_name);
		str += L" WMF \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	else if (entry.m_type == L"IMAGE")
	{
		if (entry.size() >= 4)
		{
			if (memcmp(&entry[0], "BM", 2) == 0)
			{
				str += DumpName(entry.m_type, entry.m_name);
				str += L" IMAGE \"";
				str += GetEntryFileName(entry);
				str += L"\"\r\n\r\n";
			}
			else if (memcmp(&entry[0], "GIF", 3) == 0)
			{
				str += DumpName(entry.m_type, entry.m_name);
				str += L" IMAGE \"";
				str += GetEntryFileName(entry);
				str += L"\"\r\n\r\n";
			}
			else if (memcmp(&entry[0], "\x89\x50\x4E\x47", 4) == 0)
			{
				str += DumpName(entry.m_type, entry.m_name);
				str += L" IMAGE \"";
				str += GetEntryFileName(entry);
				str += L"\"\r\n\r\n";
			}
			else if (memcmp(&entry[0], "\xFF\xD8", 2) == 0)
			{
				str += DumpName(entry.m_type, entry.m_name);
				str += L" IMAGE \"";
				str += GetEntryFileName(entry);
				str += L"\"\r\n\r\n";
			}
			else if (memcmp(&entry[0], "\x4D\x4D", 2) == 0 ||
					 memcmp(&entry[0], "\x49\x49", 2) == 0)
			{
				str += DumpName(entry.m_type, entry.m_name);
				str += L" IMAGE \"";
				str += GetEntryFileName(entry);
				str += L"\"\r\n\r\n";
			}
		}
	}

	return str;
}

MString
ResToText::DumpEntry(const EntryBase& entry)
{
	if (entry.m_type.m_id)
	{
		switch (entry.m_type.m_id)
		{
		case 1: // RT_CURSOR
			return DoCursor(entry);
		case 2: // RT_BITMAP
			return DoBitmap(entry);
		case 3: // RT_ICON
			return DoIcon(entry);
		case 4: // RT_MENU
			return DoMenu(entry);
		case 5: // RT_DIALOG
			return DoDialog(entry);
		case 6: // RT_STRING
			return DoString(entry);
		case 7: // RT_FONTDIR
			return DoFontDir(entry);
		case 8: // RT_FONT
			return DoFont(entry);
		case 9: // RT_ACCELERATOR
			return DoAccel(entry);
		case 10: // RT_RCDATA
			return DoRCData(entry);
		case 11: // RT_MESSAGETABLE
			return DoMessage(entry);
		case 12: // RT_GROUP_CURSOR
			return DoGroupCursor(entry);
		case 14: // RT_GROUP_ICON
			return DoGroupIcon(entry);
		case 16: // RT_VERSION
			return DoVersion(entry);
		case 17: // RT_DLGINCLUDE
			break;
		case 19: // RT_PLUGPLAY
			break;
		case 20: // RT_VXD
			break;
		case 21: // RT_ANICURSOR
			return DoAniCursor(entry);
		case 22: // RT_ANIICON
			return DoAniIcon(entry);
		case 23: // RT_HTML
			return DoText(entry);
		case 24: // RT_MANIFEST
			return DoManifest(entry);
		case 240: // RT_DLGINIT
			return DoDlgInit(entry);
		case 241: // RT_TOOLBAR
			return DoToolbar(entry);
		default:
			return DoUnknown(entry);
		}
	}
	else
	{
		MString type = entry.m_type.m_str;
		if (type == L"PNG" || type == L"GIF" ||
			type == L"JPEG" || type == L"TIFF" ||
			type == L"JPG" || type == L"TIF" ||
			type == L"EMF" || type == L"ENHMETAFILE" ||
			type == L"ENHMETAPICT" || type == L"WMF" || type == L"IMAGE")
		{
			return DoImage(entry);
		}
		else if (type == L"WAVE")
		{
			return DoWave(entry);
		}
		else if (type == L"MP3")
		{
			return DoMP3(entry);
		}
		else if (entry.m_type == L"AVI")
		{
			return DoAVI(entry);
		}
		else if (entry.m_type == L"TYPELIB")
		{
			return DoTypeLib(entry);
		}
		else if (entry.m_type == L"TEXTINCLUDE")
		{
			return DoTextInclude(entry);
		}
	}
	return DoUnknown(entry);
}

MStringW
ResToText::GetResTypeName(const MIdOrString& type) const
{
	if (type.m_id)
	{
		switch (type.m_id)
		{
		case 1: return L"RT_CURSOR";
		case 2: return L"RT_BITMAP";
		case 3: return L"RT_ICON";
		case 4: return L"RT_MENU";
		case 5: return L"RT_DIALOG";
		case 6: return L"RT_STRING";
		case 7: return L"RT_FONTDIR";
		case 8: return L"RT_FONT";
		case 9: return L"RT_ACCELERATOR";
		case 10: return L"RT_RCDATA";
		case 11: return L"RT_MESSAGETABLE";
		case 12: return L"RT_GROUP_CURSOR";
		case 14: return L"RT_GROUP_ICON";
		case 16: return L"RT_VERSION";
		case 17: return L"RT_DLGINCLUDE";
		case 19: return L"RT_PLUGPLAY";
		case 20: return L"RT_VXD";
		case 21: return L"RT_ANICURSOR";
		case 22: return L"RT_ANIICON";
		case 23: return L"RT_HTML";
		case 24: return L"RT_MANIFEST";
		case 240: return L"RT_DLGINIT";
		case 241: return L"RT_TOOLBAR";
		}
	}
	return type.str(true);
}

MString ResToText::DoWave(const EntryBase& entry)
{
	MString str;

	if (m_bHumanReadable)
	{
		str += LoadStringDx(IDS_WAVESOUND);
		str += L"\r\n";
	}

	// LANGUAGE ..., ...
	str += GetLanguageStatement(entry.m_lang);

	str += DumpName(entry.m_type, entry.m_name);
	str += L" WAVE \"";
	str += GetEntryFileName(entry);
	str += L"\"\r\n\r\n";

	return str;
}

MString ResToText::DoMP3(const EntryBase& entry)
{
	MString str;

	if (m_bHumanReadable)
	{
		str += LoadStringDx(IDS_MP3SOUND);
		str += L"\r\n";
	}

	// LANGUAGE ..., ...
	str += GetLanguageStatement(entry.m_lang);

	str += DumpName(entry.m_type, entry.m_name);
	str += L" MP3 \"";
	str += GetEntryFileName(entry);
	str += L"\"\r\n\r\n";

	return str;
}

MString ResToText::DoAVI(const EntryBase& entry)
{
	MString str;

	if (m_bHumanReadable)
	{
		str += LoadStringDx(IDS_AVIMOVIE);
		str += L"\r\n";
	}

	// LANGUAGE ..., ...
	str += GetLanguageStatement(entry.m_lang);

	str += DumpName(entry.m_type, entry.m_name);
	str += L" AVI \"";
	str += GetEntryFileName(entry);
	str += L"\"\r\n\r\n";

	return str;
}

MString ResToText::DoTextInclude(const EntryBase& entry)
{
	MStringW str;

	str += GetLanguageStatement(entry.m_lang);
	str += DumpTextInclude(entry);

	return str;
}

MString ResToText::DoDlgInit(const EntryBase& entry)
{
	MStringW str;

	MByteStreamEx stream(entry.m_data);
	DlgInitRes dlginit;
	if (dlginit.LoadFromStream(stream))
	{
		str += GetLanguageStatement(entry.m_lang);
		str += dlginit.Dump(entry.m_name);
	}

	return str;
}

MString ResToText::DoTypeLib(const EntryBase& entry)
{
	MString str;

	if (m_bHumanReadable)
	{
		SendMessageW(m_hwnd, MYWM_TLB_B2T, (WPARAM)&str, (LPARAM)&entry);
		if (str.size())
		{
			return str;
		}
		return L"";
	}
	else
	{
		// LANGUAGE ..., ...
		str += GetLanguageStatement(entry.m_lang);

		str += DumpName(entry.m_type, entry.m_name);
		str += L" TYPELIB \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}

	return str;
}

MString ResToText::DoRCData(const EntryBase& entry)
{
	MString str;

	if (m_bHumanReadable && entry.is_delphi_dfm())
	{
		SendMessageW(m_hwnd, MYWM_DELPHI_DFM_B2T, (WPARAM)&str, (LPARAM)&entry);
		if (str.size())
		{
			return str;
		}
		return L"";
	}
	else
	{
		// LANGUAGE ..., ...
		str += GetLanguageStatement(entry.m_lang);

		str += DumpName(entry.m_type, entry.m_name);
		str += L" RCDATA \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}

	return str;
}

MString ResToText::DoEncodedText(const EntryBase& entry, const MStringW& enc)
{
	if (entry.m_data.empty())
		return L"";

	MString str;
	if (m_bHumanReadable)
	{
		if (enc == L"ansi")
		{
			std::string str(reinterpret_cast<const char *>(&entry.m_data[0]),
							entry.m_data.size());
			MAnsiToWide a2w(CP_ACP, str.c_str());
			MStringW wide = a2w.c_str();
			mstr_replace_all(wide, L"\r\n", L"\n");
			mstr_replace_all(wide, L"\n", L"\r\n");
			return wide;
		}
		if (enc == L"wide" || enc == L"utf16n" || enc == L"utf16")
		{
			std::wstring str(reinterpret_cast<const wchar_t *>(&entry.m_data[0]),
							 entry.m_data.size() / sizeof(wchar_t));
			mstr_replace_all(str, L"\r\n", L"\n");
			mstr_replace_all(str, L"\n", L"\r\n");
			if (str.size() && str[0] == 0xFEFF)
				str.erase(0, 1);
			return str;
		}
		if (enc == L"utf8" || enc == L"utf8n")
		{
			std::string str(reinterpret_cast<const char *>(&entry.m_data[0]),
							entry.m_data.size());
			if (str.size() >= 3 && memcmp(str.c_str(), "\xEF\xBB\xBF", 3) == 0)
			{
				str.erase(0, 3);
			}
			MAnsiToWide a2w(CP_UTF8, str.c_str());
			MStringW wide = a2w.c_str();
			mstr_replace_all(wide, L"\r\n", L"\n");
			mstr_replace_all(wide, L"\n", L"\r\n");
			return wide;
		}
		if (enc == L"sjis")
		{
			std::string str((char *)&entry.m_data[0], entry.m_data.size());
			MAnsiToWide a2w(932, str.c_str());
			MStringW wide = a2w.c_str();
			mstr_replace_all(wide, L"\r\n", L"\n");
			mstr_replace_all(wide, L"\n", L"\r\n");
			return wide;
		}
	}
	else
	{
		str += GetLanguageStatement(entry.m_lang);
		str += DumpName(entry.m_type, entry.m_name);
		str += L" ";
		str += DumpEscapedName(entry.m_type);
		str += L" \"";
		str += GetEntryFileName(entry);
		str += L"\"\r\n\r\n";
	}
	return str;
}

MString ResToText::DoUnknown(const EntryBase& entry)
{
	MStringW GetResTypeEncoding(const MIdOrString& type);

	MStringW enc = GetResTypeEncoding(entry.m_type);
	if (enc.size())
	{
		return DoEncodedText(entry, enc);
	}

	MString str;
	if (entry.m_et != ET_LANG)
		return str;

	if (m_bHumanReadable)
	{
		str += LoadStringDx(IDS_UNKNOWNFORMAT);
		str += L"\r\n";
	}

	// LANGUAGE ..., ...
	str += GetLanguageStatement(entry.m_lang);

	str += DumpName(entry.m_type, entry.m_name);
	str += L" ";
	str += entry.m_type.str(true);
	str += L" \"";
	str += GetEntryFileName(entry);
	str += L"\"\r\n\r\n";

	return str;
}

MString ResToText::DoFont(const EntryBase& entry)
{
	MStringW GetResTypeEncoding(const MIdOrString& type);

	MStringW enc = GetResTypeEncoding(entry.m_type);
	if (enc.size())
	{
		return DoEncodedText(entry, enc);
	}

	MString str;
	if (entry.m_et != ET_LANG)
		return str;

	// LANGUAGE ..., ...
	str += GetLanguageStatement(entry.m_lang);

	str += DumpName(entry.m_type, entry.m_name);
	str += L" FONT \"";
	str += GetEntryFileName(entry);
	str += L"\"\r\n\r\n";

	return str;
}

MString ResToText::DoFontDir(const EntryBase& entry)
{
	MString str;

	if (!m_bHumanReadable)
	{
		return str;
	}

	size_t size = entry.m_data.size();
	const BYTE *pb = (const BYTE *)&entry.m_data[0];

	if (size <= sizeof(WORD))
	{
		return str;
	}

	WORD wCount = *(const WORD *)pb;

	TCHAR szText[MAX_PATH];
	StringCbPrintf(szText, sizeof(szText), TEXT("Count: %u\r\n---\r\n"), wCount);
	str += szText;

#define FONTDIRENTRYSIZE 165
	if (size < sizeof(WORD) + FONTDIRENTRYSIZE * wCount)
	{
		// NOTE: I think windres RT_FONTDIR is broken. Just ignore it.
		return str;
	}

	pb += 2;
	size -= 2;
	for (UINT i = 0; i < wCount; ++i)
	{
		StringCbPrintf(szText, sizeof(szText), TEXT("Font #%u: Ordinal %u ("),
					   i, *(const WORD *)pb);
		str += szText;

		if (size >= 2 + 4 && memcmp(&pb[2], "OTTO", 4) == 0)
		{
			// OpenType
			str += TEXT("OpenType");
		}
		else if (size >= 2 + 4 && memcmp(&pb[2], "\x00\x01\x00\x00", 4) == 0)
		{
			// TrueType
			str += TEXT("TrueType");
		}
		else if (size >= 2 + 4 && memcmp(&pb[2], "ttcf", 4) == 0)
		{
			// TrueType Collection
			str += TEXT("TrueType Collection");
		}
		else
		{
			// otherwise
			str += TEXT("WinFNT");
		}

		str += TEXT(")\r\n");
		pb += FONTDIRENTRYSIZE;
		size -= FONTDIRENTRYSIZE;
	}

	return str;
#undef FONTDIRENTRYSIZE
}

MString ResToText::DumpName(const MIdOrString& type, const MIdOrString& name)
{
	MString ret;
	if (name.is_str())
		ret = name.str();
	else
		ret = g_db.GetNameOfResID(g_db.IDTypeFromResType(type), name.m_id);
	return ret;
}

MString ResToText::DumpEscapedName(const MIdOrString& name)
{
	MString ret = name.str(true);
	return mstr_escape(ret);
}

//////////////////////////////////////////////////////////////////////////////

HBITMAP
CreateBitmapFromIconDx(HICON hIcon, INT width, INT height, BOOL bCursor)
{
	if (!hIcon)
		return nullptr;
	HBITMAP hbm = Create24BppBitmapDx(width, height);
	if (hbm == nullptr)
	{
		assert(0);
		return nullptr;
	}
	HBRUSH hbr = GetStockBrush(LTGRAY_BRUSH);
	FillBitmapDx(hbm, hbr);

	HDC hDC = CreateCompatibleDC(nullptr);
	HGDIOBJ hbmOld = SelectObject(hDC, hbm);
	{
		DrawIconEx(hDC, 0, 0, hIcon, width, height, 0, hbr, DI_NORMAL);
	}
	SelectObject(hDC, hbmOld);
	// NOTE: hbr is a stock brush obtained via GetStockBrush, so it should NOT be deleted.
	DeleteDC(hDC);

	return hbm;
}

MStringW
DumpBitmapInfo(HBITMAP hbm)
{
	MStringW ret;
	BITMAP bm;
	if (!GetObjectW(hbm, sizeof(bm), &bm))
		return ret;

	ret = LoadStringPrintfDx(IDS_IMAGEINFO, bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
	return ret;
}

MStringW
DumpBitmapInfo(const BITMAP& bm)
{
	MStringW ret = LoadStringPrintfDx(IDS_IMAGEINFO, bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
	return ret;
}

MStringW
DumpIconInfo(const BITMAP& bm, BOOL bIcon/* = TRUE*/)
{
	MStringW ret;

	ret = LoadStringPrintfDx(IDS_IMAGEINFO, bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);

	return ret;
}

MStringW
DumpGroupIconInfo(const std::vector<BYTE>& data)
{
	MStringW ret;

	ICONDIR dir;
	if (data.size() < sizeof(dir))
		return ret;

	memcpy(&dir, &data[0], sizeof(dir));

	if (dir.idReserved != 0 || dir.idType != 1 || dir.idCount == 0)
	{
		return ret;
	}

	ret += LoadStringPrintfDx(IDS_IMAGECOUNT, dir.idCount);
	ret += L"-------\r\n";

	const GRPICONDIRENTRY *pEntries;
	pEntries = (const GRPICONDIRENTRY *)&data[sizeof(dir)];

	for (UINT i = 0; i < dir.idCount; ++i)
	{
		WORD Width = pEntries[i].bWidth;
		WORD Height = pEntries[i].bHeight;
		WORD nID = pEntries[i].nID;

		if (Width == 0)
			Width = 256;
		if (Height == 0)
			Height = 256;

		ret += LoadStringPrintfDx(IDS_ICONINFO,
			i, Width, Height, pEntries[i].wBitCount, nID);
	}

	return ret;
}

MStringW
DumpGroupCursorInfo(const std::vector<BYTE>& data)
{
	MStringW ret;

	ICONDIR dir;
	if (data.size() < sizeof(dir))
		return ret;

	memcpy(&dir, &data[0], sizeof(dir));

	if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
	{
		return ret;
	}

	ret += LoadStringPrintfDx(IDS_IMAGECOUNT, dir.idCount);
	ret += L"-------\r\n";

	const GRPCURSORDIRENTRY *pEntries;
	pEntries = (const GRPCURSORDIRENTRY *)&data[sizeof(dir)];

	for (UINT i = 0; i < dir.idCount; ++i)
	{
		WORD Width = pEntries[i].wWidth;
		WORD Height = pEntries[i].wHeight / 2;
		WORD BitCount = pEntries[i].wBitCount;
		WORD nID = pEntries[i].nID;
		WORD xHotSpot = 0;
		WORD yHotSpot = 0;

		if (auto entry = g_res.find(ET_LANG, RT_CURSOR, nID))
		{
			auto& cursor_entry = (EntryBase&)*entry;
			LOCALHEADER header;
			if (cursor_entry.size() >= sizeof(header))
			{
				memcpy(&header, &cursor_entry[0], sizeof(header));
				xHotSpot = header.xHotSpot;
				yHotSpot = header.yHotSpot;
			}
		}

		if (Width == 0)
			Width = 256;
		if (Height == 0)
			Height = 256;

		ret += LoadStringPrintfDx(IDS_CURSORINFO,
			i, Width, Height, BitCount, xHotSpot, yHotSpot, nID);
	}

	return ret;
}

HBITMAP
CreateBitmapFromIconOrPngDx(HWND hwnd, const EntryBase& entry, BITMAP& bm)
{
	HBITMAP hbmIcon;

	if (entry.size() >= 4 &&
		memcmp(&entry[0], "\x89\x50\x4E\x47", 4) == 0)
	{
		MBitmapDx bitmap;
		bitmap.CreateFromMemory(&entry[0], entry.size());
		LONG cx, cy;
		hbmIcon = bitmap.GetHBITMAP32(cx, cy);
	}
	else
	{
		HICON hIcon;
		BITMAP bm;
		hIcon = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, TRUE);
		hbmIcon = CreateBitmapFromIconDx(hIcon,
										 bm.bmWidth, bm.bmHeight, FALSE);
		DestroyIcon(hIcon);
	}

	GetObject(hbmIcon, sizeof(bm), &bm);
	if (bm.bmBitsPixel == 32)
	{
		PremultiplyDx(hbmIcon);
	}

	return hbmIcon;
}

void
DrawBitmapDx(HBITMAP hbm, HBITMAP hbmSrc, INT x, INT y)
{
	BITMAP bmSrc;
	GetObject(hbmSrc, sizeof(bmSrc), &bmSrc);

	HDC hDC = CreateCompatibleDC(nullptr);
	HDC hDC2 = CreateCompatibleDC(nullptr);
	{
		HGDIOBJ hbmOld = SelectObject(hDC, hbm);
		HGDIOBJ hbm2Old = SelectObject(hDC2, hbmSrc);
		if (bmSrc.bmBitsPixel == 32)
		{
			BLENDFUNCTION bf;
			bf.BlendOp = AC_SRC_OVER;
			bf.BlendFlags = 0;
			bf.SourceConstantAlpha = 0xFF;
			bf.AlphaFormat = AC_SRC_ALPHA;
			AlphaBlend(hDC, x, y, bmSrc.bmWidth, bmSrc.bmHeight,
					   hDC2, 0, 0, bmSrc.bmWidth, bmSrc.bmHeight, bf);
		}
		else
		{
			BitBlt(hDC, x, y, bmSrc.bmWidth, bmSrc.bmHeight,
				   hDC2, 0, 0, SRCCOPY);
		}
		SelectObject(hDC, hbm2Old);
		SelectObject(hDC, hbmOld);
	}
	DeleteDC(hDC2);
	DeleteDC(hDC);
}

HBITMAP
CreateBitmapFromIconsDx(HWND hwnd, const EntryBase& entry)
{
	ICONDIR dir;
	if (entry.size() < sizeof(dir))
	{
		assert(0);
		return nullptr;
	}

	memcpy(&dir, &entry[0], sizeof(dir));

	if (dir.idReserved != 0 || dir.idType != RES_ICON || dir.idCount == 0)
	{
		return nullptr;
	}

	if (entry.size() < sizeof(dir) + dir.idCount * sizeof(GRPICONDIRENTRY))
	{
		assert(0);
		return nullptr;
	}

	const GRPICONDIRENTRY *pEntries;
	pEntries = (const GRPICONDIRENTRY *)&entry[sizeof(dir)];

	LONG cx = 0, cy = 0;
	for (UINT i = 0; i < dir.idCount; ++i)
	{
		auto e = g_res.find(ET_LANG, RT_ICON, pEntries[i].nID, entry.m_lang);
		if (!e)
		{
			e = g_res.find(ET_LANG, RT_ICON, pEntries[i].nID, BAD_LANG);
		}
		if (!e)
		{
			assert(0);
			return nullptr;
		}

		auto& icon_entry = (EntryBase&)*e;

		BITMAP bm;
		HBITMAP hbmIcon = CreateBitmapFromIconOrPngDx(hwnd, icon_entry, bm);

		if (cx < bm.bmWidth)
			cx = bm.bmWidth;
		cy += bm.bmHeight;

		DeleteObject(hbmIcon);
	}

	HBITMAP hbm = Create24BppBitmapDx(cx, cy);
	if (hbm == nullptr)
	{
		assert(0);
		return nullptr;
	}

	HBRUSH hbr = GetStockBrush(LTGRAY_BRUSH);
	FillBitmapDx(hbm, hbr);
	// NOTE: hbr is a stock brush obtained via GetStockBrush, so it should NOT be deleted.

	BITMAP bm;
	GetObject(hbm, sizeof(bm), &bm);

	INT y = 0;
	for (UINT i = 0; i < dir.idCount; ++i)
	{
		auto e = g_res.find(ET_LANG, RT_ICON, pEntries[i].nID, entry.m_lang);
		if (!e)
		{
			e = g_res.find(ET_LANG, RT_ICON, pEntries[i].nID, BAD_LANG);
		}
		if (!e)
		{
			DeleteObject(hbm);
			return nullptr;
		}
		auto icon_entry = (EntryBase&)*e;

		HBITMAP hbmIcon = CreateBitmapFromIconOrPngDx(hwnd, icon_entry, bm);
		DrawBitmapDx(hbm, hbmIcon, 0, y);
		DeleteObject(hbmIcon);

		y += bm.bmHeight;
	}

	return hbm;
}

HBITMAP
CreateBitmapFromCursorDx(HWND hwnd, const EntryBase& entry, BITMAP& bm)
{
	HBITMAP hbmCursor;

	HICON hCursor;
	hCursor = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, FALSE);
	assert(hCursor);
	hbmCursor = CreateBitmapFromIconDx(hCursor, bm.bmWidth, bm.bmHeight, TRUE);
	DestroyCursor(hCursor);

	GetObject(hbmCursor, sizeof(bm), &bm);
	assert(hbmCursor);
	return hbmCursor;
}

HBITMAP
CreateBitmapFromCursorsDx(HWND hwnd, const EntryBase& entry)
{
	ICONDIR dir;
	if (entry.size() < sizeof(dir))
	{
		assert(0);
		return nullptr;
	}

	memcpy(&dir, &entry[0], sizeof(dir));

	if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
	{
		return nullptr;
	}

	if (entry.size() < sizeof(dir) + dir.idCount * sizeof(GRPCURSORDIRENTRY))
	{
		assert(0);
		return nullptr;
	}

	const GRPCURSORDIRENTRY *pEntries;
	pEntries = (const GRPCURSORDIRENTRY *)&entry[sizeof(dir)];

	LONG cx = 0, cy = 0;
	for (UINT i = 0; i < dir.idCount; ++i)
	{
		auto e = g_res.find(ET_LANG, RT_CURSOR, pEntries[i].nID, entry.m_lang);
		if (!e)
		{
			e = g_res.find(ET_LANG, RT_CURSOR, pEntries[i].nID, BAD_LANG);
		}
		if (!e)
		{
			assert(0);
			return nullptr;
		}
		auto cursor_entry = (EntryBase&)*e;

		BITMAP bm;
		HBITMAP hbmCursor = CreateBitmapFromCursorDx(hwnd, cursor_entry, bm);
		assert(hbmCursor);
		assert(bm.bmWidth);
		assert(bm.bmHeight);

		if (cx < bm.bmWidth)
			cx = bm.bmWidth;
		cy += bm.bmHeight;

		DeleteObject(hbmCursor);
	}

	HBITMAP hbm = Create24BppBitmapDx(cx, cy);
	if (hbm == nullptr)
	{
		assert(0);
		return nullptr;
	}

	HBRUSH hbr = GetStockBrush(LTGRAY_BRUSH);
	FillBitmapDx(hbm, hbr);
	// NOTE: hbr is a stock brush obtained via GetStockBrush, so it should NOT be deleted.

	HDC hDC = CreateCompatibleDC(nullptr);
	HDC hDC2 = CreateCompatibleDC(nullptr);
	HGDIOBJ hbmOld = SelectObject(hDC, hbm);
	{
		INT y = 0;
		for (UINT i = 0; i < dir.idCount; ++i)
		{
			auto e = g_res.find(ET_LANG, RT_CURSOR, pEntries[i].nID, entry.m_lang);
			if (!e)
			{
				e = g_res.find(ET_LANG, RT_CURSOR, pEntries[i].nID, BAD_LANG);
			}
			if (!e)
			{
				assert(0);
				DeleteObject(hbm);
				hbm = nullptr;
				break;
			}
			auto& cursor_entry = (EntryBase&)*e;

			BITMAP bm;
			HBITMAP hbmCursor = CreateBitmapFromCursorDx(hwnd, cursor_entry, bm);
			assert(hbmCursor);
			assert(bm.bmWidth);
			assert(bm.bmHeight);
			{
				HGDIOBJ hbm2Old = SelectObject(hDC2, hbmCursor);
				BitBlt(hDC, 0, y, bm.bmWidth, bm.bmHeight, hDC2, 0, 0, SRCCOPY);
				SelectObject(hDC2, hbm2Old);
			}
			DeleteObject(hbmCursor);

			y += bm.bmHeight;
		}
	}
	SelectObject(hDC, hbmOld);
	DeleteDC(hDC2);
	DeleteDC(hDC);

	return hbm;
}

MStringW DumpTextInclude(const EntryBase& entry)
{
	MStringW str;
	MStringA data(entry.m_data.begin(), entry.m_data.end());

	while (data.size() && data[data.size() - 1] == 0)
	{
		data = data.substr(0, data.size() - 1);
	}

	MStringA nul;
	nul.append(1, 0);

	switch (entry.m_name.m_id)
	{
	case 1:
		str += entry.m_name.str(true);
		str += L" TEXTINCLUDE\r\n";
		str += generate_begin() + L"\r\n";
		str += L"    ";
		data += nul;
		{
			MAnsiToWide a2w(CP_UTF8, data.c_str(), data.size());
			str += mstr_quote(a2w.str());
		}
		str += L"\r\n";
		str += generate_end() + L"\r\n";
		break;
	default:
		str += entry.m_name.str(true);
		str += L" TEXTINCLUDE\r\n";
		str += generate_begin() + L"\r\n";
		{
			std::vector<MStringA> lines;
			mstr_split(lines, data, "\n");
			if (lines.size() && lines[lines.size() - 1].empty())
			{
				lines.resize(lines.size() - 1);
			}
			for (auto& line : lines)
			{
				// Remove trailing NUL, '\r', '\n'
				while (line.size() &&
					(line[line.size() - 1] == 0 ||
					 line[line.size() - 1] == '\r' ||
					 line[line.size() - 1] == '\n'))
				{
					line = line.substr(0, line.size() - 1);
				}

				str += L"    ";
				{
					MAnsiToWide a2w(CP_UTF8, line.c_str(), line.size());
					str += mstr_quote(a2w.str() + L"\r\n");
				}
				str += L"\r\n";
			}
		}
		str += L"    \"\\0\"\r\n";
		str += generate_end() + L"\r\n";
		break;
	}

	return str;
}
