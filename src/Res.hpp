// Res.hpp --- Win32 Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include <windows.h>
#include <commctrl.h>
#include <cassert>
#include <cctype>
#include <cwchar>
#include <set>
#include <map>
#include <memory>            // for std::shared_ptr
#include <shlwapi.h>

#include "IconRes.hpp"
#include "MString.hpp"
#include "MProcessMaker.hpp"
#include "PackedDIB.hpp"
#include "MBitmapDx.hpp"
#include "ConstantsDB.hpp"
#include "DialogRes.hpp"
#include "ResHeader.hpp"
#include "ToolbarRes.hpp"
#include "WonResWrap.h"

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

#define MYWM_TREEVIEWISEMPTY (WM_USER + 555)

///////////////////////////////////////////////////////////////////////////////

// has the resource type no name?
inline BOOL
Res_HasNoName(const MIdOrString& type)
{
	return type == RT_STRING;
}

MStringW get_type_label(const MIdOrString& type);
MStringW get_name_label(const MIdOrString& type, const MIdOrString& name);
MStringW get_lang_label(LANGID lang);

///////////////////////////////////////////////////////////////////////////////
// EntryType --- the entry type

enum EntryType
{
	ET_ANY,         // Any.
	ET_TYPE,        // TypeEntry.
	ET_STRING,      // StringEntry.
	ET_NAME,        // NameEntry.
	ET_LANG         // EntryBase.
};

///////////////////////////////////////////////////////////////////////////////
// EntryBase

// NOTE: These values work like a wildcard for EntryBase search.
#define BAD_TYPE    L"*"        // invalid type
#define BAD_NAME    L"*"        // invalid name
#define BAD_LANG    0xFFFF      // invalid language value

struct EntryBase;

// The lifetime of every EntryBase is now managed by std::shared_ptr rather
// than by manual new/delete. EntryBase objects are only ever created via
// the Res_New*Entry() factory functions below (which hand back an
// EntryPtr), and the *only* owner of that EntryPtr is the EntrySet that
// inserted it (see EntrySet::m_owned, take_ownership(), release_ownership()
// in this header, and EntrySet::delete_invalid() in Res.cpp).
//
// Everywhere else in the codebase (RisohEditor.cpp, MMainWnd*.cpp, etc.)
// continues to see and pass around plain, non-owning EntryBase* -- exactly
// as before -- because that raw pointer is just a view into the object
// that the owning EntrySet keeps alive via its shared_ptr. This keeps the
// public API (find(), get_entry(), get_parent(), on_insert_entry(), ...)
// unchanged so no caller elsewhere needs to be touched; only the
// allocation/deallocation plumbing inside Res.hpp/Res.cpp changed.
typedef std::shared_ptr<EntryBase> EntryPtr;

struct EntryBaseBase
{
#ifndef NDEBUG
	static LONG s_alive_count;
	static bool is_alive_zero() { return s_alive_count == 0; }
	EntryBaseBase() { InterlockedIncrement(&s_alive_count); }
	EntryBaseBase(const EntryBaseBase&) { InterlockedIncrement(&s_alive_count); }
	EntryBaseBase(EntryBaseBase&&) { InterlockedIncrement(&s_alive_count); }
	EntryBaseBase& operator=(const EntryBaseBase&) = default;
	EntryBaseBase& operator=(EntryBaseBase&&) = default;
	virtual ~EntryBaseBase()
	{
		assert(s_alive_count > 0);
		InterlockedDecrement(&s_alive_count);
	}
#endif
};

struct EntryBase : EntryBaseBase
{
	typedef DWORD               size_type;
	typedef std::vector<BYTE>   data_type;

	EntryType       m_et;                   // entry type
	MIdOrString     m_type;                 // resource type
	MIdOrString     m_name;                 // resource name
	LANGID          m_lang;                 // resource language
	HTREEITEM       m_hItem;                // treeview item handle
	bool            m_valid;                // "is it valid?" flag
	data_type       m_data;                 // the item data
	MStringW        m_strLabel;             // the label string

	// constructor
	EntryBase()
		: m_et(ET_ANY)
		, m_lang(BAD_LANG)
		, m_hItem(nullptr)
		, m_valid(true)
	{
	}

	// constructor
	EntryBase(EntryType et, const MIdOrString& type, const MIdOrString& name = BAD_NAME, LANGID lang = BAD_LANG)
		: m_et(et)
		, m_type(type)
		, m_name(name)
		, m_lang(lang)
		, m_hItem(nullptr)
		, m_valid(true)
	{
		if (m_name == BAD_NAME)
			m_name.clear();
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
	void mark_invalid();

	// can it be editted by GUI?
	bool can_gui_edit() const
	{
		if (m_type == RT_DIALOG || m_type == RT_MENU ||
			m_type == RT_STRING ||
			m_type == RT_ACCELERATOR || m_type == WORD(240))
		{
			return true;
		}
		if (m_type == RT_TOOLBAR)
			return true;
		if (m_type == RT_MESSAGETABLE)
			return !g_settings.bUseMSMSGTABLE;
		return false;
	}

	// is it testable?
	bool is_testable() const
	{
		return m_type == RT_DIALOG || m_type == RT_MENU;
	}

	// pattern match
	bool match(EntryType et, const MIdOrString& type, const MIdOrString& name = BAD_NAME, LANGID lang = BAD_LANG) const
	{
		if (et != ET_ANY && m_et != et)
			return false;
		if (type != BAD_TYPE && m_type != type)
			return false;
		if (name != BAD_NAME && m_name != name)
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
	MStringW get_type_label() const;
	// get the resource name label
	MStringW get_name_label() const;
	// get the resource language label
	MStringW get_lang_label() const;

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
		m_type = BAD_TYPE;
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
		assert(index < m_data.size());
		return m_data[index];
	}
	const BYTE& operator[](DWORD index) const
	{
		assert(index < m_data.size());
		return m_data[index];
	}

	// the pointer to data
	void *ptr(DWORD index = 0)
	{
		if (index >= m_data.size())
			return nullptr;
		return &m_data[index];
	}
	const void *ptr(DWORD index = 0) const
	{
		if (index >= m_data.size())
			return nullptr;
		return &m_data[index];
	}
	std::string to_string() const
	{
		return std::string((const char *)ptr(), size());
	}
	std::wstring to_wstring() const
	{
		return std::wstring((const wchar_t *)ptr(), size() / sizeof(WCHAR));
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
	BOOL is_editable(LPCWSTR pszVCBat) const;

	BOOL is_delphi_dfm() const
	{
		return m_type == RT_RCDATA && size() >= 4 && memcmp(ptr(), "TPF0", 4) == 0;
	}

	std::string get_dfm_text(LPCWSTR pszDFMSC) const;
	void set_dfm_text(LPCWSTR pszDFMSC, std::string& text);
};

inline EntryPtr
Res_NewTypeEntry(const MIdOrString& type)
{
	return std::make_shared<EntryBase>(ET_TYPE, type);
}

inline EntryPtr
Res_NewNameEntry(const MIdOrString& type, const MIdOrString& name)
{
	return std::make_shared<EntryBase>(ET_NAME, type, name);
}

inline EntryPtr
Res_NewStringEntry(LANGID lang)
{
	return std::make_shared<EntryBase>(ET_STRING, RT_STRING, BAD_NAME, lang);
}

inline EntryPtr
Res_NewLangEntry(const MIdOrString& type, const MIdOrString& name, LANGID lang = BAD_LANG)
{
	return std::make_shared<EntryBase>(ET_LANG, type, name, lang);
}

// NOTE: dfm_text_from_binary, dfm_binary_from_text, tlb_text_from_binary,
// and tlb_binary_from_text used to live here as free functions. They were
// only ever called from within MMainWnd (using its m_szDFMSC/m_szOleBow/
// m_szMidlWrap/m_szVCBat tool paths), so they have been moved there and
// renamed to MMainWnd::DoDfmTextFromBinary, MMainWnd::DoDfmBinaryFromText,
// MMainWnd::DoTlbTextFromBinary, and MMainWnd::DoTlbBinaryFromText.
// See RisohEditor.cpp.

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

	// True while delete_all() is wiping the whole set.  TVN_DELETEITEM still
	// arrives for every tree item, but on_delete_item() must not run the
	// per-entry cascade (group-icon/cursor related deletes, parent pruning,
	// ...) - that would be O(n^2) and is pointless when everything is going
	// away.  After the tree is empty, ownership is released in bulk via
	// m_owned.clear(), which destroys every EntryBase exactly once.
	bool m_bDeletingAll = false;

	// The entries actually created by (and thus owned by) this EntrySet,
	// keyed by their raw pointer identity so lookups/erasure are O(log n)
	// and unaffected by later mutation of an entry's content (m_type,
	// m_name, ... can change after insertion, which rules out ordering
	// this map with EntryLess). An EntrySet built as a search-result view
	// (e.g. the `self_type found` locals in search()/search_and_delete(),
	// or copies made via the super_type-only constructor below) never
	// populates this map, so it never owns -- and never destroys -- the
	// entries it merely references, exactly like the old raw-pointer code.
	std::map<EntryBase *, EntryPtr> m_owned;

	// take ownership of a freshly created entry; returns the raw,
	// non-owning pointer that the rest of the codebase keeps using
	EntryBase *take_ownership(const EntryPtr& entry)
	{
		EntryBase *raw = entry.get();
		m_owned[raw] = entry;
		return raw;
	}

	// give up ownership of (and, if this was the last reference, destroy)
	// an entry previously returned by take_ownership()
	void release_ownership(EntryBase *entry)
	{
		m_owned.erase(entry);
	}

	// Look up the EntryPtr backing a raw, non-owning EntryBase* that the
	// caller already has (e.g. from find()/get_entry()/get_parent()).
	// Use this when you need to *hold on to* an entry across something
	// that isn't a single, immediate call -- a modal dialog's lifetime,
	// a queued/async operation, a background thread -- so the entry
	// can't be destroyed out from under you while you're holding it.
	// Returns an empty EntryPtr if this EntrySet doesn't own `entry`
	// (e.g. it belongs to a different EntrySet instance, or it has
	// already been deleted).
	EntryPtr get_shared(EntryBase *entry) const
	{
		auto it = m_owned.find(entry);
		if (it == m_owned.end())
			return EntryPtr();
		return it->second;
	}

	// constructor
	EntrySet(HWND hwndTV = nullptr) : m_hwndTV(hwndTV)
	{
	}

	// constructor
	EntrySet(const super_type& super, HWND hwndTV = nullptr)
		: super_type(super), m_hwndTV(hwndTV)
	{
	}

	// the super class pointer
	super_type *super()
	{
		return static_cast<super_type*>(this);
	}
	const super_type *super() const
	{
		return static_cast<const super_type*>(this);
	}

	// search by pattern matching
	bool search(self_type& found, EntryType et, const MIdOrString& type = BAD_TYPE,
				const MIdOrString& name = BAD_NAME, LANGID lang = BAD_LANG, bool invalid_ok = false) const
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
	//
	// NOTE: *this is already ordered by EntryLess, so the first matching
	// entry encountered while iterating in order is exactly the same
	// entry that "build a found-set, then take *found.begin()" would have
	// returned (found is ordered by the same EntryLess). We therefore
	// short-circuit on the first match instead of collecting every match
	// into a temporary self_type just to throw all but one of them away.
	// This avoids O(log n) set-insert overhead per candidate and lets the
	// common case (an early match) return without scanning the whole set.
	EntryBase *find(EntryType et, const MIdOrString& type = BAD_TYPE, const MIdOrString& name = BAD_NAME,
					LANGID lang = BAD_LANG, bool invalid_ok = false) const
	{
		for (auto entry : *this)
		{
			if (!entry->valid() && !invalid_ok)
				continue;
			if (entry->match(et, type, name, lang))
				return entry;
		}
		return nullptr;
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
	add_lang_entry(const MIdOrString& type, const MIdOrString& name, LANGID lang)
	{
		EntryBase::data_type data;
		return add_lang_entry(type, name, lang, data);
	}
	EntryBase *
	add_lang_entry(const MIdOrString& type, const MIdOrString& name,
				   LANGID lang, const EntryBase::data_type& data);

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
	bool delete_invalid();

	// search and delete
	bool search_and_delete(EntryType et, const MIdOrString& type = BAD_TYPE, const MIdOrString& name = BAD_NAME, LANGID lang = BAD_LANG)
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
	UINT get_last_id(const MIdOrString& type, LANGID lang) const;

	// update the executable
	BOOL update_exe(LPCWSTR ExeFile, BOOL bEnableCrypt) const;

	// helper method for MRadWindow and MTestDialog
	void do_bitmap(MTitleToBitmap& title_to_bitmap, DialogItem& item, LANGID lang);

	// helper method for MRadWindow and MTestDialog
	void do_icon(MTitleToIcon& title_to_icon, DialogItem& item, LANGID lang);

	// extract the cursor as a *.cur file
	bool extract_cursor(const EntryBase& c_entry, const wchar_t *file_name) const;

	// extract the group cursor as a *.cur file
	bool extract_group_cursor(const EntryBase& group, const wchar_t *file_name) const;

	// extract the icon as a *.ico file
	BOOL extract_icon(const EntryBase& i_entry, const wchar_t *file_name) const;

	// extract the group icon as a *.ico file
	bool extract_group_icon(const EntryBase& group, const wchar_t *file_name) const;

	// add a bitmap entry
	EntryBase *add_bitmap(const MIdOrString& name, LANGID lang, const MStringW& file);

	// add a group icon
	EntryBase *
	add_group_icon(const MIdOrString& name, LANGID lang, const MStringW& file_name);

	// add a group cursor
	EntryBase *
	add_group_cursor(const MIdOrString& name, LANGID lang, const MStringW& file_name);

	// add a string entry
	EntryBase *
	add_string_entry(LANGID lang)
	{
		auto entry = find(ET_STRING, RT_STRING, BAD_NAME, lang, true);
		if (!entry)
			entry = take_ownership(Res_NewStringEntry(lang));
		return on_insert_entry(entry);
	}

	// add a name entry
	EntryBase *
	add_name_entry(const MIdOrString& type, const MIdOrString& name)
	{
		auto entry = find(ET_NAME, type, name, BAD_LANG, true);
		if (!entry)
			entry = take_ownership(Res_NewNameEntry(type, name));
		return on_insert_entry(entry);
	}

	// add a type entry
	EntryBase *
	add_type_entry(const MIdOrString& type)
	{
		auto entry = find(ET_TYPE, type, BAD_NAME, BAD_LANG, true);
		if (!entry)
			entry = take_ownership(Res_NewTypeEntry(type));
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
		search_and_delete(ET_LANG, RT_STRING, BAD_NAME, entry->m_lang);
	}

	// helper method to delete the group icon
	bool on_delete_group_icon(EntryBase *entry);

	// helper method to delete the group cursor
	bool on_delete_group_cursor(EntryBase *entry);

	// add a resource entry from an executable module
	EntryBase *
	add_res_entry(HMODULE hMod, LPCWSTR type, LPCWSTR name, LANGID lang);

	// get the child if any
	EntryBase *get_child(EntryBase *parent) const;

public:
	// add the resources in the executable module
	BOOL from_res(HMODULE hMod);

	BOOL is_protected(HMODULE hMod);

	// delete all the entries
	void delete_all(void);

	// for TVN_DELETEITEM
	void on_delete_item(EntryBase *entry)
	{
		if (!entry)
			return;

		// Bulk clear: just detach the tree handle; ownership is released
		// by delete_all() after TreeView_DeleteAllItems returns.
		if (m_bDeletingAll)
		{
			entry->m_hItem = nullptr;
			return;
		}

		if (super()->find(entry) == super()->end())
			return;

		//MTRACEW(L"on_delete_item: %p, %s, %s, %u, %s\n", entry, entry->m_type.c_str(), entry->m_name.c_str(), entry->m_lang, entry->m_strLabel.c_str());
		entry->m_hItem = nullptr;
		delete_entry(entry);
	}

	// get the LPARAM parameter of the currently selected or the specified handle
	LPARAM get_param(HTREEITEM hItem = nullptr) const
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
	EntryBase *get_entry(HTREEITEM hItem = nullptr, EntryType et = ET_ANY) const
	{
		LPARAM lParam = get_param(hItem);
		if (!lParam)
			return nullptr;
		auto e = (EntryBase *)lParam;
		if (et != ET_ANY && et != e->m_et)
			return nullptr;
		return e;
	}

	// get a language entry of the currently selected or the specified handle
	EntryBase *get_lang_entry(HTREEITEM hItem = nullptr) const
	{
		return get_entry(hItem, ET_LANG);
	}

	// get the selected item handle of treeview
	HTREEITEM get_item(void) const
	{
		return TreeView_GetSelection(m_hwndTV);
	}

	// copy the group icon
	BOOL copy_group_icon(EntryBase *entry, const MIdOrString& new_name, LANGID new_lang);

	// copy the group cursor
	BOOL copy_group_cursor(EntryBase *entry, const MIdOrString& new_name, LANGID new_lang);

	// extract one resource item as an *.res file
	BOOL extract_res(LPCWSTR pszFileName, const EntryBase *entry) const;

	// extract some resource items as an *.res file
	BOOL extract_res(LPCWSTR pszFileName, const EntrySet& res) const;

	// extract the cursor as an *.cur or *.ani file
	BOOL extract_cursor(LPCWSTR pszFileName, const EntryBase *entry) const;

	// extract the icon as an *.ico file
	BOOL extract_icon(LPCWSTR pszFileName, const EntryBase *entry) const;

	// extract the resource data as a binary file
	BOOL extract_bin(LPCWSTR pszFileName, const EntryBase *e) const;

	// import the resource data from the specified *.res file
	BOOL import_res(LPCWSTR pszResFile);

	// NOTE: load_msg_table and load_rc used to live here. They were only
	// ever called on a caller-owned EntrySet (never *g_res* directly) from
	// within MMainWnd, using its m_szWindresExe/m_szMCppExe/m_szMcdxExe
	// tool paths, so they have been moved there and renamed to
	// MMainWnd::DoCompileMsgTable and MMainWnd::DoCompileRC (which now
	// takes an EntrySet& out-parameter instead of being a method on it).
	// See RisohEditor.cpp.

	void add_default_TEXTINCLUDE();
};

// g_res
extern EntrySet g_res;
