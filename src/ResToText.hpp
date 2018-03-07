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
#include "Res.hpp"

#include "DialogRes.hpp"
#include "MenuRes.hpp"
#include "VersionRes.hpp"
#include "StringRes.hpp"
#include "AccelRes.hpp"

#include "resource.h"

HBITMAP CreateBitmapFromIconDx(HICON hIcon, INT width, INT height, BOOL bCursor);
MStringW DumpIconInfo(const BITMAP& bm, BOOL bIcon = TRUE);
MStringW DumpBitmapInfo(HBITMAP hbm);
MStringW DumpGroupIconInfo(const std::vector<BYTE>& data);
MStringW DumpGroupCursorInfo(const ResEntries& entries, const std::vector<BYTE>& data);

HBITMAP
CreateBitmapFromIconOrPngDx(HWND hwnd, const ResEntry& entry, BITMAP& bm);

HBITMAP
CreateBitmapFromIconsDx(HWND hwnd, ResEntries& entries, const ResEntry& entry);

HBITMAP
CreateBitmapFromCursorDx(HWND hwnd, const ResEntry& entry, BITMAP& bm);

HBITMAP
CreateBitmapFromCursorsDx(HWND hwnd, ResEntries& entries, const ResEntry& entry);

MString GetLanguageStatement(WORD langid, BOOL bOldStyle);

//////////////////////////////////////////////////////////////////////////////

class ResToText
{
public:
    typedef std::vector<BYTE> data_type;

    ResToText(const RisohSettings& settings, const ConstantsDB& db, const ResEntries& entries)
        : m_hwnd(NULL), m_hwndDialog(NULL), m_settings(settings), m_db(db), m_entries(entries),
          m_bHumanReadable(TRUE)
    {
    }

    MString DumpEntry(const ResEntry& entry);

public:
    HWND m_hwnd;
    HWND m_hwndDialog;
protected:
    const RisohSettings& m_settings;
    const ConstantsDB& m_db;
    const ResEntries& m_entries;
public:
    BOOL m_bHumanReadable;

    MString GetLanguageStatement(WORD langid)
    {
        return ::GetLanguageStatement(langid, m_settings.bOldStyle);
    }

    MString DoCursor(const ResEntry& entry);
    MString DoBitmap(const ResEntry& entry);
    MString DoIcon(const ResEntry& entry);
    MString DoMenu(const ResEntry& entry);
    MString DoDialog(const ResEntry& entry);
    MString DoString(const ResEntry& entry);
    MString DoAccel(const ResEntry& entry);
    MString DoGroupCursor(const ResEntry& entry);
    MString DoGroupIcon(const ResEntry& entry);
    MString DoVersion(const ResEntry& entry);
    MString DoAniCursor(const ResEntry& entry);
    MString DoAniIcon(const ResEntry& entry);
    MString DoText(const ResEntry& entry);
    MString DoImage(const ResEntry& entry);
    MString DoMessage(const ResEntry& entry);
    MString DoWave(const ResEntry& entry);
    MString DoAVI(const ResEntry& entry);
    MString DoRCData(const ResEntry& entry);
    MString DoUnknown(const ResEntry& entry);

    MString DumpName(const MIdOrString& type, const MIdOrString& name);
    MString DumpEscapedName(const MIdOrString& name);

    MString GetEntryFileName(const ResEntry& entry);
};

//////////////////////////////////////////////////////////////////////////////

inline MString
ResToText::GetEntryFileName(const ResEntry& entry)
{
    MString ret;
    
    if (entry.type == RT_CURSOR)
    {
    }
    else if (entry.type == RT_BITMAP)
    {
        ret += L"Bitmap_";
        ret += DumpEscapedName(entry.name);
        ret += L".bmp";
    }
    else if (entry.type == RT_ICON)
    {
    }
    else if (entry.type == RT_MENU)
    {
    }
    else if (entry.type == RT_DIALOG)
    {
    }
    else if (entry.type == RT_STRING)
    {
    }
    else if (entry.type == RT_FONTDIR)
    {
        ret += L"FontDir_";
        ret += DumpEscapedName(entry.name);
        ret += L".bin";
    }
    else if (entry.type == RT_FONT)
    {
        ret += L"Font_";
        ret += DumpEscapedName(entry.name);
        ret += L".bin";
    }
    else if (entry.type == RT_ACCELERATOR)
    {
    }
    else if (entry.type == RT_RCDATA)
    {
        ret += L"RCData_";
        ret += DumpEscapedName(entry.name);
        ret += L".bin";
    }
    else if (entry.type == RT_MESSAGETABLE)
    {
    }
    else if (entry.type == RT_GROUP_CURSOR)
    {
        ret += L"Cursor_";
        ret += DumpEscapedName(entry.name);
        ret += L".cur";
    }
    else if (entry.type == RT_GROUP_ICON)
    {
        ret += L"Icon_";
        ret += DumpEscapedName(entry.name);
        ret += L".ico";
    }
    else if (entry.type == RT_VERSION)
    {
    }
    else if (entry.type == RT_DLGINCLUDE)
    {
        // TODO:
    }
    else if (entry.type == RT_PLUGPLAY)
    {
        // TODO:
    }
    else if (entry.type == RT_VXD)
    {
        // TODO:
    }
    else if (entry.type == RT_ANICURSOR)
    {
        ret += L"AniCursor_";
        ret += DumpEscapedName(entry.name);
        ret += L".ani";
    }
    else if (entry.type == RT_ANIICON)
    {
        ret += L"AniIcon_";
        ret += DumpEscapedName(entry.name);
        ret += L".ani";
    }
    else if (entry.type == RT_HTML)
    {
        ret += L"Html_";
        ret += DumpEscapedName(entry.name);
        ret += L".html";
    }
    else if (entry.type == RT_MANIFEST)
    {
        ret += L"Manifest_";
        ret += DumpEscapedName(entry.name);
        ret += L".manifest";
    }
    else if (entry.type == L"AVI")
    {
        ret += L"Avi_";
        ret += DumpEscapedName(entry.name);
        ret += L".avi";
    }
    else if (entry.type == L"PNG")
    {
        ret += L"Png_";
        ret += DumpEscapedName(entry.name);
        ret += L".png";
    }
    else if (entry.type == L"GIF")
    {
        ret += L"Gif_";
        ret += DumpEscapedName(entry.name);
        ret += L".gif";
    }
    else if (entry.type == L"JPEG")
    {
        ret += L"Jpeg_";
        ret += DumpEscapedName(entry.name);
        ret += L".jpg";
    }
    else if (entry.type == L"JPG")
    {
        ret += L"Jpg_";
        ret += DumpEscapedName(entry.name);
        ret += L".jpg";
    }
    else if (entry.type == L"TIFF")
    {
        ret += L"Tiff_";
        ret += DumpEscapedName(entry.name);
        ret += L".tif";
    }
    else if (entry.type == L"TIF")
    {
        ret += L"Tif_";
        ret += DumpEscapedName(entry.name);
        ret += L".tif";
    }
    else if (entry.type == L"EMF")
    {
        ret += L"Emf_";
        ret += DumpEscapedName(entry.name);
        ret += L".emf";
    }
    else if (entry.type == L"ENHMETAFILE")
    {
        ret += L"EnhMetaFile_";
        ret += DumpEscapedName(entry.name);
        ret += L".emf";
    }
    else if (entry.type == L"WMF")
    {
        ret += L"Wmf_";
        ret += DumpEscapedName(entry.name);
        ret += L".wmf";
    }
    else if (entry.type == L"WAVE")
    {
        ret += L"Wave_";
        ret += DumpEscapedName(entry.name);
        ret += L".wav";
    }
    else if (entry.type == L"IMAGE")
    {
        if (entry.size() >= 4)
        {
            if (memcmp(&entry[0], "BM", 2) == 0)
            {
                ret += L"Image_";
                ret += DumpEscapedName(entry.name);
                ret += L".bmp";
            }
            else if (memcmp(&entry[0], "GIF", 3) == 0)
            {
                ret += L"Image_";
                ret += DumpEscapedName(entry.name);
                ret += L".gif";
            }
            else if (memcmp(&entry[0], "\x89\x50\x4E\x47", 4) == 0)
            {
                ret += L"Image_";
                ret += DumpEscapedName(entry.name);
                ret += L".png";
            }
            else if (memcmp(&entry[0], "\xFF\xD8", 2) == 0)
            {
                ret += L"Image_";
                ret += DumpEscapedName(entry.name);
                ret += L".jpg";
            }
            else if (memcmp(&entry[0], "\x4D\x4D", 2) == 0 ||
                     memcmp(&entry[0], "\x49\x49", 2) == 0)
            {
                ret += L"Image_";
                ret += DumpEscapedName(entry.name);
                ret += L".tif";
            }
        }
    }
    else
    {
        if (entry.type.is_str())
        {
            ret += entry.type.str();
            ret += DumpEscapedName(entry.name);
            ret += L".bin";
        }
    }

    return ret;
}

inline MString
ResToText::DoCursor(const ResEntry& entry)
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
    }

    return str;
}

inline MString
ResToText::DoBitmap(const ResEntry& entry)
{
    HBITMAP hbm = PackedDIB_CreateBitmap(&entry[0], entry.size());
    MString str;

    if (m_bHumanReadable)
    {
        str += DumpBitmapInfo(hbm);
    }

    // LANGUAGE ..., ...
    str += GetLanguageStatement(entry.lang);

    str += DumpName(entry.type, entry.name);
    str += L" BITMAP \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    DeleteObject(hbm);
    return str;
}

inline MString
ResToText::DoIcon(const ResEntry& entry)
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
    }

    return str;
}

inline MString
ResToText::DoMenu(const ResEntry& entry)
{
    MByteStreamEx stream(entry.data);
    MenuRes menu_res;
    if (menu_res.LoadFromStream(stream))
    {
        MString str;
        str += GetLanguageStatement(entry.lang);
        str += menu_res.Dump(entry.name, m_db);
        str += L"\r\n";
        return str;
    }
    return LoadStringDx(IDS_INVALIDDATA);
}

inline MString
ResToText::DoDialog(const ResEntry& entry)
{
    MByteStreamEx stream(entry.data);
    DialogRes dialog_res(m_db);
    if (dialog_res.LoadFromStream(stream))
    {
        MString str;
        str += GetLanguageStatement(entry.lang);
        str += dialog_res.Dump(entry.name, m_settings.bAlwaysControl);
        str += L"\r\n";
        return str;
    }
    return LoadStringDx(IDS_INVALIDDATA);
}

inline MString
ResToText::DoString(const ResEntry& entry)
{
    ResEntries found;
    Res_Search(found, m_entries, RT_STRING, entry.name, entry.lang);

    StringRes str_res;
    ResEntries::iterator it, end = found.end();
    for (it = found.begin(); it != end; ++it)
    {
        MByteStreamEx stream(it->data);
        if (!str_res.LoadFromStream(stream, it->name.m_id))
            return LoadStringDx(IDS_INVALIDDATA);
    }

    MString str;
    if (entry.name.empty())
        str += GetLanguageStatement(entry.lang);
    str += str_res.Dump(m_db);
    str += L"\r\n\r\n";
    return str;
}

inline MString
ResToText::DoMessage(const ResEntry& entry)
{
    ResEntries found;
    Res_Search(found, m_entries, RT_MESSAGETABLE, entry.name, entry.lang);

    MessageRes msg_res;
    ResEntries::iterator it, end = found.end();
    for (it = found.begin(); it != end; ++it)
    {
        MByteStreamEx stream(it->data);
        if (!msg_res.LoadFromStream(stream, it->name.m_id))
            return LoadStringDx(IDS_INVALIDDATA);
    }

    MString str;
    if (entry.name.empty())
        str += GetLanguageStatement(entry.lang);

    str += L"#ifdef MCDX_INVOKED\r\n";
    str += msg_res.Dump(m_db);
    str += L"#endif\r\n\r\n";
    return str;
}

inline MString
ResToText::DoAccel(const ResEntry& entry)
{
    MByteStreamEx stream(entry.data);
    AccelRes accel(m_db);
    if (accel.LoadFromStream(stream))
    {
        MString str;
        str += GetLanguageStatement(entry.lang);
        str += accel.Dump(entry.name);
        str += L"\r\n\r\n";
        return str;
    }
    return LoadStringDx(IDS_INVALIDDATA);
}

inline MString
ResToText::DoGroupCursor(const ResEntry& entry)
{
    MStringW str;

    if (m_bHumanReadable)
    {
        str += DumpGroupCursorInfo(m_entries, entry.data);
    }

    // LANGUAGE ..., ...
    str += GetLanguageStatement(entry.lang);

    str += DumpName(entry.type, entry.name);
    str += L" CURSOR \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    return str;
}

inline MString
ResToText::DoGroupIcon(const ResEntry& entry)
{
    MStringW str;

    if (m_bHumanReadable)
    {
        str += DumpGroupIconInfo(entry.data);
    }

    // LANGUAGE ..., ...
    str += GetLanguageStatement(entry.lang);

    str += DumpName(entry.type, entry.name);
    str += L" ICON \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    return str;
}

inline MString
ResToText::DoVersion(const ResEntry& entry)
{
    VersionRes ver_res;
    if (ver_res.LoadFromData(entry.data))
    {
        MString str;
        str += GetLanguageStatement(entry.lang);
        str += ver_res.Dump(entry.name);
        str += L"\r\n";
        return str;
    }
    return LoadStringDx(IDS_INVALIDDATA);
}

inline MString
ResToText::DoAniCursor(const ResEntry& entry)
{
    MString str;

    if (m_bHumanReadable)
    {
        str += LoadStringDx(IDS_ANICURSOR);
    }

    // LANGUAGE ..., ...
    str += GetLanguageStatement(entry.lang);

    str += DumpName(entry.type, entry.name);
    str += L" ANICURSOR \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    return str;
}

inline MString
ResToText::DoAniIcon(const ResEntry& entry)
{
    MString str;

    if (m_bHumanReadable)
    {
        str += LoadStringDx(IDS_ANIICON);
    }

    // LANGUAGE ..., ...
    str += GetLanguageStatement(entry.lang);

    str += DumpName(entry.type, entry.name);
    str += L" ANIICON \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    return str;
}

inline MString
ResToText::DoText(const ResEntry& entry)
{
    MString str;
    if (m_bHumanReadable)
    {
        MTextType type;
        type.nNewLine = MNEWLINE_CRLF;
        str = mstr_from_bin(&entry.data[0], entry.data.size(), &type);
    }
    else
    {
        str += GetLanguageStatement(entry.lang);
        str += DumpName(entry.type, entry.name);
        str += L" ";
        str += DumpEscapedName(entry.type);
        str += L" \"";
        str += GetEntryFileName(entry);
        str += L"\"\r\n\r\n";
    }
    return str;
}

inline MString
ResToText::DoImage(const ResEntry& entry)
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
    }

    // LANGUAGE ..., ...
    str += GetLanguageStatement(entry.lang);

    if (entry.type == L"PNG")
    {
        str += DumpName(entry.type, entry.name);
        str += L" PNG \"";
        str += GetEntryFileName(entry);
        str += L"\"\r\n\r\n";
    }
    else if (entry.type == L"GIF")
    {
        str += DumpName(entry.type, entry.name);
        str += L" GIF \"";
        str += GetEntryFileName(entry);
        str += L"\"\r\n\r\n";
    }
    else if (entry.type == L"JPEG")
    {
        str += DumpName(entry.type, entry.name);
        str += L" JPEG \"";
        str += GetEntryFileName(entry);
        str += L"\"\r\n\r\n";
    }
    else if (entry.type == L"JPG")
    {
        str += DumpName(entry.type, entry.name);
        str += L" JPG \"";
        str += GetEntryFileName(entry);
        str += L"\"\r\n\r\n";
    }
    else if (entry.type == L"TIFF")
    {
        str += DumpName(entry.type, entry.name);
        str += L" TIFF \"";
        str += GetEntryFileName(entry);
        str += L"\"\r\n\r\n";
    }
    else if (entry.type == L"TIF")
    {
        str += DumpName(entry.type, entry.name);
        str += L" TIF \"";
        str += GetEntryFileName(entry);
        str += L"\"\r\n\r\n";
    }
    else if (entry.type == L"EMF")
    {
        str += DumpName(entry.type, entry.name);
        str += L" EMF \"";
        str += GetEntryFileName(entry);
        str += L"\"\r\n\r\n";
    }
    else if (entry.type == L"ENHMETAFILE")
    {
        str += DumpName(entry.type, entry.name);
        str += L" ENHMETAFILE \"";
        str += GetEntryFileName(entry);
        str += L"\"\r\n\r\n";
    }
    else if (entry.type == L"WMF")
    {
        str += DumpName(entry.type, entry.name);
        str += L" WMF \"";
        str += GetEntryFileName(entry);
        str += L"\"\r\n\r\n";
    }
    else if (entry.type == L"IMAGE")
    {
        if (entry.size() >= 4)
        {
            if (memcmp(&entry[0], "BM", 2) == 0)
            {
                str += DumpName(entry.type, entry.name);
                str += L" IMAGE \"";
                str += GetEntryFileName(entry);
                str += L"\"\r\n\r\n";
            }
            else if (memcmp(&entry[0], "GIF", 3) == 0)
            {
                str += DumpName(entry.type, entry.name);
                str += L" IMAGE \"";
                str += GetEntryFileName(entry);
                str += L"\"\r\n\r\n";
            }
            else if (memcmp(&entry[0], "\x89\x50\x4E\x47", 4) == 0)
            {
                str += DumpName(entry.type, entry.name);
                str += L" IMAGE \"";
                str += GetEntryFileName(entry);
                str += L"\"\r\n\r\n";
            }
            else if (memcmp(&entry[0], "\xFF\xD8", 2) == 0)
            {
                str += DumpName(entry.type, entry.name);
                str += L" IMAGE \"";
                str += GetEntryFileName(entry);
                str += L"\"\r\n\r\n";
            }
            else if (memcmp(&entry[0], "\x4D\x4D", 2) == 0 ||
                     memcmp(&entry[0], "\x49\x49", 2) == 0)
            {
                str += DumpName(entry.type, entry.name);
                str += L" IMAGE \"";
                str += GetEntryFileName(entry);
                str += L"\"\r\n\r\n";
            }
        }
    }

    return str;
}

inline MString
ResToText::DumpEntry(const ResEntry& entry)
{
    if (entry.type.m_id)
    {
        switch (entry.type.m_id)
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
            break;
        case 8: // RT_FONT
            break;
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
            return DoText(entry);
        default:
            return DoUnknown(entry);
        }
    }
    else
    {
        MString type = entry.type.m_str;
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
        else if (entry.type == L"AVI")
        {
            return DoAVI(entry);
        }
    }
    return DoText(entry);
}

inline MString ResToText::DoWave(const ResEntry& entry)
{
    MString str;

    if (m_bHumanReadable)
    {
        str += LoadStringDx(IDS_WAVESOUND);
    }

    // LANGUAGE ..., ...
    str += GetLanguageStatement(entry.lang);

    str += DumpName(entry.type, entry.name);
    str += L" WAVE \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    return str;
}

inline MString ResToText::DoAVI(const ResEntry& entry)
{
    MString str;

    if (m_bHumanReadable)
    {
        str += LoadStringDx(IDS_AVIMOVIE);
    }

    // LANGUAGE ..., ...
    str += GetLanguageStatement(entry.lang);

    str += DumpName(entry.type, entry.name);
    str += L" AVI \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    return str;
}

inline MString ResToText::DoRCData(const ResEntry& entry)
{
    MString str;

    // LANGUAGE ..., ...
    str += GetLanguageStatement(entry.lang);

    str += DumpName(entry.type, entry.name);
    str += L" RCDATA \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    return str;
}

inline MString ResToText::DoUnknown(const ResEntry& entry)
{
    MString str;

    // LANGUAGE ..., ...
    str += GetLanguageStatement(entry.lang);

    str += DumpName(entry.type, entry.name);
    str += L" ";
    str += entry.type.str();
    str += L" \"";
    str += GetEntryFileName(entry);
    str += L"\"\r\n\r\n";

    return str;
}

inline MString ResToText::DumpName(const MIdOrString& type, const MIdOrString& name)
{
    MString ret;
    if (name.is_str())
    {
        ret += L"\"";
        ret += name.str();
        ret += L"\"";
    }
    else
    {
        ret += m_db.GetNameOfResID(m_db.IDTypeFromRes(type), name.m_id);
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

    WCHAR sz[64];
    wsprintfW(sz, LoadStringDx(IDS_IMAGEINFO),
              bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
    ret = sz;
    return ret;
}

inline MStringW
DumpIconInfo(const BITMAP& bm, BOOL bIcon/* = TRUE*/)
{
    MStringW ret;

    using namespace std;
    WCHAR sz[128];
    wsprintfW(sz, LoadStringDx(IDS_IMAGEINFO),
              bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
    ret = sz;

    return ret;
}

inline MStringW
DumpGroupIconInfo(const std::vector<BYTE>& data)
{
    MStringW ret;
    WCHAR sz[128];

    ICONDIR dir;
    if (data.size() < sizeof(dir))
        return ret;

    memcpy(&dir, &data[0], sizeof(dir));

    if (dir.idReserved != 0 || dir.idType != 1 || dir.idCount == 0)
    {
        return ret;
    }

    wsprintfW(sz, LoadStringDx(IDS_IMAGECOUNT), dir.idCount);
    ret += sz;
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

        wsprintfW(sz, LoadStringDx(IDS_ICONINFO),
                  i, Width, Height, pEntries[i].wBitCount, nID);
        ret += sz;
    }

    return ret;
}

inline MStringW
DumpGroupCursorInfo(const ResEntries& entries, const std::vector<BYTE>& data)
{
    MStringW ret;
    WCHAR sz[128];

    ICONDIR dir;
    if (data.size() < sizeof(dir))
        return ret;

    memcpy(&dir, &data[0], sizeof(dir));

    if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
    {
        return ret;
    }

    wsprintfW(sz, LoadStringDx(IDS_IMAGECOUNT), dir.idCount);
    ret += sz;
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

        INT k = Res_Find(entries, RT_CURSOR, nID, 0xFFFF, FALSE);
        if (k != -1)
        {
            const ResEntry& cursor_entry = entries[k];
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

        wsprintfW(sz, LoadStringDx(IDS_CURSORINFO),
                  i, Width, Height, BitCount, xHotSpot, yHotSpot, nID);
        ret += sz;
    }

    return ret;
}

inline HBITMAP
CreateBitmapFromIconOrPngDx(HWND hwnd, const ResEntry& entry, BITMAP& bm)
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
CreateBitmapFromIconsDx(HWND hwnd, ResEntries& entries, const ResEntry& entry)
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
        INT k = Res_Find2(entries, RT_ICON, pEntries[i].nID, entry.lang, FALSE);
        if (k == -1)
        {
            return NULL;
        }
        ResEntry& icon_entry = entries[k];

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
        INT k = Res_Find2(entries, RT_ICON, pEntries[i].nID, entry.lang, FALSE);
        if (k == -1)
        {
            DeleteObject(hbm);
            return NULL;
        }
        ResEntry& icon_entry = entries[k];

        HBITMAP hbmIcon = CreateBitmapFromIconOrPngDx(hwnd, icon_entry, bm);

        DrawBitmapDx(hbm, hbmIcon, 0, y);
        y += bm.bmHeight;
    }

    return hbm;
}

inline HBITMAP
CreateBitmapFromCursorDx(HWND hwnd, const ResEntry& entry, BITMAP& bm)
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
CreateBitmapFromCursorsDx(HWND hwnd, ResEntries& entries, const ResEntry& entry)
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
        INT k = Res_Find2(entries, RT_CURSOR, pEntries[i].nID, entry.lang, FALSE);
        if (k == -1)
        {
            return NULL;
        }
        ResEntry& cursor_entry = entries[k];

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
            INT k = Res_Find2(entries, RT_CURSOR, pEntries[i].nID, entry.lang, FALSE);
            if (k == -1)
            {
                assert(0);
                DeleteObject(hbm);
                return NULL;
            }
            ResEntry& cursor_entry = entries[k];

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
