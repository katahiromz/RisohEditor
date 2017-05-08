#ifndef STRING_RES_HPP_
#define STRING_RES_HPP_

#include <windows.h>
#include <map>

#include "Text.hpp"

class StringRes
{
public:
    typedef std::wstring    string_type;
    typedef std::map<WORD, string_type> map_type;

    StringRes()
    {
    }

    BOOL LoadFromStream(const ByteStream& stream, WORD wName)
    {
        for (WORD i = 0; i < 16; ++i)
        {
            m_map[(wName - 1) * 16 + i].clear();
        }

        WORD wLen;
        for (WORD i = 0; i < 16; ++i)
        {
            if (!stream.ReadWord(wLen))
                break;

            if (wLen > 0)
            {
                string_type str(wLen, UNICODE_NULL);
                if (!stream.ReadData(&str[0], wLen * sizeof(WCHAR)))
                    break;

                m_map[(wName - 1) * 16 + i] = str;
            }
        }

        return TRUE;
    }

    BOOL CreateSample()
    {
        WORD first, last, wName = 1;
        GetRange(wName, first, last);

        m_wName = wName;
        m_map[wName] = LoadStringDx(IDS_SAMPLETEXT);
        return TRUE;
    }

    BOOL SaveToStream(ByteStream& stream, WORD wName)
    {
        WORD first, last;
        GetRange(wName, first, last);

        for (WORD i = first; i <= last; ++i)
        {
            const string_type& str = m_map[i];
            WORD wLen = WORD(str.size());
            if (!stream.WriteWord(wLen) ||
                !stream.WriteData(&str[0], wLen * sizeof(WCHAR)))
                return FALSE;
        }

        return TRUE;
    }

    string_type Dump(WORD wName)
    {
        string_type ret;

        ret += L"STRINGTABLE\r\n";
        ret += L"{\r\n";

        WORD first, last;
        GetRange(wName, first, last);
        for (WORD i = first; i <= last; ++i)
        {
            if (m_map[i].empty())
                continue;

            ret += L"    ";
            ret += deci(i);

            ret += L", \"";
            ret += escape(m_map[i]);
            ret += L"\"\r\n";
        }
        ret += L"}\r\n";

        return ret;
    }

    string_type Dump()
    {
        string_type ret;

        ret += L"STRINGTABLE\r\n";
        ret += L"{\r\n";

        map_type::iterator it, end = m_map.end();
        for (it = m_map.begin(); it != end; ++it)
        {
            if (it->second.empty())
                continue;

            ret += L"    ";
            ret += deci(it->first);

            ret += L", \"";
            ret += escape(it->second);
            ret += L"\"\r\n";
        }

        ret += L"}\r\n";

        return ret;
    }

protected:
    WORD        m_wName;
    map_type    m_map;

    void GetRange(WORD Name, WORD& first, WORD& last) const
    {
        first = (Name - 1) * 16;
        last = first + 16 - 1;
    }
};

#endif  // ndef STRING_RES_HPP_
