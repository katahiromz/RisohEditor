// MString.hpp -- MZC4 string class                             -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MSTRING_HPP_
#define MZC4_MSTRING_HPP_       21 /* Version 21 */

// class MString;
// class MStringA;
// class MStringW;

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

template <typename T_CHAR, size_t siz>
T_CHAR *mstrcpy(T_CHAR (&dest)[siz], const T_CHAR *src);

template <typename T_CHAR, size_t siz>
T_CHAR *mstrcpyn(T_CHAR (&dest)[siz], const T_CHAR *src, size_t maxbuf);

template <typename T_CHAR>
T_CHAR *mstrrchr(T_CHAR *str, T_CHAR ch);
template <typename T_CHAR>
const T_CHAR *mstrrchr(const T_CHAR *str, T_CHAR ch);

////////////////////////////////////////////////////////////////////////////

template <typename T_CHAR>
bool mchr_is_alpha(T_CHAR ch);
template <typename T_CHAR>
bool mchr_is_alnum(T_CHAR ch);
bool mstr_is_text_utf8(const std::string& str);
inline bool mstr_is_text_utf8(const char *str, size_t len);

bool guts_escape(std::string& str, const char*& pch);
bool guts_escape(MStringW& str, const WCHAR*& pch);
bool guts_quote(std::string& str, const char*& pch);
bool guts_quote(MStringW& str, const WCHAR*& pch);

////////////////////////////////////////////////////////////////////////////

inline void mstr_upper(char* str)
{
	_strupr(str);
}
inline void mstr_upper(wchar_t* str)
{
	_wcsupr(str);
}
inline void mstr_upper(std::string& str)
{
	if (str.size())
		_strupr(&str[0]);
}
inline void mstr_upper(std::wstring& str)
{
	if (str.size())
		_wcsupr(&str[0]);
}

inline void mstr_lower(char* str)
{
	_strlwr(str);
}
inline void mstr_lower(wchar_t* str)
{
	_wcslwr(str);
}
inline void mstr_lower(std::string& str)
{
	if (str.size())
		_strlwr(&str[0]);
}
inline void mstr_lower(std::wstring& str)
{
	if (str.size())
		_wcslwr(&str[0]);
}

inline bool mstr_is_identifier(const std::wstring& str)
{
	if (str.empty())
		return false;
	if (str[0] != '_' && !mchr_is_alpha(str[0]))
		return false;
	for (size_t i = 1; i < str.size(); ++i)
	{
		if (str[i] != '_' && !mchr_is_alnum(str[i]))
			return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////

#include "MTextToText.hpp"

////////////////////////////////////////////////////////////////////////////

template <typename T_CHAR>
inline size_t mstrlen(const T_CHAR *str)
{
	return std::char_traits<T_CHAR>::length(str);
}

template <typename T_CHAR, size_t siz>
inline T_CHAR *mstrcpy(T_CHAR (&dest)[siz], const T_CHAR *src)
{
	std::char_traits<T_CHAR>::copy(dest, src, mstrlen(src) + 1);
	return dest;
}

template <typename T_CHAR, size_t siz>
inline T_CHAR *mstrcpyn(T_CHAR (&dest)[siz], const T_CHAR *src, size_t maxbuf)
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
inline bool mchr_is_digit(T_CHAR ch)
{
	return (T_CHAR('0') <= ch && ch <= T_CHAR('9'));
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
inline bool mchr_is_upper(T_CHAR ch)
{
	return (T_CHAR('A') <= ch && ch <= T_CHAR('Z'));
}

template <typename T_CHAR>
inline bool mchr_is_lower(T_CHAR ch)
{
	return (T_CHAR('a') <= ch && ch <= T_CHAR('z'));
}

template <typename T_CHAR>
inline bool mchr_is_alpha(T_CHAR ch)
{
	return mchr_is_upper(ch) || mchr_is_lower(ch);
}

template <typename T_CHAR>
inline bool mchr_is_alnum(T_CHAR ch)
{
	return mchr_is_alpha(ch) || mchr_is_digit(ch);
}

template <typename T_CHAR>
inline bool mchr_is_space(T_CHAR ch)
{
	return (ch == T_CHAR(' ') || ch == T_CHAR('\t') ||
			ch == T_CHAR('\n') || ch == T_CHAR('\r') ||
			ch == T_CHAR('\f') || ch == T_CHAR('\v'));
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
inline int mstr_parse_int(const T_CHAR *str, bool is_signed = true, int base = 0)
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

#if defined(_WIN32) && !defined(WONVER)
	inline bool mstr_is_text_unicode(const void *ptr, size_t len)
	{
		if (len == 0)
			return true;

		return !!::IsTextUnicode(ptr, int(len), NULL);
	}
#else
	#include "UTF16_validator.h"
	inline bool mstr_is_text_unicode(const void *ptr, size_t len)
	{
		if (len == 0)
			return true;

		return UTF16_validate(ptr, len);
	}
#endif

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

template <typename T_CHAR, size_t siz>
inline void mstr_trim(T_CHAR (&str)[siz], const T_CHAR *spaces)
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

template <typename T_CHAR, size_t siz>
inline void mstr_trim_left(T_CHAR (&str)[siz], const T_CHAR *spaces)
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

template <typename T_CHAR, size_t siz>
inline void mstr_trim_right(T_CHAR (&str)[siz], const T_CHAR *spaces)
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

template <typename T_CHAR>
inline std::basic_string<T_CHAR>
mstr_escape_with_wrap(const std::basic_string<T_CHAR>& str)
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

template <typename T_CHAR>
inline std::basic_string<T_CHAR>
mstr_quote_with_wrap(const std::basic_string<T_CHAR>& str)
{
	std::basic_string<T_CHAR> ret;
	ret += T_CHAR('\"');
	ret += mstr_escape_with_wrap(str);
	ret += T_CHAR('\"');
	return ret;
}

template <typename T_CHAR>
inline std::basic_string<T_CHAR>
mstr_quote_with_wrap(const T_CHAR *str)
{
	std::basic_string<T_CHAR> ret = str;
	return mstr_quote_with_wrap(ret);
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

inline void mstr_trim(MStringA& str)
{
	mstr_trim(str, " \t\n\r\f\v");
}
inline void mstr_trim(MStringW& str)
{
	mstr_trim(str, WIDE(" \t\n\r\f\v"));
}
template <size_t siz>
inline void mstr_trim(char (&str)[siz])
{
	mstr_trim(str, " \t\n\r\f\v");
}
template <size_t siz>
inline void mstr_trim(WCHAR (&str)[siz])
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
template <size_t siz>
inline void mstr_trim_left(char (&str)[siz])
{
	mstr_trim_left(str, " \t\n\r\f\v");
}
template <size_t siz>
inline void mstr_trim_left(WCHAR (&str)[siz])
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
template <size_t siz>
inline void mstr_trim_right(char (&str)[siz])
{
	mstr_trim_right(str, " \t\n\r\f\v");
}
template <size_t siz>
inline void mstr_trim_right(WCHAR (&str)[siz])
{
	mstr_trim_right(str, WIDE(" \t\n\r\f\v"));
}

template <typename T_CHAR>
inline void
mstr_to_dec(std::basic_string<T_CHAR>& str, int value, bool is_signed = true)
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

template <typename T_CHAR = TCHAR>
std::basic_string<T_CHAR> mstr_dec_word(WORD value)
{
	std::basic_string<T_CHAR> ret;
	mstr_to_dec(ret, value);
	return ret;
}

template <typename T_CHAR = TCHAR>
std::basic_string<T_CHAR> mstr_dec_short(SHORT value)
{
	std::basic_string<T_CHAR> ret;
	mstr_to_dec(ret, (short)value);
	return ret;
}

template <typename T_CHAR = TCHAR>
std::basic_string<T_CHAR> mstr_dec_dword(DWORD value)
{
	std::basic_string<T_CHAR> ret;
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

////////////////////////////////////////////////////////////////////////////

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
