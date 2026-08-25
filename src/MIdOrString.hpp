// MIdOrString.hpp --- ID and String
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

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

#ifndef _countof
	#define _countof(array) (sizeof(array) / sizeof(array[0]))
#endif

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
		else if ((TEXT('0') <= str[0] && str[0] <= TEXT('9')) ||
		         str[0] == TEXT('-') || str[0] == TEXT('+'))
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
		// NOTE: MAKEINTRESOURCE(0) is just a null pointer. This function can return an empty
		//       string to avoid the function returning NULL for (WORD)0 of the resource name.
		//       Obviously, the resource name is not an empty string.
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
		if (is_zero() || id_or_str.is_zero())
			return is_zero() > id_or_str.is_zero();
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

	bool operator!=(const MIdOrString& id_or_str) const
	{
		return !(*this == id_or_str);
	}

	MString str(bool unsign = false) const
	{
		if (m_id == 0)
		{
			if (m_str.size())
			{
				return m_str;
			}
		}
		if (unsign)
			return mstr_dec_word(m_id);
		else
			return mstr_dec_short(m_id);
	}

	MString str_or_empty() const
	{
		if (m_id == 0)
		{
			if (m_str.size())
			{
				return m_str;
			}
			else
			{
				return TEXT("");
			}
		}
		else
		{
			return mstr_dec_short(m_id);
		}
	}

    mutable MString s_strTmp;

	const TCHAR *c_str() const
	{
		s_strTmp = str();
		return s_strTmp.c_str();
	}

	const TCHAR *c_str_or_empty() const
	{
		s_strTmp = str_or_empty();
		return s_strTmp.c_str();
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

	MString quoted_wstr_with_wrap() const
	{
		MString ret;
		if (m_id == 0)
		{
			if (m_str.size())
				ret += mstr_quote_with_wrap(m_str);
			else
				ret += TEXT("\"\"");
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
