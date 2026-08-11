#pragma once

#include "MString.hpp"
#include "MTextToText.hpp"

// mstr_... functions
// mbin_... functions

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
