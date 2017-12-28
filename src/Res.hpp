// Res.hpp --- Win32 Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Win32API resource editor
// Copyright (C) 2017 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
#include "id_string.hpp"
#include "IconRes.hpp"
#include "MByteStreamEx.hpp"
#include "MString.hpp"
#include "PackedDIB.hpp"
#include "MBitmapDx.hpp"

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

    ResEntry(const MIdOrString& Type, const MIdOrString& Name, WORD Lang,
             BOOL Updated = TRUE)
     : type(Type), name(Name), lang(Lang), updated(Updated)
    {
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

    void assign(const void *ptr, size_type Size)
    {
        if (ptr && Size)
        {
            data.resize(Size);
            memcpy(&data[0], ptr, Size);
        }
        else
        {
            data.clear();
        }
        updated = TRUE;
    }
};
typedef std::vector<ResEntry> ResEntries;

#define    MEMORYFLAG_MOVEABLE     0x0010 
#define    MEMORYFLAG_PURE         0x0020 
#define    MEMORYFLAG_PRELOAD      0x0040 
#define    MEMORYFLAG_DISCARDABLE  0x1000 

struct ResourceHeader
{
    DWORD           DataSize;
    DWORD           HeaderSize;
    MIdOrString     Type;
    MIdOrString     Name;
    DWORD           DataVersion;
    WORD            MemoryFlags;
    WORD            LanguageId;
    DWORD           Version;
    DWORD           Characteristics;

    ResourceHeader()
    {
        DataSize = 0;
        HeaderSize = 0x20;
        Type = (WORD)0;
        Name = (WORD)0;
        DataVersion = 0;
        MemoryFlags = 0;
        LanguageId = 0;
        Version = 0;
        Characteristics = 0;
    }

    BOOL ReadFrom(const MByteStreamEx& bs)
    {
        if (!bs.ReadRaw(DataSize) || !bs.ReadRaw(HeaderSize) ||
            !bs.ReadID(Type) || !bs.ReadID(Name))
        {
            return FALSE;
        }
        bs.ReadDwordAlignment();

        if (!bs.ReadRaw(DataVersion) || !bs.ReadRaw(MemoryFlags) ||
            !bs.ReadRaw(LanguageId) || !bs.ReadRaw(Version) ||
            !bs.ReadRaw(Characteristics))
        {
            return FALSE;
        }
        bs.ReadDwordAlignment();

        return TRUE;
    }

    BOOL WriteTo(MByteStreamEx& bs) const
    {
        if (!bs.WriteRaw(DataSize) || !bs.WriteRaw(HeaderSize) ||
            !bs.WriteID(Type) || !bs.WriteID(Name))
        {
            return FALSE;
        }
        bs.WriteDwordAlignment();

        if (!bs.WriteRaw(DataVersion) || !bs.WriteRaw(MemoryFlags) ||
            !bs.WriteRaw(LanguageId) || !bs.WriteRaw(Version) ||
            !bs.WriteRaw(Characteristics))
        {
            return FALSE;
        }
        bs.WriteDwordAlignment();

        return TRUE;
    }

    DWORD GetHeaderSize(MIdOrString type, MIdOrString name) const
    {
        size_t size = 0;
        if (type.is_str())
            size += (type.m_Str.size() + 1) * sizeof(WCHAR);
        else
            size += sizeof(WORD) * 2;

        if (name.is_str())
            size += (name.m_Str.size() + 1) * sizeof(WCHAR);
        else
            size += sizeof(WORD) * 2;

        if (size & 3)
            size += 4 - (size & 3);

        return (DWORD)(sizeof(DWORD) * 6 + size);
    }
};

inline INT
Res_Find(const ResEntries& Entries,
         const MIdOrString& Type, 
         const MIdOrString& Name,
         WORD Lang, BOOL bEmptyOK)
{
    size_t i, count = Entries.size();
    for (i = 0; i < count; ++i)
    {
        const ResEntry& entry = Entries[i];
        if (!Type.is_zero() && entry.type != Type)
            continue;
        if (!Name.is_zero() && entry.name != Name)
            continue;
        if (Lang != 0xFFFF && entry.lang != Lang)
            continue;
        if (entry.empty() && !bEmptyOK)
            continue;
        return INT(i);
    }
    return -1;
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
Res_Find(const ResEntries& Entries, const ResEntry& Entry, BOOL bEmptyOK)
{
    return Res_Find(Entries, Entry.type, Entry.name, Entry.lang, bEmptyOK);
}

inline void
Res_Search(ResEntries& Found,
           const ResEntries& Entries,
           const MIdOrString& Type, 
           const MIdOrString& Name,
           WORD Lang)
{
    Found.clear();

    size_t i, count = Entries.size();
    for (i = 0; i < count; ++i)
    {
        const ResEntry& entry = Entries[i];
        if (!Type.is_zero() && entry.type != Type)
            continue;
        if (!Name.is_zero() && entry.name != Name)
            continue;
        if (Lang != 0xFFFF && entry.lang != Lang)
            continue;
        if (entry.empty())
            continue;

        Found.push_back(entry);
    }
}

inline void
Res_Search(ResEntries& Found, const ResEntries& Entries, const ResEntry& Entry)
{
    Res_Search(Found, Entries, Entry.type, Entry.name, Entry.lang);
}

inline UINT
Res_GetLastIconID(const ResEntries& Entries)
{
    WORD last_id = 0;

    ResEntries::const_iterator it, end = Entries.end();
    for (it = Entries.begin(); it != end; ++it)
    {
        if (it->type != RT_ICON || !it->name.is_int() || it->name.is_zero())
            continue;

        if (last_id < it->name.m_ID)
            last_id = it->name.m_ID;
    }
    return last_id;
}

inline UINT
Res_GetLastCursorID(const ResEntries& Entries)
{
    WORD last_id = 0;

    ResEntries::const_iterator it, end = Entries.end();
    for (it = Entries.begin(); it != end; ++it)
    {
        if (it->type != RT_CURSOR || !it->name.is_int() || it->name.is_zero())
            continue;

        if (last_id < it->name.m_ID)
            last_id = it->name.m_ID;
    }
    return last_id;
}

inline void
Res_AddEntryFromRes(HMODULE hMod, ResEntries& Entries,
                    LPCWSTR Type, LPCWSTR Name, WORD Lang)
{
    ResEntry Entry(Type, Name, Lang);

    HRSRC hResInfo = FindResourceExW(hMod, Type, Name, Lang);
    if (hResInfo)
    {
        DWORD Size = SizeofResource(hMod, hResInfo);
        HGLOBAL hGlobal = LoadResource(hMod, hResInfo);
        LPVOID pv = LockResource(hGlobal);
        if (pv && Size)
        {
            Entry.assign(pv, Size);
        }
    }

    Entries.push_back(Entry);
}

inline BOOL
Res_AddEntry(ResEntries& Entries, const ResEntry& Entry,
             BOOL Replace = FALSE)
{
    INT iEntry = Res_Find(Entries, Entry, TRUE);
    if (iEntry != -1)
    {
        if (!Replace && !Entries[iEntry].empty())
        {
            return FALSE;
        }

        Entries[iEntry] = Entry;
    }
    else
    {
        Entries.push_back(Entry);
    }

    return TRUE;
}

inline BOOL
Res_AddEntry(ResEntries& Entries, const MIdOrString& Type,
             const MIdOrString& Name, WORD Lang,
             const ResEntry::DataType& Data, BOOL Replace = FALSE)
{
    ResEntry Entry(Type, Name, Lang);
    Entry.data = Data;
    return Res_AddEntry(Entries, Entry, Replace);
}

inline BOOL
Res_DeleteGroupIcon(ResEntries& Entries, ResEntry& Entry)
{
    assert(Entry.type == RT_GROUP_ICON);

    MByteStreamEx bs(Entry.data);

    ICONDIR dir;
    if (!bs.ReadRaw(dir))
        return FALSE;

    DWORD size = sizeof(GRPICONDIRENTRY) * dir.idCount;
    std::vector<GRPICONDIRENTRY> DirEntries(dir.idCount);
    if (!bs.ReadData(&DirEntries[0], size))
    {
        return FALSE;
    }

    DWORD i, Count = dir.idCount;
    for (i = 0; i < Count; ++i)
    {
        INT k = Res_Find(Entries, RT_ICON, DirEntries[i].nID, Entry.lang, TRUE);
		if (k != -1)
	        Entries[k].clear_data();
    }

    Entry.clear_data();
    return TRUE;
}

inline BOOL
Res_DeleteGroupCursor(ResEntries& Entries, ResEntry& Entry)
{
    assert(Entry.type == RT_GROUP_CURSOR);

    MByteStreamEx bs(Entry.data);

    ICONDIR dir;
    if (!bs.ReadRaw(dir))
        return FALSE;

    DWORD size = sizeof(GRPCURSORDIRENTRY) * dir.idCount;
    std::vector<GRPCURSORDIRENTRY> DirEntries(dir.idCount);
    if (!bs.ReadData(&DirEntries[0], size))
    {
        return FALSE;
    }

    DWORD i, Count = dir.idCount;
    for (i = 0; i < Count; ++i)
    {
        INT k = Res_Find(Entries, RT_CURSOR, DirEntries[i].nID, Entry.lang, TRUE);
        Entries[k].clear_data();
    }

    Entry.clear_data();
    return TRUE;
}

inline BOOL
Res_DeleteEntries(ResEntries& Entries, const MIdOrString& Type,
                  const MIdOrString& Name, WORD Lang)
{
    BOOL bFound = FALSE;
    for (;;)
    {
        INT iEntry = Res_Find(Entries, Type, Name, Lang, FALSE);
        if (iEntry == -1)
            break;

        ResEntry& Entry = Entries[iEntry];

        if (Entry.type == RT_GROUP_ICON)
        {
            Res_DeleteGroupIcon(Entries, Entry);
        }
        if (Entry.type == RT_GROUP_CURSOR)
        {
            Res_DeleteGroupCursor(Entries, Entry);
        }
        else
        {
            Entry.clear_data();
        }

        bFound = TRUE;
    }

    return bFound;
}

inline BOOL
Res_DeleteEntries(ResEntries& Entries, const ResEntry& Entry)
{
    return Res_DeleteEntries(Entries, Entry.type, Entry.name, Entry.lang);
}

inline BOOL
Res_AddGroupIcon(ResEntries& Entries, const MIdOrString& Name,
                 WORD Lang, const std::wstring& FileName,
                 BOOL Replace = FALSE)
{
    IconFile icon;
    if (!icon.LoadFromFile(FileName.c_str()) || icon.type() != RES_ICON)
        return FALSE;

    UINT LastIconID = Res_GetLastIconID(Entries);
    UINT NextIconID = LastIconID + 1;
    IconFile::DataType group(icon.GetIconGroup(NextIconID));
    Res_AddEntry(Entries, RT_GROUP_ICON, Name, Lang, group, Replace);

    int i, Count = icon.GetImageCount();
    for (i = 0; i < Count; ++i)
    {
        Res_AddEntry(Entries, RT_ICON, WORD(NextIconID + i), Lang,
                     icon.GetImage(i));
    }
    return TRUE;
}

inline BOOL
Res_AddGroupCursor(ResEntries& Entries, const MIdOrString& Name,
                   WORD Lang, const std::wstring& FileName,
                   BOOL Replace = FALSE)
{
    CursorFile cur;
    if (!cur.LoadFromFile(FileName.c_str()) || cur.type() != RES_CURSOR)
        return FALSE;

    UINT LastCursorID = Res_GetLastCursorID(Entries);
    UINT NextCursorID = LastCursorID + 1;
    CursorFile::DataType group(cur.GetCursorGroup(NextCursorID));
    Res_AddEntry(Entries, RT_GROUP_CURSOR, Name, Lang, group, Replace);

    int i, Count = cur.GetImageCount();
    for (i = 0; i < Count; ++i)
    {
        Res_AddEntry(Entries, RT_CURSOR, WORD(NextCursorID + i), Lang,
                     cur.GetImage(i));
    }
    return TRUE;
}

inline BOOL
Res_AddBitmap(ResEntries& Entries, const MIdOrString& Name,
              WORD Lang, const std::wstring& BitmapFile, BOOL Replace = FALSE)
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

        Res_AddEntry(Entries, RT_BITMAP, Name, Lang, PackedDIB, Replace);
    }
    else
    {
        size_t FileHeadSize = sizeof(BITMAPFILEHEADER);
        if (stream.size() < FileHeadSize)
            return FALSE;

        size_t i0 = FileHeadSize, i1 = stream.size();
        ResEntry::DataType HeadLess(&stream[i0], &stream[i0] + (i1 - i0));
        Res_AddEntry(Entries, RT_BITMAP, Name, Lang, HeadLess, Replace);
    }

    return TRUE;
}

inline BOOL CALLBACK
EnumResLangProc(HMODULE hMod, LPCWSTR lpszType, LPCWSTR lpszName,
                WORD wIDLanguage, LPARAM lParam)
{
    ResEntries& Entries = *(ResEntries *)lParam;
    Res_AddEntryFromRes(hMod, Entries, lpszType, lpszName, wIDLanguage);
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

inline std::wstring
Res_GetType(const MIdOrString& id_or_str)
{
    wchar_t sz[32];
    std::wstring ret, name;
    switch (id_or_str.m_ID)
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
    default:
        if (id_or_str.m_ID != 0)
        {
            wsprintfW(sz, L"%u", id_or_str.m_ID);
            ret = sz;
        }
        else
        {
            ret = id_or_str.m_Str;
        }
    }
    if (name.size())
    {
        wsprintfW(sz, L" (%u)", id_or_str.m_ID);
        ret = name;
        ret += sz;
    }
    return ret;
}

inline std::wstring
Res_GetName(const MIdOrString& id_or_str)
{
    std::wstring ret;
    if (id_or_str.m_ID != 0)
    {
        ret = mstr_dec_word(id_or_str.m_ID);
    }
    else
    {
        ret = id_or_str.m_Str;
    }
    return ret;
}

std::wstring Res_GetLangName(WORD Lang);

///////////////////////////////////////////////////////////////////////////////

inline BOOL
Res_UpdateExe(HWND hwnd, LPCWSTR ExeFile, const ResEntries& Entries)
{
    HANDLE hUpdate = ::BeginUpdateResourceW(ExeFile, TRUE);
    if (hUpdate == NULL)
    {
        assert(0);
        return FALSE;
    }

    for (DWORD i = 0; i < DWORD(Entries.size()); ++i)
    {
        const ResEntry& Entry = Entries[i];
        if (Entry.empty())
            continue;

        void *pv = NULL;
        DWORD size = 0;
        if (!Entry.empty())
        {
            pv = const_cast<void *>(Entry.ptr());
            size = Entry.size();
        }
        if (!::UpdateResourceW(hUpdate,
                               Entry.type.Ptr(), Entry.name.Ptr(), Entry.lang,
                               pv, size))
        {
            DWORD dwError = GetLastError();
            WCHAR szFormat[128], sz[128];
            LoadStringW(GetModuleHandle(NULL), 27, szFormat, _countof(szFormat));
            wsprintfW(sz, szFormat, dwError);
            MessageBoxW(hwnd, sz, NULL, MB_ICONERROR);
            assert(0);
            ::EndUpdateResourceW(hUpdate, TRUE);
            dwError = dwError;
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

    TV_ITEM Item;
    ZeroMemory(&Item, sizeof(Item));
    Item.mask = TVIF_PARAM;
    Item.hItem = hItem;
    TreeView_GetItem(hwnd, &Item);
    return Item.lParam;
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
TV_MyInsert(HWND hwnd, HTREEITEM hParent, std::wstring Text,
            LPARAM lParam, BOOL Expand)
{
    TV_INSERTSTRUCTW Insert;
    ZeroMemory(&Insert, sizeof(Insert));
    Insert.hParent = hParent;
    Insert.hInsertAfter = TVI_LAST;
    Insert.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM |
                       TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    if (Expand)
    {
        Insert.item.state = TVIS_EXPANDED;
        Insert.item.stateMask = TVIS_EXPANDED;
    }
    else
    {
        Insert.item.state = 0;
        Insert.item.stateMask = 0;
    }
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
_tv_FindOrInsertDepth3(HWND hwnd, HTREEITEM hParent,
                       const ResEntries& Entries, INT i, INT k, BOOL Expand)
{
    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_LANG)
        {
            if (Entries[LOWORD(lParam)].lang == Entries[i].lang)
                return hItem;
        }
    }

    std::wstring ResLang = Res_GetLangName(Entries[i].lang);
    return TV_MyInsert(hwnd, hParent, ResLang,
                       MAKELPARAM(k, I_LANG), Expand);
}   

inline HTREEITEM
_tv_FindOrInsertDepth2(HWND hwnd, HTREEITEM hParent,
                       const ResEntries& Entries, INT i, INT k, BOOL Expand)
{
    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_NAME)
        {
            if (Entries[LOWORD(lParam)].name == Entries[i].name)
                return hItem;
        }
    }

    std::wstring ResName = Res_GetName(Entries[i].name);
    return TV_MyInsert(hwnd, hParent, ResName,
                       MAKELPARAM(k, I_NAME), Expand);
}

inline HTREEITEM
_tv_FindOrInsertDepth1(HWND hwnd, HTREEITEM hParent,
                       const ResEntries& Entries, INT i, INT k, BOOL Expand)
{
    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_TYPE)
        {
            if (Entries[LOWORD(lParam)].type == Entries[i].type)
                return hItem;
        }
    }

    std::wstring ResType = Res_GetType(Entries[i].type);
    return TV_MyInsert(hwnd, hParent, ResType,
                       MAKELPARAM(k, I_TYPE), Expand);
}

inline void
TV_SelectEntry(HWND hwnd, const ResEntries& Entries, const ResEntry& entry)
{
    HTREEITEM hParent = NULL;

    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_TYPE)
        {
            if (Entries[LOWORD(lParam)].type == entry.type)
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
            if (Entries[LOWORD(lParam)].type == entry.type &&
                Entries[LOWORD(lParam)].name == entry.name)
            {
                hParent = hItem;
                break;
            }
        }
        else if (HIWORD(lParam) == I_STRING || HIWORD(lParam) == I_MESSAGE)
        {
            if (Entries[LOWORD(lParam)].lang == entry.lang)
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
            if (Entries[LOWORD(lParam)].type == entry.type &&
                Entries[LOWORD(lParam)].name == entry.name &&
                Entries[LOWORD(lParam)].lang == entry.lang)
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
                       const ResEntries& Entries, INT iEntry, BOOL Expand)
{
    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_STRING)
        {
            if (Entries[LOWORD(lParam)].lang == Entries[iEntry].lang)
                return hItem;
        }
    }

    std::wstring ResLang = Res_GetLangName(Entries[iEntry].lang);
    return TV_MyInsert(hwnd, hParent, ResLang,
                       MAKELPARAM(iEntry, I_STRING), Expand);
}

inline HTREEITEM
_tv_FindOrInsertMessage(HWND hwnd, HTREEITEM hParent,
                       const ResEntries& Entries, INT iEntry, BOOL Expand)
{
    for (HTREEITEM hItem = TreeView_GetChild(hwnd, hParent);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwnd, hItem))
    {
        LPARAM lParam = TV_GetParam(hwnd, hItem);
        if (HIWORD(lParam) == I_MESSAGE)
        {
            if (Entries[LOWORD(lParam)].lang == Entries[iEntry].lang)
                return hItem;
        }
    }

    std::wstring ResLang = Res_GetLangName(Entries[iEntry].lang);
    return TV_MyInsert(hwnd, hParent, ResLang,
                       MAKELPARAM(iEntry, I_MESSAGE), Expand);
}

inline void
TV_RefreshInfo(HWND hwnd, ResEntries& Entries, BOOL bNewlyOpen)
{
    TreeView_DeleteAllItems(hwnd);

    BOOL Expand = FALSE;
    if (bNewlyOpen)
    {
        Expand = TRUE;
        if (Entries.size() >= 24)
            Expand = FALSE;
    }

    for (INT i = 0; i < INT(Entries.size()); ++i)
    {
        if (Entries[i].empty())
            continue;

        HTREEITEM hItem = NULL;
        hItem = _tv_FindOrInsertDepth1(hwnd, hItem, Entries, i, i, Expand);

        if (Entries[i].type == RT_STRING)
        {
            _tv_FindOrInsertString(hwnd, hItem, Entries, i, Expand);
        }
        if (Entries[i].type == RT_MESSAGETABLE)
        {
            _tv_FindOrInsertMessage(hwnd, hItem, Entries, i, Expand);
        }
    }

	INT k = 0;
    for (INT i = 0; i < INT(Entries.size()); ++i)
    {
        if (Entries[i].empty())
            continue;

        HTREEITEM hItem = NULL;
        hItem = _tv_FindOrInsertDepth1(hwnd, hItem, Entries, i, k, Expand);
        hItem = _tv_FindOrInsertDepth2(hwnd, hItem, Entries, i, k, Expand);
        hItem = _tv_FindOrInsertDepth3(hwnd, hItem, Entries, i, k, Expand);
        hItem = hItem;
		++k;
    }

    for (size_t i = Entries.size(); i > 0; )
    {
        --i;
        if (Entries[i].empty())
        {
            Entries.erase(Entries.begin() + i);
            continue;
        }
    }
}

inline void TV_Delete(HWND hwnd, HTREEITEM hItem, ResEntries& Entries)
{
    LPARAM lParam = TV_GetParam(hwnd, hItem);

    WORD i = LOWORD(lParam);
    ResEntry Entry = Entries[i];

    INT nPos = GetScrollPos(hwnd, SB_VERT);

    switch (HIWORD(lParam))
    {
    case I_TYPE:
        Entry.name.clear();
        Entry.lang = 0xFFFF;
        Res_DeleteEntries(Entries, Entry);
        break;
    case I_NAME:
        Entry.lang = 0xFFFF;
        Res_DeleteEntries(Entries, Entry);
        break;
    case I_LANG:
        Res_DeleteEntries(Entries, Entry);
        break;
    case I_STRING:
    case I_MESSAGE:
        Entry.name.clear();
        Res_DeleteEntries(Entries, Entry);
        break;
    }

    TV_RefreshInfo(hwnd, Entries, FALSE);

    SetScrollPos(hwnd, SB_VERT, nPos, TRUE);
}

inline BOOL
Res_Optimize(ResEntries& Entries)
{
    size_t count = Entries.size();
    while (count-- > 0)
    {
        if (Entries[count].empty())
        {
            Entries.erase(Entries.begin() + count);
        }
    }

    count = Entries.size();
    for (size_t i = 0; i < count; ++i)
    {
        Entries[i].updated = FALSE;
    }

    return TRUE;
}

inline BOOL
Res_ExtractGroupIcon(const ResEntries& Entries,
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
        INT k = Res_Find(Entries, RT_ICON, GroupEntries[i].nID,
                         GroupIconEntry.lang, FALSE);
        if (k == -1)
        {
            k = Res_Find(Entries, RT_ICON, GroupEntries[i].nID, 0xFFFF, FALSE);
        }
        if (k == -1)
        {
            continue;
        }
        const ResEntry& IconEntry = Entries[k];

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
        INT k = Res_Find(Entries, RT_ICON, GroupEntries[i].nID,
                         GroupIconEntry.lang, FALSE);
        if (k == -1)
        {
            k = Res_Find(Entries, RT_ICON, GroupEntries[i].nID, 0xFFFF, FALSE);
        }
        if (k == -1)
        {
            continue;
        }
        const ResEntry& IconEntry = Entries[k];

        DWORD dwSize = IconEntry.size();
        if (!stream.WriteData(&IconEntry[0], dwSize))
        {
            assert(0);
            return FALSE;
        }
    }

    return stream.SaveToFile(OutputFileName);
}

BOOL PackedDIB_GetInfo(const void *pPackedDIB, DWORD Size, BITMAP& bm);

inline BOOL
Res_ExtractIcon(const ResEntries& Entries,
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
Res_ExtractGroupCursor(const ResEntries& Entries,
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
        INT k = Res_Find(Entries, RT_CURSOR, GroupEntries[i].nID,
                         GroupCursorEntry.lang, FALSE);
        if (k == -1)
        {
            k = Res_Find(Entries, RT_CURSOR, GroupEntries[i].nID, 0xFFFF, FALSE);
        }
        if (k == -1)
        {
            continue;
        }
        const ResEntry& CursorEntry = Entries[k];
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
        INT k = Res_Find(Entries, RT_CURSOR, GroupEntries[i].nID,
                         GroupCursorEntry.lang, FALSE);
        if (k == -1)
        {
            k = Res_Find(Entries, RT_CURSOR, GroupEntries[i].nID, 0xFFFF, FALSE);
        }
        if (k == -1)
        {
            continue;
        }
        const ResEntry& CursorEntry = Entries[k];

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
Res_ExtractCursor(const ResEntries& Entries,
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
           type == RT_ACCELERATOR;
}

inline BOOL
Res_HasSample(const MIdOrString& type)
{
    return type == RT_ACCELERATOR || type == RT_DIALOG ||
           type == RT_MENU || type == RT_STRING || type == RT_VERSION ||
           type == RT_HTML || type == RT_MANIFEST;
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

#endif  // ndef RES_HPP_
