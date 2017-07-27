// MString.hpp -- MZC4 string class                             -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MSTRING_HPP_
#define MZC4_MSTRING_HPP_       6   /* Version 6 */

// class MString;
// class MStringA;
// class MStringW;
// mstr_... functions
// mbin_... functions

////////////////////////////////////////////////////////////////////////////

// MString
#ifndef MString
    #include <string>       // std::string and std::wstring
    typedef std::string     MStringA;
    typedef std::wstring    MStringW;
    #ifdef UNICODE
        #define MString     MStringW
    #else
        #define MString     MStringA
    #endif
#endif

////////////////////////////////////////////////////////////////////////////

#include "MTextToText.hpp"

////////////////////////////////////////////////////////////////////////////

enum MTextEncoding
{
    MTENC_UNKNOWN = 0,
    MTENC_ASCII,
    MTENC_ANSI,
    MTENC_UNICODE_LE,
    MTENC_UNICODE_BE,
    MTENC_UTF8,
    MTENC_UNICODE = MTENC_UNICODE_LE,
};

enum MTextNewLineType
{
    MNEWLINE_UNKNOWN,
    MNEWLINE_NOCHANGE,
    MNEWLINE_CRLF,
    MNEWLINE_LF,
    MNEWLINE_CR
};

struct MTextType
{
    MTextEncoding       nEncoding;
    MTextNewLineType    nNewLine;
    bool                bHasBOM;
};

////////////////////////////////////////////////////////////////////////////

bool mstr_is_text_ascii(const char *str, size_t len);
bool mstr_is_text_ascii(const std::string& str);

bool mstr_is_text_ascii(const wchar_t *str, size_t len);
bool mstr_is_text_ascii(const std::wstring& str);

bool mstr_is_text_utf8(const char *str, size_t len);
bool mstr_is_text_utf8(const std::string& str);

bool mstr_is_text_unicode(const void *ptr, size_t len);

void mstr_trim(std::string& str, const char *spaces = " \t\r\n");
void mstr_trim(std::wstring& str, const wchar_t *spaces = L" \t\r\n");
void mstr_trim(char *str, const char *spaces = " \t\r\n");
void mstr_trim(wchar_t *str, const wchar_t *spaces = L" \t\r\n");

std::string mstr_repeat(const std::string& str, size_t count);
std::wstring mstr_repeat(const std::wstring& str, size_t count);

std::string mstr_escape(const std::string& str);
std::wstring mstr_escape(const std::wstring& str);

template <typename T_STR>
bool mstr_replace_all(T_STR& str, const T_STR& from, const T_STR& to);
template <typename T_STR>
bool mstr_replace_all(T_STR& str,
                      const typename T_STR::value_type *from,
                      const typename T_STR::value_type *to);

void mbin_swap_endian(void *ptr, size_t len);
void mbin_swap_endian(std::string& bin);

std::wstring
mstr_from_bin(const void *bin, size_t len, MTextType *pType = NULL);
std::wstring
mstr_from_bin(const std::string& bin, MTextType *pType = NULL);

std::string mbin_from_str(const std::wstring& str, const MTextType& type);

std::string mstr_quote(const std::string& str);
std::wstring mstr_quote(const std::wstring& str);

template <typename T_STR_CONTAINER>
void mstr_split(T_STR_CONTAINER& container,
                const typename T_STR_CONTAINER::value_type& str,
                const typename T_STR_CONTAINER::value_type& chars);

template <typename T_STR_CONTAINER>
typename T_STR_CONTAINER::value_type
mstr_join(const T_STR_CONTAINER& container,
          const typename T_STR_CONTAINER::value_type& sep);

////////////////////////////////////////////////////////////////////////////

inline bool mstr_is_text_ascii(const char *str, size_t len)
{
    if (len == 0)
        return true;

    while (len-- > 0)
    {
        if ((unsigned char)*str > 0x7F)
            return false;
        ++str;
    }
    return true;
}

inline bool mstr_is_text_ascii(const std::string& str)
{
    return mstr_is_text_ascii(&str[0], str.size());
}

inline bool mstr_is_text_ascii(const wchar_t *str, size_t len)
{
    if (len == 0)
        return true;

    while (len-- > 0)
    {
        if ((unsigned short)*str > 0x7F)
            return false;
        ++str;
    }
    return true;
}

inline bool mstr_is_text_ascii(const std::wstring& str)
{
    return mstr_is_text_ascii(&str[0], str.size());
}

inline bool mstr_is_text_utf8(const char *str, size_t len)
{
    if (len == 0)
        return true;

    len = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, int(len), NULL, 0);
    return len != 0;
}

inline bool mstr_is_text_utf8(const std::string& str)
{
    return mstr_is_text_utf8(&str[0], str.size());
}

inline bool mstr_is_text_unicode(const void *ptr, size_t len)
{
    if (::IsTextUnicode(const_cast<void *>(ptr), int(len), NULL))
        return true;
    return false;
}

inline void mstr_trim(std::string& str, const char *spaces/* = " \t\r\n"*/)
{
    size_t i = str.find_first_not_of(spaces);
    size_t j = str.find_last_not_of(spaces);
    if ((i == std::string::npos) || (j == std::string::npos))
    {
        str.clear();
    }
    else
    {
        str = str.substr(i, j - i + 1);
    }
}

inline void mstr_trim(std::wstring& str, const wchar_t *spaces/* = L" \t\r\n"*/)
{
    size_t i = str.find_first_not_of(spaces);
    size_t j = str.find_last_not_of(spaces);
    if ((i == std::wstring::npos) || (j == std::wstring::npos))
    {
        str.clear();
    }
    else
    {
        str = str.substr(i, j - i + 1);
    }
}

inline void mstr_trim(char *str, const char *spaces/* = " \t\r\n"*/)
{
    std::string s = str;
    mstr_trim(s, spaces);
    std::strcpy(str, s.c_str());
}

inline void mstr_trim(wchar_t *str, const wchar_t *spaces/* = L" \t\r\n"*/)
{
    std::wstring s = str;
    mstr_trim(s, spaces);
    std::wcscpy(str, s.c_str());
}

inline std::string mstr_repeat(const std::string& str, size_t count)
{
    std::string ret;
    while (count-- > 0)
    {
        ret += str;
    }
    return ret;
}

inline std::wstring mstr_repeat(const std::wstring& str, size_t count)
{
    std::wstring ret;
    while (count-- > 0)
    {
        ret += str;
    }
    return ret;
}

inline std::string mstr_escape(const std::string& str)
{
    std::string ret;

    for (size_t i = 0; i < str.size(); ++i)
    {
        char ch = str[i];
        switch (ch)
        {
        case '\"': ret += "\"\""; break;
        case '\\': ret += "\\\\"; break;
        case '\0': ret += "\\0"; break;
        case '\a': ret += "\\a"; break;
        case '\b': ret += "\\b"; break;
        case '\f': ret += "\\f"; break;
        case '\n': ret += "\\n"; break;
        case '\r': ret += "\\r"; break;
        case '\t': ret += "\\t"; break;
        case '\v': ret += "\\v"; break;
        default:
            if (ch < 0x20)
            {
                using namespace std;
                char sz[32];
                sprintf(sz, "\\x%02X", ch);
                ret += sz;
            }
            else
            {
                ret += ch;
            }
        }
    }

    return ret;
}

inline std::wstring mstr_escape(const std::wstring& str)
{
    std::wstring ret;

    for (size_t i = 0; i < str.size(); ++i)
    {
        wchar_t ch = str[i];
        switch (ch)
        {
        case L'\"': ret += L"\"\""; break;
        case L'\\': ret += L"\\\\"; break;
        case L'\0': ret += L"\\0"; break;
        case L'\a': ret += L"\\a"; break;
        case L'\b': ret += L"\\b"; break;
        case L'\f': ret += L"\\f"; break;
        case L'\n': ret += L"\\n"; break;
        case L'\r': ret += L"\\r"; break;
        case L'\t': ret += L"\\t"; break;
        case L'\v': ret += L"\\v"; break;
        default:
            if (ch < 0x20)
            {
                using namespace std;
                wchar_t sz[32];
                wsprintfW(sz, L"\\x%02X", (BYTE)ch);
                ret += sz;
            }
            else
            {
                ret += ch;
            }
        }
    }

    return ret;
}

template <typename T_STR>
inline bool
mstr_replace_all(T_STR& str, const T_STR& from, const T_STR& to)
{
    bool ret = false;
    size_t i = 0;
    for (;;) {
        i = str.find(from, i);
        if (i == T_STR::npos)
            break;
        ret = true;
        str.replace(i, from.size(), to);
        i += to.size();
    }
    return ret;
}
template <typename T_STR>
inline bool
mstr_replace_all(T_STR& str,
                 const typename T_STR::value_type *from,
                 const typename T_STR::value_type *to)
{
    return mstr_replace_all(str, T_STR(from), T_STR(to));
}

inline void
mbin_swap_endian(void *ptr, size_t len)
{
    char *pb = (char *)ptr;
    len /= 2;
    while (--len)
    {
        char b = pb[0];
        pb[0] = pb[1];
        pb[1] = b;
        ++pb;
        ++pb;
    }
}

inline void mbin_swap_endian(std::string& bin)
{
    mbin_swap_endian(&bin[0], bin.size());
}

inline std::wstring
mstr_from_bin(const void *bin, size_t len, MTextType *pType/* = NULL*/)
{
    std::wstring ret;

    if (bin == NULL || len == 0)
    {
        // empty
        if (pType)
        {
            pType->nNewLine = MNEWLINE_CRLF;
            pType->nEncoding = MTENC_ASCII;
        }
        return ret;
    }

    if (len >= 2 && memcmp(bin, "\xFF\xFE", 2) == 0)
    {
        // UTF-16 LE
        if (pType)
        {
            pType->nEncoding = MTENC_UNICODE_LE;
            pType->bHasBOM = true;
        }
        ret.assign((const WCHAR *)bin, len / sizeof(WCHAR));
    }
    else if (len >= 2 && memcmp(bin, "\xFE\xFF", 2) == 0)
    {
        // UTF-16 BE
        if (pType)
        {
            pType->nEncoding = MTENC_UNICODE_BE;
            pType->bHasBOM = true;
        }
        ret.assign((const WCHAR *)bin, len / sizeof(WCHAR));
        mbin_swap_endian(&ret[0], len);
    }
    else
    {
        const char *pch = (const char *)bin;
        if (len >= 3 && memcmp(bin, "\xEF\xBB\xBF", 3) == 0)
        {
            // UTF-8
            if (pType)
            {
                pType->nEncoding = MTENC_UTF8;
                pType->bHasBOM = true;
            }
            std::string str(&pch[3], len - 3);
            ret = MUtf8ToWide(str);
        }
        else if (mstr_is_text_ascii((const char *)bin, len))
        {
            // ASCII
            if (pType)
            {
                pType->nEncoding = MTENC_ASCII;
                pType->bHasBOM = false;
            }
            std::string str(pch, len);
            ret = MAnsiToWide(str);
        }
        else if (mstr_is_text_utf8((const char *)bin, len))
        {
            // UTF-8
            if (pType)
            {
                pType->nEncoding = MTENC_UTF8;
                pType->bHasBOM = false;
            }
            ret = MUtf8ToWide(pch, INT(len));
        }
        else if (mstr_is_text_unicode(bin, int(len)))
        {
            // UTF-16 LE
            if (pType)
            {
                pType->nEncoding = MTENC_UNICODE_LE;
                pType->bHasBOM = false;
            }
            ret.assign((const WCHAR *)bin, len / sizeof(WCHAR));
        }
        else
        {
            // ANSI
            if (pType)
            {
                pType->nEncoding = MTENC_ANSI;
                pType->bHasBOM = false;
            }
            std::string str(pch, len);
            ret = MAnsiToWide(str);
        }
    }

    if (!pType || pType->nNewLine != MNEWLINE_NOCHANGE)
    {
        if (pType)
        {
            pType->nNewLine = MNEWLINE_UNKNOWN;
        }
        if (mstr_replace_all(ret, L"\r\n", L"\n"))
        {
            if (pType)
            {
                pType->nNewLine = MNEWLINE_CRLF;
            }
        }
        if (mstr_replace_all(ret, L"\r", L"\n"))
        {
            if (pType && pType->nNewLine != MNEWLINE_CRLF)
            {
                pType->nNewLine = MNEWLINE_CR;
            }
        }
        if (mstr_replace_all(ret, L"\n", L"\r\n"))
        {
            if (pType && pType->nNewLine != MNEWLINE_CRLF)
            {
                pType->nNewLine = MNEWLINE_LF;
            }
        }
    }

    return ret;
}

inline std::wstring
mstr_from_bin(const std::string& bin, MTextType *pType/* = NULL*/)
{
    return mstr_from_bin(&bin[0], bin.size(), pType);
}

inline std::string
mbin_from_str(const std::wstring& str, const MTextType& type)
{
    std::string ret;
    std::wstring str2 = str;

    switch (type.nNewLine)
    {
    case MNEWLINE_UNKNOWN:
    case MNEWLINE_NOCHANGE:
        break;
    case MNEWLINE_CRLF:
        mstr_replace_all(str2, L"\r\n", L"\n");
        mstr_replace_all(str2, L"\r", L"\r\n");
        mstr_replace_all(str2, L"\n", L"\r\n");
        break;
    case MNEWLINE_LF:
        mstr_replace_all(str2, L"\r\n", L"\n");
        mstr_replace_all(str2, L"\r", L"\n");
        break;
    case MNEWLINE_CR:
        mstr_replace_all(str2, L"\r\n", L"\r");
        mstr_replace_all(str2, L"\n", L"\r");
        break;
    }

    switch (type.nEncoding)
    {
    case MTENC_UNKNOWN:
    case MTENC_ASCII:
    case MTENC_ANSI:
    default:
        ret += MWideToAnsi(str2);
        break;
    case MTENC_UNICODE_LE:
        if (type.bHasBOM)
        {
            ret += "\xFF\xFE";
        }
        ret.append((const char *)str2.c_str(), str2.size() * sizeof(wchar_t));
        break;
    case MTENC_UNICODE_BE:
        if (type.bHasBOM)
        {
            ret += "\xFF\xFE";
        }
        ret.append((const char *)str2.c_str(), str2.size() * sizeof(wchar_t));
        mbin_swap_endian(ret);
        break;
    case MTENC_UTF8:
        if (type.bHasBOM)
        {
            ret += "\xEF\xBB\xBF";
        }
        ret += MWideToUtf8(str2);
        break;
    }

    return ret;
}

inline std::string mstr_quote(const std::string& str)
{
    std::string ret = "\"";
    ret += mstr_escape(str);
    ret += "\"";
    return ret;
}

inline std::wstring mstr_quote(const std::wstring& str)
{
    std::wstring ret = L"\"";
    ret += mstr_escape(str);
    ret += L"\"";
    return ret;
}

template <typename T_STR_CONTAINER>
inline void
mstr_split(T_STR_CONTAINER& container,
           const typename T_STR_CONTAINER::value_type& str,
           const typename T_STR_CONTAINER::value_type& chars)
{
    container.clear();
    size_t i = 0, k = str.find_first_of(chars);
    while (k != T_STR_CONTAINER::value_type::npos)
    {
        container.push_back(str.substr(i, k - i));
        i = k + 1;
        k = str.find_first_of(chars, i);
    }
    container.push_back(str.substr(i));
}

template <typename T_STR_CONTAINER>
inline typename T_STR_CONTAINER::value_type
mstr_join(const T_STR_CONTAINER& container,
          const typename T_STR_CONTAINER::value_type& sep)
{
    typename T_STR_CONTAINER::value_type result;
    typename T_STR_CONTAINER::const_iterator it, end;
    it = container.begin();
    end = container.end();
    if (it != end)
    {
        result = *it;
        for (++it; it != end; ++it)
        {
            result += sep;
            result += *it;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MSTRING_HPP_
