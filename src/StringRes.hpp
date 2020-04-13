// StringRes.hpp --- String Resources
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

#ifndef STRING_RES_HPP_
#define STRING_RES_HPP_

#include "MByteStreamEx.hpp"
#include "MString.hpp"
#include "ConstantsDB.hpp"
#include <map>

//////////////////////////////////////////////////////////////////////////////

struct STRING_ENTRY
{
    WCHAR StringID[128];
    WCHAR StringValue[512];
};

//////////////////////////////////////////////////////////////////////////////

class StringRes
{
public:
    typedef MStringW string_type;
    typedef std::map<WORD, string_type> map_type;
    WORD        m_wName;
    map_type    m_map;

    StringRes()
    {
    }

    bool LoadFromStream(const MByteStreamEx& stream, WORD wName)
    {
        for (UINT i = 0; i < 16; ++i)
        {
            m_map.erase((wName - 1) * 16 + i);
        }

        WORD wLen;
        for (UINT i = 0; i < 16; ++i)
        {
            if (!stream.ReadWord(wLen))
                break;

            if (wLen > 0)
            {
                string_type str(wLen, 0);
                if (!stream.ReadData(&str[0], wLen * sizeof(WCHAR)))
                    break;

                m_map[(wName - 1) * 16 + i] = str;
            }
        }

        return true;
    }

    bool SaveToStream(MByteStreamEx& stream, WORD wName)
    {
        WORD first, last;
        GetIdRange(wName, first, last);

        for (UINT i = first; i <= last; ++i)
        {
            const string_type& str = m_map[i];
            WORD wLen = WORD(str.size());
            if (!stream.WriteWord(wLen) ||
                !stream.WriteData(&str[0], wLen * sizeof(WCHAR)))
                return false;
        }

        return true;
    }

    string_type Dump(WORD wName)
    {
        string_type ret;

        ret += L"STRINGTABLE\r\n";
        if (g_settings.bUseBeginEnd)
            ret += L"BEGIN\r\n";
        else
            ret += L"{\r\n";

        WORD first, last;
        GetIdRange(wName, first, last);
        for (UINT i = first; i <= last; ++i)
        {
            if (m_map[i].empty())
                continue;

            ret += L"    ";
            if (0)
            {
                ret += mstr_dec_word(i);
            }
            else
            {
                ret += g_db.GetNameOfResID(IDTYPE_STRING, IDTYPE_PROMPT, i);
            }

            ret += L", \"";
            ret += mstr_escape(m_map[i]);
            ret += L"\"\r\n";
        }
        if (g_settings.bUseBeginEnd)
            ret += L"END\r\n";
        else
            ret += L"}\r\n";

        return ret;
    }

    string_type Dump()
    {
        string_type ret;

        ret += L"STRINGTABLE\r\n";
        if (g_settings.bUseBeginEnd)
            ret += L"BEGIN\r\n";
        else
            ret += L"{\r\n";

        for (auto& pair : m_map)
        {
            if (pair.second.empty())
                continue;

            ret += L"    ";
            if (0)
            {
                ret += mstr_dec_word(pair.first);
            }
            else
            {
                ret += g_db.GetNameOfResID(IDTYPE_STRING, IDTYPE_PROMPT, pair.first);
            }

            ret += L", \"";
            ret += mstr_escape(pair.second);
            ret += L"\"\r\n";
        }

        if (g_settings.bUseBeginEnd)
            ret += L"END\r\n";
        else
            ret += L"}\r\n";

        return ret;
    }

    map_type& map()
    {
        return m_map;
    }
    const map_type& map() const
    {
        return m_map;
    }

    void GetIdRange(WORD name, WORD& first, WORD& last) const
    {
        first = (name - 1) * 16;
        last = first + 16 - 1;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef STRING_RES_HPP_
