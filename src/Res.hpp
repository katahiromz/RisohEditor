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
    extern BOOL g_deleting_all;
#else
    inline BOOL&
    TV_GetDeletingAll(void)
    {
        static BOOL s_deleting_all = FALSE;
        return s_deleting_all;
    }
    #define g_deleting_all TV_GetDeletingAll()
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
    ET_LANG,        // EntryBase.
    ET_STRING,      // StringEntry.
    ET_MESSAGE      // MessageEntry.
};

///////////////////////////////////////////////////////////////////////////////
// EntryBase, TypeEntry, NameEnry, EntryBase, StringEntry, MessageEntry

struct EntryBase
{
    typedef DWORD               size_type;
    typedef std::vector<BYTE>   data_type;
    EntryType       m_et;

    MIdOrString     m_type;
    MIdOrString     m_name;
    WORD            m_lang = 0xFFFF;
    HTREEITEM       m_hItem = NULL;
    data_type       m_data;
    MStringW        m_strText;

    EntryBase()
    {
    }

    EntryBase(EntryType et, const MIdOrString& type, 
            const MIdOrString& name = WORD(0), WORD lang = 0xFFFF)
        : m_et(et), m_type(type), m_name(name), m_lang(lang)
    {
    }
    virtual ~EntryBase()
    {
    }

    bool can_gui_edit() const
    {
        return m_type == RT_DIALOG || m_type == RT_MENU ||
               m_type == RT_STRING || m_type == RT_MESSAGETABLE ||
               m_type == RT_ACCELERATOR || m_type == WORD(240);
    }

    bool is_testable() const
    {
        return m_type == RT_DIALOG || m_type == RT_MENU;
    }

    bool match(EntryType et, const MIdOrString& type, const MIdOrString& name, WORD lang = 0xFFFF) const
    {
        if (et != ET_ANY && m_et != et)
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
        return m_et == entry.m_et &&
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

inline EntryBase *
Res_NewTypeEntry(const MIdOrString& type)
{
    return new EntryBase(ET_TYPE, type);
}

inline EntryBase *
Res_NewNameEntry(const MIdOrString& type, const MIdOrString& name)
{
    return new EntryBase(ET_NAME, type, name);
}

inline EntryBase *
Res_NewStringEntry(WORD lang)
{
    return new EntryBase(ET_STRING, RT_STRING, WORD(0), lang);
}

inline EntryBase *
Res_NewMessageEntry(WORD lang)
{
    return new EntryBase(ET_MESSAGE, RT_MESSAGETABLE, WORD(0), lang);
}

inline EntryBase *
Res_NewLangEntry(const MIdOrString& type, const MIdOrString& name, WORD lang = 0xFFFF)
{
    return new EntryBase(ET_LANG, type, name, lang);
}

///////////////////////////////////////////////////////////////////////////////
// EntrySet

typedef std::set<EntryBase *> EntrySetBase;

struct EntrySet : protected EntrySetBase
{
    typedef EntrySetBase super_type;
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

    EntrySet(const super_type& super, HWND hwndTV = NULL)
        : super_type(super), m_hwndTV(hwndTV)
    {
    }

    super_type *super()
    {
        return dynamic_cast<super_type *>(this);
    }
    const super_type *super() const
    {
        return dynamic_cast<const super_type *>(this);
    }

    bool search(super_type& found, EntryType et, const MIdOrString& type, 
                const MIdOrString& name = WORD(0), WORD lang = 0xFFFF) const
    {
        found.clear();

        for (auto entry : *this)
        {
            if (entry->match(et, type, name, lang))
                found.insert(entry);
        }
        return !found.empty();
    }

    EntryBase *find(EntryType et, const MIdOrString& type, 
                    const MIdOrString& name = WORD(0), WORD lang = 0xFFFF) const
    {
        super_type found;
        if (search(found, et, type, name, lang))
        {
            return *found.begin();
        }
        return NULL;
    }

    bool intersect(const EntrySet& another) const
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

    void merge(const EntrySet& another, bool replace)
    {
        for (auto entry : another)
        {
            if (entry->m_et != ET_LANG)
                continue;

            add_lang_entry(entry->m_type, entry->m_name, entry->m_lang, entry->m_data, replace);
        }
    }

    EntryBase *
    add_lang_entry(const MIdOrString& type, const MIdOrString& name, 
              WORD lang, const MString& strText, bool replace)
    {
        if (replace)
        {
            search_and_delete(ET_LANG, type, name, lang);
        }
        else
        {
            if (auto entry = find(ET_LANG, type, name, lang))
                return entry;
        }

        if (type == RT_STRING)
        {
            add_string_entry(lang);
        }
        if (type == RT_MESSAGETABLE)
        {
            add_message_entry(lang);
        }

        auto entry = Res_NewLangEntry(type, name, lang);
        entry->m_strText = strText;
        insert(entry);
        return on_insert_entry(entry);
    }

    EntryBase *
    add_lang_entry(const MIdOrString& type, const MIdOrString& name, 
                   WORD lang, const EntryBase::data_type& data, bool replace)
    {
        if (replace)
        {
            search_and_delete(ET_LANG, type, name, lang);
        }
        else
        {
            if (auto entry = find(ET_LANG, type, name, lang))
                return entry;
        }

        if (m_hwndTV)
        {
            if (type == RT_STRING)
            {
                add_string_entry(lang);
            }
            if (type == RT_MESSAGETABLE)
            {
                add_message_entry(lang);
            }
        }

        auto entry = Res_NewLangEntry(type, name, lang);
        entry->assign(data);
        insert(entry);
        return on_insert_entry(entry);
    }

    void delete_entry(EntryBase *entry)
    {
        HTREEITEM hItem = entry->m_hItem;
        entry->m_hItem = NULL;

        EntryBase *pParent = NULL;
		if (!g_deleting_all)
		{
			if (m_hwndTV)
			{
				HTREEITEM hParent = TreeView_GetParent(m_hwndTV, hItem);
				pParent = get_entry(hParent);
			}
			else
			{
				pParent = get_parent(entry);
			}

            switch (entry->m_et)
            {
            case ET_TYPE:
            case ET_NAME:
                break;

            case ET_LANG:
                if (entry->m_type == RT_GROUP_CURSOR)
                {
                    on_delete_group_cursor(entry);
                }
                if (entry->m_type == RT_GROUP_ICON)
                {
                    on_delete_group_icon(entry);
                }
                break;

            case ET_STRING:
                on_delete_string(entry);
                break;

            case ET_MESSAGE:
                on_delete_message(entry);
                break;

            default:
                assert(0);
                return;
            }
        }

        if (super()->find(entry) != super()->end())
        {
            erase(entry);
            delete entry;

            if (pParent)
            {
                if (get_first_child(pParent) == NULL)
                    delete_entry(pParent);
            }
        }
    }

    void search_and_delete(EntryType et, const MIdOrString& type, 
                           const MIdOrString& name = WORD(0), WORD lang = 0xFFFF)
    {
        super_type found;
        search(found, et, type, name, lang);
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

    void detach(EntrySet& es)
    {
        for (auto entry : *this)
        {
            entry->m_hItem = NULL;
        }
        super()->swap(*es.super());
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
            if (entry->m_et != ET_LANG)
                continue;

            void *pv = NULL;
            DWORD size = 0;
            if (!(*entry).empty())
            {
                pv = const_cast<void *>((*entry).ptr());
                size = (*entry).size();
            }
            if (!::UpdateResourceW(hUpdate, (*entry).m_type.ptr(), (*entry).m_name.ptr(), (*entry).m_lang, pv, size))
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

        HBITMAP hbm = PackedDIB_CreateBitmapFromMemory(&(*entry)[0], (*entry).size());
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

        if ((*entry).size() < sizeof(ICONDIR) + sizeof(GRPICONDIRENTRY))
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
        entry = find(ET_LANG, type, pGroupIcon[n].nID, lang);
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

    bool extract_cursor(const EntryBase& c_entry, const wchar_t *file_name)
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

    bool extract_group_cursor(const EntryBase& group, const wchar_t *file_name)
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
            DirEntries[i].dwBytesInRes = (*entry).size() - sizeof(local);
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

            DWORD cbLocal = sizeof(LOCALHEADER), dwSize = (*entry).size() - cbLocal;
            LPBYTE pb = LPBYTE(&(*entry)[0]) + cbLocal;
            if (!stream.WriteData(pb, dwSize))
            {
                assert(0);
                return FALSE;
            }
        }

        return stream.SaveToFile(file_name);
    }

    BOOL extract_icon(const EntryBase& i_entry, const wchar_t *file_name)
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

    bool extract_group_icon(const EntryBase& group, const wchar_t *file_name)
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

            DWORD dwSize = (*entry).size();
            if (!stream.WriteData(&(*entry)[0], dwSize))
            {
                assert(0);
                return FALSE;
            }
        }

        return stream.SaveToFile(file_name);
    }

    EntryBase *
    add_bitmap(const MIdOrString& name, WORD lang, 
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

            return add_lang_entry(RT_BITMAP, name, lang, PackedDIB, replace);
        }

        size_t head_size = sizeof(BITMAPFILEHEADER);
        if (stream.size() < head_size)
            return NULL;

        size_t i0 = head_size, i1 = stream.size();
        EntryBase::data_type HeadLess(&stream[i0], &stream[i0] + (i1 - i0));
        return add_lang_entry(RT_BITMAP, name, lang, HeadLess, replace);
    }

    EntryBase *
    add_group_icon(const MIdOrString& name, WORD lang, 
                   const MStringW& file_name, bool replace)
    {
        if (replace)
        {
            auto entry = find(ET_LANG, RT_GROUP_ICON, name, lang);
            TreeView_DeleteItem(m_hwndTV, entry->m_hItem);
        }

        IconFile icon;
        if (!icon.LoadFromFile(file_name.c_str()) || icon.type() != RES_ICON)
            return NULL;

        UINT LastIconID = get_last_id(RT_ICON, lang);
        UINT NextIconID = LastIconID + 1;

        int i, nCount = icon.GetImageCount();
        for (i = 0; i < nCount; ++i)
        {
            add_lang_entry(RT_ICON, WORD(NextIconID + i), lang, icon.GetImage(i), FALSE);
        }

        IconFile::DataType data(icon.GetIconGroup(NextIconID));
        return add_lang_entry(RT_GROUP_ICON, name, lang, data, FALSE);
    }

    EntryBase *
    add_group_cursor(const MIdOrString& name, WORD lang, 
                     const MStringW& file_name, bool replace)
    {
        if (replace)
        {
            auto entry = find(ET_LANG, RT_GROUP_CURSOR, name, lang);
            TreeView_DeleteItem(m_hwndTV, entry->m_hItem);
        }

        CursorFile cur;
        if (!cur.LoadFromFile(file_name.c_str()) || cur.type() != RES_CURSOR)
            return NULL;

        UINT LastCursorID = get_last_id(RT_CURSOR, lang);
        UINT NextCursorID = LastCursorID + 1;

        int i, nCount = cur.GetImageCount();
        for (i = 0; i < nCount; ++i)
        {
            add_lang_entry(RT_CURSOR, WORD(NextCursorID + i), lang, cur.GetImage(i), FALSE);
        }

        CursorFile::DataType data(cur.GetCursorGroup(NextCursorID));
        return add_lang_entry(RT_GROUP_CURSOR, name, lang, data, FALSE);
    }

    EntryBase *
    add_string_entry(WORD lang)
    {
        if (auto entry = find(ET_STRING, RT_STRING, (WORD)0, lang))
            return entry;

        auto entry = Res_NewStringEntry(lang);
        insert(entry);
        return on_insert_entry(entry);
    }

    EntryBase *
    add_message_entry(WORD lang)
    {
        if (auto entry = find(ET_MESSAGE, RT_MESSAGETABLE, (WORD)0, lang))
            return entry;

        auto entry = Res_NewMessageEntry(lang);
        insert(entry);
        return on_insert_entry(entry);
    }

    EntryBase *
    add_name_entry(const MIdOrString& type, const MIdOrString& name, bool replace)
    {
        if (replace)
        {
            search_and_delete(ET_NAME, type, name);
        }
        else
        {
            if (auto entry = find(ET_NAME, type, name))
                return entry;
        }
        auto entry = Res_NewNameEntry(type, name);
        insert(entry);
        return on_insert_entry(entry);
    }

    EntryBase *
    add_type_entry(const MIdOrString& type, bool replace)
    {
        if (replace)
        {
            search_and_delete(ET_TYPE, type);
        }
        else
        {
            if (auto entry = find(ET_TYPE, type))
                return entry;
        }
        auto entry = Res_NewTypeEntry(type);
        insert(entry);
        return on_insert_entry(entry);
    }

    HTREEITEM get_insert_parent(EntryBase *entry)
    {
        if (m_hwndTV == NULL)
            return NULL;

        if (entry->m_et == ET_TYPE)
            return NULL;

        auto new_entry = add_type_entry(entry->m_type, false);
        if (!new_entry)
            return NULL;

        switch (entry->m_et)
        {
        case ET_NAME: case ET_STRING: case ET_MESSAGE:
            return new_entry->m_hItem;

        case ET_LANG:
            break;

        default:
            return NULL;
        }

        new_entry = add_name_entry(entry->m_type, entry->m_name, false);
        if (!new_entry)
            return NULL;
        return new_entry->m_hItem;
    }

    HTREEITEM get_insert_position(HTREEITEM hParent, EntryBase *entry)
    {
        if (m_hwndTV == NULL)
            return NULL;

        super_type found;

        switch (entry->m_et)
        {
        case ET_TYPE:
            search(found, ET_TYPE, (WORD)0);
            break;

        case ET_NAME:
            search(found, ET_NAME, entry->m_type);
            break;

        case ET_STRING:
            search(found, ET_STRING, entry->m_type, WORD(0), entry->m_lang);
            break;

        case ET_MESSAGE:
            search(found, ET_MESSAGE, entry->m_type, WORD(0), entry->m_lang);
            break;

        case ET_LANG:
            search(found, ET_LANG, entry->m_type, entry->m_name);
            break;

        default:
            return NULL;
        }

        EntryBase *target = NULL, *pre = NULL;
        for (auto e : found)
        {
            if (*e < *entry)
            {
                if (!target)
                {
                    target = pre = e;
                }
                else if (*target < *e)
                {
                    pre = target;
                    target = e;
                }
            }
        }

        if (pre)
            return pre->m_hItem;

        return TVI_FIRST;
    }

    EntryBase *get_parent(EntryBase *entry)
    {
        switch (entry->m_et)
        {
        case ET_NAME:
        case ET_STRING:
        case ET_MESSAGE:
            return find(ET_TYPE, entry->m_type);

        case ET_LANG:
            return find(ET_NAME, entry->m_type, entry->m_name);

        default:
            return NULL;
        }
    }

protected:
    EntryBase *on_insert_entry(EntryBase *entry)
    {
        if (m_hwndTV == NULL)
            return NULL;

        HTREEITEM hParent = get_insert_parent(entry);
        if (entry->m_et != ET_TYPE && hParent == NULL)
            return NULL;

        HTREEITEM hPosition = get_insert_position(hParent, entry);

        MStringW strText;
        switch (entry->m_et)
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

        return on_insert_after(hParent, strText, entry, hPosition);
    }

    EntryBase *
    on_insert_after(HTREEITEM hParent, MStringW strText, EntryBase *entry, 
                    HTREEITEM hInsertAfter)
    {
        assert(entry);

        if (m_hwndTV == NULL)
            return entry;

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
        if (entry->m_et < ET_LANG)
        {
            insert.item.iImage = 1;
            insert.item.iSelectedImage = 1;
        }
        else
        {
            insert.item.iImage = 0;
            insert.item.iSelectedImage = 0;
        }
        if (TreeView_InsertItem(m_hwndTV, &insert))
            return entry;

        return NULL;
    }

    void on_delete_string(EntryBase *entry)
    {
        assert(entry->m_et == ET_STRING);
        search_and_delete(ET_LANG, RT_STRING, (WORD)0, entry->m_lang);
    }

    void on_delete_message(EntryBase *entry)
    {
        assert(entry->m_et == ET_MESSAGE);
        search_and_delete(ET_LANG, RT_MESSAGETABLE, (WORD)0, entry->m_lang);
    }

    bool on_delete_group_icon(EntryBase *entry)
    {
        if (entry->m_et != ET_LANG || entry->m_type != RT_GROUP_ICON)
            return false;

        MByteStreamEx bs(entry->m_data);

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
            search_and_delete(ET_LANG, RT_ICON, DirEntries[i].nID, (*entry).m_lang);
        }

        return true;
    }

    bool on_delete_group_cursor(EntryBase *entry)
    {
        if (entry->m_et != ET_LANG || entry->m_type != RT_GROUP_CURSOR)
            return false;

        MByteStreamEx bs(entry->m_data);

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
            search_and_delete(ET_LANG, RT_CURSOR, DirEntries[i].nID, (*entry).m_lang);
        }
        return true;
    }

    struct EnumResStruct
    {
        bool replace;
        EntrySet *this_;
    };

    EntryBase *
    add_res_entry(HMODULE hMod, LPCWSTR type, LPCWSTR name, WORD lang, bool replace)
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
            return add_lang_entry(type, name, lang, data, replace);
        }

        return NULL;
    }

    static BOOL CALLBACK
    EnumResLangProc(HMODULE hMod, LPCWSTR lpszType, LPCWSTR lpszName, 
                    WORD wIDLanguage, LPARAM lParam)
    {
        auto ers = (EnumResStruct *)lParam;
        ers->this_->add_res_entry(hMod, lpszType, lpszName, wIDLanguage, ers->replace);
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

    EntryBase *get_first_child(EntryBase *pParent) const
    {
        switch (pParent->m_et)
        {
        case ET_TYPE:
            return find(ET_NAME, pParent->m_type);

        case ET_NAME:
            return find(ET_LANG, pParent->m_type);

        default:
            return NULL;
        }
    }

public:
    BOOL FromRes(HMODULE hMod, bool replace)
    {
        EnumResStruct ers;
        ers.replace = replace;
        ers.this_ = this;
        return ::EnumResourceTypesW(hMod, EnumResTypeProc, (LPARAM)&ers);
    }

    void delete_all(void)
    {
        g_deleting_all = TRUE;
        if (m_hwndTV)
        {
            TreeView_DeleteAllItems(m_hwndTV);
        }
        else
        {
            search_and_delete(ET_ANY, (WORD)0, (WORD)0, 0xFFFF);
        }
        g_deleting_all = FALSE;
    }

    void on_delete_item(EntryBase *entry)
    {
        delete_entry(entry);
    }

    LPARAM get_param(HTREEITEM hItem = NULL) const
    {
        if (!hItem)
            hItem = TreeView_GetSelection(m_hwndTV);

        TV_ITEM item;
        ZeroMemory(&item, sizeof(item));
        item.mask = TVIF_PARAM;
        item.hItem = hItem;
        TreeView_GetItem(m_hwndTV, &item);

        return item.lParam;
    }

    EntryBase *get_entry(HTREEITEM hItem = NULL, EntryType et = ET_ANY) const
    {
        LPARAM lParam = get_param(hItem);
        auto e = (EntryBase *)lParam;
        if (et != ET_ANY && et != e->m_et)
            return NULL;
        return e;
    }

    EntryBase *get_lang_entry(HTREEITEM hItem = NULL) const
    {
        return get_entry(hItem, ET_LANG);
    }

    HTREEITEM get_item(void) const
    {
        return TreeView_GetSelection(m_hwndTV);
    }

    BOOL copy_group_icon(EntryBase *entry, const MIdOrString& new_name)
    {
        assert(entry->m_et == ET_LANG);
        assert(entry->m_type == RT_GROUP_ICON);

        ICONDIR dir;
        if (entry->size() < sizeof(dir))
        {
            assert(0);
            return FALSE;
        }

        memcpy(&dir, &entry[0], sizeof(dir));

        if (dir.idReserved != 0 || dir.idType != RES_ICON || dir.idCount == 0)
        {
            assert(0);
            return FALSE;
        }

        if (entry->size() < sizeof(dir) + dir.idCount * sizeof(GRPICONDIRENTRY))
        {
            assert(0);
            return FALSE;
        }

        auto pEntries = (const GRPICONDIRENTRY *)&(*entry)[sizeof(dir)];

        LONG cx = 0, cy = 0;
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            auto e = find(ET_LANG, RT_ICON, pEntries[i].nID, entry->m_lang);
            if (!e)
                return FALSE;

            UINT nLastID = get_last_id(RT_ICON, entry->m_lang);
            UINT nNextID = nLastID + 1;

            add_lang_entry(RT_ICON, WORD(nNextID), e->m_lang, e->m_data, false);
        }

        add_lang_entry(RT_GROUP_ICON, new_name, entry->m_lang, entry->m_data, false);
        return TRUE;
    }

    BOOL copy_group_cursor(EntryBase *entry, const MIdOrString& new_name)
    {
        assert(entry->m_et == ET_LANG);
        assert(entry->m_type == RT_GROUP_CURSOR);

        ICONDIR dir;
        if (entry->size() < sizeof(dir))
        {
            assert(0);
            return FALSE;
        }

        memcpy(&dir, &entry[0], sizeof(dir));

        if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
        {
            assert(0);
            return FALSE;
        }

        if (entry->size() < sizeof(dir) + dir.idCount * sizeof(GRPCURSORDIRENTRY))
        {
            assert(0);
            return FALSE;
        }

        auto pEntries = (const GRPCURSORDIRENTRY *)&(*entry)[sizeof(dir)];

        LONG cx = 0, cy = 0;
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            auto e = find(ET_LANG, RT_CURSOR, pEntries[i].nID, entry->m_lang);
            if (!e)
                return FALSE;

            UINT nLastID = get_last_id(RT_CURSOR, entry->m_lang);
            UINT nNextID = nLastID + 1;

            add_lang_entry(RT_CURSOR, WORD(nNextID), e->m_lang, e->m_data, false);
        }

        add_lang_entry(RT_GROUP_CURSOR, new_name, entry->m_lang, entry->m_data, false);
        return TRUE;
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
