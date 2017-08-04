// is_string.hpp --- ID and Strings
//////////////////////////////////////////////////////////////////////////////

#ifndef ID_STRING_HPP_
#define ID_STRING_HPP_

#include "MString.hpp"

//////////////////////////////////////////////////////////////////////////////

struct ID_OR_STRING;

std::wstring mstr_dec(INT nID);
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

struct ID_OR_STRING
{
    WORD m_ID;
    std::wstring m_Str;

    ID_OR_STRING() : m_ID(0)
    {
    }

    ID_OR_STRING(WORD ID) : m_ID(ID)
    {
    }

    ID_OR_STRING(LPCWSTR Str)
    {
        if (IS_INTRESOURCE(Str))
        {
            m_ID = LOWORD(Str);
        }
        else
        {
            m_ID = 0;
            m_Str = Str;
        }
    }

    const WCHAR *Ptr() const
    {
        if (m_ID)
            return MAKEINTRESOURCEW(m_ID);
        return m_Str.c_str();
    }

    bool is_zero() const
    {
        return m_ID == 0 && m_Str.empty();
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
        return (!m_ID && !m_Str.empty());
    }

    bool is_int() const
    {
        return !is_str();
    }

    void clear()
    {
        m_ID = 0;
        m_Str.clear();
    }

    ID_OR_STRING& operator=(WORD ID)
    {
        m_ID = ID;
        m_Str.clear();
        return *this;
    }

    ID_OR_STRING& operator=(const WCHAR *Str)
    {
        if (IS_INTRESOURCE(Str))
        {
            m_ID = LOWORD(Str);
            m_Str.clear();
        }
        else
        {
            m_ID = 0;
            m_Str = Str;
        }
        return *this;
    }

    bool operator==(const ID_OR_STRING& id_or_str) const
    {
        if (id_or_str.m_ID != 0)
        {
            if (m_ID != 0)
                return id_or_str.m_ID == m_ID;
        }
        else
        {
            if (m_ID == 0)
                return m_Str == id_or_str.m_Str;
        }
        return false;
    }
    bool operator==(const WCHAR *psz) const
    {
        if (IS_INTRESOURCE(psz))
        {
            if (m_ID != 0)
                return LOWORD(psz) == m_ID;
        }
        else
        {
            if (m_ID == 0)
                return m_Str == psz;
        }
        return false;
    }
    bool operator==(const std::wstring& Str) const
    {
        return *this == Str.c_str();
    }
    bool operator==(WORD w) const
    {
        return m_ID == w;
    }

    bool operator!=(const ID_OR_STRING& id_or_str) const
    {
        return !(*this == id_or_str);
    }
    bool operator!=(const WCHAR *psz) const
    {
        return !(*this == psz);
    }
    bool operator!=(const std::wstring& Str) const
    {
        return !(*this == Str);
    }
    bool operator!=(WORD w) const
    {
        return m_ID != w;
    }

    std::wstring& str() const
    {
        static std::wstring s_str;
        if (m_ID == 0)
        {
            if (m_Str.size())
            {
                s_str = m_Str;
                return s_str;
            }
        }
        s_str = mstr_dec(m_ID);
        return s_str;
    }

    std::wstring& str_or_empty() const
    {
        static std::wstring s_str;
        if (m_ID == 0)
        {
            if (m_Str.size())
            {
                s_str = m_Str;
            }
            else
            {
                s_str.clear();
            }
        }
        else
        {
            s_str = mstr_dec(m_ID);
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
        if (m_ID == 0)
        {
            if (m_Str.size())
            {
                ret += L"\"";
                ret += mstr_escape(m_Str);
                ret += L"\"";
            }
            else
            {
                ret = L"\"\"";
            }
        }
        else
        {
            ret = mstr_dec(m_ID);
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

inline std::wstring mstr_dec(INT nID)
{
    using namespace std;
    wchar_t sz[32];
    wsprintfW(sz, L"%d", nID);
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

#endif  // ndef ID_STRING_HPP_

//////////////////////////////////////////////////////////////////////////////
