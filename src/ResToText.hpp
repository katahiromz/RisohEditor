// ResToText.hpp --- Dumping Resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, 
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#ifndef RES_TO_TEXT_HPP_
#define RES_TO_TEXT_HPP_

#include "MWindowBase.hpp"
#include "RisohSettings.hpp"
#include "ConstantsDB.hpp"
#include "MString.hpp"
#include "MTextToText.hpp"
#include "Res.hpp"

#include "DialogRes.hpp"
#include "MenuRes.hpp"
#include "VersionRes.hpp"
#include "StringRes.hpp"
#include "AccelRes.hpp"
#include "DlgInitRes.hpp"

#include "resource.h"

HBITMAP CreateBitmapFromIconDx(HICON hIcon, INT width, INT height, BOOL bCursor);
MStringW DumpIconInfo(const BITMAP& bm, BOOL bIcon = TRUE);
MStringW DumpBitmapInfo(HBITMAP hbm);
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
    MString DoAVI(const EntryBase& entry);
    MString DoDlgInit(const EntryBase& entry);
    MString DoRCData(const EntryBase& entry);
    MString DoUnknown(const EntryBase& entry);
    MString DoFont(const EntryBase& entry);
    MString DoFontDir(const EntryBase& entry);
    MString DoEncodedText(const EntryBase& entry, const MStringW& enc);

    MString DumpName(const MIdOrString& type, const MIdOrString& name);
    MString DumpEscapedName(const MIdOrString& name);

    MString GetEntryFileName(const EntryBase& entry);
};

//////////////////////////////////////////////////////////////////////////////

inline MString
ResToText::GetEntryFileName(const EntryBase& entry)
{
    MString ret, lang;

    if (entry.m_type.is_int())
    {
        WORD wType = entry.m_type.m_id;
        if (wType == (WORD)(UINT_PTR)RT_CURSOR)
        {
            // No output file
        }
        else if (wType == (WORD)(UINT_PTR)RT_BITMAP)
        {
            ret += L"Bitmap_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".bmp";
        }
        else if (wType == (WORD)(UINT_PTR)RT_ICON)
        {
            // No output file
        }
        else if (wType == (WORD)(UINT_PTR)RT_MENU)
        {
            // No output file
        }
        else if (wType == (WORD)(UINT_PTR)RT_DIALOG)
        {
            // No output file
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
                    ret += L"Font_";
                    ret += DumpEscapedName(entry.m_name);
                    ret += L".otf";
                }
                else if (memcmp(&entry.m_data[0], "\x00\x01\x00\x00", 4) == 0)
                {
                    // TrueType
                    ret += L"Font_";
                    ret += DumpEscapedName(entry.m_name);
                    ret += L".ttf";
                }
                else if (memcmp(&entry.m_data[0], "ttcf", 4) == 0)
                {
                    // TrueType Collection
                    ret += L"Font_";
                    ret += DumpEscapedName(entry.m_name);
                    ret += L".ttc";
                }
                else
                {
                    // otherwise
                    ret += L"Font_";
                    ret += DumpEscapedName(entry.m_name);
                    ret += L".fon";
                }
            }
        }
        else if (wType == (WORD)(UINT_PTR)RT_ACCELERATOR)
        {
            // No output file
        }
        else if (wType == (WORD)(UINT_PTR)RT_RCDATA)
        {
            ret += L"RCData_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".bin";
        }
        else if (wType == (WORD)(UINT_PTR)RT_MESSAGETABLE)
        {
            // No output file
        }
        else if (wType == (WORD)(UINT_PTR)RT_GROUP_CURSOR)
        {
            ret += L"Cursor_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".cur";
        }
        else if (wType == (WORD)(UINT_PTR)RT_GROUP_ICON)
        {
            ret += L"Icon_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".ico";
        }
        else if (wType == (WORD)(UINT_PTR)RT_VERSION)
        {
            // No output file
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
            ret += L"AniCursor_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".ani";
        }
        else if (wType == (WORD)(UINT_PTR)RT_ANIICON)
        {
            ret += L"AniIcon_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".ani";
        }
        else if (wType == (WORD)(UINT_PTR)RT_HTML)
        {
            ret += L"Html_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".html";
        }
        else if (wType == (WORD)(UINT_PTR)RT_MANIFEST)
        {
            ret += L"Manifest_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".manifest";
        }
        else if (wType == (WORD)(UINT_PTR)RT_FONT)
        {
            ret += L"Font_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".fon";
        }
        else
        {
            ret += entry.m_type.str();
            ret += L"_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".bin";
        }
    }
    else
    {
        if (entry.m_type == L"AVI")
        {
            ret += L"Avi_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".avi";
        }
        else if (entry.m_type == L"PNG")
        {
            ret += L"Png_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".png";
        }
        else if (entry.m_type == L"GIF")
        {
            ret += L"Gif_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".gif";
        }
        else if (entry.m_type == L"JPEG")
        {
            ret += L"Jpeg_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".jpg";
        }
        else if (entry.m_type == L"JPG")
        {
            ret += L"Jpg_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".jpg";
        }
        else if (entry.m_type == L"TIFF")
        {
            ret += L"Tiff_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".tif";
        }
        else if (entry.m_type == L"TIF")
        {
            ret += L"Tif_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".tif";
        }
        else if (entry.m_type == L"EMF")
        {
            ret += L"Emf_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".emf";
        }
        else if (entry.m_type == L"ENHMETAFILE")
        {
            ret += L"EnhMetaFile_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".emf";
        }
        else if (entry.m_type == L"WMF")
        {
            ret += L"Wmf_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".wmf";
        }
        else if (entry.m_type == L"WAVE")
        {
            ret += L"Wave_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".wav";
        }
        else if (entry.m_type == L"IMAGE")
        {
            if (entry.m_et == ET_LANG)
            {
                if (entry.size() >= 4)
                {
                    if (memcmp(&entry[0], "BM", 2) == 0)
                    {
                        ret += L"Image_";
                        ret += DumpEscapedName(entry.m_name);
                        ret += L".bmp";
                    }
                    else if (memcmp(&entry[0], "GIF", 3) == 0)
                    {
                        ret += L"Image_";
                        ret += DumpEscapedName(entry.m_name);
                        ret += L".gif";
                    }
                    else if (memcmp(&entry[0], "\x89\x50\x4E\x47", 4) == 0)
                    {
                        ret += L"Image_";
                        ret += DumpEscapedName(entry.m_name);
                        ret += L".png";
                    }
                    else if (memcmp(&entry[0], "\xFF\xD8", 2) == 0)
                    {
                        ret += L"Image_";
                        ret += DumpEscapedName(entry.m_name);
                        ret += L".jpg";
                    }
                    else if (memcmp(&entry[0], "\x4D\x4D", 2) == 0 ||
                             memcmp(&entry[0], "\x49\x49", 2) == 0)
                    {
                        ret += L"Image_";
                        ret += DumpEscapedName(entry.m_name);
                        ret += L".tif";
                    }
                }
            }
        }
        else
        {
            ret += entry.m_type.str();
            ret += L"_";
            ret += DumpEscapedName(entry.m_name);
            ret += L".bin";
        }
    }

    if (ret.size())
    {
        if (entry.m_lang != 0 && entry.m_lang != BAD_LANG)
        {
            WCHAR sz[32];
            StringCchPrintfW(sz, _countof(sz), L"%u_", entry.m_lang);
            ret = sz + ret;
        }
        ret = m_strFilePrefix + ret;
    }

    return ret;
}

inline MString
ResToText::DoCursor(const EntryBase& entry)
{
    MString str;

    if (m_bHumanReadable)
    {
        BITMAP bm;
        HCURSOR hCursor = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, FALSE);
        HBITMAP hbm = CreateBitmapFromIconDx(hCursor, bm.bmWidth, bm.bmHeight, TRUE);
        str += DumpIconInfo(bm, FALSE);
        DestroyCursor(hCursor);
        DeleteObject(hbm);

        str += L"\r\n";
    }

    return str;
}

inline MString
ResToText::DoBitmap(const EntryBase& entry)
{
    HBITMAP hbm = PackedDIB_CreateBitmap(&entry[0], entry.size());
    MString str;

    if (m_bHumanReadable)
    {
        str += DumpBitmapInfo(hbm);
        str += L"\r\n";
    }

    // LANGUAGE ..., ...
    str += GetLanguageStatement(entry.m_lang);

    str += DumpName(entry.m_type, entry.m_name);
    str += L" BITMAP \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    DeleteObject(hbm);
    return str;
}

inline MString
ResToText::DoIcon(const EntryBase& entry)
{
    MString str;

    if (m_bHumanReadable)
    {
        BITMAP bm;
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

inline MString
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
    return LoadStringDx(IDS_INVALIDDATA);
}

inline MString
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
    return LoadStringDx(IDS_INVALIDDATA);
}

inline MString
ResToText::DoString(const EntryBase& entry)
{
    EntrySet::super_type found;
    g_res.search(found, ET_LANG, RT_STRING, entry.m_name, entry.m_lang);

    StringRes str_res;
    for (auto e : found)
    {
        MByteStreamEx stream(e->m_data);
        if (!str_res.LoadFromStream(stream, e->m_name.m_id))
            return LoadStringDx(IDS_INVALIDDATA);
    }

    MString str;
    if (entry.m_name.empty())
        str += GetLanguageStatement(entry.m_lang);
    str += str_res.Dump();
    str += L"\r\n\r\n";
    return str;
}

inline MString
ResToText::DoMessage(const EntryBase& entry)
{
    EntrySet::super_type found;
    g_res.search(found, ET_LANG, RT_MESSAGETABLE, entry.m_name, entry.m_lang);

    MessageRes msg_res;
    for (auto e : found)
    {
        MByteStreamEx stream(entry.m_data);
        if (!msg_res.LoadFromStream(stream, entry.m_name.m_id))
            return LoadStringDx(IDS_INVALIDDATA);
    }

    MString str;
    if (entry.m_name.empty())
        str += GetLanguageStatement(entry.m_lang);

    str += L"#ifdef MCDX_INVOKED\r\n";
    str += msg_res.Dump();
    str += L"#endif\r\n\r\n";
    return str;
}

inline MString
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
    return LoadStringDx(IDS_INVALIDDATA);
}

inline MString
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

inline MString
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

inline MString
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
    return LoadStringDx(IDS_INVALIDDATA);
}

inline MString
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
    str += L" ANICURSOR \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    return str;
}

inline MString
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
    str += L" ANIICON \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    return str;
}

inline MString
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

inline MString
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
        str += L" ";
        str += DumpEscapedName(entry.m_type);
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

inline MString
ResToText::DoImage(const EntryBase& entry)
{
    MString str;

    if (m_bHumanReadable)
    {
        HBITMAP hbm = NULL;

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

inline MString
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
            type == L"EMF" || type == L"ENHMETAFILE" || type == L"WMF" ||
            type == L"IMAGE")
        {
            return DoImage(entry);
        }
        else if (type == L"WAVE")
        {
            return DoWave(entry);
        }
        else if (entry.m_type == L"AVI")
        {
            return DoAVI(entry);
        }
    }
    return DoUnknown(entry);
}

inline MStringW
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
        }
    }
    return type.str();
}

inline MString ResToText::DoWave(const EntryBase& entry)
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

inline MString ResToText::DoAVI(const EntryBase& entry)
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

inline MString ResToText::DoRCData(const EntryBase& entry)
{
    MString str;

    // LANGUAGE ..., ...
    str += GetLanguageStatement(entry.m_lang);

    str += DumpName(entry.m_type, entry.m_name);
    str += L" RCDATA \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    return str;
}

inline MString ResToText::DoEncodedText(const EntryBase& entry, const MStringW& enc)
{
    MString str;
    if (m_bHumanReadable)
    {
        if (enc == L"ansi")
        {
            std::string str((char *)&entry.m_data[0], entry.m_data.size());
            MAnsiToWide a2w(CP_ACP, str.c_str());
            return a2w.c_str();
        }
        if (enc == L"wide")
        {
            std::wstring str((wchar_t *)&entry.m_data[0], entry.m_data.size());
            return str;
        }
        if (enc == L"utf8" || enc == L"utf8n")
        {
            std::string str((char *)&entry.m_data[0], entry.m_data.size());
            if (str.size() >= 3 && memcmp(str.c_str(), "\xEF\xBB\xBF", 3) == 0)
            {
                str.erase(0, 3);
            }
            MAnsiToWide a2w(CP_UTF8, str.c_str());
            return a2w.c_str();
        }
        if (enc == L"sjis")
        {
            std::string str((char *)&entry.m_data[0], entry.m_data.size());
            MAnsiToWide a2w(932, str.c_str());
            return a2w.c_str();
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

inline MString ResToText::DoUnknown(const EntryBase& entry)
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
    str += entry.m_type.str();
    str += L" \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    return str;
}

inline MString ResToText::DoFont(const EntryBase& entry)
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

inline MString ResToText::DoFontDir(const EntryBase& entry)
{
    MString str;
    return str;
}

inline MString ResToText::DumpName(const MIdOrString& type, const MIdOrString& name)
{
    MString ret;
    if (name.is_str())
    {
        ret += name.str();
    }
    else
    {
        ret += g_db.GetNameOfResID(g_db.IDTypeFromResType(type), name.m_id);
    }
    return ret;
}

inline MString ResToText::DumpEscapedName(const MIdOrString& name)
{
    MString ret = name.str();
    mstr_escape(ret);
    return ret;
}

//////////////////////////////////////////////////////////////////////////////

inline HBITMAP
CreateBitmapFromIconDx(HICON hIcon, INT width, INT height, BOOL bCursor)
{
    HBITMAP hbm = Create24BppBitmapDx(width, height);
    if (hbm == NULL)
    {
        assert(0);
        return NULL;
    }
    FillBitmapDx(hbm, GetStockBrush(LTGRAY_BRUSH));

    HDC hDC = CreateCompatibleDC(NULL);
    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        HBRUSH hbr = GetStockBrush(LTGRAY_BRUSH);
        DrawIconEx(hDC, 0, 0, hIcon, width, height, 0, hbr, DI_NORMAL);
    }
    SelectObject(hDC, hbmOld);
    DeleteDC(hDC);

    return hbm;
}

inline MStringW
DumpBitmapInfo(HBITMAP hbm)
{
    MStringW ret;
    BITMAP bm;
    if (!GetObjectW(hbm, sizeof(bm), &bm))
        return ret;

    ret = LoadStringPrintfDx(IDS_IMAGEINFO, bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
    return ret;
}

inline MStringW
DumpIconInfo(const BITMAP& bm, BOOL bIcon/* = TRUE*/)
{
    MStringW ret;

    ret = LoadStringPrintfDx(IDS_IMAGEINFO, bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);

    return ret;
}

inline MStringW
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

    ret += LoadStringPrintfDx(IDS_IMAGECOUNT, dir.idCount);;
    ret += L"-------\r\n";

    const GRPICONDIRENTRY *pEntries;
    pEntries = (const GRPICONDIRENTRY *)&data[sizeof(dir)];

    for (WORD i = 0; i < dir.idCount; ++i)
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

inline MStringW
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

    for (WORD i = 0; i < dir.idCount; ++i)
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

inline HBITMAP
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
        assert(hIcon);
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

inline void
DrawBitmapDx(HBITMAP hbm, HBITMAP hbmSrc, INT x, INT y)
{
    BITMAP bmSrc;
    GetObject(hbmSrc, sizeof(bmSrc), &bmSrc);

    HDC hDC = CreateCompatibleDC(NULL);
    HDC hDC2 = CreateCompatibleDC(NULL);
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

inline HBITMAP
CreateBitmapFromIconsDx(HWND hwnd, const EntryBase& entry)
{
    ICONDIR dir;
    if (entry.size() < sizeof(dir))
    {
        assert(0);
        return NULL;
    }

    memcpy(&dir, &entry[0], sizeof(dir));

    if (dir.idReserved != 0 || dir.idType != RES_ICON || dir.idCount == 0)
    {
        assert(0);
        return NULL;
    }

    if (entry.size() < sizeof(dir) + dir.idCount * sizeof(GRPICONDIRENTRY))
    {
        assert(0);
        return FALSE;
    }

    const GRPICONDIRENTRY *pEntries;
    pEntries = (const GRPICONDIRENTRY *)&entry[sizeof(dir)];

    LONG cx = 0, cy = 0;
    for (WORD i = 0; i < dir.idCount; ++i)
    {
        auto e = g_res.find(ET_LANG, RT_ICON, pEntries[i].nID, entry.m_lang);
        if (!e)
            return NULL;

        auto& icon_entry = (EntryBase&)*e;

        BITMAP bm;
        HBITMAP hbmIcon = CreateBitmapFromIconOrPngDx(hwnd, icon_entry, bm);

        if (cx < bm.bmWidth)
            cx = bm.bmWidth;
        cy += bm.bmHeight;

        DeleteObject(hbmIcon);
    }

    HBITMAP hbm = Create24BppBitmapDx(cx, cy);
    if (hbm == NULL)
    {
        assert(0);
        return NULL;
    }
    FillBitmapDx(hbm, GetStockBrush(LTGRAY_BRUSH));
    
    BITMAP bm;
    GetObject(hbm, sizeof(bm), &bm);

    INT y = 0;
    for (WORD i = 0; i < dir.idCount; ++i)
    {
        auto e = g_res.find(ET_LANG, RT_ICON, pEntries[i].nID, entry.m_lang);
        if (!e)
        {
            DeleteObject(hbm);
            return NULL;
        }
        auto icon_entry = (EntryBase&)*e;

        HBITMAP hbmIcon = CreateBitmapFromIconOrPngDx(hwnd, icon_entry, bm);

        DrawBitmapDx(hbm, hbmIcon, 0, y);
        y += bm.bmHeight;
    }

    return hbm;
}

inline HBITMAP
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

inline HBITMAP
CreateBitmapFromCursorsDx(HWND hwnd, const EntryBase& entry)
{
    ICONDIR dir;
    if (entry.size() < sizeof(dir))
    {
        assert(0);
        return NULL;
    }

    memcpy(&dir, &entry[0], sizeof(dir));

    if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
    {
        assert(0);
        return NULL;
    }

    if (entry.size() < sizeof(dir) + dir.idCount * sizeof(GRPCURSORDIRENTRY))
    {
        assert(0);
        return FALSE;
    }

    const GRPCURSORDIRENTRY *pEntries;
    pEntries = (const GRPCURSORDIRENTRY *)&entry[sizeof(dir)];

    LONG cx = 0, cy = 0;
    for (WORD i = 0; i < dir.idCount; ++i)
    {
        auto e = g_res.find(ET_LANG, RT_CURSOR, pEntries[i].nID, entry.m_lang);
        if (!e)
            return NULL;

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
    if (hbm == NULL)
    {
        assert(0);
        return NULL;
    }
    FillBitmapDx(hbm, GetStockBrush(LTGRAY_BRUSH));

    HDC hDC = CreateCompatibleDC(NULL);
    HDC hDC2 = CreateCompatibleDC(NULL);
    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        INT y = 0;
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            auto e = g_res.find(ET_LANG, RT_CURSOR, pEntries[i].nID, entry.m_lang);
            if (!e)
            {
                assert(0);
                DeleteObject(hbm);
                return NULL;
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

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef RES_TO_TEXT_HPP_
