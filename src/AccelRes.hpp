// AccelRes.hpp --- Accelerator-Table Resource
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

//////////////////////////////////////////////////////////////////////////////

struct ACCEL_ENTRY
{
    MStringW sz0;
    MStringW sz1;
    MStringW sz2;
};

struct AccelTableEntry
{
    WORD   fFlags;
    WORD   wAscii;
    WORD   wId;
    WORD   padding;
};

//////////////////////////////////////////////////////////////////////////////

class AccelRes
{
public:
    typedef AccelTableEntry             entry_type;
    typedef std::vector<entry_type>     entries_type;
    AccelRes() = default;

    bool LoadFromStream(const MByteStreamEx& stream);
    MStringW Dump(const MIdOrString &id_or_str) const;
    void Update();

    std::vector<BYTE> data() const;

    entries_type& entries()
    {
        return m_entries;
    }
    const entries_type& entries() const
    {
        return m_entries;
    }

protected:
    entries_type    m_entries;
};

//////////////////////////////////////////////////////////////////////////////

inline MStringW GetKeyFlags(WORD fFlags)
{
    MStringW str;

    if (fFlags & FVIRTKEY)
        str += L"V ";

    if (fFlags & FNOINVERT)
        str += L"N ";

    if (fFlags & FCONTROL)
        str += L"C ";

    if (fFlags & FSHIFT)
        str += L"S ";

    if (fFlags & FALT)
        str += L"A ";

    return str;
}

inline void SetKeyFlags(WORD& fFlags, const MStringW& str)
{
    fFlags = 0;
    if (str.find(L'V') != MStringW::npos)
        fFlags |= FVIRTKEY;
    if (str.find(L'N') != MStringW::npos)
        fFlags |= FNOINVERT;
    if (str.find(L'C') != MStringW::npos)
        fFlags |= FCONTROL;
    if (str.find(L'S') != MStringW::npos)
        fFlags |= FSHIFT;
    if (str.find(L'A') != MStringW::npos)
        fFlags |= FALT;
}
