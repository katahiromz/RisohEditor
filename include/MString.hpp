// MString.hpp -- MZC4 string class                             -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MSTRING_HPP_
#define MZC4_MSTRING_HPP_       16  /* Version 16 */

// class MString;
// class MStringA;
// class MStringW;
// mstr_... functions
// mbin_... functions

////////////////////////////////////////////////////////////////////////////

#if __cplusplus >= 201103L          /* C++11 */
    #include <cstdint>
#else
    #include "pstdint.h"
#endif

#include <algorithm>    // for std::reverse
#include <cstring>      // for std::memcmp

// WCHAR
#ifndef __WCHAR_DEFINED
    #define __WCHAR_DEFINED
    #ifdef _WIN32
        typedef wchar_t WCHAR;
    #else
        #if __cplusplus >= 201103L
            typedef char16_t WCHAR;
        #else
            typedef uint16_t WCHAR;
        #endif
    #endif
#endif

// MString
#ifndef MString
    #include <string>       // for std::basic_string, std::string, ...
    typedef std::string MStringA;
    #if defined(_WIN32) && !defined(WONVER)
        #include <tchar.h>      // Windows generic text mapping
        #ifdef _MBCS
            #include <mbstring.h>   // for _mbsrchr
        #endif
        typedef std::wstring MStringW;
    #else
        typedef std::basic_string<WCHAR> MStringW;
    #endif
    #ifdef UNICODE
        #define MString     MStringW
    #else
        #define MString     MStringA
    #endif
#endif

// WIDE
#ifndef WIDE
    #ifdef _WIN32
        #define WIDE(sz) L##sz
    #else
        #define WIDE(sz) u##sz
    #endif
#endif

// TEXT
#ifndef TEXT
    #ifdef UNICODE
        #define TEXT(sz)   WIDE(sz)
    #else
        #define TEXT(sz)   sz
    #endif
#endif

////////////////////////////////////////////////////////////////////////////
// C string

template <typename T_CHAR>
inline size_t mstrlen(const T_CHAR *str);

template <typename T_CHAR>
T_CHAR *mstrcpy(T_CHAR *dest, const T_CHAR *src);

template <typename T_CHAR>
T_CHAR *mstrcpyn(T_CHAR *dest, const T_CHAR *src, size_t maxbuf);

template <typename T_CHAR>
T_CHAR *mstrrchr(T_CHAR *str, T_CHAR ch);
template <typename T_CHAR>
const T_CHAR *mstrrchr(const T_CHAR *str, T_CHAR ch);

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
// string

template <typename T_CHAR>
std::basic_string<T_CHAR> mchr_to_hex(T_CHAR ch);


template <typename T_CHAR>
bool mchr_is_xdigit(T_CHAR ch);

template <typename T_CHAR>
int mstr_parse_int(const T_CHAR *str, bool is_signed = true, int base = 0);

template <typename T_CHAR>
void mstr_to_hex(std::basic_string<T_CHAR>& str, unsigned int value);
template <typename T_CHAR>
void mstr_to_hex(std::basic_string<T_CHAR>& str, unsigned int value);

template <typename T_CHAR>
void mstr_to_dec(std::basic_string<T_CHAR>& str, int value, bool is_signed = true);

template <typename T_CHAR>
bool mstr_is_text_ascii(const T_CHAR *str, size_t len);

template <typename T_CHAR>
bool mstr_is_text_ascii(const std::basic_string<T_CHAR>& str);

bool mstr_is_text_utf8(const char *str, size_t len);
bool mstr_is_text_utf8(const std::string& str);

bool mstr_is_text_unicode(const void *ptr, size_t len);

template <typename T_CHAR>
void mstr_trim(std::basic_string<T_CHAR>& str, const T_CHAR *spaces);
template <typename T_CHAR>
void mstr_trim(T_CHAR *str, const T_CHAR *spaces);

template <typename T_CHAR>
void mstr_trim_left(std::basic_string<T_CHAR>& str, const T_CHAR *spaces);
template <typename T_CHAR>
void mstr_trim_left(T_CHAR *str, const T_CHAR *spaces);

template <typename T_CHAR>
void mstr_trim_right(std::basic_string<T_CHAR>& str, const T_CHAR *spaces);
template <typename T_CHAR>
void mstr_trim_right(T_CHAR *str, const T_CHAR *spaces);

template <typename T_CHAR>
T_CHAR *mstr_skip_space(T_CHAR *pch, const T_CHAR *spaces);

template <typename T_CHAR>
const T_CHAR *mstr_skip_space(const T_CHAR *pch, const T_CHAR *spaces);

inline char *mstr_skip_space(char *pch)
{
    return mstr_skip_space(pch, " \t\n\r\f\v");
}
inline const char *mstr_skip_space(const char *pch)
{
    return mstr_skip_space(pch, " \t\n\r\f\v");
}
inline WCHAR *mstr_skip_space(WCHAR *pch)
{
    return mstr_skip_space(pch, WIDE(" \t\n\r\f\v"));
}
inline const WCHAR *mstr_skip_space(const WCHAR *pch)
{
    return mstr_skip_space(pch, WIDE(" \t\n\r\f\v"));
}

template <typename T_CHAR>
std::basic_string<T_CHAR>
mstr_repeat(const std::basic_string<T_CHAR>& str, size_t count);
template <typename T_CHAR>
std::basic_string<T_CHAR>
mstr_repeat(const T_CHAR *str, size_t count);

template <typename T_CHAR>
std::basic_string<T_CHAR>
mstr_escape(const std::basic_string<T_CHAR>& str);

template <typename T_STR>
bool mstr_replace_all(T_STR& str, const T_STR& from, const T_STR& to);
template <typename T_STR>
bool mstr_replace_all(T_STR& str,
                      const typename T_STR::value_type *from,
                      const typename T_STR::value_type *to);

template <typename T_CHAR>
std::basic_string<T_CHAR>
mstr_quote(const std::basic_string<T_CHAR>& str);

template <typename T_CHAR>
std::basic_string<T_CHAR>
mstr_quote(const T_CHAR *str);

template <typename T_STR_CONTAINER>
void mstr_split(T_STR_CONTAINER& container,
                const typename T_STR_CONTAINER::value_type& str,
                const typename T_STR_CONTAINER::value_type& chars);

template <typename T_STR_CONTAINER>
typename T_STR_CONTAINER::value_type
mstr_join(const T_STR_CONTAINER& container,
          const typename T_STR_CONTAINER::value_type& sep);

////////////////////////////////////////////////////////////////////////////
// binary

void mbin_swap_endian(void *ptr, size_t len);
void mbin_swap_endian(std::string& bin);

MStringW
mstr_from_bin(const void *bin, size_t len, MTextType *pType = NULL);
MStringW
mstr_from_bin(const std::string& bin, MTextType *pType = NULL);

std::string mbin_from_str(const MStringW& str, const MTextType& type);

////////////////////////////////////////////////////////////////////////////

#include "MTextToText.hpp"

////////////////////////////////////////////////////////////////////////////

template <typename T_CHAR>
inline size_t mstrlen(const T_CHAR *str)
{
    return std::char_traits<T_CHAR>::length(str);
}

template <typename T_CHAR>
inline T_CHAR *mstrcpy(T_CHAR *dest, const T_CHAR *src)
{
    std::char_traits<T_CHAR>::copy(dest, src, mstrlen(src) + 1);
    return dest;
}

template <typename T_CHAR>
inline T_CHAR *mstrcpyn(T_CHAR *dest, const T_CHAR *src, size_t maxbuf)
{
    size_t len = mstrlen(src) + 1;
    if (len >= maxbuf)
        len = maxbuf;
    std::char_traits<T_CHAR>::copy(dest, src, len);
    if (maxbuf)
        dest[maxbuf - 1] = 0;
    return dest;
}

template <typename T_CHAR>
inline T_CHAR *mstrrchr(T_CHAR *str, T_CHAR ch)
{
#if defined(_WIN32) && defined(_MBCS)
    if (sizeof(T_CHAR) == 1)
    {
        return (T_CHAR *)(_mbsrchr((BYTE *)str, ch));
    }
#endif
    T_CHAR *ptr = NULL;
    while (*str)
    {
        if (*str == ch)
            ptr = str;
        ++str;
    }
    return ptr;
}

template <typename T_CHAR>
inline const T_CHAR *mstrrchr(const T_CHAR *str, T_CHAR ch)
{
#if defined(_WIN32) && defined(_MBCS)
    if (sizeof(T_CHAR) == 1)
    {
        return (const T_CHAR *)(_mbsrchr((const BYTE *)str, ch));
    }
#endif
    const T_CHAR *ptr = NULL;
    while (*str)
    {
        if (*str == ch)
            ptr = str;
        ++str;
    }
    return ptr;
}

////////////////////////////////////////////////////////////////////////////

template <typename T_CHAR>
inline std::basic_string<T_CHAR>
mchr_to_hex(T_CHAR value)
{
    std::basic_string<T_CHAR> ret;
    if (sizeof(T_CHAR) == 1)
        mstr_to_hex(ret, (value & 0xFF));
    else if (sizeof(T_CHAR) == 2)
        mstr_to_hex(ret, (value & 0xFFFF));
    else if (sizeof(T_CHAR) == 4)
        mstr_to_hex(ret, (value & 0xFFFFFFFF));
    return ret;
}

template <typename T_CHAR>
inline bool mchr_is_xdigit(T_CHAR ch)
{
    if (T_CHAR('0') <= ch && ch <= T_CHAR('9'))
        return true;
    if (T_CHAR('A') <= ch && ch <= T_CHAR('F'))
        return true;
    if (T_CHAR('a') <= ch && ch <= T_CHAR('f'))
        return true;
    return false;
}

template <typename T_CHAR>
inline int mstr_parse_int(const T_CHAR *str, bool is_signed, int base)
{
    str = mstr_skip_space(str);

    if (*str == T_CHAR('+'))
        ++str;

    bool minus = false;
    if (is_signed && *str == T_CHAR('-'))
    {
        minus = true;
        ++str;
    }

    if (str[0] == T_CHAR('0'))
    {
        if (str[1] == T_CHAR('x') || str[1] == T_CHAR('X'))
        {
            if (base == 0)
            {
                base = 16;
            }
            str += 2;
        }
        else
        {
            if (base == 0)
            {
                base = 8;
            }
            ++str;
        }
    }

    if (base == 0)
    {
        base = 10;
    }
    assert(base == 10 || base == 8 || base == 16);

	int num;
    for (num = 0; *str; ++str)
    {
        if (base == 8)
        {
            if (T_CHAR('0') <= *str && *str <= T_CHAR('7'))
            {
                num *= base;
                num += *str - T_CHAR('0');
                continue;
            }
        }
        else if (base == 16)
        {
            if (T_CHAR('0') <= *str && *str <= T_CHAR('9'))
            {
                num *= base;
                num += *str - T_CHAR('0');
                continue;
            }
            else if (T_CHAR('A') <= *str && *str <= T_CHAR('F'))
            {
                num *= base;
                num += *str - T_CHAR('A') + 10;
                continue;
            }
            else if (T_CHAR('a') <= *str && *str <= T_CHAR('f'))
            {
                num *= base;
                num += *str - T_CHAR('a') + 10;
                continue;
            }
        }
        else if (base == 10)
        {
            if (T_CHAR('0') <= *str && *str <= T_CHAR('9'))
            {
                num *= base;
                num += *str - T_CHAR('0');
                continue;
            }
        }
        break;
    }

    return (minus ? -num : num);
}

template <typename T_CHAR>
inline void
mstr_to_hex(std::basic_string<T_CHAR>& str, unsigned int value)
{
    static const char hex[] = "0123456789ABCDEF";
    str.clear();
    while (value)
    {
        str += T_CHAR(hex[value & 0xF]);
        value >>= 4;
    }
    std::reverse(str.begin(), str.end());
    if (str.empty())
        str += T_CHAR('0');
}

template <typename T_CHAR>
inline void
mstr_to_dec(std::basic_string<T_CHAR>& str, int value, bool is_signed)
{
    static const char dec[] = "0123456789";
    str.clear();
    bool is_minus = false;
    if (is_signed && value < 0)
    {
        is_minus = true;
        value = -value;
    }
    size_t i = 0;
    unsigned int uvalue = value;
    while (uvalue)
    {
        str += T_CHAR(dec[uvalue % 10]);
        uvalue /= 10;
        ++i;
    }
    if (is_minus)
        str += T_CHAR('-');
    std::reverse(str.begin(), str.end());
    if (str.empty())
        str += T_CHAR('0');
}

template <typename T_CHAR>
inline bool mstr_is_text_ascii(const T_CHAR *str, size_t len)
{
    if (!len)
        return true;

    while (len-- > 0)
    {
        if (*str < 0 || *str > 0x7F)
            return false;
        ++str;
    }
    return true;
}

template <typename T_CHAR>
inline bool mstr_is_text_ascii(const std::basic_string<T_CHAR>& str)
{
    return mstr_is_text_ascii(&str[0], str.size());
}

inline bool mstr_is_text_utf8(const std::string& str)
{
    return mstr_is_text_utf8(&str[0], str.size());
}

inline bool mstr_is_text_unicode(const void *ptr, size_t len)
{
    assert(0);
    //if (::IsTextUnicode(const_cast<void *>(ptr), int(len), NULL))
    //    return true;
    //return false;
    return true;    // ...
}

template <typename T_CHAR>
inline void mstr_trim(std::basic_string<T_CHAR>& str, const T_CHAR *spaces)
{
    typedef std::basic_string<T_CHAR> string_type;
    size_t i = str.find_first_not_of(spaces);
    size_t j = str.find_last_not_of(spaces);
    if ((i == string_type::npos) || (j == string_type::npos))
    {
        str.clear();
    }
    else
    {
        str = str.substr(i, j - i + 1);
    }
}

template <typename T_CHAR>
inline void mstr_trim(T_CHAR *str, const T_CHAR *spaces)
{
    typedef std::basic_string<T_CHAR> string_type;
    string_type s = str;
    mstr_trim(s, spaces);
    mstrcpy(str, s.c_str());
}

template <typename T_CHAR>
inline void mstr_trim_left(std::basic_string<T_CHAR>& str, const T_CHAR *spaces)
{
    typedef std::basic_string<T_CHAR> string_type;
    size_t i = str.find_first_not_of(spaces);
    if (i == string_type::npos)
    {
        str.clear();
    }
    else
    {
        str = str.substr(i);
    }
}

template <typename T_CHAR>
inline void mstr_trim_left(T_CHAR *str, const T_CHAR *spaces)
{
    typedef std::basic_string<T_CHAR> string_type;
    string_type s = str;
    mstr_trim_left(s, spaces);
    mstrcpy(str, s.c_str());
}

template <typename T_CHAR>
inline void mstr_trim_right(std::basic_string<T_CHAR>& str, const T_CHAR *spaces)
{
    typedef std::basic_string<T_CHAR> string_type;
    size_t j = str.find_last_not_of(spaces);
    if (j == string_type::npos)
    {
        str.clear();
    }
    else
    {
        str = str.substr(0, j + 1);
    }
}

template <typename T_CHAR>
inline void mstr_trim_right(T_CHAR *str, const T_CHAR *spaces)
{
    typedef std::basic_string<T_CHAR> string_type;
    string_type s = str;
    mstr_trim_right(s, spaces);
    mstrcpy(str, s.c_str());
}

template <typename T_CHAR>
inline std::basic_string<T_CHAR>
mstr_repeat(const std::basic_string<T_CHAR>& str, size_t count)
{
    std::basic_string<T_CHAR> ret;
    while (count-- > 0)
    {
        ret += str;
    }
    return ret;
}

template <typename T_CHAR>
inline std::basic_string<T_CHAR>
mstr_repeat(const T_CHAR *str, size_t count)
{
    return mstr_repeat(std::basic_string<T_CHAR>(str), count);
}

template <typename T_CHAR>
inline std::basic_string<T_CHAR>
mstr_escape(const std::basic_string<T_CHAR>& str)
{
    std::basic_string<T_CHAR> ret;

    for (size_t i = 0; i < str.size(); ++i)
    {
        T_CHAR ch = str[i];
        switch (ch)
        {
        case T_CHAR('\"'): ret += T_CHAR('\"'); ret += T_CHAR('\"'); break;
        case T_CHAR('\\'): ret += T_CHAR('\\'); ret += T_CHAR('\\'); break;
        case T_CHAR('\0'): ret += T_CHAR('\\'); ret += T_CHAR('0'); break;
        case T_CHAR('\a'): ret += T_CHAR('\\'); ret += T_CHAR('a'); break;
        case T_CHAR('\b'): ret += T_CHAR('\\'); ret += T_CHAR('b'); break;
        case T_CHAR('\f'): ret += T_CHAR('\\'); ret += T_CHAR('f'); break;
        case T_CHAR('\n'): ret += T_CHAR('\\'); ret += T_CHAR('n'); break;
        case T_CHAR('\r'): ret += T_CHAR('\\'); ret += T_CHAR('r'); break;
        case T_CHAR('\t'): ret += T_CHAR('\\'); ret += T_CHAR('t'); break;
        case T_CHAR('\v'): ret += T_CHAR('\\'); ret += T_CHAR('v'); break;
        default:
            if (ch < 0x20)
            {
                ret += T_CHAR('\\');
                ret += T_CHAR('x');
                ret += mchr_to_hex(ch);
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

inline MStringW
mstr_from_bin(const void *bin, size_t len, MTextType *pType/* = NULL*/)
{
    MStringW ret;

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

    if (len >= 2 && std::memcmp(bin, "\xFF\xFE", 2) == 0)
    {
        // UTF-16 LE
        if (pType)
        {
            pType->nEncoding = MTENC_UNICODE_LE;
            pType->bHasBOM = true;
        }
        ret.assign((const WCHAR *)bin, len / sizeof(WCHAR));
    }
    else if (len >= 2 && std::memcmp(bin, "\xFE\xFF", 2) == 0)
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
        if (len >= 3 && std::memcmp(bin, "\xEF\xBB\xBF", 3) == 0)
        {
            // UTF-8
            if (pType)
            {
                pType->nEncoding = MTENC_UTF8;
                pType->bHasBOM = true;
            }
            std::string str(&pch[3], len - 3);
            ret = MAnsiToWide(CP_UTF8, str);
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
            ret = MAnsiToWide(CP_ACP, str);
        }
        else if (mstr_is_text_utf8((const char *)bin, len))
        {
            // UTF-8
            if (pType)
            {
                pType->nEncoding = MTENC_UTF8;
                pType->bHasBOM = false;
            }
            ret = MAnsiToWide(CP_UTF8, pch, int(len));
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
            ret = MAnsiToWide(CP_ACP, str);
        }
    }

    if (!pType || pType->nNewLine != MNEWLINE_NOCHANGE)
    {
        if (pType)
        {
            pType->nNewLine = MNEWLINE_UNKNOWN;
        }
        if (mstr_replace_all(ret, WIDE("\r\n"), WIDE("\n")))
        {
            if (pType)
            {
                pType->nNewLine = MNEWLINE_CRLF;
            }
        }
        if (mstr_replace_all(ret, WIDE("\r"), WIDE("\n")))
        {
            if (pType && pType->nNewLine != MNEWLINE_CRLF)
            {
                pType->nNewLine = MNEWLINE_CR;
            }
        }
        if (mstr_replace_all(ret, WIDE("\n"), WIDE("\r\n")))
        {
            if (pType && pType->nNewLine != MNEWLINE_CRLF)
            {
                pType->nNewLine = MNEWLINE_LF;
            }
        }
    }

    return ret;
}

inline MStringW
mstr_from_bin(const std::string& bin, MTextType *pType/* = NULL*/)
{
    return mstr_from_bin(&bin[0], bin.size(), pType);
}

inline std::string
mbin_from_str(const MStringW& str, const MTextType& type)
{
    std::string ret;
    MStringW str2 = str;

    switch (type.nNewLine)
    {
    case MNEWLINE_UNKNOWN:
    case MNEWLINE_NOCHANGE:
        break;
    case MNEWLINE_CRLF:
        mstr_replace_all(str2, WIDE("\r\n"), WIDE("\n"));
        mstr_replace_all(str2, WIDE("\r"), WIDE("\r\n"));
        mstr_replace_all(str2, WIDE("\n"), WIDE("\r\n"));
        break;
    case MNEWLINE_LF:
        mstr_replace_all(str2, WIDE("\r\n"), WIDE("\n"));
        mstr_replace_all(str2, WIDE("\r"), WIDE("\n"));
        break;
    case MNEWLINE_CR:
        mstr_replace_all(str2, WIDE("\r\n"), WIDE("\r"));
        mstr_replace_all(str2, WIDE("\n"), WIDE("\r"));
        break;
    }

    switch (type.nEncoding)
    {
    case MTENC_UNKNOWN:
    case MTENC_ASCII:
    case MTENC_ANSI:
    default:
        ret += MWideToAnsi(CP_ACP, str2);
        break;
    case MTENC_UNICODE_LE:
        if (type.bHasBOM)
        {
            ret += "\xFF\xFE";
        }
        ret.append((const char *)str2.c_str(), str2.size() * sizeof(WCHAR));
        break;
    case MTENC_UNICODE_BE:
        if (type.bHasBOM)
        {
            ret += "\xFF\xFE";
        }
        ret.append((const char *)str2.c_str(), str2.size() * sizeof(WCHAR));
        mbin_swap_endian(ret);
        break;
    case MTENC_UTF8:
        if (type.bHasBOM)
        {
            ret += "\xEF\xBB\xBF";
        }
        ret += MWideToAnsi(CP_UTF8, str2);
        break;
    }

    return ret;
}

template <typename T_CHAR>
inline std::basic_string<T_CHAR>
mstr_quote(const std::basic_string<T_CHAR>& str)
{
    std::basic_string<T_CHAR> ret;
    ret += T_CHAR('\"');
    ret += mstr_escape(str);
    ret += T_CHAR('\"');
    return ret;
}

template <typename T_CHAR>
inline std::basic_string<T_CHAR>
mstr_quote(const T_CHAR *str)
{
    std::basic_string<T_CHAR> ret = str;
    return mstr_quote(ret);
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

template <typename T_CHAR>
inline T_CHAR *
mstr_skip_space(T_CHAR *pch, const T_CHAR *spaces)
{
    const T_CHAR *ptr;
    while (*pch)
    {
        for (ptr = spaces; *ptr; ++ptr)
        {
            if (*ptr == *pch)
            {
                ptr = NULL;
                break;
            }
        }
        if (ptr)
            return pch;

        ++pch;
    }
    return pch;
}

template <typename T_CHAR>
inline const T_CHAR *
mstr_skip_space(const T_CHAR *pch, const T_CHAR *spaces)
{
    const T_CHAR *ptr;
    while (*pch)
    {
        for (ptr = spaces; *ptr; ++ptr)
        {
            if (*ptr == *pch)
            {
                ptr = NULL;
                break;
            }
        }
        if (ptr)
            return pch;

        ++pch;
    }
    return pch;
}

////////////////////////////////////////////////////////////////////////////

inline void mstr_trim(MStringA& str)
{
    mstr_trim(str, " \t\n\r\f\v");
}
inline void mstr_trim(MStringW& str)
{
    mstr_trim(str, WIDE(" \t\n\r\f\v"));
}
inline void mstr_trim(char *str)
{
    mstr_trim(str, " \t\n\r\f\v");
}
inline void mstr_trim(WCHAR *str)
{
    mstr_trim(str, WIDE(" \t\n\r\f\v"));
}

inline void mstr_trim_left(MStringA& str)
{
    mstr_trim_left(str, " \t\n\r\f\v");
}
inline void mstr_trim_left(MStringW& str)
{
    mstr_trim_left(str, WIDE(" \t\n\r\f\v"));
}
inline void mstr_trim_left(char *str)
{
    mstr_trim_left(str, " \t\n\r\f\v");
}
inline void mstr_trim_left(WCHAR *str)
{
    mstr_trim_left(str, WIDE(" \t\n\r\f\v"));
}

inline void mstr_trim_right(MStringA& str)
{
    mstr_trim_right(str, " \t\n\r\f\v");
}
inline void mstr_trim_right(MStringW& str)
{
    mstr_trim_right(str, WIDE(" \t\n\r\f\v"));
}
inline void mstr_trim_right(char *str)
{
    mstr_trim_right(str, " \t\n\r\f\v");
}
inline void mstr_trim_right(WCHAR *str)
{
    mstr_trim_right(str, WIDE(" \t\n\r\f\v"));
}

////////////////////////////////////////////////////////////////////////////
// UTF-8 checking

#if defined(_WIN32) && !defined(WONVER)
    inline bool mstr_is_text_utf8(const char *str, size_t len)
    {
        if (len == 0)
            return true;

        len = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, int(len), NULL, 0);
        return len != 0;
    }
#else
    #include "UTF8_validator.h"
    inline bool mstr_is_text_utf8(const char *str, size_t len)
    {
        if (len == 0)
            return true;

        return UTF8_validate(str, len);
    }
#endif

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MSTRING_HPP_
