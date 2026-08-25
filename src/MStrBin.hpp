// MStrBin.hpp --- String and binary
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#include "MString.hpp"
#include "MTextToText.hpp"

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
	if (bin.size())
		mbin_swap_endian(&bin[0], bin.size());
}

inline void
mstr_normalize_newlines_to_crlf(MStringW& ret, MTextType *pType = nullptr)
{
	bool bHadCRLF = false;
	bool bHadAnyBreak = false;

	MStringW out;
	out.reserve(ret.size());

	const size_t n = ret.size();
	for (size_t i = 0; i < n; ++i)
	{
		WCHAR ch = ret[i];
		if (ch == WCHAR('\r'))
		{
			bHadAnyBreak = true;
			if (i + 1 < n && ret[i + 1] == WCHAR('\n'))
			{
				bHadCRLF = true;
				++i;
			}
			out += WIDE("\r\n");
		}
		else if (ch == WCHAR('\n'))
		{
			bHadAnyBreak = true;
			out += WIDE("\r\n");
		}
		else
		{
			out += ch;
		}
	}

	ret.swap(out);

	if (pType)
	{
		if (bHadCRLF)
			pType->nNewLine = MNEWLINE_CRLF;
		else if (bHadAnyBreak)
			pType->nNewLine = MNEWLINE_LF;
		else
			pType->nNewLine = MNEWLINE_UNKNOWN;
	}
}

inline MStringW
mstr_from_bin(const void *bin, size_t len, MTextType *pType = nullptr)
{
	MStringW ret;

	if (bin == nullptr || len == 0)
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
		ret.assign((const WCHAR *)((const char *)bin + 2), (len - 2) / sizeof(WCHAR));
	}
	else if (len >= 2 && std::memcmp(bin, "\xFE\xFF", 2) == 0)
	{
		// UTF-16 BE
		if (pType)
		{
			pType->nEncoding = MTENC_UNICODE_BE;
			pType->bHasBOM = true;
		}
		ret.assign((const WCHAR *)((const char *)bin + 2), (len - 2) / sizeof(WCHAR));
		mbin_swap_endian(&ret[0], ret.size() * sizeof(WCHAR));
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
			ret = MAnsiToWide(CP_UTF8, &pch[3], int(len - 3));
		}
		else if (mstr_is_text_ascii((const char *)bin, len))
		{
			// ASCII
			if (pType)
			{
				pType->nEncoding = MTENC_ASCII;
				pType->bHasBOM = false;
			}
			ret = MAnsiToWide(CP_ACP, pch, int(len));
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
			ret = MAnsiToWide(CP_ACP, pch, int(len));
		}
	}

	if (!pType || pType->nNewLine != MNEWLINE_NOCHANGE)
	{
		mstr_normalize_newlines_to_crlf(ret, pType);
	}

	return ret;
}

inline MStringW
mstr_from_bin(const std::string& bin, MTextType *pType = nullptr)
{
	return mstr_from_bin(&bin[0], bin.size(), pType);
}

inline void
mstr_convert_newlines(MStringW& str, const WCHAR *newline, size_t newline_len)
{
	MStringW out;
	out.reserve(str.size());

	const size_t n = str.size();
	for (size_t i = 0; i < n; ++i)
	{
		WCHAR ch = str[i];
		if (ch == WCHAR('\r'))
		{
			if (i + 1 < n && str[i + 1] == WCHAR('\n'))
				++i;
			out.append(newline, newline_len);
		}
		else if (ch == WCHAR('\n'))
		{
			out.append(newline, newline_len);
		}
		else
		{
			out += ch;
		}
	}

	str.swap(out);
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
		mstr_convert_newlines(str2, WIDE("\r\n"), 2);
		break;
	case MNEWLINE_LF:
		mstr_convert_newlines(str2, WIDE("\n"), 1);
		break;
	case MNEWLINE_CR:
		mstr_convert_newlines(str2, WIDE("\r"), 1);
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
