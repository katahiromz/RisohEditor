// MIdOrString.hpp --- ID and String
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

#ifndef MIDORSTRING_HPP_
#define MIDORSTRING_HPP_

#include "MString.hpp"

#ifndef IS_INTRESOURCE
    #define IS_INTRESOURCE(_r) (((ULONG_PTR)(_r) >> 16) == 0)
#endif

#ifndef MAKEINTRESOURCEA
    #define MAKEINTRESOURCEA(i) (char *)((ULONG_PTR)((WORD)(i)))
    #define MAKEINTRESOURCEW(i) (WCHAR *)((ULONG_PTR)((WORD)(i)))
    #ifdef UNICODE
        #define MAKEINTRESOURCE  MAKEINTRESOURCEW
    #else
        #define MAKEINTRESOURCE  MAKEINTRESOURCEA
    #endif
#endif

//////////////////////////////////////////////////////////////////////////////

struct MIdOrString;

MString mstr_dec_short(SHORT value);
MString mstr_dec_word(WORD value);
MString mstr_dec_dword(DWORD value);
MString mstr_dec(int value);
MString mstr_hex(int value);
MString mstr_hex_word(WORD value);
bool mstr_unquote(std::string& str);
bool mstr_unquote(MStringW& str);
template <size_t siz>
bool mstr_unquote(char (&str)[siz]);
template <size_t siz>
bool mstr_unquote(WCHAR (&str)[siz]);
bool guts_escape(std::string& str, const char*& pch);
bool guts_escape(MStringW& str, const WCHAR*& pch);
bool guts_quote(std::string& str, const char*& pch);
bool guts_quote(MStringW& str, const WCHAR*& pch);

template <typename T_CHAR>
size_t
mstr_repeat_count(const std::basic_string<T_CHAR>& str1, const std::basic_string<T_CHAR>& str2);

//////////////////////////////////////////////////////////////////////////////

struct MIdOrString
{
    WORD m_id;
    MString m_str;

    MIdOrString() : m_id(0)
    {
    }

    MIdOrString(WORD ID) : m_id(ID)
    {
    }

    MIdOrString(const TCHAR *str)
    {
        if (IS_INTRESOURCE(str))
        {
            m_id = LOWORD(str);
        }
        else if ((L'0' <= str[0] && str[0] <= L'9') ||
                 str[0] == L'-' || str[0] == L'+')
        {
            m_id = (WORD)mstr_parse_int(str);
        }
        else
        {
            m_id = 0;
            m_str = str;
        }
    }

    const TCHAR *ptr() const
    {
        if (m_id)
            return MAKEINTRESOURCE(m_id);
        return m_str.c_str();
    }

    bool is_zero() const
    {
        return m_id == 0 && m_str.empty();
    }

    bool is_null() const
    {
        return is_zero();
    }

    bool empty() const
    {
        return is_zero();
    }

    bool is_str() const
    {
        return (!m_id && !m_str.empty());
    }

    bool is_int() const
    {
        return !is_str();
    }

    void clear()
    {
        m_id = 0;
        m_str.clear();
    }

    MIdOrString& operator=(WORD ID)
    {
        m_id = ID;
        m_str.clear();
        return *this;
    }

    MIdOrString& operator=(const TCHAR *str)
    {
        if (IS_INTRESOURCE(str))
        {
            m_id = LOWORD(str);
            m_str.clear();
        }
        else
        {
            m_id = 0;
            m_str = str;
        }
        return *this;
    }

    bool operator==(const MIdOrString& id_or_str) const
    {
        if (id_or_str.m_id != 0)
        {
            if (m_id != 0)
                return id_or_str.m_id == m_id;
        }
        else
        {
            if (m_id == 0)
                return m_str == id_or_str.m_str;
        }
        return false;
    }
    bool operator<(const MIdOrString& id_or_str) const
    {
        if (id_or_str.m_id != 0)
        {
            if (m_id != 0)
                return m_id < id_or_str.m_id;
            return false;
        }
        else
        {
            if (m_id == 0)
                return m_str < id_or_str.m_str;
            return true;
        }
    }
    bool operator>(const MIdOrString& id_or_str) const
    {
        return !(*this < id_or_str) && !(*this == id_or_str);
    }

    bool operator==(const TCHAR *psz) const
    {
        if (IS_INTRESOURCE(psz))
        {
            if (m_id != 0)
                return LOWORD(psz) == m_id;
        }
        else
        {
            if (m_id == 0)
                return m_str == psz;
        }
        return false;
    }
    bool operator==(const MStringW& str) const
    {
        return *this == str.c_str();
    }
    bool operator==(WORD w) const
    {
        return m_id == w;
    }

    bool operator!=(const MIdOrString& id_or_str) const
    {
        return !(*this == id_or_str);
    }
    bool operator!=(const WCHAR *psz) const
    {
        return !(*this == psz);
    }
    bool operator!=(const MStringW& str) const
    {
        return !(*this == str);
    }
    bool operator!=(WORD w) const
    {
        return m_id != w;
    }

    MString& str() const
    {
        static MString s_str;
        if (m_id == 0)
        {
            if (m_str.size())
            {
                s_str = m_str;
                return s_str;
            }
        }
        s_str = mstr_dec_short(m_id);
        return s_str;
    }

    MString& str_or_empty() const
    {
        static MString s_str;
        if (m_id == 0)
        {
            if (m_str.size())
            {
                s_str = m_str;
            }
            else
            {
                s_str.clear();
            }
        }
        else
        {
            s_str = mstr_dec_short(m_id);
        }
        return s_str;
    }

    const TCHAR *c_str() const
    {
        return str().c_str();
    }

    const TCHAR *c_str_or_empty() const
    {
        return str_or_empty().c_str();
    }

    MString quoted_wstr() const
    {
        MString ret;
        if (m_id == 0)
        {
            if (m_str.size())
            {
                ret += TEXT("\"");
                ret += mstr_escape(m_str);
                ret += TEXT("\"");
            }
            else
            {
                ret += TEXT("\"\"");
            }
        }
        else
        {
            ret = mstr_dec_short(m_id);
        }
        return ret;
    }
};

inline bool guts_escape(std::string& str, const char*& pch)
{
    using namespace std;
    switch (*pch)
    {
    case '\\': str += '\\'; ++pch; break;
    case '"': str += '\"'; ++pch; break;
    case 'a': str += '\a'; ++pch; break;
    case 'b': str += '\b'; ++pch; break;
    case 'f': str += '\f'; ++pch; break;
    case 'n': str += '\n'; ++pch; break;
    case 'r': str += '\r'; ++pch; break;
    case 't': str += '\t'; ++pch; break;
    case 'v': str += '\v'; ++pch; break;
    case 'x':
        {
            ++pch;
            std::string strNum;
            if (isxdigit(*pch))
            {
                strNum += *pch;
                ++pch;
                if (isxdigit(*pch))
                {
                    strNum += *pch;
                    ++pch;
                }
            }
            str += mstr_parse_int(strNum.c_str(), false, 16);
        }
        break;
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
        {
            std::string strNum;
            if ('0' <= *pch && *pch <= '7')
            {
                strNum += *pch;
                ++pch;
                if ('0' <= *pch && *pch <= '7')
                {
                    strNum += *pch;
                    ++pch;
                    if ('0' <= *pch && *pch <= '7')
                    {
                        strNum += *pch;
                        ++pch;
                    }
                }
            }
            str += (char)mstr_parse_int(strNum.c_str(), false, 8);
        }
        break;
    default:
        str += *pch;
        ++pch;
        return false;
    }
    return true;
}

inline bool guts_escape(MStringW& str, const WCHAR*& pch)
{
    using namespace std;
    switch (*pch)
    {
    case L'\\': str += L'\\'; ++pch; break;
    case L'"': str += L'\"'; ++pch; break;
    case L'a': str += L'\a'; ++pch; break;
    case L'b': str += L'\b'; ++pch; break;
    case L'f': str += L'\f'; ++pch; break;
    case L'n': str += L'\n'; ++pch; break;
    case L'r': str += L'\r'; ++pch; break;
    case L't': str += L'\t'; ++pch; break;
    case L'v': str += L'\v'; ++pch; break;
    case L'x':
        {
            ++pch;
            MStringW strNum;
            if (mchr_is_xdigit(*pch))
            {
                strNum += *pch;
                ++pch;
                if (mchr_is_xdigit(*pch))
                {
                    strNum += *pch;
                    ++pch;
                }
            }
            str += (WCHAR)mstr_parse_int(strNum.c_str(), false, 16);
        }
        break;
    case L'0': case L'1': case L'2': case L'3':
    case L'4': case L'5': case L'6': case L'7':
        {
            MStringW strNum;
            if (L'0' <= *pch && *pch <= L'7')
            {
                strNum += *pch;
                ++pch;
                if (L'0' <= *pch && *pch <= L'7')
                {
                    strNum += *pch;
                    ++pch;
                    if (L'0' <= *pch && *pch <= L'7')
                    {
                        strNum += *pch;
                        ++pch;
                    }
                }
            }
            str += (WCHAR)mstr_parse_int(strNum.c_str(), false, 8);
        }
        break;
    case 'u':
        {
            ++pch;
            MStringW strNum;
            if (mchr_is_xdigit(*pch))
            {
                strNum += *pch;
                ++pch;
                if (mchr_is_xdigit(*pch))
                {
                    strNum += *pch;
                    ++pch;
                    if (mchr_is_xdigit(*pch))
                    {
                        strNum += *pch;
                        ++pch;
                        if (mchr_is_xdigit(*pch))
                        {
                            strNum += *pch;
                            ++pch;
                        }
                    }
                }
            }
            str += (WCHAR)mstr_parse_int(strNum.c_str(), false, 16);
        }
        break;
    default:
        str += *pch;
        ++pch;
        return false;
    }
    return true;
}

inline bool guts_quote(std::string& str, const char*& pch)
{
    using namespace std;
    str.clear();

    pch = mstr_skip_space(pch);
    if (*pch != L'\"')
        return false;

    for (++pch; *pch; ++pch)
    {
        if (*pch == L'\\')
        {
            ++pch;
            guts_escape(str, pch);
            --pch;
        }
        else if (*pch == L'\"')
        {
            ++pch;
            if (*pch == L'\"')
            {
                str += L'\"';
            }
            else
            {
                break;
            }
        }
        else
        {
            str += *pch;
        }
    }

    return true;
}

inline bool guts_quote(MStringW& str, const WCHAR*& pch)
{
    using namespace std;
    str.clear();

    pch = mstr_skip_space(pch);
    if (*pch != L'\"')
        return false;

    for (++pch; *pch; ++pch)
    {
        if (*pch == L'\\')
        {
            ++pch;
            guts_escape(str, pch);
            --pch;
        }
        else if (*pch == L'\"')
        {
            ++pch;
            if (*pch == L'\"')
            {
                str += L'\"';
            }
            else
            {
                break;
            }
        }
        else
        {
            str += *pch;
        }
    }

    return true;
}

inline MString mstr_dec_short(SHORT value)
{
    MString ret;
    mstr_to_dec(ret, (short)value);
    return ret;
}

inline MString mstr_dec_word(WORD value)
{
    MString ret;
    mstr_to_dec(ret, value);
    return ret;
}

inline MString mstr_dec_dword(DWORD value)
{
    MString ret;
    mstr_to_dec(ret, value);
    return ret;
}

inline MString mstr_dec(int value)
{
    MString ret;
    mstr_to_dec(ret, value);
    return ret;
}

inline MString mstr_hex(int value)
{
    MString ret, str;
    if (value == 0)
    {
        ret = TEXT("0");
    }
    else
    {
        ret += TEXT("0x");
        mstr_to_hex(str, value);
        ret += str;
    }
    return ret;
}

inline MString mstr_hex_word(WORD value)
{
    MString ret, str;
    ret += TEXT("0x");
    mstr_to_hex(str, value);
    if (str.size() < 4)
    {
        ret += MString(4 - str.size(), TEXT('0'));
    }
    ret += str;
    return ret;
}

inline bool mstr_unquote(std::string& str)
{
    std::string str2 = str;
    const char *pch = str2.c_str();
    return guts_quote(str, pch);
}

inline bool mstr_unquote(MStringW& str)
{
    MStringW str2 = str;
    const WCHAR *pch = str2.c_str();
    return guts_quote(str, pch);
}

template <size_t siz>
inline bool mstr_unquote(char (&str)[siz])
{
    std::string s = str;
    bool ret = mstr_unquote(s);
    mstrcpy(str, s.c_str());
    return ret;
}

template <size_t siz>
inline bool mstr_unquote(WCHAR (&str)[siz])
{
    MStringW s = str;
    bool ret = mstr_unquote(s);
    mstrcpy(str, s.c_str());
    return ret;
}

template <typename T_CHAR>
inline size_t
mstr_repeat_count(const std::basic_string<T_CHAR>& str1, const std::basic_string<T_CHAR>& str2)
{
    size_t count = 0;
    for (size_t i = 0; i < str1.size(); i += str2.size())
    {
        if (str1.find(str2, i) != i)
            break;

        ++count;
    }
    return count;
}

template <typename T_CHAR>
inline size_t
mstr_repeat_count(const T_CHAR *str1, const std::basic_string<T_CHAR>& str2)
{
    std::basic_string<T_CHAR> s1(str1);
    return mstr_repeat_count(s1, str2);
}

template <typename T_CHAR>
inline size_t
mstr_repeat_count(const std::basic_string<T_CHAR>& str1, const T_CHAR *str2)
{
    std::basic_string<T_CHAR> s2(str2);
    return mstr_repeat_count(str1, s2);
}

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MIDORSTRING_HPP_

//////////////////////////////////////////////////////////////////////////////
