// Res.hpp --- Win32 Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#pragma once

#include <windows.h>
#include <commctrl.h>
#include <cctype>
#include <cwchar>
#include <set>
#include <unordered_set>     // for std::unordered_set
#include <shlwapi.h>

#include "IconRes.hpp"
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

#ifndef PROCESS_TIMEOUT
    #define PROCESS_TIMEOUT     (20 * 1000)     // 20 seconds
#endif

///////////////////////////////////////////////////////////////////////////////

// is the resource type an "entity type" ?
BOOL Res_IsEntityType(const MIdOrString& type);

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
    WORD            m_lang;                 // resource language
    HTREEITEM       m_hItem;                // treeview item handle
    bool            m_valid;                // "is it valid?" flag
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
        if (m_type < entry.m_type)
            return true;
        if (m_type > entry.m_type)
            return false;
        if (m_et < entry.m_et)
            return true;
        if (m_et > entry.m_et)
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
    MStringW get_name_label() const;

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
    BOOL is_editable() const;

    BOOL is_delphi_dfm() const
    {
        return m_type == RT_RCDATA && size() >= 4 && memcmp(ptr(), "TPF0", 4) == 0;
    }

    std::string get_dfm_text(LPCWSTR pszDFMSC) const;
    void set_dfm_text(LPCWSTR pszDFMSC, std::string& text);
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

std::string
dfm_text_from_binary(LPCWSTR pszDFMSC, const void *binary, size_t size,
                     INT codepage, BOOL bComments);

EntryBase::data_type
dfm_binary_from_text(LPCWSTR pszDFMSC, const std::string& text,
                     INT codepage, BOOL no_unicode);

std::string
tlb_text_from_binary(LPCWSTR pszTLB2IDL, const void *binary, size_t size);

EntryBase::data_type
tlb_binary_from_text(LPCWSTR pszTLB2IDL, const std::string& text);

///////////////////////////////////////////////////////////////////////////////
// EntrySet

// https://msdn.microsoft.com/ja-jp/library/windows/desktop/bb773793.aspx
// NOTE: It is not safe to delete items in response to a notification such as TVN_SELCHANGING.

struct EntryLess
{
    bool operator()(const EntryBase *e1, const EntryBase *e2) const
    {
        return *e1 < *e2;
    }
};

typedef std::set<EntryBase *, EntryLess> EntrySetBase;

struct EntrySet : protected EntrySetBase
{
    typedef EntrySetBase super_type;
    typedef EntrySet self_type;
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
    bool search(self_type& found, EntryType et, const MIdOrString& type = WORD(0), 
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
        self_type found;
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
    bool intersect(const EntrySet& another) const;

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
                   WORD lang, const EntryBase::data_type& data);

    // delete an entry (and related entries)
    void delete_entry(EntryBase *entry);

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
    void delete_invalid();

    // search and delete
    bool search_and_delete(EntryType et, const MIdOrString& type = WORD(0), 
                           const MIdOrString& name = WORD(0), WORD lang = BAD_LANG)
    {
        // search
        self_type found;
        search(found, et, type, name, lang);

        // delete
        bool ret = false;
        for (auto entry : found)
        {
            ret = true;
            delete_entry(entry);
        }
        return ret;
    }

    // get last ID of the specified type and language
    UINT get_last_id(const MIdOrString& type, WORD lang) const;

    // update the executable
    BOOL update_exe(LPCWSTR ExeFile) const;

    // helper method for MRadWindow and MTestDialog
    void do_bitmap(MTitleToBitmap& title_to_bitmap, DialogItem& item, WORD lang);

    // helper method for MRadWindow and MTestDialog
    void do_icon(MTitleToIcon& title_to_icon, DialogItem& item, WORD lang);

    // extract the cursor as a *.cur file
    bool extract_cursor(const EntryBase& c_entry, const wchar_t *file_name) const;

    // extract the group cursor as a *.cur file
    bool extract_group_cursor(const EntryBase& group, const wchar_t *file_name) const;

    // extract the icon as a *.ico file
    BOOL extract_icon(const EntryBase& i_entry, const wchar_t *file_name) const;

    // extract the group icon as a *.ico file
    bool extract_group_icon(const EntryBase& group, const wchar_t *file_name) const;

    // add a bitmap entry
    EntryBase *add_bitmap(const MIdOrString& name, WORD lang, const MStringW& file);

    // add a group icon
    EntryBase *
    add_group_icon(const MIdOrString& name, WORD lang, 
                   const MStringW& file_name);

    // add a group cursor
    EntryBase *
    add_group_cursor(const MIdOrString& name, WORD lang, 
                     const MStringW& file_name);

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
    HTREEITEM get_insert_parent(EntryBase *entry);

    // get the insertion position
    HTREEITEM get_insert_position(EntryBase *entry);

    // get the parent entry
    EntryBase *get_parent(EntryBase *entry);

    // is it a childless entry?
    bool is_childless_parent(EntryBase *entry) const;

    // get the label for a treeview item
    MStringW get_label(const EntryBase *entry);

    // helper method for entry insertion
    EntryBase *on_insert_entry(EntryBase *entry);

    EntryBase *
    on_insert_after(HTREEITEM hParent, EntryBase *entry, HTREEITEM hInsertAfter);

    // helper method to delete the dialog
    void on_delete_dialog(EntryBase *entry)
    {
        assert(entry->m_et == ET_LANG && entry->m_type == RT_DIALOG);
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
    bool on_delete_group_icon(EntryBase *entry);

    // helper method to delete the group cursor
    bool on_delete_group_cursor(EntryBase *entry);

    struct EnumResStruct
    {
        EntrySet *this_;
    };

    // add a resource entry from an executable module
    EntryBase *
    add_res_entry(HMODULE hMod, LPCWSTR type, LPCWSTR name, WORD lang);

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
    EntryBase *get_child(EntryBase *parent) const;

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
    BOOL copy_group_icon(EntryBase *entry, const MIdOrString& new_name, WORD new_lang);

    // copy the group cursor
    BOOL copy_group_cursor(EntryBase *entry, const MIdOrString& new_name, WORD new_lang);

    // extract one resource item as an *.res file
    BOOL extract_res(LPCWSTR pszFileName, const EntryBase *entry) const;

    // extract some resource items as an *.res file
    BOOL extract_res(LPCWSTR pszFileName, const EntrySet& res) const;

    // extract the cursor as an *.cur or *.ani file
    BOOL extract_cursor(LPCWSTR pszFileName, const EntryBase *entry) const;

    // extract the icon as an *.ico file
    BOOL extract_icon(LPCWSTR pszFileName, const EntryBase *entry) const;

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
    BOOL import_res(LPCWSTR pszResFile);

    // load the message table from a *.rc file
    BOOL load_msg_table(LPCWSTR pszRCFile, MStringA& strOutput, const MString& strMcdxExe,
                        const MStringW& strMacrosDump, const MStringW& strIncludesDump);

    BOOL IsUTF16File(LPCWSTR pszRCFile) const;

    // load the resources from a *.rc file
    BOOL load_rc(LPCWSTR pszRCFile, MStringA& strOutput,
        const MString& strWindresExe, const MString& strCppExe,
        const MString& strMcdxExe, const MStringW& strMacrosDump,
        const MStringW& strIncludesDump);
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
