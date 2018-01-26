// ResToText --- Dumping Resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Win32API resource editor
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

#include "RisohEditor.hpp"
#include "DialogRes.hpp"
#include "MenuRes.hpp"
#include "VersionRes.hpp"

//////////////////////////////////////////////////////////////////////////////

class ResToText
{
public:
    typedef std::vector<BYTE> data_type;

    ResToText(const RisohSettings& settings, const ConstantsDB& db, const ResEntries& entries)
        : m_hwnd(NULL), m_hwndDialog(NULL), m_settings(settings), m_db(db), m_entries(entries)
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
};

//////////////////////////////////////////////////////////////////////////////

inline MString
ResToText::DoCursor(const ResEntry& entry)
{
    BITMAP bm;

    HCURSOR hCursor = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, FALSE);
    HBITMAP hbm = CreateBitmapFromIconDx(hCursor, bm.bmWidth, bm.bmHeight, TRUE);
    MString str = DumpIconInfo(bm, FALSE);
    DestroyCursor(hCursor);
    DeleteObject(hbm);

    return str;
}

inline MString
ResToText::DoBitmap(const ResEntry& entry)
{
    HBITMAP hbm = PackedDIB_CreateBitmap(&entry[0], entry.size());
    MString str = DumpBitmapInfo(hbm);
    DeleteObject(hbm);
    return str;
}

inline MString
ResToText::DoIcon(const ResEntry& entry)
{
    BITMAP bm;
    HBITMAP hbm = CreateBitmapFromIconOrPngDx(m_hwnd, entry, bm);

    MString str;
    HICON hIcon = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, TRUE);
    if (hIcon)
    {
        str = DumpIconInfo(bm, TRUE);
    }
    else
    {
        str = DumpBitmapInfo(hbm);
    }
    DestroyIcon(hIcon);
    DeleteObject(hbm);

    return str;
}

inline MString
ResToText::DoMenu(const ResEntry& entry)
{
    MByteStreamEx stream(entry.data);
    MenuRes menu_res;
    if (menu_res.LoadFromStream(stream))
    {
        return menu_res.Dump(entry.name, m_db);
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
        return dialog_res.Dump(entry.name, m_settings.bAlwaysControl);
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

    return str_res.Dump(m_db);
}

inline MString
ResToText::DoAccel(const ResEntry& entry)
{
    MByteStreamEx stream(entry.data);
    AccelRes accel(m_db);
    if (accel.LoadFromStream(stream))
    {
        return accel.Dump(entry.name);
    }
    return LoadStringDx(IDS_INVALIDDATA);
}

inline MString
ResToText::DoGroupCursor(const ResEntry& entry)
{
    return DumpGroupCursorInfo(m_entries, entry.data);
}

inline MString
ResToText::DoGroupIcon(const ResEntry& entry)
{
    return DumpGroupIconInfo(entry.data);
}

inline MString
ResToText::DoVersion(const ResEntry& entry)
{
    VersionRes ver_res;
    if (ver_res.LoadFromData(entry.data))
    {
        return ver_res.Dump(entry.name);
    }
    return LoadStringDx(IDS_INVALIDDATA);
}

inline MString
ResToText::DoAniCursor(const ResEntry& entry)
{
    return LoadStringDx(IDS_ANICURSOR);
}

inline MString
ResToText::DoAniIcon(const ResEntry& entry)
{
    return LoadStringDx(IDS_ANIICON);
}

inline MString
ResToText::DoText(const ResEntry& entry)
{
    MTextType type;
    type.nNewLine = MNEWLINE_CRLF;
    MString str = mstr_from_bin(&entry.data[0], entry.data.size(), &type);
    return str;
}

inline MString
ResToText::DoImage(const ResEntry& entry)
{
    HBITMAP hbm = NULL;
    MBitmapDx bitmap;
    if (bitmap.CreateFromMemory(&entry[0], entry.size()))
    {
        LONG cx, cy;
        hbm = bitmap.GetHBITMAP(cx, cy);
    }

    MString str = DumpBitmapInfo(hbm);
    DeleteObject(hbm);
    return str;
}

inline MString
ResToText::DumpEntry(const ResEntry& entry)
{
    if (entry.type.m_id)
    {
        switch (entry.type.m_id)
        {
        case (WORD)(INT_PTR)RT_CURSOR:
            return DoCursor(entry);
        case (WORD)(INT_PTR)RT_BITMAP:
            return DoBitmap(entry);
        case (WORD)(INT_PTR)RT_ICON:
            return DoIcon(entry);
        case (WORD)(INT_PTR)RT_MENU:
            return DoMenu(entry);
        case (WORD)(INT_PTR)RT_DIALOG:
            return DoDialog(entry);
        case (WORD)(INT_PTR)RT_STRING:
            return DoString(entry);
        case (WORD)(INT_PTR)RT_FONTDIR:
            break;
        case (WORD)(INT_PTR)RT_FONT:
            break;
        case (WORD)(INT_PTR)RT_ACCELERATOR:
            return DoAccel(entry);
        case (WORD)(INT_PTR)RT_RCDATA:
            return DoText(entry);
        case (WORD)(INT_PTR)RT_MESSAGETABLE:
            break;
        case (WORD)(INT_PTR)RT_GROUP_CURSOR:
            return DoGroupCursor(entry);
        case (WORD)(INT_PTR)RT_GROUP_ICON:
            return DoGroupIcon(entry);
        case (WORD)(INT_PTR)RT_VERSION:
            return DoVersion(entry);
        case (WORD)(INT_PTR)RT_DLGINCLUDE:
            break;
        case (WORD)(INT_PTR)RT_PLUGPLAY:
            break;
        case (WORD)(INT_PTR)RT_VXD:
            break;
        case (WORD)(INT_PTR)RT_ANICURSOR:
            return DoAniCursor(entry);
        case (WORD)(INT_PTR)RT_ANIICON:
            return DoAniIcon(entry);
        case (WORD)(INT_PTR)RT_HTML:
            return DoText(entry);
        case (WORD)(INT_PTR)RT_MANIFEST:
            return DoText(entry);
        default:
            return DoText(entry);
        }
    }
    else
    {
        MString type = entry.type.m_str;
        if (type == L"PNG" || type == L"GIF" ||
            type == L"JPEG" || type == L"TIFF" ||
            type == L"JPG" || type == L"TIF" ||
            type == L"EMF" || type == L"WMF")
        {
            return DoImage(entry);
        }
        else if (type == L"WAVE")
        {
            return LoadStringDx(IDS_WAVESOUND);
        }
        else if (entry.type == L"AVI")
        {
            return LoadStringDx(IDS_AVIMOVIE);
        }
    }
    return DoText(entry);
}

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef RES_TO_TEXT_HPP_
