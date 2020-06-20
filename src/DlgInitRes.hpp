// DlgInitRes.hpp --- DLGINIT Resource
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
    WCHAR sz0[64];
    WCHAR sz1[64];
    WCHAR sz2[256];
};

struct DlgInitEntry
{
    WORD        wCtrl;
    WORD        wMsg;
    MStringA    strText;

    DlgInitEntry()
    {
    }
    
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
    DlgInitRes() { }

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
