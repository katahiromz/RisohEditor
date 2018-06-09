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
#include <set>          // for std::set
#include <algorithm>    // for std::sort

#include "IconRes.hpp"
#include "MByteStreamEx.hpp"
#include "MString.hpp"
#include "PackedDIB.hpp"
#include "MBitmapDx.hpp"
#include "ConstantsDB.hpp"
#include "DialogRes.hpp"

struct ResEntry;
struct EntrySet;

typedef std::map<MIdOrString, HBITMAP>  MTitleToBitmap;
typedef std::map<MIdOrString, HICON>    MTitleToIcon;

BOOL PackedDIB_GetInfo(const void *pPackedDIB, DWORD dwSize, BITMAP& bm);

///////////////////////////////////////////////////////////////////////////////

#ifndef RT_HTML
    #define RT_HTML         MAKEINTRESOURCE(23)
#endif
#ifndef RT_MANIFEST
    #define RT_MANIFEST     MAKEINTRESOURCE(24)
#endif

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
Res_IsPlainText(const MIdOrString& type)
{
    return type == RT_HTML || type == RT_MANIFEST ||
           type == RT_DLGINCLUDE || type == L"RISOHTEMPLATE";
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
           type == WORD(240) || type == L"RISOHTEMPLATE";
}

inline BOOL
Res_HasNoName(const MIdOrString& type)
{
    return type == RT_STRING || type == RT_MESSAGETABLE;
}

inline MStringW
Res_GetTypeString(const MIdOrString& id_or_str)
{
    // FIXME: Use ConstantsDB
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

inline HWND&
Res_GetTreeViewHandle(void)
{
    static HWND hwndTV = NULL;
    return hwndTV;
}

MStringW Res_GetLangName(WORD lang);

///////////////////////////////////////////////////////////////////////////////
// EntryType

enum EntryType
{
    I_NONE,     // None. Don't use.
    I_TYPE,     // TypeEntry.
    I_NAME,     // NameEntry.
    I_LANG,     // LangEntry.
    I_STRING,   // StringEntry.
    I_MESSAGE   // MessageEntry.
};

///////////////////////////////////////////////////////////////////////////////
// EntryBase, TypeEntry, NameEnry, LangEntry, StringEntry, MessageEntry

struct EntryBase
{
    typedef DWORD               size_type;
    typedef std::vector<BYTE>   data_type;
    EntryType       m_e_type;

    MIdOrString     m_type;
    MIdOrString     m_name;
    WORD            m_lang = 0xFFFF;
    INT             m_cRefs = -1;
    HTREEITEM       m_hItem = NULL;

    EntryBase(EntryType e_type, const MIdOrString& type,
              const MIdOrString& name = L"", WORD lang = 0xFFFF)
        : m_e_type(e_type), m_type(type), m_name(name), m_lang(lang)
    {
    }
    virtual ~EntryBase()
    {
    }

    bool match(EntryType e_type, const MIdOrString& type, const MIdOrString& name, WORD lang = 0xFFFF) const
    {
        if (e_type != I_NONE && m_e_type != e_type)
            return false;
        if (!type.is_zero() && m_type != type)
            return false;
        if (!name.is_zero() && m_name != name)
            return false;
        if (lang != 0xFFFF && m_lang != lang)
            return false;
        return true;
    }

    bool operator==(const EntryBase& entry) const
    {
        return m_lang == entry.m_lang &&
               m_type == entry.m_type &&
               m_name == entry.m_name;
    }
    bool operator!=(const EntryBase& entry) const
    {
        return !(*this == entry);
    }
    bool operator<(const EntryBase& entry) const
    {
        if (m_type < entry.m_type)
            return true;
        if (m_type > entry.m_type)
            return false;
        if (m_name < entry.m_name)
            return true;
        if (m_name > entry.m_name)
            return false;
        if (m_lang < entry.m_lang)
            return true;
        if (m_lang > entry.m_lang)
            return false;
        return false;
    }
};

struct TypeEntry : EntryBase
{
    TypeEntry(const MIdOrString& type) : EntryBase(I_TYPE, type)
    {
    }
};

struct NameEntry : EntryBase
{
    NameEntry(const MIdOrString& type, const MIdOrString& name)
        : EntryBase(I_NAME, type, name)
    {
    }
};

struct StringEntry : EntryBase
{
    StringEntry(const MIdOrString& type, WORD lang)
        : EntryBase(I_STRING, RT_STRING, L"", lang)
    {
    }
};

struct MessageEntry : EntryBase
{
    MessageEntry(const MIdOrString& type, WORD lang)
        : EntryBase(I_STRING, RT_STRING, L"", lang)
    {
    }
};

struct LangEntry : EntryBase
{
    data_type m_data;
    MStringW  m_strText;

    LangEntry(const MIdOrString& type, const MIdOrString& name, WORD lang = 0xFFFF)
        : EntryBase(I_NAME, type, name, lang)
    {
    }
    LangEntry(const MIdOrString& type, const MIdOrString& name, WORD lang,
              const data_type& data)
        : EntryBase(I_NAME, type, name, lang), m_data(data)
    {
    }
    LangEntry(const MIdOrString& type, const MIdOrString& name, WORD lang,
              const MStringW& strText)
        : EntryBase(I_NAME, type, name, lang), m_strText(strText)
    {
    }
    virtual ~LangEntry()
    {
    }

    void clear_data()
    {
        m_data.clear();
        m_strText.clear();
    }

    void clear()
    {
        clear_data();
        m_lang = 0xFFFF;
        m_name = (WORD)0;
        m_type = (WORD)0;
    }

    bool empty() const
    {
        return size() == 0 && m_strText.empty();
    }
    size_type size() const
    {
        return size_type(m_data.size());
    }
    BYTE& operator[](DWORD index)
    {
        assert(index <= m_data.size());
        return m_data[index];
    }
    const BYTE& operator[](DWORD index) const
    {
        assert(index <= m_data.size());
        return m_data[index];
    }

    void *ptr(DWORD index = 0)
    {
        return &m_data[index];
    }
    const void *ptr(DWORD index = 0) const
    {
        return &m_data[index];
    }

    void assign(const void *ptr, size_type nSize)
    {
        if (ptr && nSize)
        {
            m_data.resize(nSize);
            memcpy(&m_data[0], ptr, nSize);
        }
        else
        {
            m_data.clear();
        }
    }

    MStringW get_name(const ConstantsDB& db) const
    {
        // FIXME: Use ConstantsDB
        if (m_name.m_id == 0)
        {
            return m_name.m_str;
        }

        MStringW ret;
        switch (WORD id = m_name.m_id)
        {
        case (UINT_PTR)RT_BITMAP:
            ret += db.GetNameOfResID(IDTYPE_BITMAP, id);
            break;

        case (UINT_PTR)RT_MENU:
            ret += db.GetNameOfResID(IDTYPE_MENU, id);
            break;

        case (UINT_PTR)RT_DIALOG:
        case 240:   // RT_DLGINIT
            ret += db.GetNameOfResID(IDTYPE_DIALOG, id);
            break;

        case (UINT_PTR)RT_ACCELERATOR:
            ret += db.GetNameOfResID(IDTYPE_ACCEL, id);
            break;

        case (UINT_PTR)RT_GROUP_CURSOR:
            ret += db.GetNameOfResID(IDTYPE_CURSOR, id);
            break;

        case (UINT_PTR)RT_GROUP_ICON:
            ret += db.GetNameOfResID(IDTYPE_ICON, id);
            break;

        case (UINT_PTR)RT_ANICURSOR:
            ret += db.GetNameOfResID(IDTYPE_ANICURSOR, id);
            break;

        case (UINT_PTR)RT_ANIICON:
            ret += db.GetNameOfResID(IDTYPE_ANIICON, id);
            break;

        case (UINT_PTR)RT_HTML:
            ret += db.GetNameOfResID(IDTYPE_HTML, id);
            break;

        default:
            ret += db.GetNameOfResID(IDTYPE_RESOURCE, id);
            break;
        }

        if (ret.size())
        {
            if (!mchr_is_digit(ret[0]))
            {
                ret += L" (";
                ret += mstr_dec_word(m_name.m_id);
                ret += L")";
            }
        }
        else
        {
            ret += mstr_dec_word(m_name.m_id);
        }
        return ret;
    }
};

///////////////////////////////////////////////////////////////////////////////
// EntrySet

struct EntrySet : private std::set<EntryBase *>
{
    typedef std::set<EntryBase *> super_type;
    using super_type::empty;
    using super_type::size;
    using super_type::begin;
    using super_type::end;
    using super_type::insert;
    using super_type::erase;

    HWND m_hwndTV;
    typedef bool (*scan_proc_t)(EntryBase *base, void *pUserData);

    EntrySet(HWND hwndTV) : m_hwndTV(hwndTV);
    {
    }

    bool search(super_type& found, EntryType e_type, const MIdOrString& type,
                const MIdOrString& name, WORD lang = 0xFFFF)
    {
        found.clear();

        for (auto item : *this)
        {
            if (item->match(e_type, type, name, lang))
                found.insert(item);
        }
        return !found.empty();
    }

    bool intersect(const super_type& another) const
    {
        if (size() == 0 && another.size() == 0)
            return false;

        for (auto item1 : *this)
        {
            for (auto item2 : another)
            {
                if (*item1 == *item2)
                    return true;
            }
        }
        return false;
    }

    void search_and_delete(EntryType e_type, const MIdOrString& type,
                           const MIdOrString& name = "", WORD lang = 0xFFFF)
    {
        HWND hwndTV = Res_GetTreeViewHandle();
        super_type found;
        search(found, e_type, type, name, lang);
        for (auto item : found)
        {
            TreeView_DeleteItem(hwndTV, item->hItem);
        }
    }

    UINT last_icon_id() const
    {
        WORD last_id = 0;
        for (auto item : *this)
        {
            if (item->type != RT_ICON || !item->m_name.is_int() || item->m_name.is_zero())
                continue;
            if (last_id < item->name.m_id)
                last_id = item->name.m_id;
        }
        return last_id;
    }

    UINT last_cursor_id()
    {
        WORD last_id = 0;
        for (auto item : *this)
        {
            if (item->type != RT_CURSOR || !item->m_name.is_int() || item->m_name.is_zero())
                continue;

            if (last_id < item->name.m_id)
                last_id = item->name.m_id;
        }
        return last_id;
    }

    bool AddFromRes(HMODULE hMod, LPCWSTR type, LPCWSTR name, WORD lang)
    {
        HRSRC hResInfo = FindResourceExW(hMod, type, name, lang);
        if (!hResInfo)
            return false;

        DWORD dwSize = SizeofResource(hMod, hResInfo);
        HGLOBAL hGlobal = LoadResource(hMod, hResInfo);
        LPVOID pv = LockResource(hGlobal);
        if (pv && dwSize)
        {
            auto entry = new ResEntry(type, name, lang);
            entry->assign(pv, dwSize);
            push_back(entry);
            return true;
        }
        return false;
    }

    BOOL Add(const MIdOrString& type, const MIdOrString& name, WORD lang,
             const MStringW& strText, const Entry::DataType& data,
             BOOL Replace = FALSE)
    {
        ...
    }

    bool DeleteGroupIcon(ResEntry& entry)
    {
        assert(entry.M_type == RT_GROUP_ICON);

        MByteStreamEx bs(entry.m_data);

        ICONDIR dir;
        if (!bs.ReadRaw(dir))
            return false;

        DWORD size = sizeof(GRPICONDIRENTRY) * dir.idCount;
        std::vector<GRPICONDIRENTRY> DirEntries(dir.idCount);
        if (!bs.ReadData(&DirEntries[0], size))
            return false;

        DWORD i, nCount = dir.idCount;
        for (i = 0; i < nCount; ++i)
        {
            SearchAndDelete(RT_ICON, DirEntries[i].nID, entry.lang);
        }

        SearchAndDelete(entry.m_type, entry.m_name, entry.m_lang);
        return true;
    }

    bool DeleteGroupCursor(Entry& entry)
    {
        assert(entry.m_type == RT_GROUP_CURSOR);

        MByteStreamEx bs(entry.m_data);

        ICONDIR dir;
        if (!bs.ReadRaw(dir))
            return false;

        DWORD size = sizeof(GRPCURSORDIRENTRY) * dir.idCount;
        std::vector<GRPCURSORDIRENTRY> DirEntries(dir.idCount);
        if (!bs.ReadData(&DirEntries[0], size))
            return false;

        DWORD i, nCount = dir.idCount;
        for (i = 0; i < nCount; ++i)
        {
            SearchAndDelete(RT_CURSOR, DirEntries[i].nID, entry.lang);
        }

        SearchAndDelete(entry.m_type, entry.m_name, entry.m_lang);
        return true;
    }

    bool
    AddGroupIcon(const MIdOrString& name, WORD lang,
                 const MStringW& FileName, BOOL Replace = FALSE)
    {
        IconFile icon;
        if (!icon.LoadFromFile(FileName.c_str()) || icon.type() != RES_ICON)
            return false;

        UINT LastIconID = GetLastIconID(*this);
        UINT NextIconID = LastIconID + 1;
        IconFile::DataType group(icon.GetIconGroup(NextIconID));
        Add(RT_GROUP_ICON, name, lang, L"", group, Replace);

        int i, nCount = icon.GetImageCount();
        for (i = 0; i < nCount; ++i)
        {
            Add(RT_ICON, WORD(NextIconID + i), lang, L"", icon.GetImage(i));
        }

        return true;
    }

    bool AddGroupCursor(const MIdOrString& name, WORD lang,
                        const MStringW& FileName, BOOL Replace = FALSE)
    {
        CursorFile cur;
        if (!cur.LoadFromFile(FileName.c_str()) || cur.type() != RES_CURSOR)
            return false;

        UINT LastCursorID = GetLastCursorID(*this);
        UINT NextCursorID = LastCursorID + 1;
        CursorFile::DataType group(cur.GetCursorGroup(NextCursorID));
        Add(RT_GROUP_CURSOR, name, lang, L"", group, Replace);

        int i, nCount = cur.GetImageCount();
        for (i = 0; i < nCount; ++i)
        {
            Add(RT_CURSOR, WORD(NextCursorID + i), lang, L"", cur.GetImage(i));
        }
        return true;
    }

    bool AddBitmap(const MIdOrString& name, WORD lang,
                   const MStringW& BitmapFile, BOOL Replace = FALSE)
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

            AddEntry(RT_BITMAP, name, lang, L"", PackedDIB, Replace);
        }
        else
        {
            size_t FileHeadSize = sizeof(BITMAPFILEHEADER);
            if (stream.size() < FileHeadSize)
                return FALSE;

            size_t i0 = FileHeadSize, i1 = stream.size();
            Entry::DataType HeadLess(&stream[i0], &stream[i0] + (i1 - i0));
            Add(RT_BITMAP, name, lang, L"", HeadLess, Replace);
        }

        return TRUE;
    }

private:
    static BOOL CALLBACK
    EnumResLangProc(HMODULE hMod, LPCWSTR lpszType, LPCWSTR lpszName,
                    WORD wIDLanguage, LPARAM lParam)
    {
        EntrySet& entries = *(Entries *)lParam;
        AddFromRes(hMod, lpszType, lpszName, wIDLanguage);
        return TRUE;
    }

    static BOOL CALLBACK
    EnumResNameProc(HMODULE hMod, LPCWSTR lpszType, LPWSTR lpszName, LPARAM lParam)
    {
        return ::EnumResourceLanguagesW(hMod, lpszType, lpszName, EnumResLangProc, lParam);
    }

    static BOOL CALLBACK
    EnumResTypeProc(HMODULE hMod, LPWSTR lpszType, LPARAM lParam)
    {
        return ::EnumResourceNamesW(hMod, lpszType, EnumResNameProc, lParam);
    }

public:
    BOOL GetListFromRes(HMODULE hMod, LPARAM lParam)
    {
        return ::EnumResourceTypesW(hMod, EnumResTypeProc, lParam);
    }

    BOOL UpdateExe(HWND hwnd, LPCWSTR ExeFile) const
    {
        HANDLE hUpdate = ::BeginUpdateResourceW(ExeFile, TRUE);

        if (hUpdate == NULL)
        {
            assert(0);
            return FALSE;
        }

        for (auto item : *this)
        {
            void *pv = NULL;
            DWORD size = 0;
            if (!item->empty())
            {
                pv = const_cast<void *>(entry.ptr());
                size = entry.size();
            }
            if (!::UpdateResourceW(hUpdate,
                                   item->type.ptr(), item->name.ptr(), item->lang,
                                   pv, size))
            {
                assert(0);
                return FALSE;
            }
        }
        return ::EndUpdateResourceW(hUpdate, FALSE);
    }

    void DoBitmap(MTitleToBitmap& title_to_bitmap, DialogItem& item, WORD lang)
    {
        MIdOrString type = RT_BITMAP;
        auto entry = Find2(type, item.m_title, lang);
        if (!entry)
            return;

        HBITMAP hbm = PackedDIB_CreateBitmapFromMemory(&(*entry)[0], entry->size());
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

    void DoIcon(MTitleToIcon& title_to_icon, DialogItem& item, WORD lang)
    {
        MIdOrString type = RT_GROUP_ICON;
        auto entry = Find2(type, item.m_title, lang);
        if (!entry)
            return;

        if (entry->size() < sizeof(ICONDIR) + sizeof(GRPICONDIRENTRY))
            return;

        ICONDIR& dir = (ICONDIR&)(*entry)[0];
        GRPICONDIRENTRY *pGroupIcon = (GRPICONDIRENTRY *)&(*entry)[sizeof(ICONDIR)];

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
        entry = Find2(type, pGroupIcon[n].nID, lang);
        if (!entry)
            return;

        HICON hIcon = CreateIconFromResource((PBYTE)&(*entry)[0], (*entry).size(), TRUE, 0x00030000);
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

    void Optimize()
    {
        for (auto entry : *this)
        {
            if (entry->empty())
            {
                erase(entry);
                delete entry;
            }
        }
    }

    BOOL ExtractCursor(const EntryBase& cur_entry,
                       const wchar_t *OutputFileName)
    {
        ICONDIR dir;
        dir.idReserved = 0;
        dir.idType = RES_CURSOR;
        dir.idCount = 1;

        LOCALHEADER local;
        if (cur_entry.size() < sizeof(local))
        {
            assert(0);
            return FALSE;
        }
        memcpy(&local, &cur_entry[0], sizeof(local));

        BITMAP bm;
        LPBYTE pb = LPBYTE(&cur_entry[0]) + sizeof(local);
        DWORD cb = cur_entry.size() - sizeof(local);
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
        entry.dwBytesInRes = cur_entry.size() - sizeof(local);
        entry.dwImageOffset = sizeof(dir) + sizeof(ICONDIRENTRY);

        DWORD cbLocal = sizeof(LOCALHEADER);
        pb = LPBYTE(&cur_entry[0]) + cbLocal;
        cb = cur_entry.size() - cbLocal;

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

    BOOL
    ExtractGroupCursor(const Entry& grp_cur_entry, const wchar_t *OutputFileName)
    {
        ICONDIR dir;
        if (grp_cur_entry.type != RT_GROUP_CURSOR ||
            grp_cur_entry.size() < sizeof(dir))
        {
            assert(0);
            return FALSE;
        }

        memcpy(&dir, &grp_cur_entry[0], sizeof(dir));
        if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
        {
            assert(0);
            return FALSE;
        }

        DWORD SizeOfCursorEntries = sizeof(GRPCURSORDIRENTRY) * dir.idCount;
        if (grp_cur_entry.size() < sizeof(dir) + SizeOfCursorEntries)
        {
            assert(0);
            return FALSE;
        }

        std::vector<GRPCURSORDIRENTRY> GroupEntries(dir.idCount);
        memcpy(&GroupEntries[0], &grp_cur_entry[sizeof(dir)],
               SizeOfCursorEntries);

        DWORD offset = sizeof(dir) + sizeof(ICONDIRENTRY) * dir.idCount;
        std::vector<ICONDIRENTRY> DirEntries(dir.idCount);
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            auto entry = Find2(RT_CURSOR, GroupEntries[i].nID, grp_cur_entry.lang);
            if (!entry)
                continue;

            LOCALHEADER local;
            if (entry->size() >= sizeof(local))
                memcpy(&local, &(*entry)[0], sizeof(local));

            DirEntries[i].bWidth = (BYTE)GroupEntries[i].wWidth;
            DirEntries[i].bHeight = (BYTE)GroupEntries[i].wHeight;
            if (GroupEntries[i].wBitCount >= 8)
                DirEntries[i].bColorCount = 0;
            else
                DirEntries[i].bColorCount = 1 << GroupEntries[i].wBitCount;
            DirEntries[i].bReserved = 0;
            DirEntries[i].xHotSpot = local.xHotSpot;
            DirEntries[i].yHotSpot = local.yHotSpot;
            DirEntries[i].dwBytesInRes = entry->size() - sizeof(local);
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
            auto entry = Find2(RT_CURSOR, GroupEntries[i].nID, grp_cur_entry.lang, FALSE);
            if (!entry)
                continue;

            DWORD cbLocal = sizeof(LOCALHEADER), dwSize = (*entry).size() - cbLocal;
            LPBYTE pb = LPBYTE(&(*entry)[0]) + cbLocal;
            if (!stream.WriteData(pb, dwSize))
            {
                assert(0);
                return FALSE;
            }
        }

        return stream.SaveToFile(OutputFileName);
    }

    BOOL ExtractIcon(const Entry& i_entry, const wchar_t *OutputFileName)
    {
        ICONDIR dir;
        dir.idReserved = 0;
        dir.idType = RES_ICON;
        dir.idCount = 1;

        BITMAP bm;
        if (!PackedDIB_GetInfo(&i_entry[0], i_entry.size(), bm))
        {
            MBitmapDx bitmap;
            bitmap.CreateFromMemory(&i_entry[0], i_entry.size());

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
        entry.dwBytesInRes = i_entry.size();
        entry.dwImageOffset = sizeof(dir) + sizeof(ICONDIRENTRY);

        MByteStreamEx stream;
        if (!stream.WriteRaw(dir) ||
            !stream.WriteData(&entry, sizeof(entry)) ||
            !stream.WriteData(&i_entry[0], i_entry.size()))
        {
            assert(0);
            return FALSE;
        }

        return stream.SaveToFile(OutputFileName);
    }

    BOOL
    ExtractGroupIcon(const Entry& grp_ico_entry, const wchar_t *OutputFileName)
    {
        ICONDIR dir;
        if (grp_ico_entry.type != RT_GROUP_ICON ||
            grp_ico_entry.size() < sizeof(dir))
        {
            assert(0);
            return FALSE;
        }

        memcpy(&dir, &grp_ico_entry[0], sizeof(dir));
        if (dir.idReserved != 0 || dir.idType != RES_ICON || dir.idCount == 0)
        {
            assert(0);
            return FALSE;
        }

        DWORD SizeOfIconEntries = sizeof(GRPICONDIRENTRY) * dir.idCount;
        if (grp_ico_entry.size() < sizeof(dir) + SizeOfIconEntries)
        {
            assert(0);
            return FALSE;
        }

        std::vector<GRPICONDIRENTRY> GroupEntries(dir.idCount);
        memcpy(&GroupEntries[0], &grp_ico_entry[sizeof(dir)], SizeOfIconEntries);

        DWORD offset = sizeof(dir) + sizeof(ICONDIRENTRY) * dir.idCount;
        std::vector<ICONDIRENTRY> DirEntries(dir.idCount);
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            auto entry = Find2(RT_ICON, GroupEntries[i].nID, grp_ico_entry.lang);
            if (!entry)
                continue;

            DirEntries[i].bWidth = GroupEntries[i].bWidth;
            DirEntries[i].bHeight = GroupEntries[i].bHeight;
            if (GroupEntries[i].wBitCount >= 8)
                DirEntries[i].bColorCount = 0;
            else
                DirEntries[i].bColorCount = GroupEntries[i].bColorCount;
            DirEntries[i].bReserved = 0;
            DirEntries[i].wPlanes = 1;
            DirEntries[i].wBitCount = GroupEntries[i].wBitCount;
            DirEntries[i].dwBytesInRes = (*entry).size();
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
            auto entry = Find2(RT_ICON, GroupEntries[i].nID, grp_ico_entry.lang, FALSE);
            if (!entry)
                continue;

            DWORD dwSize = (*entry).size();
            if (!stream.WriteData(&(*entry)[0], dwSize))
            {
                assert(0);
                return FALSE;
            }
        }

        return stream.SaveToFile(OutputFileName);
    }
};

///////////////////////////////////////////////////////////////////////////////

inline LPARAM
TV_GetParam(HWND hwndTV, HTREEITEM hItem = NULL)
{
    if (!hItem)
        hItem = TreeView_GetSelection(hwndTV);

    TV_ITEM item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    TreeView_GetItem(hwndTV, &item);

    return item.lParam;
}

inline EntryBase *
TV_GetEntry(HWND hwndTV, HTREEITEM hItem = NULL)
{
    LPARAM lParam = TV_GetParam(hwndTV, hItem);
    return (EntryBase *)lParam;
}

inline HTREEITEM
TV_FindOrInsertDepth3(HWND hwnd, const ConstantsDB& db, HTREEITEM hParent, 
                      const Entries& entries, INT i, INT k)
{
    HTREEITEM hInsertAfter = TVI_LAST;
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
TV_FindOrInsertDepth2(HWND hwnd, const ConstantsDB& db, HTREEITEM hParent, 
                       const Entries& entries, INT i, INT k)
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
TV_FindOrInsertDepth1(HWND hwnd, const ConstantsDB& db, HTREEITEM hParent,
                       const Entries& entries, INT i, INT k)
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

inline HTREEITEM
TV_FindOrInsertString(HWND hwnd, HTREEITEM hParent,
                       const Entries& entries, INT iEntry)
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
TV_FindOrInsertMessage(HWND hwnd, HTREEITEM hParent,
                        const Entries& entries, INT iEntry)
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

inline void
TV_RefreshInfo(HWND hwnd, ConstantsDB& db, Entries& entries)
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
        hItem = TV_FindOrInsertDepth1(hwnd, db, hItem, entries, i, i);

        if (entries[i].type == RT_STRING)
        {
            TV_FindOrInsertString(hwnd, hItem, entries, i);
        }
        if (entries[i].type == RT_MESSAGETABLE)
        {
            TV_FindOrInsertMessage(hwnd, hItem, entries, i);
        }
    }

    // NOTE: Here, i is old index. k is new index.
    INT k = 0;
    for (INT i = 0; i < INT(entries.size()); ++i)
    {
        if (entries[i].empty())
            continue;

        HTREEITEM hItem = NULL;
        hItem = TV_FindOrInsertDepth1(hwnd, db, hItem, entries, i, k);
        hItem = TV_FindOrInsertDepth2(hwnd, db, hItem, entries, i, k);
        hItem = TV_FindOrInsertDepth3(hwnd, db, hItem, entries, i, k);
        hItem = hItem;
        ++k;
    }

    Res_EraseEmpty(entries);
}

///////////////////////////////////////////////////////////////////////////////

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

inline EntrySet&
Res_GetMaster(void)
{
    static EntrySet eset;
    return eset;
}

inline void
TV_OnDeleteGroupCursor(HWND hwndTV, EntryBase *entry)
{
    switch (entry->m_e_type)
    {
    case I_TYPE:
    case I_NAME:
    case I_LANG:
        // TODO:
        break;
    default:
        assert(0);
        break;
    }
}

inline void
TV_OnDeleteGroupIcon(HWND hwndTV, EntryBase *entry)
{
    switch (entry->m_e_type)
    {
    case I_TYPE:
    case I_NAME:
    case I_LANG:
        // TODO:
        break;
    default:
        assert(0);
        break;
    }
}

inline void
TV_OnDeleteString(HWND hwndTV, EntryBase *entry)
{
    assert(entry->m_e_type == I_STRING);
    // TODO:
}

inline void
TV_OnDeleteMessage(HWND hwndTV, EntryBase *entry)
{
    assert(entry->m_e_type == I_MESSAGE);
    // TODO:
}

inline void
TV_OnDeleteItem(HWND hwndTV, HTREEITEM hItem)
{
    auto entry = TV_GetEntry(hwndTV, hItem);
    entry->hItem = NULL;

    HTREEITEM hParent = TreeView_GetParent(hwndTV, hItem);

    switch (entry->m_e_type)
    {
    case I_NONE:
        assert(0);
        break;

    case I_TYPE:
        if (entry->m_type == RT_GROUP_CURSOR)
        {
            TV_OnDeleteGroupCursor(HWND hwndTV, entry);
        }
        if (entry->m_type == RT_GROUP_ICON)
        {
            TV_OnDeleteGroupIcon(HWND hwndTV, entry);
        }
        break;

    case I_NAME:
        if (entry->m_type == RT_GROUP_CURSOR)
        {
            TV_OnDeleteGroupCursor(HWND hwndTV, entry);
        }
        if (entry->m_type == RT_GROUP_ICON)
        {
            TV_OnDeleteGroupIcon(HWND hwndTV, entry);
        }
        break;

    case I_LANG:
        if (entry->m_type == RT_GROUP_CURSOR)
        {
            TV_OnDeleteGroupCursor(HWND hwndTV, entry);
        }
        if (entry->m_type == RT_GROUP_ICON)
        {
            TV_OnDeleteGroupIcon(HWND hwndTV, entry);
        }
        break;

    case I_STRING:
        TV_OnDeleteString(HWND hwndTV, entry);
        break;

    case I_MESSAGE:
        TV_OnDeleteMessage(HWND hwndTV, entry);
        break;

    default:
        break;
    }

    delete entry;

    if (TreeView_GetChild(hwndTV, hParent) == NULL)
        TreeView_DeleteItem(hwndTV, hParent);
}

inline HTREEITEM
TV_Insert(HWND hwndTV, HTREEITEM hParent, MStringW strText, EntryBase *entry,
          HTREEITEM hInsertAfter = TVI_LAST)
{
    assert(entry);

    TV_INSERTSTRUCTW insert;
    ZeroMemory(&insert, sizeof(insert));

    insert.hParent = hParent;
    insert.hInsertAfter = hInsertAfter;
    insert.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM |
                       TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    insert.item.state = 0;
    insert.item.stateMask = 0;
    insert.item.pszText = &strText[0];
    insert.item.lParam = (LPARAM)entry;
    if (entry->m_e_type < I_LANG)
    {
        insert.item.iImage = 1;
        insert.item.iSelectedImage = 1;
    }
    else
    {
        insert.item.iImage = 0;
        insert.item.iSelectedImage = 0;
    }
    return TreeView_InsertItem(hwndTV, &insert);
}

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef RES_HPP_
