// StringRes.hpp --- String Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "MByteStreamEx.hpp"
#include "MString.hpp"
#include <map>

//////////////////////////////////////////////////////////////////////////////

// A STRINGTABLE resource block always packs this many consecutive
// string IDs together (IDs [ (name-1)*16, (name-1)*16 + 15 ]).
#define STRINGRES_BLOCK_SIZE 16

struct STRING_ENTRY
{
	WCHAR StringID[128];
	WCHAR StringValue[512];
};

class StringRes
{
public:
	typedef MStringW string_type;
	typedef std::map<WORD, string_type> map_type;
	map_type    m_map;

	StringRes()
	{
	}

	bool LoadFromStream(const MByteStreamEx& stream, WORD wName);
	bool SaveToStream(MByteStreamEx& stream, WORD wName) const;

	string_type Dump(WORD wName) const;
	string_type Dump() const;

	map_type& map()
	{
		return m_map;
	}
	const map_type& map() const
	{
		return m_map;
	}

	void Clear()
	{
		m_map.clear();
	}

	bool IsEmpty() const
	{
		return m_map.empty();
	}

	void IdRangeFromName(WORD name, WORD& first, WORD& last) const
	{
		first = (name - 1) * STRINGRES_BLOCK_SIZE;
		last = first + STRINGRES_BLOCK_SIZE - 1;
	}

	WORD NameFromId(WORD id) const
	{
		return (id / STRINGRES_BLOCK_SIZE) + 1;
	}

	bool HasAnyValues() const
	{
		for (auto& pair : m_map)
		{
			if (pair.second.size())
				return true;
		}
		return false;
	}

	// Only scans the entries that actually belong to this block instead of
	// the whole map, using the fact that m_map is ordered by ID.
	bool HasAnyValues(WORD name) const
	{
		WORD first, last;
		IdRangeFromName(name, first, last);

		auto itEnd = m_map.upper_bound(last);
		for (auto it = m_map.lower_bound(first); it != itEnd; ++it)
		{
			if (it->second.size())
				return true;
		}
		return false;
	}

private:
	string_type DumpRange(map_type::const_iterator itBegin, map_type::const_iterator itEnd) const;
};
