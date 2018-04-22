// Res.hpp --- Win32 Resources
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

#ifndef RES_HPP_
#define RES_HPP_

#pragma once

#include <windows.h>
#include <commctrl.h>
#include <cctype>
#include <cwchar>
#include <algorithm>    // for std::sort

#include "IconRes.hpp"
#include "MByteStreamEx.hpp"
#include "MString.hpp"
#include "PackedDIB.hpp"
#include "MBitmapDx.hpp"
#include "ConstantsDB.hpp"
#include "DialogRes.hpp"

class ResEntry;
// class ResEntries;

typedef std::map<MIdOrString, HBITMAP>  MTitleToBitmap;
typedef std::map<MIdOrString, HICON>    MTitleToIcon;

///////////////////////////////////////////////////////////////////////////////

#ifndef RT_HTML
    #define RT_HTML         MAKEINTRESOURCE(23)
#endif
#ifndef RT_MANIFEST
    #define RT_MANIFEST     MAKEINTRESOURCE(24)
#endif

#define I_NONE      0
#define I_TYPE      1
#define I_NAME      2
#define I_LANG      3
#define I_STRING    4
#define I_MESSAGE   5

class ResEntry
{
public:
    typedef DWORD               size_type;
    typedef std::vector<BYTE>   DataType;

    WORD            lang;
    BOOL            updated;
    MIdOrString     type;
    MIdOrString     name;
    DataType        data;

    ResEntry() : lang(0xFFFF), updated(FALSE)
    {
    }

    ResEntry(const MIdOrString& type, const MIdOrString& name, WORD lang,
             BOOL Updated = TRUE)
     : type(type), name(name), lang(lang), updated(Updated)
    {
    }

    void clear()
    {
        clear_data();
        lang = 0xFFFF;
        name = (WORD)0;
        type = (WORD)0;
    }

    void clear_data()
    {
        data.clear();
        updated = TRUE;
    }

    bool empty() const
    {
        return size() == 0;
    }
    size_type size() const
    {
        return size_type(data.size());
    }

    void *ptr(DWORD index = 0)
    {
        return &data[index];
    }
    const void *ptr(DWORD index = 0) const
    {
        return &data[index];
    }

    BYTE& operator[](DWORD index)
    {
        assert(index <= data.size());
        return data[index];
    }
    const BYTE& operator[](DWORD index) const
    {
        assert(index <= data.size());
        return data[index];
    }

    void assign(const void *ptr, size_type nSize)
    {
        if (ptr && nSize)
        {
            data.resize(nSize);
            memcpy(&data[0], ptr, nSize);
        }
        else
        {
            data.clear();
        }
        updated = TRUE;
    }

    bool operator==(const ResEntry& entry) const
    {
        return lang == entry.lang &&
               type == entry.type &&
               name == entry.name;
    }
    bool operator!=(const ResEntry& entry) const
    {
        return !(*this == entry);
    }
};
typedef std::vector<ResEntry> ResEntries;

///////////////////////////////////////////////////////////////////////////////

inline BOOL
Res_IsEntityType(const MIdOrString& type)
{
    if (type == RT_CURSOR || type == RT_ICON)
        return FALSE;
    if (type == RT_STRING || type == RT_MESSAGETABLE)
        return FALSE;
    if (type == RT_VERSION || type == RT_MANIFEST)
        return FALSE;
    return TRUE;
}

inline INT
Res_Find(const ResEntries& entries,
         const MIdOrString& type, 
         const MIdOrString& name,
         WORD lang, BOOL bEmptyOK = FALSE)
{
    size_t i, count = entries.size();
    for (i = 0; i < count; ++i)
    {
        const ResEntry& entry = entries[i];
        if (!type.is_zero() && entry.type != type)
            continue;
        if (!name.is_zero() && entry.name != name)
            continue;
        if (lang != 0xFFFF && entry.lang != lang)
            continue;
        if (entry.empty() && !bEmptyOK)
            continue;
        return INT(i);
    }
    return -1;
}

inline INT
Res_Find2(const ResEntries& entries,
          const MIdOrString& type, 
          const MIdOrString& name,
          WORD lang, BOOL bEmptyOK = FALSE)
{
    INT ret = Res_Find(entries, type, name, lang, bEmptyOK);
    if (ret == -1 && lang != 0xFFFF)
        ret = Res_Find(entries, type, name, 0xFFFF, bEmptyOK);
    return ret;
}

inline bool
Res_Equal(const ResEntry& entry1, const ResEntry& entry2)
{
    if (entry1.type != entry2.type)
        return false;
    if (entry1.name != entry2.name)
        return false;
    if (entry1.lang != entry2.lang)
        return false;
    return true;
}

inline bool
Res_Intersect(const ResEntries& entries1, const ResEntries& entries2)
{
    size_t i1, count1 = entries1.size();
    size_t i2, count2 = entries2.size();
    if (count1 == 0 || count2 == 0)
        return false;

    for (i1 = 0; i1 < count1; ++i1)
    {
        for (i2 = 0; i2 < count2; ++i2)
        {
            if (entries1[i1].empty() || entries2[i2].empty())
                continue;

            if (Res_Equal(entries1[i1], entries2[i2]))
                return true;
        }
    }

    return false;
}

inline INT
Res_Find(const ResEntries& entries, const ResEntry& entry, BOOL bEmptyOK)
{
    return Res_Find(entries, entry.type, entry.name, entry.lang, bEmptyOK);
}

inline INT
Res_Find2(const ResEntries& entries, const ResEntry& entry, BOOL bEmptyOK)
{
    return Res_Find2(entries, entry.type, entry.name, entry.lang, bEmptyOK);
}

inline void
Res_Search(ResEntries& found,
           const ResEntries& entries,
           const MIdOrString& type, 
           const MIdOrString& name,
           WORD lang)
{
    found.clear();

    size_t i, count = entries.size();
    for (i = 0; i < count; ++i)
    {
        const ResEntry& entry = entries[i];
        if (!type.is_zero() && entry.type != type)
            continue;
        if (!name.is_zero() && entry.name != name)
            continue;
        if (lang != 0xFFFF && entry.lang != lang)
            continue;
        if (entry.empty())
            continue;

        found.push_back(entry);
    }
}

inline void
Res_Search(ResEntries& found, const ResEntries& entries, const ResEntry& entry)
{
    Res_Search(found, entries, entry.type, entry.name, entry.lang);
}

inline UINT
Res_GetLastIconID(const ResEntries& entries)
{
    WORD last_id = 0;

    ResEntries::const_iterator it, end = entries.end();
    for (it = entries.begin(); it != end; ++it)
    {
        if (it->type != RT_ICON || !it->name.is_int() || it->name.is_zero())
            continue;

        if (last_id < it->name.m_id)
            last_id = it->name.m_id;
    }
    return last_id;
}

inline UINT
Res_GetLastCursorID(const ResEntries& entries)
{
    WORD last_id = 0;

    ResEntries::const_iterator it, end = entries.end();
    for (it = entries.begin(); it != end; ++it)
    {
        if (it->type != RT_CURSOR || !it->name.is_int() || it->name.is_zero())
            continue;

        if (last_id < it->name.m_id)
            last_id = it->name.m_id;
    }
    return last_id;
}

inline void
Res_AddEntryFromRes(HMODULE hMod, ResEntries& entries,
                    LPCWSTR type, LPCWSTR name, WORD lang)
{
    ResEntry entry(type, name, lang);

    HRSRC hResInfo = FindResourceExW(hMod, type, name, lang);
    if (hResInfo)
    {
        DWORD dwSize = SizeofResource(hMod, hResInfo);
        HGLOBAL hGlobal = LoadResource(hMod, hResInfo);
        LPVOID pv = LockResource(hGlobal);
        if (pv && dwSize)
        {
            entry.assign(pv, dwSize);
        }
    }

    entries.push_back(entry);
}

inline BOOL
Res_AddEntry(ResEntries& entries, const ResEntry& entry,
             BOOL Replace = FALSE)
{
    INT iEntry = Res_Find(entries, entry, TRUE);
    if (iEntry != -1)
    {
        if (!Replace && !entries[iEntry].empty())
        {
            return FALSE;
        }
        entries[iEntry] = entry;
    }
    else
    {
        entries.push_back(entry);
    }

    return TRUE;
}

inline BOOL
Res_AddEntry(ResEntries& entries, const MIdOrString& type,
             const MIdOrString& name, WORD lang,
             const ResEntry::DataType& vecData, BOOL Replace = FALSE)
{
    ResEntry entry(type, name, lang);
    entry.data = vecData;
    return Res_AddEntry(entries, entry, Replace);
}

inline BOOL
Res_DeleteGroupIcon(ResEntries& entries, ResEntry& entry)
{
    assert(entry.type == RT_GROUP_ICON);

    MByteStreamEx bs(entry.data);

    ICONDIR dir;
    if (!bs.ReadRaw(dir))
        return FALSE;

    DWORD size = sizeof(GRPICONDIRENTRY) * dir.idCount;
    std::vector<GRPICONDIRENTRY> DirEntries(dir.idCount);
    if (!bs.ReadData(&DirEntries[0], size))
    {
        return FALSE;
    }

    DWORD i, nCount = dir.idCount;
    for (i = 0; i < nCount; ++i)
    {
        INT k = Res_Find(entries, RT_ICON, DirEntries[i].nID, entry.lang, TRUE);
        if (k != -1)
            entries[k].clear_data();
    }

    entry.clear_data();
    return TRUE;
}

inline BOOL
Res_DeleteGroupCursor(ResEntries& entries, ResEntry& entry)
{
    assert(entry.type == RT_GROUP_CURSOR);

    MByteStreamEx bs(entry.data);

    ICONDIR dir;
    if (!bs.ReadRaw(dir))
        return FALSE;

    DWORD size = sizeof(GRPCURSORDIRENTRY) * dir.idCount;
    std::vector<GRPCURSORDIRENTRY> DirEntries(dir.idCount);
    if (!bs.ReadData(&DirEntries[0], size))
    {
        return FALSE;
    }

    DWORD i, nCount = dir.idCount;
    for (i = 0; i < nCount; ++i)
    {
        INT k = Res_Find(entries, RT_CURSOR, DirEntries[i].nID, entry.lang, TRUE);
        entries[k].clear_data();
    }

    entry.clear_data();
    return TRUE;
}

inline BOOL
Res_DeleteEntries(ResEntries& entries, const MIdOrString& type,
                  const MIdOrString& name, WORD lang)
{
    BOOL bFound = FALSE;
    for (;;)
    {
        INT iEntry = Res_Find(entries, type, name, lang, FALSE);
        if (iEntry == -1)
            break;

        ResEntry& entry = entries[iEntry];

        if (entry.type == RT_GROUP_ICON)
        {
            Res_DeleteGroupIcon(entries, entry);
        }
        if (entry.type == RT_GROUP_CURSOR)
        {
            Res_DeleteGroupCursor(entries, entry);
        }
        else
        {
            entry.clear_data();
        }

        bFound = TRUE;
    }

    return bFound;
}

inline BOOL
Res_DeleteEntries(ResEntries& entries, const ResEntry& entry)
{
    return Res_DeleteEntries(entries, entry.type, entry.name, entry.lang);
}

inline BOOL
Res_AddGroupIcon(ResEntries& entries, const MIdOrString& name,
                 WORD lang, const MStringW& FileName,
                 BOOL Replace = FALSE)
{
    IconFile icon;
    if (!icon.LoadFromFile(FileName.c_str()) || icon.type() != RES_ICON)
        return FALSE;

    UINT LastIconID = Res_GetLastIconID(entries);
    UINT NextIconID = LastIconID + 1;
    IconFile::DataType group(icon.GetIconGroup(NextIconID));
    Res_AddEntry(entries, RT_GROUP_ICON, name, lang, group, Replace);

    int i, nCount = icon.GetImageCount();
    for (i = 0; i < nCount; ++i)
    {
        Res_AddEntry(entries, RT_ICON, WORD(NextIconID + i), lang,
                     icon.GetImage(i));
    }
    return TRUE;
}

inline BOOL
Res_AddGroupCursor(ResEntries& entries, const MIdOrString& name,
                   WORD lang, const MStringW& FileName,
                   BOOL Replace = FALSE)
{
    CursorFile cur;
    if (!cur.LoadFromFile(FileName.c_str()) || cur.type() != RES_CURSOR)
        return FALSE;

    UINT LastCursorID = Res_GetLastCursorID(entries);
    UINT NextCursorID = LastCursorID + 1;
    CursorFile::DataType group(cur.GetCursorGroup(NextCursorID));
    Res_AddEntry(entries, RT_GROUP_CURSOR, name, lang, group, Replace);

    int i, nCount = cur.GetImageCount();
    for (i = 0; i < nCount; ++i)
    {
        Res_AddEntry(entries, RT_CURSOR, WORD(NextCursorID + i), lang,
                     cur.GetImage(i));
    }
    return TRUE;
}

inline BOOL
Res_AddBitmap(ResEntries& entries, const MIdOrString& name,
              WORD lang, const MStringW& BitmapFile, BOOL Replace = FALSE)
{
    MByteStreamEx stream;
    if (!stream.LoadFromFile(BitmapFile.c_str()) || stream.size() <= 4)
        return FALSE;

    if (stream.size() >= 4 &&
        (memcmp(&stream[0], "\xFF\xD8\xFF", 3)== 0 ||    // JPEG
         memcmp(&stream[0], "GIF", 3) == 0 ||            // GIF
         memcmp(&stream[0], "\x89\x50\x4E\x47", 4) == 0)) // PNG
    {
        MBitmapDx bitmap;
        if (!bitmap.CreateFromMemory(&stream[0], (DWORD)stream.size()))
            return FALSE;

        LONG cx, cy;
        HBITMAP hbm = bitmap.GetHBITMAP32(cx, cy);

        std::vector<BYTE> PackedDIB;
        if (!PackedDIB_CreateFromHandle(PackedDIB, hbm))
        {
            DeleteObject(hbm);
            return FALSE;
        }
        DeleteObject(hbm);

        Res_AddEntry(entries, RT_BITMAP, name, lang, PackedDIB, Replace);
    }
    else
    {
        size_t FileHeadSize = sizeof(BITMAPFILEHEADER);
        if (stream.size() < FileHeadSize)
            return FALSE;

        size_t i0 = FileHeadSize, i1 = stream.size();
        ResEntry::DataType HeadLess(&stream[i0], &stream[i0] + (i1 - i0));
        Res_AddEntry(entries, RT_BITMAP, name, lang, HeadLess, Replace);
    }

    return TRUE;
}

inline BOOL CALLBACK
EnumResLangProc(HMODULE hMod, LPCWSTR lpszType, LPCWSTR lpszName,
                WORD wIDLanguage, LPARAM lParam)
{
    ResEntries& entries = *(ResEntries *)lParam;
    Res_AddEntryFromRes(hMod, entries, lpszType, lpszName, wIDLanguage);
    return TRUE;
}

inline BOOL CALLBACK
EnumResNameProc(HMODULE hMod, LPCWSTR lpszType, LPWSTR lpszName, LPARAM lParam)
{
    return ::EnumResourceLanguagesW(hMod, lpszType, lpszName, EnumResLangProc, lParam);
}

inline BOOL CALLBACK
EnumResTypeProc(HMODULE hMod, LPWSTR lpszType, LPARAM lParam)
{
    return ::EnumResourceNamesW(hMod, lpszType, EnumResNameProc, lParam);
}

inline BOOL
Res_GetListFromRes(HMODULE hMod, LPARAM lParam)
{
    return ::EnumResourceTypesW(hMod, EnumResTypeProc, lParam);
}

///////////////////////////////////////////////////////////////////////////////

inline MStringW
Res_GetTypeString(const MIdOrString& id_or_str)
{
    wchar_t sz[32];
    MStringW ret, name;
    switch (id_or_str.m_id)
    {
    case 1: name = L"RT_CURSOR"; break;
    case 2: name = L"RT_BITMAP"; break;
    case 3: name = L"RT_ICON"; break;
    case 4: name = L"RT_MENU"; break;
    case 5: name = L"RT_DIALOG"; break;
    case 6: name = L"RT_STRING"; break;
    case 7: name = L"RT_FONTDIR"; break;
    case 8: name = L"RT_FONT"; break;
    case 9: name = L"RT_ACCELERATOR"; break;
    case 10: name = L"RT_RCDATA"; break;
    case 11: name = L"RT_MESSAGETABLE"; break;
    case 12: name = L"RT_GROUP_CURSOR"; break;
    //
    case 14: name = L"RT_GROUP_ICON"; break;
    //
    case 16: name = L"RT_VERSION"; break;
    case 17: name = L"RT_DLGINCLUDE"; break;
    //
    case 19: name = L"RT_PLUGPLAY"; break;
    case 20: name = L"RT_VXD"; break;
    case 21: name = L"RT_ANICURSOR"; break;
    case 22: name = L"RT_ANIICON"; break;
    case 23: name = L"RT_HTML"; break;
    case 24: name = L"RT_MANIFEST"; break;
    case 240: name = L"RT_DLGINIT"; break;
    default:
        if (id_or_str.m_id != 0)
        {
            StringCchPrintfW(sz, _countof(sz), L"%u", id_or_str.m_id);
            ret = sz;
        }
        else
        {
            ret = id_or_str.m_str;
        }
    }
    if (name.size())
    {
        StringCchPrintfW(sz, _countof(sz), L" (%u)", id_or_str.m_id);
        ret = name;
        ret += sz;
    }
    return ret;
}

inline MStringW
Res_GetName(const ConstantsDB& db, const ResEntry& entry)
{
    MStringW ret;
    if (entry.name.m_id != 0)
    {
        WORD id = entry.name.m_id;
        if (entry.type == RT_CURSOR)
        {
            ;
        }
        else if (entry.type == RT_BITMAP)
        {
            ret += db.GetNameOfResID(IDTYPE_BITMAP, id);
        }
        else if (entry.type == RT_ICON)
        {
            ;
        }
        else if (entry.type == RT_MENU)
        {
            ret += db.GetNameOfResID(IDTYPE_MENU, id);
        }
        else if (entry.type == RT_DIALOG)
        {
            ret += db.GetNameOfResID(IDTYPE_DIALOG, id);
        }
        else if (entry.type == RT_STRING)
        {
            ;
        }
        else if (entry.type == RT_FONTDIR)
        {
            ret += db.GetNameOfResID(IDTYPE_RESOURCE, id);
        }
        else if (entry.type == RT_FONT)
        {
            ret += db.GetNameOfResID(IDTYPE_RESOURCE, id);
        }
        else if (entry.type == RT_ACCELERATOR)
        {
            ret += db.GetNameOfResID(IDTYPE_ACCEL, id);
        }
        else if (entry.type == RT_RCDATA)
        {
            ret += db.GetNameOfResID(IDTYPE_RESOURCE, id);
        }
        else if (entry.type == RT_MESSAGETABLE)
        {
            ;
        }
        else if (entry.type == RT_GROUP_CURSOR)
        {
            ret += db.GetNameOfResID(IDTYPE_CURSOR, id);
        }
        else if (entry.type == RT_GROUP_ICON)
        {
            ret += db.GetNameOfResID(IDTYPE_ICON, id);
        }
        else if (entry.type == RT_VERSION)
        {
            ;
        }
        else if (entry.type == RT_DLGINCLUDE)
        {
            ret += db.GetNameOfResID(IDTYPE_RESOURCE, id);
        }
        else if (entry.type == RT_PLUGPLAY)
        {
            ret += db.GetNameOfResID(IDTYPE_RESOURCE, id);
        }
        else if (entry.type == RT_VXD)
        {
            ret += db.GetNameOfResID(IDTYPE_RESOURCE, id);
        }
        else if (entry.type == RT_ANICURSOR)
        {
            ret += db.GetNameOfResID(IDTYPE_ANICURSOR, id);
        }
        else if (entry.type == RT_ANIICON)
        {
            ret += db.GetNameOfResID(IDTYPE_ANIICON, id);
        }
        else if (entry.type == RT_HTML)
        {
            ret += db.GetNameOfResID(IDTYPE_HTML, id);
        }
        else if (entry.type == RT_MANIFEST)
        {
            ;
        }
        else
        {
            ret += db.GetNameOfResID(IDTYPE_RESOURCE, id);
        }

        if (ret.size())
        {
            if (!mchr_is_digit(ret[0]))
            {
                ret += L" (";
                ret += mstr_dec_word(entry.name.m_id);
                ret += L")";
            }
        }
        else
        {
            ret += mstr_dec_word(entry.name.m_id);
        }
    }
    else
    {
        ret = entry.name.m_str;
    }
    return ret;
}

MStringW Res_GetLangName(WORD lang);

///////////////////////////////////////////////////////////////////////////////

inline BOOL
Res_UpdateExe(HWND hwnd, LPCWSTR ExeFile, const ResEntries& entries)
{
    HANDLE hUpdate = ::BeginUpdateResourceW(ExeFile, TRUE);
    if (hUpdate == NULL)
    {
        assert(0);
        return FALSE;
    }

    for (DWORD i = 0; i < DWORD(entries.size()); ++i)
    {
        const ResEntry& entry = entries[i];
        if (entry.empty())
            continue;

        void *pv = NULL;
        DWORD size = 0;
        if (!entry.empty())
        {
            pv = const_cast<void *>(entry.ptr());
            size = entry.size();
        }
        if (!::UpdateResourceW(hUpdate,
                               entry.type.ptr(), entry.name.ptr(), entry.lang,
                               pv, size))
        {
            assert(0);
            return FALSE;
        }
    }

    return ::EndUpdateResourceW(hUpdate, FALSE);
}

///////////////////////////////////////////////////////////////////////////////

inline LPARAM
TV_GetParam(HWND hwnd, HTREEITEM hItem = NULL)
{
    if (hItem == NULL)
    {
        hItem = TreeView_GetSelection(hwnd);
    }

    TV_ITEM item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    TreeView_GetItem(hwnd, &item);

    return item.lParam;
}

inline INT
TV_GetSelection(HWND hwnd, ResEntries& selection,
                const ResEntries& entries, HTREEITEM hItem = NULL)
{
    selection.clear();

    LPARAM lParam = TV_GetParam(hwnd, hItem);
    WORD i = LOWORD(lParam);
    if (i >= entries.size())
        return 0;

    ResEntry entry;
    switch (HIWORD(lParam))
    {
    case I_TYPE:
        entry = entries[i];
        entry.name.clear();
        entry.lang = 0xFFFF;
        break;
    case I_NAME:
        entry = entries[i];
        entry.lang = 0xFFFF;
        break;
    case I_LANG:
        entry = entries[i];
        break;
    case I_STRING:
    case I_MESSAGE:
        entry = entries[i];
        entry.name.clear();
        break;
    default:
        return 0;
    }

    Res_Search(selection, entries, entry);
    return INT(selection.size());
}

inline HTREEITEM
TV_MyInsert(HWND hwnd, HTREEITEM hParent, MStringW Text, LPARAM lParam)
{
    TV_INSERTSTRUCTW Insert;
    ZeroMemory(&Insert, sizeof(Insert));
    Insert.hParent = hParent;
    Insert.hInsertAfter = TVI_LAST;
    Insert.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM |
                       TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    Insert.item.state = 0;
    Insert.item.stateMask = 0;
    Insert.item.pszText = &Text[0];
    Insert.item.lParam = lParam;
    if (HIWORD(lParam) < I_LANG)
    {
        Insert.item.iImage = 1;
        Insert.item.iSelectedImage = 1;
    }
    else
    {
        Insert.item.iImage = 0;
        Insert.item.iSelectedImage = 0;
    }
    return TreeView_InsertItem(hwnd, &Insert);
}

inline HTREEITEM
_tv_FindOrInsertDepth3(HWND hwnd, const ConstantsDB& db, HTREEITEM hParent, 
                       const ResEntries& entries, INT i, INT k)
{
    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_LANG)
        {
            if (entries[LOWORD(lParam)].lang == entries[i].lang)
                return hItem;
        }
    }

    MStringW ResLang = Res_GetLangName(entries[i].lang);
    return TV_MyInsert(hwnd, hParent, ResLang, MAKELPARAM(k, I_LANG));
}   

inline HTREEITEM
_tv_FindOrInsertDepth2(HWND hwnd, const ConstantsDB& db, HTREEITEM hParent, 
                       const ResEntries& entries, INT i, INT k)
{
    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_NAME)
        {
            if (entries[LOWORD(lParam)].name == entries[i].name)
                return hItem;
        }
    }

    MStringW ResName = Res_GetName(db, entries[i]);
    return TV_MyInsert(hwnd, hParent, ResName, MAKELPARAM(k, I_NAME));
}

inline HTREEITEM
_tv_FindOrInsertDepth1(HWND hwnd, const ConstantsDB& db, HTREEITEM hParent,
                       const ResEntries& entries, INT i, INT k)
{
    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_TYPE)
        {
            if (entries[LOWORD(lParam)].type == entries[i].type)
                return hItem;
        }
    }

    MStringW ResType = Res_GetTypeString(entries[i].type);
    return TV_MyInsert(hwnd, hParent, ResType, MAKELPARAM(k, I_TYPE));
}

inline void
TV_SelectEntry(HWND hwnd, const ResEntries& entries, const ResEntry& entry)
{
    HTREEITEM hParent = NULL;

    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_TYPE)
        {
            if (entries[LOWORD(lParam)].type == entry.type)
            {
                hParent = hItem;
                break;
            }
        }
    }

    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_NAME)
        {
            if (entries[LOWORD(lParam)].type == entry.type &&
                entries[LOWORD(lParam)].name == entry.name)
            {
                hParent = hItem;
                break;
            }
        }
        else if (HIWORD(lParam) == I_STRING || HIWORD(lParam) == I_MESSAGE)
        {
            if (entries[LOWORD(lParam)].lang == entry.lang)
            {
                hParent = hItem;
                TreeView_SelectItem(hwnd, hParent);
                return;
            }
        }
    }

    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_LANG)
        {
            if (entries[LOWORD(lParam)].type == entry.type &&
                entries[LOWORD(lParam)].name == entry.name &&
                entries[LOWORD(lParam)].lang == entry.lang)
            {
                hParent = hItem;
                break;
            }
        }
    }

    TreeView_SelectItem(hwnd, hParent);
}

inline HTREEITEM
_tv_FindOrInsertString(HWND hwnd, HTREEITEM hParent,
                       const ResEntries& entries, INT iEntry)
{
    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_STRING)
        {
            if (entries[LOWORD(lParam)].lang == entries[iEntry].lang)
                return hItem;
        }
    }

    MStringW ResLang = Res_GetLangName(entries[iEntry].lang);
    return TV_MyInsert(hwnd, hParent, ResLang, MAKELPARAM(iEntry, I_STRING));
}

inline HTREEITEM
_tv_FindOrInsertMessage(HWND hwnd, HTREEITEM hParent,
                        const ResEntries& entries, INT iEntry)
{
    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_MESSAGE)
        {
            if (entries[LOWORD(lParam)].lang == entries[iEntry].lang)
                return hItem;
        }
    }

    MStringW ResLang = Res_GetLangName(entries[iEntry].lang);
    return TV_MyInsert(hwnd, hParent, ResLang, MAKELPARAM(iEntry, I_MESSAGE));
}

inline bool Res_Less(const ResEntry& entry1, const ResEntry& entry2)
{
    if (entry1.type < entry2.type)
        return true;
    if (entry1.type > entry2.type)
        return false;
    if (entry1.name < entry2.name)
        return true;
    if (entry1.name > entry2.name)
        return false;
    if (entry1.lang < entry2.lang)
        return true;
    if (entry1.lang > entry2.lang)
        return false;
    return false;
}

inline void Res_Sort(ResEntries& entries)
{
    std::sort(entries.begin(), entries.end(), Res_Less);
    std::unique(entries.begin(), entries.end());
}

inline void
TV_RefreshInfo(HWND hwnd, ConstantsDB& db, ResEntries& entries)
{
    TreeView_DeleteAllItems(hwnd);

    for (size_t i = entries.size(); i > 0;)
    {
        --i;
        if (entries[i].empty())
        {
            entries.erase(entries.begin() + i);
        }
    }

    Res_Sort(entries);

    for (INT i = 0; i < INT(entries.size()); ++i)
    {
        if (entries[i].empty())
            continue;

        HTREEITEM hItem = NULL;
        hItem = _tv_FindOrInsertDepth1(hwnd, db, hItem, entries, i, i);

        if (entries[i].type == RT_STRING)
        {
            _tv_FindOrInsertString(hwnd, hItem, entries, i);
        }
        if (entries[i].type == RT_MESSAGETABLE)
        {
            _tv_FindOrInsertMessage(hwnd, hItem, entries, i);
        }
    }

    INT k = 0;
    for (INT i = 0; i < INT(entries.size()); ++i)
    {
        if (entries[i].empty())
            continue;

        HTREEITEM hItem = NULL;
        hItem = _tv_FindOrInsertDepth1(hwnd, db, hItem, entries, i, k);
        hItem = _tv_FindOrInsertDepth2(hwnd, db, hItem, entries, i, k);
        hItem = _tv_FindOrInsertDepth3(hwnd, db, hItem, entries, i, k);
        hItem = hItem;
        ++k;
    }

    for (size_t i = entries.size(); i > 0; )
    {
        --i;
        if (entries[i].empty())
        {
            entries.erase(entries.begin() + i);
            continue;
        }
    }
}

inline void TV_Delete(HWND hwnd, ConstantsDB& db, HTREEITEM hItem, ResEntries& entries)
{
    LPARAM lParam = TV_GetParam(hwnd, hItem);

    WORD i = LOWORD(lParam);
    ResEntry entry = entries[i];

    INT nPos = GetScrollPos(hwnd, SB_VERT);

    switch (HIWORD(lParam))
    {
    case I_TYPE:
        entry.name.clear();
        entry.lang = 0xFFFF;
        Res_DeleteEntries(entries, entry);
        break;
    case I_NAME:
        entry.lang = 0xFFFF;
        Res_DeleteEntries(entries, entry);
        break;
    case I_LANG:
        Res_DeleteEntries(entries, entry);
        break;
    case I_STRING:
    case I_MESSAGE:
        entry.name.clear();
        Res_DeleteEntries(entries, entry);
        break;
    }

    TV_RefreshInfo(hwnd, db, entries);

    SetScrollPos(hwnd, SB_VERT, nPos, TRUE);
}

inline BOOL
Res_Optimize(ResEntries& entries)
{
    size_t count = entries.size();
    while (count-- > 0)
    {
        if (entries[count].empty())
        {
            entries.erase(entries.begin() + count);
        }
    }

    count = entries.size();
    for (size_t i = 0; i < count; ++i)
    {
        entries[i].updated = FALSE;
    }

    return TRUE;
}

inline BOOL
Res_ExtractGroupIcon(const ResEntries& entries,
                     const ResEntry& GroupIconEntry,
                     const wchar_t *OutputFileName)
{
    ICONDIR dir;
    if (GroupIconEntry.type != RT_GROUP_ICON ||
        GroupIconEntry.size() < sizeof(dir))
    {
        assert(0);
        return FALSE;
    }

    memcpy(&dir, &GroupIconEntry[0], sizeof(dir));
    if (dir.idReserved != 0 || dir.idType != RES_ICON || dir.idCount == 0)
    {
        assert(0);
        return FALSE;
    }

    DWORD SizeOfIconEntries = sizeof(GRPICONDIRENTRY) * dir.idCount;
    if (GroupIconEntry.size() < sizeof(dir) + SizeOfIconEntries)
    {
        assert(0);
        return FALSE;
    }

    std::vector<GRPICONDIRENTRY> GroupEntries(dir.idCount);
    memcpy(&GroupEntries[0], &GroupIconEntry[sizeof(dir)], SizeOfIconEntries);

    DWORD offset = sizeof(dir) + sizeof(ICONDIRENTRY) * dir.idCount;
    std::vector<ICONDIRENTRY> DirEntries(dir.idCount);
    for (WORD i = 0; i < dir.idCount; ++i)
    {
        INT k = Res_Find2(entries, RT_ICON, GroupEntries[i].nID,
                          GroupIconEntry.lang, FALSE);
        if (k == -1)
        {
            continue;
        }
        const ResEntry& IconEntry = entries[k];

        DirEntries[i].bWidth = GroupEntries[i].bWidth;
        DirEntries[i].bHeight = GroupEntries[i].bHeight;
        if (GroupEntries[i].wBitCount >= 8)
            DirEntries[i].bColorCount = 0;
        else
            DirEntries[i].bColorCount = GroupEntries[i].bColorCount;
        DirEntries[i].bReserved = 0;
        DirEntries[i].wPlanes = 1;
        DirEntries[i].wBitCount = GroupEntries[i].wBitCount;
        DirEntries[i].dwBytesInRes = IconEntry.size();
        DirEntries[i].dwImageOffset = offset;
        offset += DirEntries[i].dwBytesInRes;
    }

    MByteStreamEx stream;
    if (!stream.WriteRaw(dir))
    {
        assert(0);
        return FALSE;
    }

    DWORD SizeOfDirEntries = sizeof(ICONDIRENTRY) * dir.idCount;
    if (!stream.WriteData(&DirEntries[0], SizeOfDirEntries))
    {
        assert(0);
        return FALSE;
    }

    for (WORD i = 0; i < dir.idCount; ++i)
    {
        INT k = Res_Find2(entries, RT_ICON, GroupEntries[i].nID,
                          GroupIconEntry.lang, FALSE);
        if (k == -1)
        {
            continue;
        }
        const ResEntry& IconEntry = entries[k];

        DWORD dwSize = IconEntry.size();
        if (!stream.WriteData(&IconEntry[0], dwSize))
        {
            assert(0);
            return FALSE;
        }
    }

    return stream.SaveToFile(OutputFileName);
}

BOOL PackedDIB_GetInfo(const void *pPackedDIB, DWORD dwSize, BITMAP& bm);

inline BOOL
Res_ExtractIcon(const ResEntries& entries,
                const ResEntry& IconEntry,
                const wchar_t *OutputFileName)
{
    ICONDIR dir;
    dir.idReserved = 0;
    dir.idType = RES_ICON;
    dir.idCount = 1;

    BITMAP bm;
    if (!PackedDIB_GetInfo(&IconEntry[0], IconEntry.size(), bm))
    {
        MBitmapDx bitmap;
        bitmap.CreateFromMemory(&IconEntry[0], IconEntry.size());

        LONG cx, cy;
        HBITMAP hbm = bitmap.GetHBITMAP32(cx, cy);
        GetObject(hbm, sizeof(bm), &bm);
        DeleteObject(hbm);
    }

    ICONDIRENTRY entry;
    entry.bWidth = (BYTE)bm.bmWidth;
    entry.bHeight = (BYTE)bm.bmHeight;
    entry.bColorCount = 0;
    entry.bReserved = 0;
    entry.wPlanes = 1;
    entry.wBitCount = bm.bmBitsPixel;
    entry.dwBytesInRes = IconEntry.size();
    entry.dwImageOffset = sizeof(dir) + sizeof(ICONDIRENTRY);

    MByteStreamEx stream;
    if (!stream.WriteRaw(dir) ||
        !stream.WriteData(&entry, sizeof(entry)) ||
        !stream.WriteData(&IconEntry[0], IconEntry.size()))
    {
        assert(0);
        return FALSE;
    }

    return stream.SaveToFile(OutputFileName);
}

inline BOOL
Res_ExtractGroupCursor(const ResEntries& entries,
                       const ResEntry& GroupCursorEntry,
                       const wchar_t *OutputFileName)
{
    ICONDIR dir;
    if (GroupCursorEntry.type != RT_GROUP_CURSOR ||
        GroupCursorEntry.size() < sizeof(dir))
    {
        assert(0);
        return FALSE;
    }

    memcpy(&dir, &GroupCursorEntry[0], sizeof(dir));
    if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
    {
        assert(0);
        return FALSE;
    }

    DWORD SizeOfCursorEntries = sizeof(GRPCURSORDIRENTRY) * dir.idCount;
    if (GroupCursorEntry.size() < sizeof(dir) + SizeOfCursorEntries)
    {
        assert(0);
        return FALSE;
    }

    std::vector<GRPCURSORDIRENTRY> GroupEntries(dir.idCount);
    memcpy(&GroupEntries[0], &GroupCursorEntry[sizeof(dir)],
           SizeOfCursorEntries);

    DWORD offset = sizeof(dir) + sizeof(ICONDIRENTRY) * dir.idCount;
    std::vector<ICONDIRENTRY> DirEntries(dir.idCount);
    for (WORD i = 0; i < dir.idCount; ++i)
    {
        INT k = Res_Find2(entries, RT_CURSOR, GroupEntries[i].nID,
                          GroupCursorEntry.lang, FALSE);
        if (k == -1)
        {
            continue;
        }
        const ResEntry& CursorEntry = entries[k];
        LOCALHEADER local;
        if (CursorEntry.size() >= sizeof(local))
            memcpy(&local, &CursorEntry[0], sizeof(local));

        DirEntries[i].bWidth = (BYTE)GroupEntries[i].wWidth;
        DirEntries[i].bHeight = (BYTE)GroupEntries[i].wHeight;
        if (GroupEntries[i].wBitCount >= 8)
            DirEntries[i].bColorCount = 0;
        else
            DirEntries[i].bColorCount = 1 << GroupEntries[i].wBitCount;
        DirEntries[i].bReserved = 0;
        DirEntries[i].xHotSpot = local.xHotSpot;
        DirEntries[i].yHotSpot = local.yHotSpot;
        DirEntries[i].dwBytesInRes = CursorEntry.size() - sizeof(local);
        DirEntries[i].dwImageOffset = offset;
        offset += DirEntries[i].dwBytesInRes;
    }

    MByteStreamEx stream;
    if (!stream.WriteRaw(dir))
    {
        assert(0);
        return FALSE;
    }

    DWORD SizeOfDirEntries = sizeof(ICONDIRENTRY) * dir.idCount;
    if (!stream.WriteData(&DirEntries[0], SizeOfDirEntries))
    {
        assert(0);
        return FALSE;
    }

    for (WORD i = 0; i < dir.idCount; ++i)
    {
        INT k = Res_Find2(entries, RT_CURSOR, GroupEntries[i].nID,
                          GroupCursorEntry.lang, FALSE);
        if (k == -1)
        {
            continue;
        }
        const ResEntry& CursorEntry = entries[k];

        DWORD cbLocal = sizeof(LOCALHEADER);
        DWORD dwSize = CursorEntry.size() - cbLocal;
        LPBYTE pb = LPBYTE(&CursorEntry[0]) + cbLocal;
        if (!stream.WriteData(pb, dwSize))
        {
            assert(0);
            return FALSE;
        }
    }

    return stream.SaveToFile(OutputFileName);
}

inline BOOL
Res_ExtractCursor(const ResEntries& entries,
                  const ResEntry& CursorEntry,
                  const wchar_t *OutputFileName)
{
    ICONDIR dir;
    dir.idReserved = 0;
    dir.idType = RES_CURSOR;
    dir.idCount = 1;

    LOCALHEADER local;
    if (CursorEntry.size() < sizeof(local))
    {
        assert(0);
        return FALSE;
    }
    memcpy(&local, &CursorEntry[0], sizeof(local));

    BITMAP bm;
    LPBYTE pb = LPBYTE(&CursorEntry[0]) + sizeof(local);
    DWORD cb = CursorEntry.size() - sizeof(local);
    if (!PackedDIB_GetInfo(pb, cb, bm))
    {
        assert(0);
        return FALSE;
    }

    ICONDIRENTRY entry;
    entry.bWidth = (BYTE)bm.bmWidth;
    entry.bHeight = (BYTE)(bm.bmHeight / 2);
    entry.bColorCount = 0;
    entry.bReserved = 0;
    entry.xHotSpot = local.xHotSpot;
    entry.yHotSpot = local.yHotSpot;
    entry.dwBytesInRes = CursorEntry.size() - sizeof(local);
    entry.dwImageOffset = sizeof(dir) + sizeof(ICONDIRENTRY);

    DWORD cbLocal = sizeof(LOCALHEADER);
    pb = LPBYTE(&CursorEntry[0]) + cbLocal;
    cb = CursorEntry.size() - cbLocal;

    MByteStreamEx stream;
    if (!stream.WriteRaw(dir) ||
        !stream.WriteData(&entry, sizeof(entry)) ||
        !stream.WriteData(pb, cb))
    {
        assert(0);
        return FALSE;
    }

    return stream.SaveToFile(OutputFileName);
}

inline INT
Res_IsPlainText(const MIdOrString& type)
{
    return type == RT_HTML || type == RT_MANIFEST || type == RT_DLGINCLUDE;
}

inline INT
Res_IsTestable(const MIdOrString& type)
{
    return type == RT_DIALOG || type == RT_MENU;
}

inline BOOL
Res_CanGuiEdit(const MIdOrString& type)
{
    return type == RT_DIALOG || type == RT_MENU ||
           type == RT_STRING || type == RT_MESSAGETABLE ||
           type == RT_ACCELERATOR || type == WORD(240);
}

inline BOOL
Res_HasSample(const MIdOrString& type)
{
    return type == RT_ACCELERATOR || type == RT_DIALOG ||
           type == RT_MENU || type == RT_STRING || type == RT_VERSION ||
           type == RT_HTML || type == RT_MANIFEST || type == RT_MESSAGETABLE ||
           type == WORD(240);
}

inline BOOL
Res_HasNoName(const MIdOrString& type)
{
    return type == RT_STRING || type == RT_MESSAGETABLE;
}

inline void
Res_DeleteNames(ResEntries& entries, const MIdOrString& type, WORD lang)
{
    INT k;
    for (;;)
    {
        k = Res_Find(entries, type, (WORD)0, lang, FALSE);
        if (k == -1)
            break;

        ResEntry& entry = entries[k];
        entry.clear_data();
    }
}

///////////////////////////////////////////////////////////////////////////////

inline void
Res_DoIcon(ResEntries& entries, MTitleToIcon& title_to_icon, DialogItem& item, WORD lang)
{
    MIdOrString type = RT_GROUP_ICON;
    INT k = Res_Find2(entries, type, item.m_title, lang);
    if (k < 0 || k >= (INT)entries.size())
        return;

    ResEntry entry = entries[k];
    if (entry.size() < sizeof(ICONDIR) + sizeof(GRPICONDIRENTRY))
        return;

    ICONDIR& dir = (ICONDIR&)entry[0];
    GRPICONDIRENTRY *pGroupIcon = (GRPICONDIRENTRY *)&entry[sizeof(ICONDIR)];

    int cx = 0, cy = 0, bits = 0, n = 0;
    for (int m = 0; m < dir.idCount; ++m)
    {
        if (cx < pGroupIcon[m].bWidth ||
            cy < pGroupIcon[m].bHeight ||
            bits < pGroupIcon[m].wBitCount)
        {
            cx = pGroupIcon[m].bWidth;
            cy = pGroupIcon[m].bHeight;
            bits = pGroupIcon[m].wBitCount;
            n = m;
        }
    }

    type = RT_ICON;
    k = Res_Find2(entries, type, pGroupIcon[n].nID, lang);
    if (k < 0 || k >= (INT)entries.size())
        return;

    entry = entries[k];
    HICON hIcon = CreateIconFromResource((PBYTE)&entry[0], entry.size(), TRUE, 0x00030000);
    if (hIcon)
    {
        if (!item.m_title.empty())
        {
            if (title_to_icon[item.m_title])
                DestroyIcon(title_to_icon[item.m_title]);
            title_to_icon[item.m_title] = hIcon;
        }
    }
}

inline void
Res_DoBitmap(ResEntries& entries, MTitleToBitmap& title_to_bitmap, DialogItem& item, WORD lang)
{
    MIdOrString type = RT_BITMAP;
    INT k = Res_Find2(entries, type, item.m_title, lang);
    if (k < 0 || k >= (INT)entries.size())
        return;

    ResEntry& entry = entries[k];
    HBITMAP hbm = PackedDIB_CreateBitmapFromMemory(&entry[0], entry.size());
    if (hbm)
    {
        if (!item.m_title.empty())
        {
            if (title_to_bitmap[item.m_title])
                DeleteObject(title_to_bitmap[item.m_title]);
            title_to_bitmap[item.m_title] = hbm;
        }
    }
}

inline void
ClearMaps(MTitleToBitmap& title_to_bitmap, MTitleToIcon& title_to_icon)
{
    {
        MTitleToBitmap::iterator it, end = title_to_bitmap.end();
        for (it = title_to_bitmap.begin(); it != end; ++it)
        {
            DeleteObject(it->second);
        }
        title_to_bitmap.clear();
    }
    {
        MTitleToIcon::iterator it, end = title_to_icon.end();
        for (it = title_to_icon.begin(); it != end; ++it)
        {
            DestroyIcon(it->second);
        }
        title_to_icon.clear();
    }
}

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef RES_HPP_
