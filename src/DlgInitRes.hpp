// DlgInitRes.hpp --- DLGINIT Resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#ifndef _INC_WINDOWS
	#include <windows.h>
#endif
#include <cassert>
#include <vector>

#include "MByteStreamEx.hpp"
#include "MString.hpp"
#include "DlgInit.h"

//////////////////////////////////////////////////////////////////////////////

struct DLGINIT_ENTRY
{
	MStringW sz0;
	MStringW sz1;
	MStringW sz2;
};

struct DlgInitEntry
{
	WORD        wCtrl;
	WORD        wMsg;
	MStringA    strText;

	DlgInitEntry() = default;

	DlgInitEntry(WORD ctrl, WORD msg, const MStringA& str)
		: wCtrl(ctrl), wMsg(msg), strText(str)
	{
	}
};

//////////////////////////////////////////////////////////////////////////////

class DlgInitRes
{
public:
	typedef DlgInitEntry                entry_type;
	typedef std::vector<entry_type>     entries_type;

protected:
	entries_type    m_entries;

public:
	DlgInitRes() = default;

	bool LoadFromStream(const MByteStreamEx& stream);
	bool SaveToStream(MByteStreamEx& stream) const;
	MStringW Dump(const MIdOrString& id_or_str) const;

	entries_type& entries()
	{
		return m_entries;
	}
	const entries_type& entries() const
	{
		return m_entries;
	}

	bool empty() const
	{
		return size() == 0;
	}
	size_t size() const
	{
		return m_entries.size();
	}
	entry_type& operator[](size_t i)
	{
		return m_entries[i];
	}
	const entry_type& operator[](size_t i) const
	{
		return m_entries[i];
	}
	void push_back(const DlgInitEntry& entry)
	{
		m_entries.push_back(entry);
	}
	void clear()
	{
		m_entries.clear();
	}

	std::vector<BYTE> data() const;
};
