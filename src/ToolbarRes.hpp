// ToolbarRes.hpp --- TOOLBAR Resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#ifndef _INC_WINDOWS
	#include <windows.h>
#endif
#include <cassert>
#include <vector>

#include "MByteStreamEx.hpp"
#include "MString.hpp"

#ifndef RT_TOOLBAR
	#define RT_TOOLBAR  MAKEINTRESOURCE(241)
#endif

//////////////////////////////////////////////////////////////////////////////

class ToolbarRes
{
protected:
	DWORD m_width;
	DWORD m_height;
	std::vector<DWORD> m_items;

public:
	ToolbarRes() = default;

	bool LoadFromStream(const MByteStreamEx& stream);
	bool SaveToStream(MByteStreamEx& stream) const;
	MStringW Dump(const MIdOrString& id_or_str) const;

	INT width() const
	{
		return m_width;
	}
	void width(INT cx)
	{
		m_width = cx;
	}
	INT height() const
	{
		return m_height;
	}
	void height(INT cy)
	{
		m_height = cy;
	}

	bool empty() const
	{
		return size() == 0;
	}
	size_t size() const
	{
		return m_items.size();
	}
	DWORD& operator[](size_t i)
	{
		return m_items[i];
	}
	const DWORD& operator[](size_t i) const
	{
		return m_items[i];
	}
	void push_back(DWORD entry)
	{
		m_items.push_back(entry);
	}
	void clear()
	{
		m_items.clear();
	}

	std::vector<BYTE> data() const;
};
