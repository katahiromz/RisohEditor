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

#include "IconRes.hpp"
#include "MByteStreamEx.hpp"
#include "MString.hpp"
#include "PackedDIB.hpp"
#include "MBitmapDx.hpp"
#include "ConstantsDB.hpp"
#include "DialogRes.hpp"

struct BaseEntry;
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

#ifdef USE_GLOBALS
    extern HWND g_tv;
#else
    inline HWND&
    TV_GetMaster(void)
    {
        static HWND hwndTV = NULL;
        return hwndTV;
    }
    #define g_tv TV_GetMaster()
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

///////////////////////////////////////////////////////////////////////////////
// EntryType

enum EntryType
{
    ET_ANY,         // Any.
    ET_TYPE,        // TypeEntry.
    ET_NAME,        // NameEntry.
    ET_LANG,        // LangEntry.
    ET_STRING,      // StringEntry.
    ET_MESSAGE      // MessageEntry.
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
    HTREEITEM       m_hItem = NULL;

    EntryBase()
    {
    }

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
        if (e_type != ET_ANY && m_e_type != e_type)
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
        return m_e_type == entry.m_e_type &&
               m_lang == entry.m_lang &&
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

    MStringW get_type_label() const
    {
        if (!m_type.m_id)
            return m_type.m_str;

        MStringW label = g_db.GetName(L"RESOURCE", m_type.m_id);
        if (label.empty())
            return mstr_dec_word(m_type.m_id);

        label += L" (";
        label += mstr_dec_word(m_type.m_id);
        label += L")";
        return label;
    }

    MStringW get_name_label() const
    {
        WORD id = m_name.m_id;
        if (!id)
            return m_name.m_str;

        INT nIDTYPE_ = g_db.IDTypeFromResType(m_type);
        MStringW label = g_db.GetNameOfResID(nIDTYPE_, id);
        if (label.empty())
            return mstr_dec_word(id);

        label += L" (";
        label += mstr_dec_word(id);
        label += L")";
        return label;
    }

    MStringW get_lang_label() const
    {
        MStringW Res_GetLangName(WORD lang);
        return Res_GetLangName(m_lang);
    }
};

struct TypeEntry : EntryBase
{
    TypeEntry(const MIdOrString& type) : EntryBase(ET_TYPE, type)
    {
    }
};

struct NameEntry : EntryBase
{
    NameEntry(const MIdOrString& type, const MIdOrString& name)
        : EntryBase(ET_NAME, type, name)
    {
    }
};

struct StringEntry : EntryBase
{
    StringEntry(WORD lang) : EntryBase(ET_STRING, RT_STRING, L"", lang)
    {
    }
};

struct MessageEntry : EntryBase
{
    MessageEntry(WORD lang) : EntryBase(ET_MESSAGE, RT_MESSAGETABLE, L"", lang)
    {
    }
};

struct LangEntry : EntryBase
{
    data_type m_data;
    MStringW  m_strText;

    LangEntry()
    {
    }

    LangEntry(const MIdOrString& type, const MIdOrString& name, WORD lang = 0xFFFF)
        : EntryBase(ET_NAME, type, name, lang)
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

    void assign(const data_type& data)
    {
        m_data = data;
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
};

///////////////////////////////////////////////////////////////////////////////
// EntrySet

inline HTREEITEM
TV_InsertEntry(EntryBase *entry);

HTREEITEM TV_AddLangEntry(const MIdOrString& type, const MIdOrString& name, 
                          WORD lang, const EntryBase::data_type& data, bool replace);
HTREEITEM TV_AddStringEntry(WORD lang);
HTREEITEM TV_AddMessageEntry(WORD lang);

struct EntrySet : private std::set<EntryBase *>
{
    typedef std::set<EntryBase *> super_type;
    using super_type::empty;
    using super_type::size;
    using super_type::clear;
    using super_type::begin;
    using super_type::end;
    using super_type::insert;
    using super_type::erase;
    using super_type::swap;

    HWND m_hwndTV;

    EntrySet(HWND hwndTV = NULL) : m_hwndTV(hwndTV)
    {
    }

    EntrySet(HWND hwndTV, const super_type& super)
        : super_type(super), m_hwndTV(hwndTV)
    {
    }

    bool search(super_type& found, EntryType e_type, const MIdOrString& type, 
                const MIdOrString& name, WORD lang = 0xFFFF)
    {
        found.clear();

        for (auto entry : *this)
        {
            if (entry->match(e_type, type, name, lang))
                found.insert(entry);
        }
        return !found.empty();
    }

    EntryBase *find(EntryType e_type, const MIdOrString& type, 
              const MIdOrString& name = L"", WORD lang = 0xFFFF)
    {
        super_type found;
        if (search(found, e_type, type, name, lang))
        {
            return *found.begin();
        }
        return NULL;
    }

    HTREEITEM find2(EntryType e_type, const MIdOrString& type, 
              const MIdOrString& name = L"", WORD lang = 0xFFFF)
    {
        if (auto entry = find(e_type, type, name, lang))
            return entry->m_hItem;
        return NULL;
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

    void add_entries(super_type& super, bool replace)
    {
        for (auto entry : super)
        {
            if (replace)
            {
                search_and_delete(ET_LANG, entry->m_type, entry->m_name, entry->m_lang);
            }
            else
            {
                if (auto hItem = find2(ET_LANG, entry->m_type, entry->m_name, entry->m_lang))
                    continue;
            }

            if (entry->m_type == RT_STRING)
            {
                TV_AddStringEntry(entry->m_lang);
            }
            if (entry->m_type == RT_MESSAGETABLE)
            {
                TV_AddMessageEntry(entry->m_lang);
            }

            TV_InsertEntry(entry);
        }
    }

    HTREEITEM
    add_entry(const MIdOrString& type, const MIdOrString& name, 
              WORD lang, const EntryBase::data_type& data, bool replace)
    {
        return TV_AddLangEntry(type, name, lang, data, replace);
    }

    void delete_entry(EntryBase *entry)
    {
        void Res_DeleteEntry(EntryBase *entry);
        Res_DeleteEntry(entry);
    }

    void search_and_delete(EntryType e_type, const MIdOrString& type, 
                           const MIdOrString& name = L"", WORD lang = 0xFFFF)
    {
        super_type found;
        search(found, e_type, type, name, lang);
        for (auto entry : found)
        {
            delete_entry(entry);
        }
    }

    UINT get_last_id(const MIdOrString& type, WORD lang) const
    {
        WORD wLastID = 0;
        for (auto entry : *this)
        {
            if (entry->m_type != type || !entry->m_name.is_int() || entry->m_name.is_zero())
                continue;
            if (entry->m_lang != lang)
                continue;
            if (wLastID < entry->m_name.m_id)
                wLastID = entry->m_name.m_id;
        }
        return wLastID;
    }

    BOOL update_exe(LPCWSTR ExeFile) const
    {
        HANDLE hUpdate = ::BeginUpdateResourceW(ExeFile, TRUE);

        if (hUpdate == NULL)
        {
            assert(0);
            return FALSE;
        }

        for (auto entry : *this)
        {
            if (entry->m_e_type != ET_LANG)
                continue;

            LangEntry& le = (LangEntry&)(*entry);

            void *pv = NULL;
            DWORD size = 0;
            if (!le.empty())
            {
                pv = const_cast<void *>(le.ptr());
                size = le.size();
            }
            if (!::UpdateResourceW(hUpdate, le.m_type.ptr(), le.m_name.ptr(), le.m_lang, pv, size))
            {
                assert(0);
                ::EndUpdateResourceW(hUpdate, TRUE);
                return FALSE;
            }
        }
        return ::EndUpdateResourceW(hUpdate, FALSE);
    }

    void do_bitmap(MTitleToBitmap& title_to_bitmap, DialogItem& item, WORD lang)
    {
        MIdOrString type = RT_BITMAP;
        auto entry = find(ET_LANG, type, item.m_title, lang);
        if (!entry)
            return;

        LangEntry& le = (LangEntry&)(*entry);
        HBITMAP hbm = PackedDIB_CreateBitmapFromMemory(&le[0], le.size());
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

    void do_icon(MTitleToIcon& title_to_icon, DialogItem& item, WORD lang)
    {
        MIdOrString type = RT_GROUP_ICON;
        auto entry = find(ET_LANG, type, item.m_title, lang);
        if (!entry)
            return;

        LangEntry& le = (LangEntry&)(*entry);
        if (le.size() < sizeof(ICONDIR) + sizeof(GRPICONDIRENTRY))
            return;

        ICONDIR& dir = (ICONDIR&)le[0];
        GRPICONDIRENTRY *pGroupIcon = (GRPICONDIRENTRY *)&le[sizeof(ICONDIR)];

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
        entry = find(ET_LANG, type, pGroupIcon[n].nID, lang);
        if (!entry)
            return;

        HICON hIcon = CreateIconFromResource((PBYTE)&le[0], le.size(), TRUE, 0x00030000);
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

    bool extract_cursor(const LangEntry& c_entry, const wchar_t *file_name)
    {
        ICONDIR dir = { 0, RES_CURSOR, 1 };

        LOCALHEADER local;
        if (c_entry.size() < sizeof(local))
        {
            assert(0);
            return false;
        }
        memcpy(&local, &c_entry[0], sizeof(local));

        BITMAP bm;
        LPBYTE pb = LPBYTE(&c_entry[0]) + sizeof(local);
        DWORD cb = c_entry.size() - sizeof(local);
        if (!PackedDIB_GetInfo(pb, cb, bm))
        {
            assert(0);
            return false;
        }

        ICONDIRENTRY entry;
        entry.bWidth = (BYTE)bm.bmWidth;
        entry.bHeight = (BYTE)(bm.bmHeight / 2);
        entry.bColorCount = 0;
        entry.bReserved = 0;
        entry.xHotSpot = local.xHotSpot;
        entry.yHotSpot = local.yHotSpot;
        entry.dwBytesInRes = c_entry.size() - sizeof(local);
        entry.dwImageOffset = sizeof(dir) + sizeof(ICONDIRENTRY);

        DWORD cbLocal = sizeof(LOCALHEADER);
        pb = LPBYTE(&c_entry[0]) + cbLocal;
        cb = c_entry.size() - cbLocal;

        MByteStreamEx stream;
        if (!stream.WriteRaw(dir) ||
            !stream.WriteData(&entry, sizeof(entry)) ||
            !stream.WriteData(pb, cb))
        {
            assert(0);
            return false;
        }

        return stream.SaveToFile(file_name);
    }

    bool extract_group_cursor(const LangEntry& group, const wchar_t *file_name)
    {
        ICONDIR dir;
        if (group.m_type != RT_GROUP_CURSOR ||
            group.size() < sizeof(dir))
        {
            assert(0);
            return false;
        }

        memcpy(&dir, &group[0], sizeof(dir));
        if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
        {
            assert(0);
            return false;
        }

        DWORD SizeOfCursorEntries = sizeof(GRPCURSORDIRENTRY) * dir.idCount;
        if (group.size() < sizeof(dir) + SizeOfCursorEntries)
        {
            assert(0);
            return false;
        }

        std::vector<GRPCURSORDIRENTRY> GroupEntries(dir.idCount);
        memcpy(&GroupEntries[0], &group[sizeof(dir)], 
               SizeOfCursorEntries);

        DWORD offset = sizeof(dir) + sizeof(ICONDIRENTRY) * dir.idCount;
        std::vector<ICONDIRENTRY> DirEntries(dir.idCount);
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            auto entry = find(ET_LANG, RT_CURSOR, GroupEntries[i].nID, group.m_lang);
            if (!entry)
                continue;

            auto& le = (LangEntry&)*entry;

            LOCALHEADER local;
            if (le.size() >= sizeof(local))
                memcpy(&local, &le[0], sizeof(local));

            DirEntries[i].bWidth = (BYTE)GroupEntries[i].wWidth;
            DirEntries[i].bHeight = (BYTE)GroupEntries[i].wHeight;
            if (GroupEntries[i].wBitCount >= 8)
                DirEntries[i].bColorCount = 0;
            else
                DirEntries[i].bColorCount = 1 << GroupEntries[i].wBitCount;
            DirEntries[i].bReserved = 0;
            DirEntries[i].xHotSpot = local.xHotSpot;
            DirEntries[i].yHotSpot = local.yHotSpot;
            DirEntries[i].dwBytesInRes = le.size() - sizeof(local);
            DirEntries[i].dwImageOffset = offset;
            offset += DirEntries[i].dwBytesInRes;
        }

        MByteStreamEx stream;
        if (!stream.WriteRaw(dir))
        {
            assert(0);
            return false;
        }

        DWORD SizeOfDirEntries = sizeof(ICONDIRENTRY) * dir.idCount;
        if (!stream.WriteData(&DirEntries[0], SizeOfDirEntries))
        {
            assert(0);
            return false;
        }

        for (WORD i = 0; i < dir.idCount; ++i)
        {
            auto entry = find(ET_LANG, RT_CURSOR, GroupEntries[i].nID, group.m_lang);
            if (!entry)
                continue;

            auto& le = (LangEntry&)*entry;

            DWORD cbLocal = sizeof(LOCALHEADER), dwSize = le.size() - cbLocal;
            LPBYTE pb = LPBYTE(&le[0]) + cbLocal;
            if (!stream.WriteData(pb, dwSize))
            {
                assert(0);
                return FALSE;
            }
        }

        return stream.SaveToFile(file_name);
    }

    BOOL extract_icon(const LangEntry& i_entry, const wchar_t *file_name)
    {
        ICONDIR dir = { 0, RES_ICON, 1 };

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
            return false;
        }

        return stream.SaveToFile(file_name);
    }

    bool extract_group_icon(const LangEntry& group, const wchar_t *file_name)
    {
        ICONDIR dir;
        if (group.m_type != RT_GROUP_ICON ||
            group.size() < sizeof(dir))
        {
            assert(0);
            return false;
        }

        memcpy(&dir, &group[0], sizeof(dir));
        if (dir.idReserved != 0 || dir.idType != RES_ICON || dir.idCount == 0)
        {
            assert(0);
            return false;
        }

        DWORD SizeOfIconEntries = sizeof(GRPICONDIRENTRY) * dir.idCount;
        if (group.size() < sizeof(dir) + SizeOfIconEntries)
        {
            assert(0);
            return false;
        }

        std::vector<GRPICONDIRENTRY> GroupEntries(dir.idCount);
        memcpy(&GroupEntries[0], &group[sizeof(dir)], SizeOfIconEntries);

        DWORD offset = sizeof(dir) + sizeof(ICONDIRENTRY) * dir.idCount;
        std::vector<ICONDIRENTRY> DirEntries(dir.idCount);
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            auto entry = find(ET_LANG, RT_ICON, GroupEntries[i].nID, group.m_lang);
            if (!entry)
                continue;

            auto& le = (LangEntry&)*entry;

            DirEntries[i].bWidth = GroupEntries[i].bWidth;
            DirEntries[i].bHeight = GroupEntries[i].bHeight;
            if (GroupEntries[i].wBitCount >= 8)
                DirEntries[i].bColorCount = 0;
            else
                DirEntries[i].bColorCount = GroupEntries[i].bColorCount;
            DirEntries[i].bReserved = 0;
            DirEntries[i].wPlanes = 1;
            DirEntries[i].wBitCount = GroupEntries[i].wBitCount;
            DirEntries[i].dwBytesInRes = le.size();
            DirEntries[i].dwImageOffset = offset;
            offset += DirEntries[i].dwBytesInRes;
        }

        MByteStreamEx stream;
        if (!stream.WriteRaw(dir))
        {
            assert(0);
            return false;
        }

        DWORD SizeOfDirEntries = sizeof(ICONDIRENTRY) * dir.idCount;
        if (!stream.WriteData(&DirEntries[0], SizeOfDirEntries))
        {
            assert(0);
            return false;
        }

        for (WORD i = 0; i < dir.idCount; ++i)
        {
            auto entry = find(ET_LANG, RT_ICON, GroupEntries[i].nID, group.m_lang);
            if (!entry)
                continue;

            auto& le = (LangEntry&)*entry;

            DWORD dwSize = le.size();
            if (!stream.WriteData(&le[0], dwSize))
            {
                assert(0);
                return FALSE;
            }
        }

        return stream.SaveToFile(file_name);
    }
};

#ifdef USE_GLOBALS
    extern EntrySet g_res;
#else
    inline EntrySet&
    Res_GetMaster(void)
    {
        static EntrySet eset;
        return eset;
    }
    #define g_res   Res_GetMaster()
#endif

///////////////////////////////////////////////////////////////////////////////

inline LPARAM
TV_GetParam(HTREEITEM hItem = NULL)
{
    if (!hItem)
        hItem = TreeView_GetSelection(g_tv);

    TV_ITEM item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    TreeView_GetItem(g_tv, &item);

    return item.lParam;
}

inline EntryBase *
TV_GetEntry(HTREEITEM hItem = NULL)
{
    LPARAM lParam = TV_GetParam(hItem);
    return (EntryBase *)lParam;
}

inline LangEntry *
TV_GetLangEntry(HTREEITEM hItem = NULL)
{
    auto e = TV_GetEntry(hItem);
    if (e->m_e_type == ET_LANG)
        return (LangEntry *)e;
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////

inline bool
Res_OnDeleteGroupCursor(EntryBase *entry)
{
    if (entry->m_e_type != ET_LANG || entry->m_type != RT_GROUP_CURSOR)
        return false;

    auto& le = (LangEntry&)*entry;

    MByteStreamEx bs(le.m_data);

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
        g_res.search_and_delete(ET_LANG, RT_CURSOR, DirEntries[i].nID, le.m_lang);
    }
    return true;
}

inline bool
Res_OnDeleteGroupIcon(EntryBase *entry)
{
    if (entry->m_e_type != ET_LANG || entry->m_type != RT_GROUP_ICON)
        return false;

    auto& le = (LangEntry&)*entry;

    MByteStreamEx bs(le.m_data);

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
        g_res.search_and_delete(ET_LANG, RT_ICON, DirEntries[i].nID, le.m_lang);
    }

    return true;
}

inline void
Res_OnDeleteString(EntryBase *entry)
{
    assert(entry->m_e_type == ET_STRING);
    g_res.search_and_delete(ET_LANG, RT_STRING, (WORD)0, entry->m_lang);
}

inline void
Res_OnDeleteMessage(EntryBase *entry)
{
    assert(entry->m_e_type == ET_MESSAGE);
    g_res.search_and_delete(ET_LANG, RT_MESSAGETABLE, (WORD)0, entry->m_lang);
}

inline INT
TV_GetChildrenAboutCount(HTREEITEM hItem)
{
    if (HTREEITEM hItem = TreeView_GetChild(g_tv, hItem))
    {
        if (TreeView_GetNextSibling(g_tv, hItem))
            return 2;
        return 1;
    }
    return 0;
}

inline EntryBase *
Res_GetParent(EntryBase *entry)
{
    switch (entry->m_e_type)
    {
    case ET_NAME:
    case ET_STRING:
    case ET_MESSAGE:
        return g_res.find(ET_TYPE, entry->m_type);

    case ET_LANG:
        return g_res.find(ET_NAME, entry->m_type, entry->m_name);

    default:
        return NULL;
    }
}

inline void
Res_DeleteEntry(EntryBase *entry)
{
    HTREEITEM hItem = entry->m_hItem;
    entry->m_hItem = NULL;

    HTREEITEM hParent = NULL;
    EntryBase *pParent;
    if (g_tv)
    {
        hParent = TreeView_GetParent(g_tv, hItem);
        pParent = TV_GetEntry(hParent);
    }
    else
    {
        pParent = Res_GetParent(entry);
    }

    switch (entry->m_e_type)
    {
    case ET_TYPE:
    case ET_NAME:
        break;

    case ET_LANG:
        if (entry->m_type == RT_GROUP_CURSOR)
        {
            Res_OnDeleteGroupCursor(entry);
        }
        if (entry->m_type == RT_GROUP_ICON)
        {
            Res_OnDeleteGroupIcon(entry);
        }
        break;

    case ET_STRING:
        Res_OnDeleteString(entry);
        break;

    case ET_MESSAGE:
        Res_OnDeleteMessage(entry);
        break;

    default:
        assert(0);
        return;
    }

    g_res.erase(entry);
    delete entry;

    if (g_tv)
    {
        if (hParent)
        {
            if (entry->m_e_type == ET_STRING || entry->m_e_type == ET_MESSAGE)
            {
                INT nCount = TV_GetChildrenAboutCount(hParent);
                if (nCount == 1)
                    TreeView_DeleteItem(g_tv, hParent);
            }
            else
            {
                if (TreeView_GetChild(g_tv, hParent) == NULL)
                    TreeView_DeleteItem(g_tv, hParent);
            }
        }
    }
    else
    {
        if (pParent)
            Res_DeleteEntry(pParent);
    }
}

inline void
TV_OnDeleteItem(HTREEITEM hItem)
{
    auto entry = TV_GetEntry(hItem);
    Res_DeleteEntry(entry);
}

inline HTREEITEM
TV_InsertAfter(HTREEITEM hParent, MStringW strText, EntryBase *entry, 
               HTREEITEM hInsertAfter)
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
    if (entry->m_e_type < ET_LANG)
    {
        insert.item.iImage = 1;
        insert.item.iSelectedImage = 1;
    }
    else
    {
        insert.item.iImage = 0;
        insert.item.iSelectedImage = 0;
    }
    return TreeView_InsertItem(g_tv, &insert);
}

inline HTREEITEM
TV_GetInsertParent(EntryBase *entry)
{
    HTREEITEM TV_AddTypeEntry(const MIdOrString& type, bool replace);
    HTREEITEM TV_AddNameEntry(const MIdOrString& type, const MIdOrString& name, bool replace);

    if (entry->m_e_type == ET_TYPE)
        return NULL;

    HTREEITEM hType = TV_AddTypeEntry(entry->m_type, false);
    if (!hType)
        return NULL;

    switch (entry->m_e_type)
    {
    case ET_NAME: case ET_STRING: case ET_MESSAGE:
        return hType;

    case ET_LANG:
        break;

    default:
        return NULL;
    }

    return TV_AddNameEntry(entry->m_type, entry->m_name, false);
}

inline HTREEITEM
TV_GetInsertPosition(HTREEITEM hParent, EntryBase *entry)
{
    if (entry->m_e_type == ET_TYPE)
        return NULL;

    // TODO:
    // ...
    return NULL;
}

inline HTREEITEM
TV_InsertEntry(EntryBase *entry)
{
    HTREEITEM hParent = TV_GetInsertParent(entry);
    if (entry->m_e_type != ET_TYPE && hParent == NULL)
        return NULL;

    HTREEITEM hPosition = TV_GetInsertPosition(hParent, entry);

    MStringW strText;
    switch (entry->m_e_type)
    {
    case ET_TYPE:
        strText = entry->get_type_label();
        break;
    case ET_NAME:
        strText = entry->get_name_label();
        break;
    case ET_LANG:
    case ET_STRING:
    case ET_MESSAGE:
        strText = entry->get_lang_label();
        break;
    default:
        assert(0);
        break;
    }
    return TV_InsertAfter(hParent, strText, entry, hPosition);
}

inline HTREEITEM
TV_AddTypeEntry(const MIdOrString& type, bool replace)
{
    if (replace)
    {
        g_res.search_and_delete(ET_TYPE, type);
    }
    else
    {
        if (auto hItem = g_res.find2(ET_TYPE, type))
            return hItem;
    }
    auto entry = new TypeEntry(type);
    return TV_InsertEntry(entry);
}

inline HTREEITEM
TV_AddNameEntry(const MIdOrString& type, const MIdOrString& name, 
                bool replace)
{
    if (replace)
    {
        g_res.search_and_delete(ET_NAME, type, name);
    }
    else
    {
        if (auto hItem = g_res.find2(ET_NAME, type, name))
            return hItem;
    }
    auto entry = new NameEntry(type, name);
    return TV_InsertEntry(entry);
}

inline HTREEITEM
TV_AddStringEntry(WORD lang)
{
    if (auto hItem = g_res.find2(ET_STRING, RT_STRING, (WORD)0, lang))
        return hItem;

    auto entry = new StringEntry(lang);
    return TV_InsertEntry(entry);
}

inline HTREEITEM
TV_AddMessageEntry(WORD lang)
{
    if (auto hItem = g_res.find2(ET_MESSAGE, RT_MESSAGETABLE, (WORD)0, lang))
        return hItem;

    auto entry = new MessageEntry(lang);
    return TV_InsertEntry(entry);
}

inline HTREEITEM
TV_AddLangEntry(const MIdOrString& type, const MIdOrString& name, 
                WORD lang, const EntryBase::data_type& data, bool replace)
{
    if (replace)
    {
        g_res.search_and_delete(ET_LANG, type, name, lang);
    }
    else
    {
        if (auto hItem = g_res.find2(ET_LANG, type, name, lang))
            return hItem;
    }

    if (type == RT_STRING)
    {
        TV_AddStringEntry(lang);
    }
    if (type == RT_MESSAGETABLE)
    {
        TV_AddMessageEntry(lang);
    }

    auto entry = new LangEntry(type, name, lang);
    entry->assign(data);
    return TV_InsertEntry(entry);
}

inline HTREEITEM
TV_AddLangEntry(const MIdOrString& type, const MIdOrString& name, 
                WORD lang, const MStringW& strText, bool replace)
{
    if (replace)
    {
        g_res.search_and_delete(ET_LANG, type, name, lang);
    }
    else
    {
        if (auto hItem = g_res.find2(ET_LANG, type, name, lang))
            return hItem;
    }

    if (type == RT_STRING)
    {
        TV_AddStringEntry(lang);
    }
    if (type == RT_MESSAGETABLE)
    {
        TV_AddMessageEntry(lang);
    }

    auto entry = new LangEntry(type, name, lang);
    entry->m_strText = strText;
    return TV_InsertEntry(entry);
}

inline HTREEITEM
TV_AddResEntry(HMODULE hMod, LPCWSTR type, LPCWSTR name, WORD lang, bool replace)
{
    HRSRC hResInfo = FindResourceExW(hMod, type, name, lang);
    if (!hResInfo)
        return NULL;

    DWORD dwSize = SizeofResource(hMod, hResInfo);
    HGLOBAL hGlobal = LoadResource(hMod, hResInfo);
    LPVOID pv = LockResource(hGlobal);
    if (pv && dwSize)
    {
        EntryBase::data_type data((LPBYTE)(pv), (LPBYTE)(pv) + dwSize);
        return TV_AddLangEntry(type, name, lang, data, replace);
    }

    return NULL;
}

inline HTREEITEM
TV_AddGroupIcon(const MIdOrString& name, WORD lang, 
                const MStringW& file_name, bool replace)
{
    if (replace)
    {
        HTREEITEM hItem = g_res.find2(ET_LANG, RT_GROUP_ICON, name, lang);
        TreeView_DeleteItem(g_tv, hItem);
    }

    IconFile icon;
    if (!icon.LoadFromFile(file_name.c_str()) || icon.type() != RES_ICON)
        return NULL;

    UINT LastIconID = g_res.get_last_id(RT_ICON, lang);
    UINT NextIconID = LastIconID + 1;

    int i, nCount = icon.GetImageCount();
    for (i = 0; i < nCount; ++i)
    {
        TV_AddLangEntry(RT_ICON, WORD(NextIconID + i), lang, icon.GetImage(i), FALSE);
    }

    IconFile::DataType data(icon.GetIconGroup(NextIconID));
    return TV_AddLangEntry(RT_GROUP_ICON, name, lang, data, FALSE);
}

inline HTREEITEM
TV_AddGroupCursor(const MIdOrString& name, WORD lang, 
                  const MStringW& file_name, bool replace)
{
    if (replace)
    {
        auto hItem = g_res.find2(ET_LANG, RT_GROUP_CURSOR, name, lang);
        TreeView_DeleteItem(g_tv, hItem);
    }

    CursorFile cur;
    if (!cur.LoadFromFile(file_name.c_str()) || cur.type() != RES_CURSOR)
        return NULL;

    UINT LastCursorID = g_res.get_last_id(RT_CURSOR, lang);
    UINT NextCursorID = LastCursorID + 1;

    int i, nCount = cur.GetImageCount();
    for (i = 0; i < nCount; ++i)
    {
        TV_AddLangEntry(RT_CURSOR, WORD(NextCursorID + i), lang, cur.GetImage(i), FALSE);
    }

    CursorFile::DataType data(cur.GetCursorGroup(NextCursorID));
    return TV_AddLangEntry(RT_GROUP_CURSOR, name, lang, data, FALSE);
}

inline HTREEITEM
TV_AddBitmap(const MIdOrString& name, WORD lang, 
             const MStringW& file, bool replace = false)
{
    MByteStreamEx stream;
    if (!stream.LoadFromFile(file.c_str()) || stream.size() <= 4)
        return NULL;

    if (stream.size() >= 4 &&
        (memcmp(&stream[0], "\xFF\xD8\xFF", 3)== 0 ||    // JPEG
         memcmp(&stream[0], "GIF", 3) == 0 ||            // GIF
         memcmp(&stream[0], "\x89\x50\x4E\x47", 4) == 0)) // PNG
    {
        MBitmapDx bitmap;
        if (!bitmap.CreateFromMemory(&stream[0], (DWORD)stream.size()))
            return NULL;

        LONG cx, cy;
        HBITMAP hbm = bitmap.GetHBITMAP32(cx, cy);

        std::vector<BYTE> PackedDIB;
        if (!PackedDIB_CreateFromHandle(PackedDIB, hbm))
        {
            DeleteObject(hbm);
            return NULL;
        }
        DeleteObject(hbm);

        return TV_AddLangEntry(RT_BITMAP, name, lang, PackedDIB, replace);
    }

    size_t head_size = sizeof(BITMAPFILEHEADER);
    if (stream.size() < head_size)
        return NULL;

    size_t i0 = head_size, i1 = stream.size();
    EntryBase::data_type HeadLess(&stream[i0], &stream[i0] + (i1 - i0));
    return TV_AddLangEntry(RT_BITMAP, name, lang, HeadLess, replace);
}

inline BOOL CALLBACK
TV_EnumResLangProc(HMODULE hMod, LPCWSTR lpszType, LPCWSTR lpszName, 
                   WORD wIDLanguage, LPARAM lParam)
{
    bool replace = (bool)lParam;
    TV_AddResEntry(hMod, lpszType, lpszName, wIDLanguage, replace);
    return TRUE;
}

inline BOOL CALLBACK
TV_EnumResNameProc(HMODULE hMod, LPCWSTR lpszType, LPWSTR lpszName, LPARAM lParam)
{
    return ::EnumResourceLanguagesW(hMod, lpszType, lpszName, TV_EnumResLangProc, lParam);
}

inline BOOL CALLBACK
TV_EnumResTypeProc(HMODULE hMod, LPWSTR lpszType, LPARAM lParam)
{
    return ::EnumResourceNamesW(hMod, lpszType, TV_EnumResNameProc, lParam);
}

inline BOOL
TV_FromRes(HMODULE hMod, bool replace)
{
    return ::EnumResourceTypesW(hMod, TV_EnumResTypeProc, replace);
}

//////////////////////////////////////////////////////////////////////////////

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
