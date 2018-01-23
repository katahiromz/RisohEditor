// is_string.hpp --- ID and Strings
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Win32API resource editor
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

//////////////////////////////////////////////////////////////////////////////

struct MIdOrString;

std::wstring mstr_dec_short(SHORT nID);
std::wstring mstr_dec_word(WORD nID);
std::wstring mstr_dec_dword(DWORD nID);
std::wstring mstr_dec(LONG nID);
std::wstring mstr_hex(INT nID);
bool mstr_unquote(std::string& str);
bool mstr_unquote(std::wstring& str);
bool mstr_unquote(char *str);
bool mstr_unquote(wchar_t *str);
bool guts_escape(std::string& str, const char*& pch);
bool guts_escape(std::wstring& str, const wchar_t*& pch);
bool guts_quote(std::string& str, const char*& pch);
bool guts_quote(std::wstring& str, const wchar_t*& pch);
int mstr_repeat_count(const std::string& str1, const std::string& str2);
int mstr_repeat_count(const std::wstring& str1, const std::wstring& str2);
const char *skip_space(const char *pch);
const wchar_t *skip_space(const wchar_t *pch);;

template <typename T_STR>
bool mstr_is_ascii(const T_STR& str);

//////////////////////////////////////////////////////////////////////////////

struct MIdOrString
{
    WORD m_id;
    std::wstring m_str;

    MIdOrString() : m_id(0)
    {
    }

    MIdOrString(WORD ID) : m_id(ID)
    {
    }

    MIdOrString(LPCWSTR str)
    {
        if (IS_INTRESOURCE(str))
        {
            m_id = LOWORD(str);
        }
        else if ((L'0' <= str[0] && str[0] <= L'9') ||
                 str[0] == L'-' || str[0] == L'+')
        {
            m_id = (WORD)wcstol(str, NULL, 0);
        }
        else
        {
            m_id = 0;
            m_str = str;
        }
    }

    const WCHAR *ptr() const
    {
        if (m_id)
            return MAKEINTRESOURCEW(m_id);
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

    MIdOrString& operator=(const WCHAR *str)
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
            return true;
        }
        else
        {
            if (m_id == 0)
                return m_str < id_or_str.m_str;
            return false;
        }
    }
    bool operator>(const MIdOrString& id_or_str) const
    {
        if (id_or_str.m_id != 0)
        {
            if (m_id != 0)
                return m_id > id_or_str.m_id;
            return false;
        }
        else
        {
            if (m_id == 0)
                return m_str > id_or_str.m_str;
            return true;
        }
    }

    bool operator==(const WCHAR *psz) const
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
    bool operator==(const std::wstring& str) const
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
    bool operator!=(const std::wstring& str) const
    {
        return !(*this == str);
    }
    bool operator!=(WORD w) const
    {
        return m_id != w;
    }

    std::wstring& str() const
    {
        static std::wstring s_str;
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

    std::wstring& str_or_empty() const
    {
        static std::wstring s_str;
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

    LPCWSTR c_str() const
    {
        return str().c_str();
    }

    LPCWSTR c_str_or_empty() const
    {
        return str_or_empty().c_str();
    }

    std::wstring quoted_wstr() const
    {
        std::wstring ret;
        if (m_id == 0)
        {
            if (m_str.size())
            {
                ret += L"\"";
                ret += mstr_escape(m_str);
                ret += L"\"";
            }
            else
            {
                ret = L"\"\"";
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
            str += (char)strtoul(strNum.c_str(), NULL, 16);
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
            str += (char)strtoul(strNum.c_str(), NULL, 8);
        }
        break;
    default:
        str += *pch;
        ++pch;
        return false;
    }
    return true;
}

inline bool guts_escape(std::wstring& str, const wchar_t*& pch)
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
            std::wstring strNum;
            if (iswxdigit(*pch))
            {
                strNum += *pch;
                ++pch;
                if (iswxdigit(*pch))
                {
                    strNum += *pch;
                    ++pch;
                }
            }
            str += (wchar_t)wcstoul(strNum.c_str(), NULL, 16);
        }
        break;
    case L'0': case L'1': case L'2': case L'3':
    case L'4': case L'5': case L'6': case L'7':
        {
            std::wstring strNum;
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
            str += (wchar_t)wcstoul(strNum.c_str(), NULL, 8);
        }
        break;
    case 'u':
        {
            ++pch;
            std::wstring strNum;
            if (iswxdigit(*pch))
            {
                strNum += *pch;
                ++pch;
                if (iswxdigit(*pch))
                {
                    strNum += *pch;
                    ++pch;
                    if (iswxdigit(*pch))
                    {
                        strNum += *pch;
                        ++pch;
                        if (iswxdigit(*pch))
                        {
                            strNum += *pch;
                            ++pch;
                        }
                    }
                }
            }
            str += (wchar_t)wcstoul(strNum.c_str(), NULL, 16);
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

    pch = skip_space(pch);
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

inline bool guts_quote(std::wstring& str, const wchar_t*& pch)
{
    using namespace std;
    str.clear();

    pch = skip_space(pch);
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

inline std::wstring mstr_dec_short(SHORT nID)
{
    using namespace std;
    wchar_t sz[32];
    wsprintfW(sz, L"%d", nID);
    std::wstring ret(sz);
    return ret;
}

inline std::wstring mstr_dec_word(WORD nID)
{
    using namespace std;
    wchar_t sz[32];
    wsprintfW(sz, L"%u", nID);
    std::wstring ret(sz);
    return ret;
}

inline std::wstring mstr_dec_dword(DWORD nID)
{
    using namespace std;
    wchar_t sz[32];
    wsprintfW(sz, L"%lu", nID);
    std::wstring ret(sz);
    return ret;
}

inline std::wstring mstr_dec(LONG nID)
{
    using namespace std;
    wchar_t sz[32];
    wsprintfW(sz, L"%ld", nID);
    std::wstring ret(sz);
    return ret;
}

inline std::wstring mstr_hex(INT nID)
{
    std::wstring ret;
    if (nID == 0)
    {
        ret = L"0";
    }
    else
    {
        using namespace std;
        wchar_t sz[32];
        wsprintfW(sz, L"0x%X", nID);
        ret = sz;
    }
    return ret;
}

inline bool mstr_unquote(std::string& str)
{
    std::string str2 = str;
    const char *pch = str2.c_str();
    return guts_quote(str, pch);
}

inline bool mstr_unquote(std::wstring& str)
{
    std::wstring str2 = str;
    const wchar_t *pch = str2.c_str();
    return guts_quote(str, pch);
}

inline bool mstr_unquote(char *str)
{
    std::string s = str;
    bool ret = mstr_unquote(s);
    std::strcpy(str, s.c_str());
    return ret;
}

inline bool mstr_unquote(wchar_t *str)
{
    std::wstring s = str;
    bool ret = mstr_unquote(s);
    std::wcscpy(str, s.c_str());
    return ret;
}

template <typename T_STR>
inline bool mstr_is_ascii(const T_STR& str)
{
    size_t i, count = str.size();
    for (i = 0; i < count; ++i)
    {
        typename T_STR::value_type ch = str[i];
        if (ch < 0x20 || 0x7F < ch)
            return false;
    }
    return true;
}

inline int mstr_repeat_count(const std::string& str1, const std::string& str2)
{
    int count = 0;
    for (size_t i = 0; i < str1.size(); i += str2.size())
    {
        if (str1.find(str2, i) != i)
            break;

        ++count;
    }
    return count;
}

inline int mstr_repeat_count(const std::wstring& str1, const std::wstring& str2)
{
    int count = 0;
    for (size_t i = 0; i < str1.size(); i += str2.size())
    {
        if (str1.find(str2, i) != i)
            break;

        ++count;
    }
    return count;
}

inline const char *skip_space(const char *pch)
{
    using namespace std;
    while (*pch && isspace(*pch))
    {
        ++pch;
    }
    return pch;
}

inline const wchar_t *skip_space(const wchar_t *pch)
{
    using namespace std;
    while (*pch && iswspace(*pch))
    {
        ++pch;
    }
    return pch;
}

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MIDORSTRING_HPP_

//////////////////////////////////////////////////////////////////////////////
