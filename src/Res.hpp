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
#include "MProcessMaker.hpp"
#include "PackedDIB.hpp"
#include "MBitmapDx.hpp"
#include "ConstantsDB.hpp"
#include "DialogRes.hpp"
#include "ResHeader.hpp"

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
    ET_STRING,      // StringEntry.
    ET_MESSAGE,     // MessageEntry.
    ET_NAME,        // NameEntry.
    ET_LANG         // EntryBase.
};

///////////////////////////////////////////////////////////////////////////////
// EntryBase

struct EntryBase
{
    typedef DWORD               size_type;
    typedef std::vector<BYTE>   data_type;
    EntryType       m_et;

    MIdOrString     m_type;
    MIdOrString     m_name;
    WORD            m_lang = 0xFFFF;
    HTREEITEM       m_hItem = NULL;
    bool            m_valid = true;
    data_type       m_data;
    MStringW        m_strLabel;

    EntryBase() : m_lang(0xFFFF), m_hItem(NULL), m_valid(true)
    {
    }

    EntryBase(EntryType et, const MIdOrString& type, 
            const MIdOrString& name = WORD(0), WORD lang = 0xFFFF)
        : m_et(et), m_type(type), m_name(name), m_lang(lang), m_hItem(NULL), m_valid(true)
    {
    }

    virtual ~EntryBase()
    {
    }

    bool valid() const
    {
        if (m_et == ET_LANG)
            return !empty();
        if (!m_hItem)
            return false;
        return m_valid;
    }

    void mark_invalid()
    {
        m_valid = false;
        m_data.clear();
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
        if (m_et < entry.m_et)
            return true;
        if (m_et > entry.m_et)
            return false;
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

        if (!mchr_is_digit(label[0]))
        {
            label += L" (";
            label += mstr_dec_word(m_type.m_id);
            label += L")";
        }
        return label;
    }

    MStringW get_name_label() const
    {
        WORD id = m_name.m_id;
        if (!id)
            return m_name.m_str;

        IDTYPE_ nIDTYPE_ = g_db.IDTypeFromResType(m_type);
        MStringW label = g_db.GetNameOfResID(nIDTYPE_, id);
        if (label.empty() || m_type == RT_STRING || m_type == RT_MESSAGETABLE)
            return mstr_dec_word(id);

        if (!mchr_is_digit(label[0]))
        {
            label += L" (";
            label += mstr_dec_word(id);
            label += L")";
        }
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
        return size() == 0;
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

    BOOL is_editable() const
    {
        if (!this)
            return FALSE;

        const MIdOrString& type = m_type;
        switch (m_et)
        {
        case ET_LANG:
            if (type == RT_ACCELERATOR || type == RT_DIALOG || type == RT_HTML ||
                type == RT_MANIFEST || type == RT_MENU || type == RT_VERSION ||
                type == RT_DLGINIT || type == TEXT("RISOHTEMPLATE"))
            {
                ;
            }
            else
            {
                return FALSE;
            }
            break;
        case ET_STRING: case ET_MESSAGE:
            break;
        default:
            return FALSE;
        }
        return TRUE;
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

// https://msdn.microsoft.com/ja-jp/library/windows/desktop/bb773793.aspx
// NOTE: It is not safe to delete items in response to a notification such as TVN_SELCHANGING.

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
                const MIdOrString& name = WORD(0), WORD lang = 0xFFFF, bool invalid_ok = false) const
    {
        for (auto entry : *this)
        {
            if (!entry->valid() && !invalid_ok)
                continue;
            if (entry->match(et, type, name, lang))
                found.insert(entry);
        }
        return !found.empty();
    }

    EntryBase *find(EntryType et, const MIdOrString& type, const MIdOrString& name = WORD(0),
                    WORD lang = 0xFFFF, bool invalid_ok = false) const
    {
        super_type found;
        if (search(found, et, type, name, lang, invalid_ok))
        {
            return *found.begin();
        }
        return NULL;
    }

    EntryBase *find(EntryBase *entry, bool invalid_ok = false) const
    {
        return find(entry->m_et, entry->m_type, entry->m_name, entry->m_lang, invalid_ok);
    }

    bool intersect(const EntrySet& another) const
    {
        if (size() == 0 && another.size() == 0)
            return false;

        for (auto item1 : *this)
        {
            for (auto item2 : another)
            {
                if (*item1 == *item2 && item1->size())
                    return true;
            }
        }
        return false;
    }

    void merge(const EntrySet& another)
    {
        for (auto entry : another)
        {
            if (entry->m_et != ET_LANG)
                continue;

            add_lang_entry(entry->m_type, entry->m_name, entry->m_lang, entry->m_data);
        }
    }

    EntryBase *
    add_lang_entry(const MIdOrString& type, const MIdOrString& name, WORD lang)
    {
        EntryBase::data_type data;
        return add_lang_entry(type, name, lang, data);
    }

    EntryBase *
    add_lang_entry(const MIdOrString& type, const MIdOrString& name, 
                   WORD lang, const EntryBase::data_type& data)
    {
        auto entry = find(ET_LANG, type, name, lang, true);

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

        if (!entry)
        {
            entry = Res_NewLangEntry(type, name, lang);
        }

        entry->m_data = data;
        return on_insert_entry(entry);
    }

    void delete_entry(EntryBase *entry)
    {
        EntryBase *parent = get_parent(entry);
        switch (entry->m_et)
        {
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
            break;
        }

        entry->mark_invalid();

        do
        {
            if (!parent)
                break;
            if (get_first_child(parent))
                break;
            if (parent)
                delete_entry(parent);
        } while (0);
    }

    void search_invalid(super_type& found)
    {
        for (auto entry : *this)
        {
            if (!entry->valid())
                found.insert(entry);
            if (is_childless_parent(entry))
                found.insert(entry);
        }
    }

    void delete_invalid()
    {
        super_type found;
        search_invalid(found);

        for (auto entry : found)
        {
            if (super()->find(entry) == super()->end())
                continue;

            if (entry->m_hItem && entry == get_entry(entry->m_hItem))
            {
                TreeView_DeleteItem(m_hwndTV, entry->m_hItem);
            }

            erase(entry);
            delete entry;
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
            if (!entry->valid())
                continue;
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

    bool extract_cursor(const EntryBase& c_entry, const wchar_t *file_name) const
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

    bool extract_group_cursor(const EntryBase& group, const wchar_t *file_name) const
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

    BOOL extract_icon(const EntryBase& i_entry, const wchar_t *file_name) const
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

    bool extract_group_icon(const EntryBase& group, const wchar_t *file_name) const
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
    add_bitmap(const MIdOrString& name, WORD lang, const MStringW& file)
    {
        MByteStreamEx stream;
        if (!stream.LoadFromFile(file.c_str()) || stream.size() <= 4)
            return NULL;

        if (stream.size() >= 4 &&
            (memcmp(&stream[0], "\xFF\xD8\xFF", 3) == 0 ||    // JPEG
             memcmp(&stream[0], "GIF", 3) == 0 ||             // GIF
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

            return add_lang_entry(RT_BITMAP, name, lang, PackedDIB);
        }

        size_t head_size = sizeof(BITMAPFILEHEADER);
        if (stream.size() < head_size)
            return NULL;

        size_t i0 = head_size, i1 = stream.size();
        EntryBase::data_type HeadLess(&stream[i0], &stream[i0] + (i1 - i0));
        return add_lang_entry(RT_BITMAP, name, lang, HeadLess);
    }

    EntryBase *
    add_group_icon(const MIdOrString& name, WORD lang, 
                   const MStringW& file_name)
    {
        IconFile icon;
        if (!icon.LoadFromFile(file_name.c_str()) || icon.type() != RES_ICON)
            return NULL;

        UINT LastIconID = get_last_id(RT_ICON, lang);
        UINT NextIconID = LastIconID + 1;

        int i, nCount = icon.GetImageCount();
        for (i = 0; i < nCount; ++i)
        {
            add_lang_entry(RT_ICON, WORD(NextIconID + i), lang, icon.GetImage(i));
        }

        IconFile::DataType data(icon.GetIconGroup(NextIconID));
        return add_lang_entry(RT_GROUP_ICON, name, lang, data);
    }

    EntryBase *
    add_group_cursor(const MIdOrString& name, WORD lang, 
                     const MStringW& file_name)
    {
        CursorFile cur;
        if (!cur.LoadFromFile(file_name.c_str()) || cur.type() != RES_CURSOR)
            return NULL;

        UINT LastCursorID = get_last_id(RT_CURSOR, lang);
        UINT NextCursorID = LastCursorID + 1;

        int i, nCount = cur.GetImageCount();
        for (i = 0; i < nCount; ++i)
        {
            add_lang_entry(RT_CURSOR, WORD(NextCursorID + i), lang, cur.GetImage(i));
        }

        CursorFile::DataType data(cur.GetCursorGroup(NextCursorID));
        return add_lang_entry(RT_GROUP_CURSOR, name, lang, data);
    }

    EntryBase *
    add_string_entry(WORD lang)
    {
		auto entry = find(ET_STRING, RT_STRING, (WORD)0, lang, true);
        if (!entry)
			entry = Res_NewStringEntry(lang);
        return on_insert_entry(entry);
    }

    EntryBase *
    add_message_entry(WORD lang)
    {
		auto entry = find(ET_MESSAGE, RT_MESSAGETABLE, (WORD)0, lang, true);
        if (!entry)
			entry = Res_NewMessageEntry(lang);
        return on_insert_entry(entry);
    }

    EntryBase *
    add_name_entry(const MIdOrString& type, const MIdOrString& name)
    {
		auto entry = find(ET_NAME, type, name, 0xFFFF, true);
		if (!entry)
		{
			entry = Res_NewNameEntry(type, name);
		}
        return on_insert_entry(entry);
    }

    EntryBase *
    add_type_entry(const MIdOrString& type, bool replace)
    {
		auto entry = find(ET_TYPE, type, (WORD)0, 0xFFFF, true);
		if (!entry)
		{
			entry = Res_NewTypeEntry(type);
		}
        return on_insert_entry(entry);
    }

    HTREEITEM get_insert_parent(EntryBase *entry)
    {
        if (m_hwndTV == NULL)
            return NULL;

        if (entry->m_et == ET_TYPE)
            return TVI_ROOT;

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

        new_entry = add_name_entry(entry->m_type, entry->m_name);
        if (!new_entry)
            return NULL;
        return new_entry->m_hItem;
    }

    HTREEITEM get_insert_position(EntryBase *entry)
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
            if (entry->m_type == RT_STRING)
                search(found, ET_STRING, entry->m_type);
            if (entry->m_type == RT_MESSAGETABLE)
                search(found, ET_MESSAGE, entry->m_type);
            break;

        case ET_STRING:
            search(found, ET_STRING, entry->m_type, WORD(0), entry->m_lang);
            search(found, ET_NAME, entry->m_type);
            break;

        case ET_MESSAGE:
            search(found, ET_MESSAGE, entry->m_type, WORD(0), entry->m_lang);
            search(found, ET_NAME, entry->m_type);
            break;

        case ET_LANG:
            search(found, ET_LANG, entry->m_type, entry->m_name);
            break;

        default:
            return NULL;
        }

        EntryBase *target = NULL;
        for (auto e : found)
        {
            if (*e < *entry)
            {
                if (!target)
                {
                    target = e;
                }
                else if (*target < *e)
                {
                    target = e;
                }
            }
        }

        if (target)
            return target->m_hItem;

        return TVI_FIRST;
    }

    EntryBase *get_parent(EntryBase *entry)
    {
        if (!entry)
            return NULL;

        EntryBase *parent;
        switch (entry->m_et)
        {
        case ET_NAME:
        case ET_STRING:
        case ET_MESSAGE:
            parent = find(ET_TYPE, entry->m_type);
            break;

        case ET_LANG:
            parent = find(ET_NAME, entry->m_type, entry->m_name);
            break;

        default:
            parent = NULL;
            break;
        }
        return parent;
    }

    bool is_childless_parent(EntryBase *entry) const
    {
        assert(entry);
        switch (entry->m_et)
        {
        case ET_TYPE:
            return !find(ET_NAME, entry->m_type);
        case ET_NAME:
            return !find(ET_LANG, entry->m_type, entry->m_name);
        case ET_STRING:
            return false;
        case ET_MESSAGE:
            return false;
        case ET_LANG:
            return false;
        default:
            return false;
        }
    }

    MStringW get_label(const EntryBase *entry)
    {
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
        return strText;
    }

    EntryBase *on_insert_entry(EntryBase *entry)
    {
        DebugPrintDx(L"on_insert_entry: %p, %s, %s, %u, %s\n", entry, entry->m_type.c_str(), entry->m_name.c_str(), entry->m_lang, entry->m_strLabel.c_str());

        if (m_hwndTV == NULL)
        {
            entry->m_valid = true;
            insert(entry);
            return NULL;
        }

        HTREEITEM hParent = get_insert_parent(entry);
        HTREEITEM hPosition = get_insert_position(entry);

        return on_insert_after(hParent, entry, hPosition);
    }

    EntryBase *
    on_insert_after(HTREEITEM hParent, EntryBase *entry, HTREEITEM hInsertAfter)
    {
        assert(entry);

        entry->m_valid = true;

        if (m_hwndTV == NULL)
            return entry;

        if (entry->m_hItem && entry == get_entry(entry->m_hItem))
        {
            insert(entry);
            return entry;
        }

        TV_INSERTSTRUCTW insert_struct;
        ZeroMemory(&insert_struct, sizeof(insert_struct));

        insert_struct.hParent = hParent;
        insert_struct.hInsertAfter = hInsertAfter;
        insert_struct.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM |
                                  TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        insert_struct.item.state = 0;
        insert_struct.item.stateMask = 0;

        entry->m_strLabel = get_label(entry);
        insert_struct.item.pszText = &entry->m_strLabel[0];

        insert_struct.item.lParam = (LPARAM)entry;
        if (entry->m_et == ET_TYPE || entry->m_et == ET_NAME)
        {
            insert_struct.item.iImage = 1;
            insert_struct.item.iSelectedImage = 1;
        }
        else
        {
            insert_struct.item.iImage = 0;
            insert_struct.item.iSelectedImage = 0;
        }
        if (auto hItem = TreeView_InsertItem(m_hwndTV, &insert_struct))
        {
            entry->m_hItem = hItem;
            insert(entry);
            return entry;
        }

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
        EntrySet *this_;
    };

    EntryBase *
    add_res_entry(HMODULE hMod, LPCWSTR type, LPCWSTR name, WORD lang)
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
            return add_lang_entry(type, name, lang, data);
        }

        return NULL;
    }

    static BOOL CALLBACK
    EnumResLangProc(HMODULE hMod, LPCWSTR lpszType, LPCWSTR lpszName, 
                    WORD wIDLanguage, LPARAM lParam)
    {
        auto ers = (EnumResStruct *)lParam;
        ers->this_->add_res_entry(hMod, lpszType, lpszName, wIDLanguage);
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

    EntryBase *get_first_child(EntryBase *parent) const
    {
        if (!parent)
            return NULL;

        EntryBase *child;
        switch (parent->m_et)
        {
        case ET_TYPE:
            child = find(ET_NAME, parent->m_type);
            break;

        case ET_NAME:
            child = find(ET_LANG, parent->m_type, parent->m_name);
            break;

        default:
            child = NULL;
            break;
        }
        return child;
    }

public:
    BOOL from_res(HMODULE hMod)
    {
        EnumResStruct ers;
        ers.this_ = this;
        return ::EnumResourceTypesW(hMod, EnumResTypeProc, (LPARAM)&ers);
    }

    void copy_to(EntrySet& es) const
    {
        for (auto entry : *this)
        {
            if (entry->m_et == ET_LANG)
                es.add_lang_entry(entry->m_type, entry->m_name, entry->m_lang, entry->m_data);
        }
    }

    void delete_all(void)
    {
        if (m_hwndTV)
        {
            TreeView_DeleteAllItems(m_hwndTV);
        }
        else
        {
            search_and_delete(ET_ANY, (WORD)0, (WORD)0, 0xFFFF);
            delete_invalid();
        }
    }

    void on_delete_item(EntryBase *entry)
    {
        if (!entry || super()->find(entry) == super()->end())
            return;

        DebugPrintDx(L"on_delete_item: %p, %s, %s, %u, %s\n", entry, entry->m_type.c_str(), entry->m_name.c_str(), entry->m_lang, entry->m_strLabel.c_str());
        entry->m_hItem = NULL;
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
        if (!lParam)
            return NULL;
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

        memcpy(&dir, &(*entry)[0], sizeof(dir));

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

            add_lang_entry(RT_ICON, WORD(nNextID), e->m_lang, e->m_data);
        }

        add_lang_entry(RT_GROUP_ICON, new_name, entry->m_lang, entry->m_data);
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

        memcpy(&dir, &(*entry)[0], sizeof(dir));

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

            add_lang_entry(RT_CURSOR, WORD(nNextID), e->m_lang, e->m_data);
        }

        add_lang_entry(RT_GROUP_CURSOR, new_name, entry->m_lang, entry->m_data);
        return TRUE;
    }

    BOOL extract_res(LPCWSTR pszFileName, const EntryBase *entry) const
    {
        MByteStreamEx bs;
        ResHeader header;
        if (!header.WriteTo(bs))
            return FALSE;

        if (entry->m_et != ET_LANG)
            return FALSE;

        header.DataSize = entry->size();
        header.HeaderSize = header.GetHeaderSize(entry->m_type, entry->m_name);
        if (header.HeaderSize == 0 || header.HeaderSize >= 0x10000)
            return FALSE;

        header.type = entry->m_type;
        header.name = entry->m_name;
        header.DataVersion = 0;
        header.MemoryFlags = MEMORYFLAG_DISCARDABLE | MEMORYFLAG_PURE |
                             MEMORYFLAG_MOVEABLE;
        header.LanguageId = entry->m_lang;
        header.Version = 0;
        header.Characteristics = 0;

        if (!header.WriteTo(bs))
            return FALSE;

        if (!bs.WriteData(&(*entry)[0], entry->size()))
            return FALSE;

        bs.WriteDwordAlignment();

        return bs.SaveToFile(pszFileName);
    }

    BOOL extract_res(LPCWSTR pszFileName, const EntrySet& res) const
    {
        MByteStreamEx bs;
        ResHeader header;
        if (!header.WriteTo(bs))
            return FALSE;

        for (auto entry : res)
        {
            if (entry->m_et != ET_LANG)
                continue;

            header.DataSize = entry->size();
            header.HeaderSize = header.GetHeaderSize(entry->m_type, entry->m_name);
            if (header.HeaderSize == 0 || header.HeaderSize >= 0x10000)
                return FALSE;

            header.type = entry->m_type;
            header.name = entry->m_name;
            header.DataVersion = 0;
            header.MemoryFlags = MEMORYFLAG_DISCARDABLE | MEMORYFLAG_PURE |
                                 MEMORYFLAG_MOVEABLE;
            header.LanguageId = entry->m_lang;
            header.Version = 0;
            header.Characteristics = 0;

            if (!header.WriteTo(bs))
                return FALSE;

            if (!bs.WriteData(&(*entry)[0], entry->size()))
                return FALSE;

            bs.WriteDwordAlignment();
        }

        return bs.SaveToFile(pszFileName);
    }

    BOOL extract_cursor(LPCWSTR pszFileName, const EntryBase *entry) const
    {
        if (entry->m_type == RT_GROUP_CURSOR)
        {
            return extract_group_cursor(*entry, pszFileName);
        }
        else if (entry->m_type == RT_CURSOR)
        {
            return extract_cursor(*entry, pszFileName);
        }
        else if (entry->m_type == RT_ANICURSOR)
        {
            MFile file;
            DWORD cbWritten = 0;
            if (file.OpenFileForOutput(pszFileName) &&
                file.WriteFile(&(*entry)[0], entry->size(), &cbWritten))
            {
                file.FlushFileBuffers();
                file.CloseHandle();
                return TRUE;
            }
        }
        return FALSE;
    }

    BOOL extract_icon(LPCWSTR pszFileName, const EntryBase *entry) const
    {
        if (entry->m_type == RT_GROUP_ICON)
        {
            return extract_group_icon(*entry, pszFileName);
        }
        else if (entry->m_type == RT_ICON)
        {
            return extract_icon(*entry, pszFileName);
        }
        else if (entry->m_type == RT_ANIICON)
        {
            MFile file;
            DWORD cbWritten = 0;
            if (file.OpenFileForOutput(pszFileName) &&
                file.WriteFile(&(*entry)[0], entry->size(), &cbWritten))
            {
                file.FlushFileBuffers();
                file.CloseHandle();
                return TRUE;
            }
        }
        return FALSE;
    }

    BOOL extract_bin(LPCWSTR pszFileName, const EntryBase *e) const
    {
        if (e->m_et != ET_LANG)
            return FALSE;

        MByteStreamEx bs(e->m_data);
        return bs.SaveToFile(pszFileName);
    }

    BOOL import_res(LPCWSTR pszResFile)
    {
        MByteStreamEx stream;
        if (!stream.LoadFromFile(pszResFile))
            return FALSE;

        BOOL bAdded = FALSE;
        ResHeader header;
        while (header.ReadFrom(stream))
        {
            bAdded = TRUE;
            if (header.DataSize == 0)
            {
                stream.ReadDwordAlignment();
                continue;
            }

            if (header.DataSize > stream.remainder())
                return FALSE;

            EntryBase::data_type data;
            if (header.DataSize)
            {
                data.resize(header.DataSize);
                if (!stream.ReadData(&data[0], header.DataSize))
                {
                    break;
                }
            }

            add_lang_entry(header.type, header.name, header.LanguageId, data);
            stream.ReadDwordAlignment();
        }
        return bAdded;
    }

    BOOL load_msg_table(LPCWSTR pszRCFile, MStringA& strOutput, const MString& strMcdxExe,
        const MStringW& strMacrosDump, const MStringW& strIncludesDump)
    {
        WCHAR szPath3[MAX_PATH];
        lstrcpynW(szPath3, GetTempFileNameDx(L"R3"), MAX_PATH);

        MStringW strCmdLine;
        strCmdLine += L'\"';
        strCmdLine += strMcdxExe;
        strCmdLine += L"\" ";
        strCmdLine += strMacrosDump;
        strCmdLine += strIncludesDump;
        strCmdLine += L" -o \"";
        strCmdLine += szPath3;
        strCmdLine += L"\" -J rc -O res \"";
        strCmdLine += pszRCFile;
        strCmdLine += L'\"';
        //MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

        BOOL bSuccess = FALSE;

        MProcessMaker pmaker;
        pmaker.SetShowWindow(SW_HIDE);
        pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

        EntrySet res;
        MFile hInputWrite, hOutputRead;
        SetEnvironmentVariableW(L"LANG", L"en_US");
        if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
            pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
        {
            pmaker.ReadAll(strOutput, hOutputRead);

            if (pmaker.GetExitCode() == 0)
            {
                Sleep(500);
                if (res.import_res(szPath3))
                {
                    bSuccess = TRUE;
                }
            }
        }

        DeleteFileW(szPath3);
        return bSuccess;
    }

    BOOL load_rc(LPCWSTR pszRCFile, MStringA& strOutput,
        const MString& strWindresExe, const MString& strCppExe, const MString& strMcdxExe, 
        const MStringW& strMacrosDump, const MStringW& strIncludesDump)
    {
        WCHAR szPath3[MAX_PATH];
        lstrcpynW(szPath3, GetTempFileNameDx(L"R3"), MAX_PATH);

        MStringW strCmdLine;
        strCmdLine += L'\"';
        strCmdLine += strWindresExe;
        strCmdLine += L"\" -DRC_INVOKED ";
        strCmdLine += strMacrosDump;
        strCmdLine += strIncludesDump;
        strCmdLine += L" -o \"";
        strCmdLine += szPath3;
        strCmdLine += L"\" -J rc -O res -F pe-i386 --preprocessor=\"";
        strCmdLine += strCppExe;
        strCmdLine += L"\" --preprocessor-arg=\"\" \"";
        strCmdLine += pszRCFile;
        strCmdLine += L'\"';
        //MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

        BOOL bSuccess = FALSE;

        MProcessMaker pmaker;
        pmaker.SetShowWindow(SW_HIDE);
        pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

        MFile hInputWrite, hOutputRead;
        SetEnvironmentVariableW(L"LANG", L"en_US");
        if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
            pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
        {
            pmaker.ReadAll(strOutput, hOutputRead);

            if (pmaker.GetExitCode() == 0)
            {
                Sleep(500);
                bSuccess = import_res(szPath3);
            }
            else if (strOutput.find(": no resources") != MStringA::npos)
            {
                bSuccess = TRUE;
                strOutput.clear();
            }
        }

        if (bSuccess)
        {
            EntrySet es;
            bSuccess = es.load_msg_table(pszRCFile, strOutput, strMcdxExe, strMacrosDump, strIncludesDump);
            if (bSuccess)
            {
                merge(es);
            }
        }

#ifdef NDEBUG
        DeleteFileW(szPath3);
#endif

        return bSuccess;
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
    for (auto& pair : title_to_bitmap)
    {
        DeleteObject(pair.second);
    }
    title_to_bitmap.clear();
    for (auto& pair : title_to_icon)
    {
        DestroyIcon(pair.second);
    }
    title_to_icon.clear();
}

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef RES_HPP_
