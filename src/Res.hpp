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

#ifndef FILE_WAIT_TIME
    #define FILE_WAIT_TIME      (800)           // 0.8 seconds
#endif
#ifndef PROCESS_TIMEOUT
    #define PROCESS_TIMEOUT     (5 * 1000)      // 5 seconds
#endif

///////////////////////////////////////////////////////////////////////////////

// is the resource type an "entity type" ?
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

// is the resource type a "plain text" type?
inline INT
Res_IsPlainText(const MIdOrString& type)
{
    return type == RT_HTML || type == RT_MANIFEST ||
           type == RT_DLGINCLUDE || type == L"RISOHTEMPLATE";
}

// has the resource type no name?
inline BOOL
Res_HasNoName(const MIdOrString& type)
{
    return type == RT_STRING || type == RT_MESSAGETABLE;
}

///////////////////////////////////////////////////////////////////////////////
// EntryType --- the entry type

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

#define BAD_LANG    0xFFFF      // invalid language value

struct EntryBase
{
    typedef DWORD               size_type;
    typedef std::vector<BYTE>   data_type;

    EntryType       m_et;                   // entry type
    MIdOrString     m_type;                 // resource type
    MIdOrString     m_name;                 // resource name
    WORD            m_lang = BAD_LANG;      // resource language
    HTREEITEM       m_hItem = NULL;         // treeview item handle
    bool            m_valid = true;         // "is it valid?" flag
    data_type       m_data;                 // the item data
    MStringW        m_strLabel;             // the label string

    // constructor
    EntryBase() : m_lang(BAD_LANG), m_hItem(NULL), m_valid(true)
    {
    }

    // constructor
    EntryBase(EntryType et, const MIdOrString& type, 
            const MIdOrString& name = WORD(0), WORD lang = BAD_LANG)
        : m_et(et), m_type(type), m_name(name), m_lang(lang), m_hItem(NULL), m_valid(true)
    {
    }

    // destructor
    virtual ~EntryBase()
    {
    }

    // is it valid?
    bool valid() const
    {
        if (m_et == ET_LANG)
            return !empty() && m_valid;

        if (!m_hItem)
            return false;

        return m_valid;
    }

    // make it invalid
    void mark_invalid()
    {
        m_valid = false;
        m_data.clear();
    }

    // can it be editted by GUI?
    bool can_gui_edit() const
    {
        return m_type == RT_DIALOG || m_type == RT_MENU ||
               m_type == RT_STRING || m_type == RT_MESSAGETABLE ||
               m_type == RT_ACCELERATOR || m_type == WORD(240);
    }

    // is it testable?
    bool is_testable() const
    {
        return m_type == RT_DIALOG || m_type == RT_MENU;
    }

    // pattern match
    bool match(EntryType et, const MIdOrString& type, const MIdOrString& name,
               WORD lang = BAD_LANG) const
    {
        if (et != ET_ANY && m_et != et)
            return false;
        if (!type.is_zero() && m_type != type)
            return false;
        if (!name.is_zero() && m_name != name)
            return false;
        if (lang != BAD_LANG && m_lang != lang)
            return false;
        return true;
    }

    // compare
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

    // get the resource type label
    MStringW get_type_label() const
    {
        if (!m_type.m_id)
            return m_type.m_str;    // string name type

        // it was integer name type

        MStringW label = g_db.GetName(L"RESOURCE", m_type.m_id);
        if (label.empty())  // unable to get the label
            return mstr_dec_word(m_type.m_id);  // returns the numeric text

        // got the label
        if (!mchr_is_digit(label[0]))   // first character is not digit
        {
            // add a parenthesis pair and numeric text
            label += L" (";
            label += mstr_dec_word(m_type.m_id);
            label += L")";
        }

        return label;
    }

    // get the resource name label
    MStringW get_name_label() const
    {
        WORD id = m_name.m_id;
        if (!id)
            return m_name.m_str;        // string name resource name

        // get an IDTYPE_ value
        IDTYPE_ nIDTYPE_ = g_db.IDTypeFromResType(m_type);

        // RT_DLGINIT uses dialog name
        if (m_type == RT_DLGINIT)
            nIDTYPE_ = IDTYPE_DIALOG;

        // get the label from an IDTYPE_ value
        MStringW label = g_db.GetNameOfResID(nIDTYPE_, id);
        if (label.empty() || m_type == RT_STRING || m_type == RT_MESSAGETABLE)
        {
            return mstr_dec_word(id);   // returns numeric text
        }

        // got the label 
        if (!mchr_is_digit(label[0]))   // first character is not digit
        {
            // add a parenthesis pair and numeric text
            label += L" (";
            label += mstr_dec_word(id);
            label += L")";
        }
        return label;
    }

    // get the resource language label
    MStringW get_lang_label() const
    {
        // use an external helper function
        MStringW TextFromLang(WORD lang);
        return TextFromLang(m_lang);
    }

    // clear the data
    void clear_data()
    {
        m_data.clear();
    }

    // clear all
    void clear()
    {
        clear_data();
        m_lang = BAD_LANG;
        m_name = (WORD)0;
        m_type = (WORD)0;
    }

    // is it empty?
    bool empty() const
    {
        return size() == 0;
    }
    // the size of data
    size_type size() const
    {
        return size_type(m_data.size());
    }
    // the index accessor
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

    // the pointer to data
    void *ptr(DWORD index = 0)
    {
        return &m_data[index];
    }
    const void *ptr(DWORD index = 0) const
    {
        return &m_data[index];
    }

    // assign the data
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

    // is it editable?
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
                return TRUE;
            }
            return FALSE;
        case ET_STRING: case ET_MESSAGE:
            return TRUE;
        default:
            return FALSE;
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
Res_NewLangEntry(const MIdOrString& type, const MIdOrString& name, WORD lang = BAD_LANG)
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

    HWND m_hwndTV;      // the treeview handle

    // constructor
    EntrySet(HWND hwndTV = NULL) : m_hwndTV(hwndTV)
    {
    }

    // constructor
    EntrySet(const super_type& super, HWND hwndTV = NULL)
        : super_type(super), m_hwndTV(hwndTV)
    {
    }

    // the super class pointer
    super_type *super()
    {
        return dynamic_cast<super_type *>(this);
    }
    const super_type *super() const
    {
        return dynamic_cast<const super_type *>(this);
    }

    // search by pattern matching
    bool search(super_type& found, EntryType et, const MIdOrString& type = WORD(0), 
                const MIdOrString& name = WORD(0), WORD lang = BAD_LANG, bool invalid_ok = false) const
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

    // find by pattern matching
    EntryBase *find(EntryType et, const MIdOrString& type = WORD(0),
                    const MIdOrString& name = WORD(0),
                    WORD lang = BAD_LANG, bool invalid_ok = false) const
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

    // is it overlapped with another?
    bool intersect(const EntrySet& another) const
    {
        if (size() == 0 && another.size() == 0)
            return false;

        for (auto item1 : *this)
        {
            if (item1->m_et != ET_LANG)
                continue;

            for (auto item2 : another)
            {
                if (item2->m_et != ET_LANG)
                    continue;

                if (*item1 == *item2 && item1->valid() && item2->valid())
                    return true;    // found
            }
        }

        return false;   // not found
    }

    // merge another
    void merge(const EntrySet& another)
    {
        for (auto entry : another)
        {
            if (entry->m_et != ET_LANG)
                continue;   // we will merge the ET_LANG entries only

            add_lang_entry(entry->m_type, entry->m_name, entry->m_lang, entry->m_data);
        }
    }

    // add a language entry
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
        if (m_hwndTV)   // it has the treeview handle
        {
            // add the related entries
            if (type == RT_STRING)
            {
                add_string_entry(lang);
            }
            if (type == RT_MESSAGETABLE)
            {
                add_message_entry(lang);
            }
        }

        // find the entry
        auto entry = find(ET_LANG, type, name, lang, true);
        if (!entry)
        {
            // if not found, then create it
            entry = Res_NewLangEntry(type, name, lang);
        }

        // store the data
        entry->m_data = data;

        // finish
        return on_insert_entry(entry);
    }

    // delete an entry (and related entries)
    void delete_entry(EntryBase *entry)
    {
        // delete the related entries
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

        // mark it as invalid. real deletion is done in delete_invalid
        entry->mark_invalid();

        // delete the parent if necessary
        do
        {
            EntryBase *parent = get_parent(entry);
            if (!parent)
                break;  // no parent

            if (get_child(parent))
                break;  // no child

            delete_entry(parent);   // delete the parent
        } while (0);
    }

    // search the invalid entries
    void search_invalid(super_type& found)
    {
        for (auto entry : *this)
        {
            // add the invalid
            if (!entry->valid())
                found.insert(entry);

            // add the childless
            if (is_childless_parent(entry))
                found.insert(entry);
        }
    }

    // delete the invalid entries
    void delete_invalid()
    {
        // search the invalid
        super_type found;
        search_invalid(found);

        // for all the invalid entries
        for (auto entry : found)
        {
            if (super()->find(entry) == super()->end())
                continue;   // not owned. skip it

            if (m_hwndTV && entry->m_hItem && entry == get_entry(entry->m_hItem))
            {
                // delete from treeview
                TreeView_DeleteItem(m_hwndTV, entry->m_hItem);
            }

            // real delete
            erase(entry);
            delete entry;
        }
    }

    // search and delete
    void search_and_delete(EntryType et, const MIdOrString& type = WORD(0), 
                           const MIdOrString& name = WORD(0), WORD lang = BAD_LANG)
    {
        // search
        super_type found;
        search(found, et, type, name, lang);

        // delete
        for (auto entry : found)
        {
            delete_entry(entry);
        }
    }

    // get last ID of the specified type and language
    UINT get_last_id(const MIdOrString& type, WORD lang) const
    {
        WORD wLastID = 0;
        for (auto entry : *this)
        {
            // invalid?
            if (!entry->valid())
                continue;

            // not matched?
            if (entry->m_type != type || !entry->m_name.is_int() || entry->m_name.is_zero())
                continue;

            // not matched language?
            if (entry->m_lang != lang)
                continue;

            // update wLastID if necessary
            if (wLastID < entry->m_name.m_id)
                wLastID = entry->m_name.m_id;
        }
        return wLastID;
    }

    // update the executable
    BOOL update_exe(LPCWSTR ExeFile) const
    {
        // begin the update
        HANDLE hUpdate = ::BeginUpdateResourceW(ExeFile, TRUE);
        if (hUpdate == NULL)
        {
            return FALSE;   // failure
        }

        // for all the language entries
        for (auto entry : *this)
        {
            if (entry->m_et != ET_LANG)
                continue;

            // get the pointer and size
            void *pv = NULL;
            DWORD size = 0;
            if (!(*entry).empty())
            {
                pv = const_cast<void *>((*entry).ptr());
                size = (*entry).size();
            }

            // skip the empty entries
            if (!pv || !size)
                continue;

            // do update
            if (!::UpdateResourceW(hUpdate, (*entry).m_type.ptr(), (*entry).m_name.ptr(),
                                   (*entry).m_lang, pv, size))
            {
                assert(0);
                ::EndUpdateResourceW(hUpdate, TRUE);    // discard
                return FALSE;   // failure
            }
        }

        // finish
        return ::EndUpdateResourceW(hUpdate, FALSE);
    }

    // helper method for MRadWindow and MTestDialog
    void do_bitmap(MTitleToBitmap& title_to_bitmap, DialogItem& item, WORD lang)
    {
        MIdOrString type = RT_BITMAP;

        // find the entry
        auto entry = find(ET_LANG, type, item.m_title, lang);
        if (!entry)
            return;

        // create the bitmap object
        HBITMAP hbm = PackedDIB_CreateBitmapFromMemory(&(*entry)[0], (*entry).size());
        if (hbm)
        {
            if (!item.m_title.empty())  // title is not empty
            {
                // delete the previous
                if (title_to_bitmap[item.m_title])
                    DeleteObject(title_to_bitmap[item.m_title]);

                // update title_to_bitmap
                title_to_bitmap[item.m_title] = hbm;
            }
        }
    }

    // helper method for MRadWindow and MTestDialog
    void do_icon(MTitleToIcon& title_to_icon, DialogItem& item, WORD lang)
    {
        MIdOrString type = RT_GROUP_ICON;

        // find the entry
        auto entry = find(ET_LANG, type, item.m_title, lang);
        if (!entry)
            return;

        // too small?
        if (entry->size() < sizeof(ICONDIR) + sizeof(GRPICONDIRENTRY))
            return;

        // get the entries information
        ICONDIR& dir = (ICONDIR&)(*entry)[0];
        GRPICONDIRENTRY *pGroupIcon = (GRPICONDIRENTRY *)&(*entry)[sizeof(ICONDIR)];

        // get the largest icon image
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

        // find the entry of the largest icon
        type = RT_ICON;
        entry = find(ET_LANG, type, pGroupIcon[n].nID, lang);
        if (!entry)
            return;

        // create an icon object
        HICON hIcon = CreateIconFromResource((PBYTE)&(*entry)[0], (*entry).size(), TRUE, 0x00030000);
        if (hIcon)
        {
            if (!item.m_title.empty())  // the title was not empty
            {
                // delete the previous
                if (title_to_icon[item.m_title])
                    DestroyIcon(title_to_icon[item.m_title]);

                // update title_to_icon
                title_to_icon[item.m_title] = hIcon;
            }
        }
    }

    // extract the cursor as a *.cur file
    bool extract_cursor(const EntryBase& c_entry, const wchar_t *file_name) const
    {
        // copy the header
        LOCALHEADER local;
        if (c_entry.size() < sizeof(local))
        {
            assert(0);
            return false;   // too small
        }
        memcpy(&local, &c_entry[0], sizeof(local));

        // get the remainder pointer and size
        LPBYTE pb = LPBYTE(&c_entry[0]) + sizeof(local);
        DWORD cb = c_entry.size() - sizeof(local);

        // get the BITMAP info
        BITMAP bm;
        if (!PackedDIB_GetInfo(pb, cb, bm))
        {
            assert(0);
            return false;   // unable to get
        }

        // store data to the structures
        ICONDIR dir = { 0, RES_CURSOR, 1 };
        ICONDIRENTRY entry;
        entry.bWidth = (BYTE)bm.bmWidth;
        entry.bHeight = (BYTE)(bm.bmHeight / 2);
        entry.bColorCount = 0;
        entry.bReserved = 0;
        entry.xHotSpot = local.xHotSpot;
        entry.yHotSpot = local.yHotSpot;
        entry.dwBytesInRes = c_entry.size() - sizeof(local);
        entry.dwImageOffset = sizeof(dir) + sizeof(ICONDIRENTRY);

        // write them to the stream
        MByteStreamEx stream;
        if (!stream.WriteRaw(dir) ||
            !stream.WriteData(&entry, sizeof(entry)) ||
            !stream.WriteData(pb, cb))
        {
            assert(0);
            return false;
        }

        // save the stream to a file
        return stream.SaveToFile(file_name);
    }

    // extract the group cursor as a *.cur file
    bool extract_group_cursor(const EntryBase& group, const wchar_t *file_name) const
    {
        ICONDIR dir;
        if (group.m_type != RT_GROUP_CURSOR || group.size() < sizeof(dir))
        {
            assert(0);
            return false;   // invalid
        }

        // group --> dir
        memcpy(&dir, &group[0], sizeof(dir));
        if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
        {
            assert(0);
            return false;   // invalid
        }

        // check the size
        DWORD SizeOfCursorEntries = sizeof(GRPCURSORDIRENTRY) * dir.idCount;
        if (group.size() < sizeof(dir) + SizeOfCursorEntries)
        {
            assert(0);
            return false;   // invalid
        }

        // group --> GroupEntries
        std::vector<GRPCURSORDIRENTRY> GroupEntries(dir.idCount);
        memcpy(&GroupEntries[0], &group[sizeof(dir)], 
               SizeOfCursorEntries);

        // set the current offset
        DWORD offset = sizeof(dir) + sizeof(ICONDIRENTRY) * dir.idCount;

        // store the entries to DirEntries
        std::vector<ICONDIRENTRY> DirEntries(dir.idCount);
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            // find the RT_CURSOR
            auto entry = find(ET_LANG, RT_CURSOR, GroupEntries[i].nID, group.m_lang);
            if (!entry)
                continue;   // not found

            // get the LOCALHEADER header
            LOCALHEADER local;
            if (entry->size() >= sizeof(local))
                memcpy(&local, &(*entry)[0], sizeof(local));

            // GroupEntries[i] --> DirEntries[i]
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

            // move the offset
            offset += DirEntries[i].dwBytesInRes;
        }

        // write the header to the stream
        MByteStreamEx stream;
        if (!stream.WriteRaw(dir))
        {
            assert(0);
            return false;   // unable to write
        }

        // write the dir entries to the stream
        DWORD SizeOfDirEntries = sizeof(ICONDIRENTRY) * dir.idCount;
        if (!stream.WriteData(&DirEntries[0], SizeOfDirEntries))
        {
            assert(0);
            return false;   // unable to write
        }

        // write the images to the stream
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            // find RT_CURSOR
            auto entry = find(ET_LANG, RT_CURSOR, GroupEntries[i].nID, group.m_lang);
            if (!entry)
                continue;

            DWORD cbLocal = sizeof(LOCALHEADER);

            // get the current pointer and size
            LPBYTE pb = LPBYTE(&(*entry)[0]) + cbLocal;
            DWORD dwSize = (*entry).size() - cbLocal;
            if (!stream.WriteData(pb, dwSize))
            {
                assert(0);
                return FALSE;   // unable to write
            }
        }

        // save the stream to a file
        return stream.SaveToFile(file_name);
    }

    // extract the icon as a *.ico file
    BOOL extract_icon(const EntryBase& i_entry, const wchar_t *file_name) const
    {
        // get the BITMAP info
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

        // store
        ICONDIR dir = { 0, RES_ICON, 1 };
        ICONDIRENTRY entry;
        entry.bWidth = (BYTE)bm.bmWidth;
        entry.bHeight = (BYTE)bm.bmHeight;
        entry.bColorCount = 0;
        entry.bReserved = 0;
        entry.wPlanes = 1;
        entry.wBitCount = bm.bmBitsPixel;
        entry.dwBytesInRes = i_entry.size();
        entry.dwImageOffset = sizeof(dir) + sizeof(ICONDIRENTRY);

        // write the data to the straem
        MByteStreamEx stream;
        if (!stream.WriteRaw(dir) ||
            !stream.WriteData(&entry, sizeof(entry)) ||
            !stream.WriteData(&i_entry[0], i_entry.size()))
        {
            assert(0);
            return false;
        }

        // save the stream to a file
        return stream.SaveToFile(file_name);
    }

    // extract the group icon as a *.ico file
    bool extract_group_icon(const EntryBase& group, const wchar_t *file_name) const
    {
        ICONDIR dir;

        // check the format
        if (group.m_type != RT_GROUP_ICON || group.size() < sizeof(dir))
        {
            assert(0);
            return false;   // invalid
        }

        // group --> dir
        memcpy(&dir, &group[0], sizeof(dir));

        // check the dir
        if (dir.idReserved != 0 || dir.idType != RES_ICON || dir.idCount == 0)
        {
            assert(0);
            return false;   // invalid
        }

        // check the size
        DWORD SizeOfIconEntries = sizeof(GRPICONDIRENTRY) * dir.idCount;
        if (group.size() < sizeof(dir) + SizeOfIconEntries)
        {
            assert(0);
            return false;   // invalid
        }

        // group --> GroupEntries
        std::vector<GRPICONDIRENTRY> GroupEntries(dir.idCount);
        memcpy(&GroupEntries[0], &group[sizeof(dir)], SizeOfIconEntries);

        // set the current offset
        DWORD offset = sizeof(dir) + sizeof(ICONDIRENTRY) * dir.idCount;

        std::vector<ICONDIRENTRY> DirEntries(dir.idCount);
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            // find the RT_ICON entry
            auto entry = find(ET_LANG, RT_ICON, GroupEntries[i].nID, group.m_lang);
            if (!entry)
                continue;

            // GroupEntries[i] --> DirEntries[i]
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

            // move the offset
            offset += DirEntries[i].dwBytesInRes;
        }

        // write the header
        MByteStreamEx stream;
        if (!stream.WriteRaw(dir))
        {
            assert(0);
            return false;   // unable to write
        }

        // write the dir entries
        DWORD SizeOfDirEntries = sizeof(ICONDIRENTRY) * dir.idCount;
        if (!stream.WriteData(&DirEntries[0], SizeOfDirEntries))
        {
            assert(0);
            return false;   // unable to write
        }

        // write the images
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            // find the RT_ICON entry
            auto entry = find(ET_LANG, RT_ICON, GroupEntries[i].nID, group.m_lang);
            if (!entry)
                continue;

            // write it
            DWORD dwSize = (*entry).size();
            if (!stream.WriteData(&(*entry)[0], dwSize))
            {
                assert(0);
                return false;   // unable to write
            }
        }

        // save the stream to a file
        return stream.SaveToFile(file_name);
    }

    // add a bitmap entry
    EntryBase *
    add_bitmap(const MIdOrString& name, WORD lang, const MStringW& file)
    {
        // load the data from an *.bmp file
        MByteStreamEx stream;
        if (!stream.LoadFromFile(file.c_str()) || stream.size() <= 4)
            return NULL;

        // is it a JPEG, GIF or PNG image?
        if (stream.size() >= 4 &&
            (memcmp(&stream[0], "\xFF\xD8\xFF", 3) == 0 ||    // JPEG
             memcmp(&stream[0], "GIF", 3) == 0 ||             // GIF
             memcmp(&stream[0], "\x89\x50\x4E\x47", 4) == 0)) // PNG
        {
            // create a bitmap object from memory
            MBitmapDx bitmap;
            if (!bitmap.CreateFromMemory(&stream[0], (DWORD)stream.size()))
                return NULL;

            LONG cx, cy;
            HBITMAP hbm = bitmap.GetHBITMAP32(cx, cy);

            // create a packed DIB from bitmap handle
            std::vector<BYTE> PackedDIB;
            if (!PackedDIB_CreateFromHandle(PackedDIB, hbm))
            {
                DeleteObject(hbm);
                return NULL;
            }
            DeleteObject(hbm);

            // add the entry
            return add_lang_entry(RT_BITMAP, name, lang, PackedDIB);
        }

        // check the size
        size_t head_size = sizeof(BITMAPFILEHEADER);
        if (stream.size() < head_size)
            return NULL;    // invalid

        // add the entry
        size_t i0 = head_size, i1 = stream.size();
        EntryBase::data_type HeadLess(&stream[i0], &stream[i0] + (i1 - i0));
        return add_lang_entry(RT_BITMAP, name, lang, HeadLess);
    }

    // add a group icon
    EntryBase *
    add_group_icon(const MIdOrString& name, WORD lang, 
                   const MStringW& file_name)
    {
        // load the data from an *.ico file
        IconFile icon;
        if (!icon.LoadFromFile(file_name.c_str()) || icon.type() != RES_ICON)
            return NULL;

        // get the next icon ID
        UINT LastIconID = get_last_id(RT_ICON, lang);
        UINT NextIconID = LastIconID + 1;

        // add the icon images (RT_ICON)
        int i, nCount = icon.GetImageCount();
        for (i = 0; i < nCount; ++i)
        {
            add_lang_entry(RT_ICON, WORD(NextIconID + i), lang, icon.GetImage(i));
        }

        // add the entry
        IconFile::DataType data(icon.GetIconGroup(NextIconID));
        return add_lang_entry(RT_GROUP_ICON, name, lang, data);
    }

    // add a group cursor
    EntryBase *
    add_group_cursor(const MIdOrString& name, WORD lang, 
                     const MStringW& file_name)
    {
        // load the data from an *.cur file
        CursorFile cur;
        if (!cur.LoadFromFile(file_name.c_str()) || cur.type() != RES_CURSOR)
            return NULL;

        // get the next cursor ID
        UINT LastCursorID = get_last_id(RT_CURSOR, lang);
        UINT NextCursorID = LastCursorID + 1;

        // add the cursor images (RT_CURSOR)
        int i, nCount = cur.GetImageCount();
        for (i = 0; i < nCount; ++i)
        {
            add_lang_entry(RT_CURSOR, WORD(NextCursorID + i), lang, cur.GetImage(i));
        }

        // add the entry
        CursorFile::DataType data(cur.GetCursorGroup(NextCursorID));
        return add_lang_entry(RT_GROUP_CURSOR, name, lang, data);
    }

    // add a string entry
    EntryBase *
    add_string_entry(WORD lang)
    {
        auto entry = find(ET_STRING, RT_STRING, (WORD)0, lang, true);
        if (!entry)
            entry = Res_NewStringEntry(lang);
        return on_insert_entry(entry);
    }

    // add a message entry
    EntryBase *
    add_message_entry(WORD lang)
    {
        auto entry = find(ET_MESSAGE, RT_MESSAGETABLE, (WORD)0, lang, true);
        if (!entry)
            entry = Res_NewMessageEntry(lang);
        return on_insert_entry(entry);
    }

    // add a name entry
    EntryBase *
    add_name_entry(const MIdOrString& type, const MIdOrString& name)
    {
        auto entry = find(ET_NAME, type, name, BAD_LANG, true);
        if (!entry)
            entry = Res_NewNameEntry(type, name);
        return on_insert_entry(entry);
    }

    // add a type entry
    EntryBase *
    add_type_entry(const MIdOrString& type, bool replace)
    {
        auto entry = find(ET_TYPE, type, (WORD)0, BAD_LANG, true);
        if (!entry)
            entry = Res_NewTypeEntry(type);
        return on_insert_entry(entry);
    }

    // insert or get the parent entry to be insert there
    HTREEITEM get_insert_parent(EntryBase *entry)
    {
        if (m_hwndTV == NULL)
            return NULL;    // no treeview handle

        if (entry->m_et == ET_TYPE)
            return TVI_ROOT;    // the root handle

        auto new_entry = add_type_entry(entry->m_type, false);
        if (!new_entry)
            return NULL;    // unable to add

        switch (entry->m_et)
        {
        case ET_NAME: case ET_STRING: case ET_MESSAGE:
            return new_entry->m_hItem;  // success

        case ET_LANG:
            break;

        default:
            return NULL;
        }

        // add the name entry
        new_entry = add_name_entry(entry->m_type, entry->m_name);
        if (!new_entry)
            return NULL;    // unable to add

        return new_entry->m_hItem;  // success
    }

    // get the insertion position
    HTREEITEM get_insert_position(EntryBase *entry)
    {
        if (m_hwndTV == NULL)
            return NULL;    // no treeview handle

        // get the entries to determine the position
        super_type found;
        switch (entry->m_et)
        {
        case ET_TYPE:
            search(found, ET_TYPE);
            break;

        case ET_NAME:
            search(found, ET_NAME, entry->m_type);
            if (entry->m_type == RT_STRING)
                search(found, ET_STRING, entry->m_type);
            if (entry->m_type == RT_MESSAGETABLE)
                search(found, ET_MESSAGE, entry->m_type);
            break;

        case ET_STRING:
            search(found, ET_STRING, entry->m_type);
            search(found, ET_NAME, entry->m_type);
            break;

        case ET_MESSAGE:
            search(found, ET_MESSAGE, entry->m_type);
            search(found, ET_NAME, entry->m_type);
            break;

        case ET_LANG:
            search(found, ET_LANG, entry->m_type, entry->m_name);
            break;

        default:
            return NULL;
        }

        // determine the target
        EntryBase *target = NULL;
        for (auto e : found)
        {
            if (*e < *entry)
            {
                if (!target)    // there is no target yet
                {
                    target = e;     // set the target
                }
                else if (*target < *e)  // check the position
                {
                    target = e;     // set the new target
                }
            }
        }

        if (target)
            return target->m_hItem;     // returns the target

        return TVI_FIRST;   // insert as a first
    }

    // get the parent entry
    EntryBase *get_parent(EntryBase *entry)
    {
        if (!entry)
            return NULL;    // no parent

        EntryBase *parent;
        switch (entry->m_et)
        {
        case ET_NAME:
        case ET_STRING:
        case ET_MESSAGE:
            // parent is a type entry
            parent = find(ET_TYPE, entry->m_type);
            break;

        case ET_LANG:
            // parent is a name entry
            parent = find(ET_NAME, entry->m_type, entry->m_name);
            break;

        default:
            parent = NULL;  // no parent
            break;
        }

        return parent;
    }

    // is it a childless entry?
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
        case ET_MESSAGE:
        case ET_LANG:
        default:
            return false;   // not parent
        }
    }

    // get the label for a treeview item
    MStringW get_label(const EntryBase *entry)
    {
        MStringW strText;

        // get the preferred label by the entry type
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

    // helper method for entry insertion
    EntryBase *on_insert_entry(EntryBase *entry)
    {
        DebugPrintDx(L"on_insert_entry: %p, %s, %s, %u, %s\n", entry, entry->m_type.c_str(), entry->m_name.c_str(), entry->m_lang, entry->m_strLabel.c_str());

        if (m_hwndTV == NULL)   // no treeview handle
        {
            entry->m_valid = true;
            insert(entry);
            return NULL;
        }

        // get/insert the insertion parent
        HTREEITEM hParent = get_insert_parent(entry);

        // get the insertion position
        HTREEITEM hPosition = get_insert_position(entry);

        // ok, insert it
        return on_insert_after(hParent, entry, hPosition);
    }
    EntryBase *
    on_insert_after(HTREEITEM hParent, EntryBase *entry, HTREEITEM hInsertAfter)
    {
        assert(entry);

        // make it valid
        entry->m_valid = true;

        if (m_hwndTV == NULL)
            return entry;   // no treeview handle

        if (entry->m_hItem && entry == get_entry(entry->m_hItem))
        {
            // it already has its item handle
            insert(entry);
            return entry;
        }

        // initialize the TV_INSERTSTRUCT structure
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

        // insert it to the treeview
        if (auto hItem = TreeView_InsertItem(m_hwndTV, &insert_struct))
        {
            entry->m_hItem = hItem;     // set the item handle

            insert(entry);  // insert the entry pointer to this instance

            return entry;
        }

        return NULL;    // failure
    }

    // helper method to delete the strings
    void on_delete_string(EntryBase *entry)
    {
        assert(entry->m_et == ET_STRING);
        search_and_delete(ET_LANG, RT_STRING, (WORD)0, entry->m_lang);
    }

    // helper method to delete the messages
    void on_delete_message(EntryBase *entry)
    {
        assert(entry->m_et == ET_MESSAGE);
        search_and_delete(ET_LANG, RT_MESSAGETABLE, (WORD)0, entry->m_lang);
    }

    // helper method to delete the group icon
    bool on_delete_group_icon(EntryBase *entry)
    {
        // validate the entry
        if (entry->m_et != ET_LANG || entry->m_type != RT_GROUP_ICON)
            return false;   // invalid

        // store the data to the stream
        MByteStreamEx bs(entry->m_data);

        // read the header from the stream
        ICONDIR dir;
        if (!bs.ReadRaw(dir))
            return false;   // unable to read

        // read the dir entries from the stream
        DWORD size = sizeof(GRPICONDIRENTRY) * dir.idCount;
        std::vector<GRPICONDIRENTRY> DirEntries(dir.idCount);
        if (!bs.ReadData(&DirEntries[0], size))
            return false;   // unable to read

        // delete the related RT_ICON entries
        DWORD i, nCount = dir.idCount;
        for (i = 0; i < nCount; ++i)
        {
            search_and_delete(ET_LANG, RT_ICON, DirEntries[i].nID, (*entry).m_lang);
        }

        return true;    // success
    }

    // helper method to delete the group cursor
    bool on_delete_group_cursor(EntryBase *entry)
    {
        // validate the entry
        if (entry->m_et != ET_LANG || entry->m_type != RT_GROUP_CURSOR)
            return false;   // invalid

        // store the data to the stream
        MByteStreamEx bs(entry->m_data);

        // read the header from the stream
        ICONDIR dir;
        if (!bs.ReadRaw(dir))
            return false;   // unable to read

        // read the dir entries from the stream
        DWORD size = sizeof(GRPCURSORDIRENTRY) * dir.idCount;
        std::vector<GRPCURSORDIRENTRY> DirEntries(dir.idCount);
        if (!bs.ReadData(&DirEntries[0], size))
            return false;   // unable to read

        // delete the related RT_CURSOR entries
        DWORD i, nCount = dir.idCount;
        for (i = 0; i < nCount; ++i)
        {
            search_and_delete(ET_LANG, RT_CURSOR, DirEntries[i].nID, (*entry).m_lang);
        }

        return true;    // success
    }

    struct EnumResStruct
    {
        EntrySet *this_;
    };

    // add a resource entry from an executable module
    EntryBase *
    add_res_entry(HMODULE hMod, LPCWSTR type, LPCWSTR name, WORD lang)
    {
        // find the resource in hMod
        HRSRC hResInfo = FindResourceExW(hMod, type, name, lang);
        if (!hResInfo)
            return NULL;

        // get the size and the pointer
        DWORD dwSize = SizeofResource(hMod, hResInfo);
        HGLOBAL hGlobal = LoadResource(hMod, hResInfo);
        LPVOID pv = LockResource(hGlobal);
        if (pv && dwSize)
        {
            // got it. add a language entry
            EntryBase::data_type data((LPBYTE)(pv), (LPBYTE)(pv) + dwSize);
            return add_lang_entry(type, name, lang, data);
        }

        return NULL;    // unable to get
    }

    // callback to insert the resource in the executable
    static BOOL CALLBACK
    EnumResLangProc(HMODULE hMod, LPCWSTR lpszType, LPCWSTR lpszName, 
                    WORD wIDLanguage, LPARAM lParam)
    {
        auto ers = (EnumResStruct *)lParam;
        ers->this_->add_res_entry(hMod, lpszType, lpszName, wIDLanguage);
        return TRUE;
    }

    // callback to insert the resource in the executable
    static BOOL CALLBACK
    EnumResNameProc(HMODULE hMod, LPCWSTR lpszType, LPWSTR lpszName, LPARAM lParam)
    {
        return ::EnumResourceLanguagesW(hMod, lpszType, lpszName, EnumResLangProc, lParam);
    }

    // callback to insert the resource in the executable
    static BOOL CALLBACK
    EnumResTypeProc(HMODULE hMod, LPWSTR lpszType, LPARAM lParam)
    {
        return ::EnumResourceNamesW(hMod, lpszType, EnumResNameProc, lParam);
    }

    // get the child if any
    EntryBase *get_child(EntryBase *parent) const
    {
        if (!parent)
            return NULL;    // no parent, no child

        // get child
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
            child = NULL;   // no child
            break;
        }

        return child;
    }

public:
    // add the resources in the executable module
    BOOL from_res(HMODULE hMod)
    {
        EnumResStruct ers;
        ers.this_ = this;
        return ::EnumResourceTypesW(hMod, EnumResTypeProc, (LPARAM)&ers);
    }

    // delete all the entries
    void delete_all(void)
    {
        if (m_hwndTV)
        {
            TreeView_DeleteAllItems(m_hwndTV);
        }
        else
        {
            search_and_delete(ET_ANY, (WORD)0, (WORD)0, BAD_LANG);
            delete_invalid();
        }
    }

    // for TVN_DELETEITEM
    void on_delete_item(EntryBase *entry)
    {
        if (!entry || super()->find(entry) == super()->end())
            return;

        DebugPrintDx(L"on_delete_item: %p, %s, %s, %u, %s\n", entry, entry->m_type.c_str(), entry->m_name.c_str(), entry->m_lang, entry->m_strLabel.c_str());
        entry->m_hItem = NULL;
        delete_entry(entry);
    }

    // get the LPARAM parameter of the currently selected or the specified handle
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

    // get the entry pointer of the currently selected or the specified info
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

    // get a language entry of the currently selected or the specified handle
    EntryBase *get_lang_entry(HTREEITEM hItem = NULL) const
    {
        return get_entry(hItem, ET_LANG);
    }

    // get the selected item handle of treeview
    HTREEITEM get_item(void) const
    {
        return TreeView_GetSelection(m_hwndTV);
    }

    // copy the group icon
    BOOL copy_group_icon(EntryBase *entry, const MIdOrString& new_name, WORD new_lang)
    {
        assert(entry->m_et == ET_LANG);
        assert(entry->m_type == RT_GROUP_ICON);

        // check the size
        ICONDIR dir;
        if (entry->size() < sizeof(dir))
        {
            assert(0);
            return FALSE;   // invalid
        }

        // entry --> dir
        memcpy(&dir, &(*entry)[0], sizeof(dir));

        // check the format
        if (dir.idReserved != 0 || dir.idType != RES_ICON || dir.idCount == 0)
        {
            assert(0);
            return FALSE;   // invalid
        }

        // check the size
        if (entry->size() < sizeof(dir) + dir.idCount * sizeof(GRPICONDIRENTRY))
        {
            assert(0);
            return FALSE;   // invalid
        }

        // get the pointers of old and new entries
        auto data = entry->m_data;
        auto old_entries = (GRPCURSORDIRENTRY *)&(*entry)[sizeof(dir)];
        auto new_entries = (GRPCURSORDIRENTRY *)&data[sizeof(dir)];

        LONG cx = 0, cy = 0;
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            // find the RT_ICON entry
            auto e = find(ET_LANG, RT_ICON, old_entries[i].nID, entry->m_lang);
            if (!e)
                return FALSE;

            // get the next ID
            UINT nLastID = get_last_id(RT_ICON, new_lang);
            UINT nNextID = nLastID + 1;

            // add a RT_ICON entry
            add_lang_entry(RT_ICON, WORD(nNextID), new_lang, e->m_data);

            // update the ID in new_entries
            new_entries[i].nID = (WORD)nNextID;
        }

        // add a RT_GROUP_ICON entry
        add_lang_entry(RT_GROUP_ICON, new_name, new_lang, data);
        return TRUE;
    }

    // copy the group cursor
    BOOL copy_group_cursor(EntryBase *entry, const MIdOrString& new_name, WORD new_lang)
    {
        assert(entry->m_et == ET_LANG);
        assert(entry->m_type == RT_GROUP_CURSOR);

        // check the size
        ICONDIR dir;
        if (entry->size() < sizeof(dir))
        {
            assert(0);
            return FALSE;   // invalid
        }

        // entry --> dir
        memcpy(&dir, &(*entry)[0], sizeof(dir));

        // check the format
        if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
        {
            assert(0);
            return FALSE;   // invalid
        }

        // check the size
        if (entry->size() < sizeof(dir) + dir.idCount * sizeof(GRPCURSORDIRENTRY))
        {
            assert(0);
            return FALSE;   // invalid
        }

        // get the pointers of old and new entries
        auto data = entry->m_data;
        auto old_entries = (GRPCURSORDIRENTRY *)&(*entry)[sizeof(dir)];
        auto new_entries = (GRPCURSORDIRENTRY *)&data[sizeof(dir)];

        LONG cx = 0, cy = 0;
        for (WORD i = 0; i < dir.idCount; ++i)
        {
            // find the RT_CURSOR entry
            auto e = find(ET_LANG, RT_CURSOR, old_entries[i].nID, entry->m_lang);
            if (!e)
                return FALSE;

            // get the next ID
            UINT nLastID = get_last_id(RT_CURSOR, new_lang);
            UINT nNextID = nLastID + 1;

            add_lang_entry(RT_CURSOR, WORD(nNextID), new_lang, e->m_data);

            // update the ID in new_entries
            new_entries[i].nID = (WORD)nNextID;
        }

        // add a RT_GROUP_CURSOR entry
        add_lang_entry(RT_GROUP_CURSOR, new_name, new_lang, data);
        return TRUE;
    }

    // extract one resource item as an *.res file
    BOOL extract_res(LPCWSTR pszFileName, const EntryBase *entry) const
    {
        EntrySet found;

        switch (entry->m_et)
        {
        case ET_ANY:
            search(found, ET_LANG);
            break;

        case ET_TYPE:
            search(found, ET_LANG, entry->m_type);
            break;

        case ET_STRING:
        case ET_MESSAGE:
            search(found, ET_LANG, entry->m_type, WORD(0), entry->m_lang);
            break;

        case ET_NAME:
            search(found, ET_LANG, entry->m_type, entry->m_name);
            break;

        case ET_LANG:
            search(found, ET_LANG, entry->m_type, entry->m_name, entry->m_lang);
            break;

        default:
            return FALSE;
        }

        return extract_res(pszFileName, found);
    }

    // extract some resource items as an *.res file
    BOOL extract_res(LPCWSTR pszFileName, const EntrySet& res) const
    {
        MByteStreamEx bs;   // the stream

        // write the header to the stream
        ResHeader header;
        if (!header.WriteTo(bs))
            return FALSE;   // unable to write

        // for all the language entries in res
        for (auto entry : res)
        {
            if (entry->m_et != ET_LANG)
                continue;

            header.DataSize = entry->size();

            // check the header size
            header.HeaderSize = header.GetHeaderSize(entry->m_type, entry->m_name);
            if (header.HeaderSize == 0 || header.HeaderSize >= 0x10000)
                return FALSE;   // invalid

            header.type = entry->m_type;
            header.name = entry->m_name;
            header.DataVersion = 0;
            header.MemoryFlags = MEMORYFLAG_DISCARDABLE | MEMORYFLAG_PURE |
                                 MEMORYFLAG_MOVEABLE;
            header.LanguageId = entry->m_lang;
            header.Version = 0;
            header.Characteristics = 0;

            // write the header to the stream
            if (!header.WriteTo(bs))
                return FALSE;   // unable to write

            // write the data to the stream
            if (!bs.WriteData(&(*entry)[0], entry->size()))
                return FALSE;   // unable to write

            // adjust the alignment
            bs.WriteDwordAlignment();
        }

        // save the stream to an *.res file
        return bs.SaveToFile(pszFileName);
    }

    // extract the cursor as an *.cur or *.ani file
    BOOL extract_cursor(LPCWSTR pszFileName, const EntryBase *entry) const
    {
        if (entry->m_type == RT_GROUP_CURSOR)
        {
            // RT_GROUP_CURSOR
            return extract_group_cursor(*entry, pszFileName);
        }
        else if (entry->m_type == RT_CURSOR)
        {
            // RT_CURSOR
            return extract_cursor(*entry, pszFileName);
        }
        else if (entry->m_type == RT_ANICURSOR)
        {
            // RT_ANICURSOR
            MFile file;
            DWORD cbWritten = 0;
            if (file.OpenFileForOutput(pszFileName) &&
                file.WriteFile(&(*entry)[0], entry->size(), &cbWritten))
            {
                // written to the file
                file.CloseHandle();
                return TRUE;    // success
            }
        }
        return FALSE;   // failure
    }

    // extract the icon as an *.ico file
    BOOL extract_icon(LPCWSTR pszFileName, const EntryBase *entry) const
    {
        if (entry->m_type == RT_GROUP_ICON)
        {
            // RT_GROUP_ICON
            return extract_group_icon(*entry, pszFileName);
        }
        else if (entry->m_type == RT_ICON)
        {
            // RT_ICON
            return extract_icon(*entry, pszFileName);
        }
        else if (entry->m_type == RT_ANIICON)
        {
            // RT_ANIICON
            MFile file;
            DWORD cbWritten = 0;
            if (file.OpenFileForOutput(pszFileName) &&
                file.WriteFile(&(*entry)[0], entry->size(), &cbWritten))
            {
                // written to the file
                file.CloseHandle();
                return TRUE;    // success
            }
        }
        return FALSE;   // failure
    }

    // extract the resource data as a binary file
    BOOL extract_bin(LPCWSTR pszFileName, const EntryBase *e) const
    {
        if (e->m_et != ET_LANG)
            return FALSE;   // invalid

        // write the resource data to a binary file
        MByteStreamEx bs(e->m_data);
        return bs.SaveToFile(pszFileName);
    }

    // import the resource data from the specified *.res file
    BOOL import_res(LPCWSTR pszResFile)
    {
        // load the file to the stream
        MByteStreamEx stream;
        if (!stream.LoadFromFile(pszResFile))
            return FALSE;   // failure

        BOOL bAdded = FALSE;
        ResHeader header;
        while (header.ReadFrom(stream))     // repeat reading
        {
            // header was loaded

            bAdded = TRUE;
            if (header.DataSize == 0)   // no data
            {
                stream.ReadDwordAlignment();
                continue;   // go to next
            }

            if (header.DataSize > stream.remainder())
                return FALSE;   // invalid size

            // read the data
            EntryBase::data_type data;
            if (header.DataSize)
            {
                // store to data
                data.resize(header.DataSize);
                if (!stream.ReadData(&data[0], header.DataSize))
                {
                    break;
                }
            }

            // add a language entry with data
            add_lang_entry(header.type, header.name, header.LanguageId, data);

            // adjust the alignment
            stream.ReadDwordAlignment();
        }

        return bAdded;
    }

    // load the message table from a *.rc file
    BOOL load_msg_table(LPCWSTR pszRCFile, MStringA& strOutput, const MString& strMcdxExe,
        const MStringW& strMacrosDump, const MStringW& strIncludesDump)
    {
        // get the temporary file path
        WCHAR szPath3[MAX_PATH];
        StringCchCopyW(szPath3, _countof(szPath3), GetTempFileNameDx(L"R3"));

        // create the temporary file and wait
        MFile r3(szPath3, TRUE);
        r3.CloseHandle();
        Sleep(FILE_WAIT_TIME);

        // build the command line text
        MStringW strCmdLine;
        strCmdLine += L'\"';
        strCmdLine += strMcdxExe;
        strCmdLine += L"\" -DMCDX_INVOKED=1 ";
        strCmdLine += strMacrosDump;
        strCmdLine += L' ';
        strCmdLine += strIncludesDump;
        strCmdLine += L" -o \"";
        strCmdLine += szPath3;
        strCmdLine += L"\" -J rc -O res \"";
        strCmdLine += pszRCFile;
        strCmdLine += L'\"';
        //MessageBoxW(NULL, strCmdLine.c_str(), NULL, 0);

        BOOL bSuccess = FALSE;

        // create an mcdx.exe process
        MProcessMaker pmaker;
        pmaker.SetShowWindow(SW_HIDE);
        pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

        MFile hInputWrite, hOutputRead;
        SetEnvironmentVariableW(L"LANG", L"en_US");
        if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
            pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
        {
            // read all with timeout
            pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);

            if (pmaker.GetExitCode() == 0)
            {
                // wait
                Sleep(FILE_WAIT_TIME);

                // import from the temporary file
                if (import_res(szPath3))
                {
                    bSuccess = TRUE;
                }
            }
        }

#ifdef NDEBUG
        // delete the temporary file
        DeleteFileW(szPath3);
#endif

        return bSuccess;
    }

    // load the resources from a *.rc file
    BOOL load_rc(LPCWSTR pszRCFile, MStringA& strOutput,
        const MString& strWindresExe, const MString& strCppExe, const MString& strMcdxExe, 
        const MStringW& strMacrosDump, const MStringW& strIncludesDump)
    {
        // get the temporary file path
        WCHAR szPath3[MAX_PATH];
        StringCchCopyW(szPath3, _countof(szPath3), GetTempFileNameDx(L"R3"));

        // create the temporary file and wait
        MFile r3(szPath3, TRUE);
        r3.CloseHandle();
        Sleep(FILE_WAIT_TIME);

        // build the command line text
        MStringW strCmdLine;
        strCmdLine += L'\"';
        strCmdLine += strWindresExe;
        strCmdLine += L"\" -DRC_INVOKED ";
        strCmdLine += strMacrosDump;
        strCmdLine += L' ';
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

        // create a windres.exe process
        MProcessMaker pmaker;
        pmaker.SetShowWindow(SW_HIDE);
        pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

        MFile hInputWrite, hOutputRead;
        SetEnvironmentVariableW(L"LANG", L"en_US");
        if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
            pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
        {
            // read all with timeout
            pmaker.ReadAll(strOutput, hOutputRead, PROCESS_TIMEOUT);

            if (pmaker.GetExitCode() == 0)
            {
                // wait
                Sleep(FILE_WAIT_TIME);

                // import the resource from the temporary file
                bSuccess = import_res(szPath3);
            }
            else if (strOutput.find(": no resources") != MStringA::npos)
            {
                // there is no resource data
                bSuccess = TRUE;
                strOutput.clear();
            }
        }

        if (bSuccess)
        {
            // load the message table if any
            EntrySet es;
            bSuccess = es.load_msg_table(pszRCFile, strOutput, strMcdxExe, strMacrosDump, strIncludesDump);
            if (bSuccess)
            {
                merge(es);
            }
        }

#ifdef NDEBUG
        // delete the temporary file
        DeleteFileW(szPath3);
#endif

        return bSuccess;
    }
};

// g_res
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

#endif  // ndef RES_HPP_
