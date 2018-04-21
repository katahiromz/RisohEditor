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

#ifndef ACCEL_RES_HPP_
#define ACCEL_RES_HPP_

#include <windows.h>
#include <cassert>
#include <vector>

#include "MByteStreamEx.hpp"
#include "MString.hpp"
#include "ConstantsDB.hpp"

//////////////////////////////////////////////////////////////////////////////

struct ACCEL_ENTRY
{
    WCHAR sz0[128];
    WCHAR sz1[16];
    WCHAR sz2[128];
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
    const ConstantsDB& m_db;
    AccelRes(const ConstantsDB& db) : m_db(db) { }

    bool LoadFromStream(const MByteStreamEx& stream)
    {
        m_entries.clear();
        if (stream.size() < sizeof(entry_type))
            return false;

        entry_type entry;
        size_t i, count = stream.size() / sizeof(entry_type);
        for (i = 0; i < count; ++i)
        {
            if (!stream.ReadRaw(entry))
                return false;

            m_entries.push_back(entry);

            if (entry.fFlags & 0x80)
                break;
        }

        return true;
    }

    void Update()
    {
        size_t i, count = m_entries.size();
        for (i = 0; i < count; ++i)
        {
            entry_type& entry = m_entries[i];

            if (i + 1 == count)
                entry.fFlags |= 0x80;
            else
                entry.fFlags &= ~0x80;
        }
    }

    std::vector<BYTE> data() const
    {
        size_t size = m_entries.size() * sizeof(entry_type);
        const BYTE *pb = (const BYTE *)&m_entries[0];
        return std::vector<BYTE>(pb, pb + size);
    }

    MStringW Dump(const MIdOrString &id_or_str) const
    {
        MStringW ret;

        if (id_or_str.m_id == 0)
        {
            ret += id_or_str.str();
        }
        else
        {
            ret += m_db.GetNameOfResID(IDTYPE_ACCEL, id_or_str.m_id);
        }
        ret += L" ";
        ret += L"ACCELERATORS\r\n";
        ret += L"{\r\n";

        entries_type::const_iterator it, end = m_entries.end();
        for (it = m_entries.begin(); it != end; ++it)
        {
            bool VIRTKEY = !!(it->fFlags & FVIRTKEY);
            bool NOINVERT = !!(it->fFlags & FNOINVERT);
            bool SHIFT = !!(it->fFlags & FSHIFT);
            bool CONTROL = !!(it->fFlags & FCONTROL);
            bool ALT = !!(it->fFlags & FALT);

            ret += L"    ";
            if (VIRTKEY)
            {
                ret += m_db.GetName(L"VIRTUALKEYS", it->wAscii);
            }
            else
            {
                std::string str;
                str += (char)it->wAscii;
                ret += MAnsiToWide(CP_ACP, mstr_quote(str));
            }
            ret += L", ";
            if (0)
            {
                ret += mstr_dec_word(it->wId);
            }
            else
            {
                ret += m_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, it->wId);
            }

            if (NOINVERT)
                ret += L", NOINVERT";
            if (ALT)
                ret += L", ALT";
            if (CONTROL)
                ret += L", CONTROL";
            if (SHIFT)
                ret += L", SHIFT";

            if (VIRTKEY)
                ret += L", VIRTKEY";
            else
                ret += L", ASCII";

            ret += L"\r\n";
        }

        ret += L"}\r\n";
        return ret;
    }

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
    if (str.find(L"V") != MStringW::npos)
        fFlags |= FVIRTKEY;
    if (str.find(L"N") != MStringW::npos)
        fFlags |= FNOINVERT;
    if (str.find(L"C") != MStringW::npos)
        fFlags |= FCONTROL;
    if (str.find(L"S") != MStringW::npos)
        fFlags |= FSHIFT;
    if (str.find(L"A") != MStringW::npos)
        fFlags |= FALT;
}

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef ACCEL_RES_HPP_
