// VersionRes.cpp --- Version Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#include "VersionRes.hpp"
#ifndef NO_CONSTANTS_DB
	#include "ConstantsDB.hpp"
	#define NO_DB 0
#else
	#define NO_DB 1
#endif

bool VersionRes::VarsFromStream(Vars& vars, const MByteStreamEx& stream)
{
	Var var;

	stream.ReadDwordAlignment();

	size_t pos0 = stream.pos();

	if (!stream.ReadRaw(var.head) || !stream.ReadSz(var.key))
		return false;

	size_t pos1 = pos0 + var.head.wLength;
	stream.ReadDwordAlignment();

	if (var.head.wValueLength)
	{
		DWORD dwSize = var.head.wValueLength;
		if (dwSize > 0x7FFFFFFF)
			return false;
		if (var.head.wType == 1)
			dwSize *= 2;
		if (dwSize > 0x7FFFFFFF)
			return false;
		var.value.resize(dwSize);
		if (!stream.ReadData(&var.value[0], dwSize))
			return false;
	}
	stream.ReadDwordAlignment();

	while (stream.pos() < pos1)
	{
		if (!VarsFromStream(var.vars, stream))
			return false;
	}

	vars.push_back(var);

	return true;
}

bool VersionRes::LoadFromData(const std::vector<BYTE>& data)
{
	ZeroMemory(&m_fixed, sizeof(m_fixed));

	MByteStreamEx stream(data);
	if (!VarsFromStream(m_vars, stream))
		return false;

	if (m_vars.size() != 1)
		return false;

	Var& var = m_vars[0];
	if (var.key != L"VS_VERSION_INFO")
		return false;

	if (var.value.size() == sizeof(VS_FIXEDFILEINFO))
	{
		CopyMemory(&m_fixed, &var.value[0], var.value.size());
	}

	return true;
}

MStringW
VersionRes::DumpValue(WORD wType, const Var& value, int depth) const
{
	MStringW ret = MStringW(depth * 4, L' ');
	ret += L"VALUE ";
	ret += mstr_quote_with_wrap(value.key);

	if (value.value.size() >= 2)
	{
		if (wType == 0)
		{
			const WORD *pw = reinterpret_cast<const WORD *>(&value.value[0]);
			WCHAR buf[MAX_PATH];
			size_t cWords = value.value.size() / sizeof(WORD);
			for (size_t i = 0; i < cWords; ++i)
			{
				StringCchPrintfW(buf, _countof(buf), L", 0x%04X", *pw++);
				ret += buf;
			}
		}
		else
		{
			const WCHAR *pch = reinterpret_cast<const WCHAR *>(&value.value[0]);
			MStringW str(pch, value.value.size() / 2);
			if (str.size() && str[str.size() - 1] == 0)
				str.resize(str.size() - 1);
			ret += L", ";
			ret += mstr_quote_with_wrap(str);
		}
	}
	else
	{
		ret += L", \"\"";
	}

	ret += L"\r\n";
	return ret;
}

static inline BOOL is_8digit_hex(const MStringW& str)
{
	if (str.size() != 8)
		return FALSE;
	for (auto& ch : str)
	{
		if (!mchr_is_xdigit(ch))
			return FALSE;
	}
	return TRUE;
}

static inline PCWSTR GetCharSet2String(WORD wCharSet)
{
#define DEFINE_CODEPAGE2(index, value, name) if (wCharSet == value) return name;
#include "codepages2.h"
#undef DEFINE_CODEPAGE2
	return L"(Unknown codepage)";
}

MStringW
VersionRes::DumpBlock(const Var& var, int depth) const
{
	MStringW ret;

	ret += MStringW(depth * 4, L' ');
	ret += L"BLOCK \"";
	ret += var.key;
#ifndef NO_CONSTANTS_DB
	if (is_8digit_hex(var.key))
	{
		ret += L"\" // ";
		LANGID wLangID = (LANGID)std::wcstoul(var.key.substr(0, 4).c_str(), nullptr, 16);
		WORD wCharSet = (WORD)std::wcstoul(var.key.substr(4, 4).c_str(), nullptr, 16);

		MString lang_name = g_db.GetLangName(wLangID);
		ret += lang_name;
		ret += L", ";
		ret += GetCharSet2String(wCharSet);
		ret += L"\r\n";
	}
	else
#endif
	{
		ret += L"\"\r\n";
	}
	ret += MStringW(depth * 4, L' ');
	if (g_settings.bUseBeginEnd)
		ret += L"BEGIN\r\n";
	else
		ret += L"{\r\n";

	for (auto& item : var.vars)
	{
		if (var.key == L"StringFileInfo")
		{
			ret += DumpBlock(item, depth + 1);
		}
		else
		{
			ret += DumpValue(item.head.wType, item, depth + 1);
		}
	}

	ret += MStringW(depth * 4, L' ');
	if (g_settings.bUseBeginEnd)
		ret += L"END\r\n";
	else
		ret += L"}\r\n";

	return ret;
}

MStringW
VersionRes::Dump(const MIdOrString& name) const
{
	MStringW ret, str;
	WCHAR line[MAX_PATH];
	DWORD dwValue;

	ret += name.str();
	ret += L" VERSIONINFO\r\n";

	StringCchPrintfW(line, _countof(line),
		L"FILEVERSION     %u, %u, %u, %u\r\n",
		HIWORD(m_fixed.dwFileVersionMS),
		LOWORD(m_fixed.dwFileVersionMS),
		HIWORD(m_fixed.dwFileVersionLS),
		LOWORD(m_fixed.dwFileVersionLS));
	ret += line;

	StringCchPrintfW(line, _countof(line),
		L"PRODUCTVERSION  %u, %u, %u, %u\r\n",
		HIWORD(m_fixed.dwProductVersionMS),
		LOWORD(m_fixed.dwProductVersionMS),
		HIWORD(m_fixed.dwProductVersionLS),
		LOWORD(m_fixed.dwProductVersionLS));
	ret += line;

	// FILEOS
	dwValue = m_fixed.dwFileOS;
	if (g_settings.bHideID || NO_DB)
	{
		StringCchPrintfW(line, _countof(line), L"FILEOS          0x%lX\r\n", dwValue);
	}
#ifndef NO_CONSTANTS_DB
	else
	{
		str = g_db.DumpValue(L"VOS_", dwValue, true);
		StringCchPrintfW(line, _countof(line), L"FILEOS          %s\r\n", str.c_str());
	}
#endif
	ret += line;

	// FILETYPE
	dwValue = m_fixed.dwFileType;
	if (g_settings.bHideID || NO_DB)
	{
		StringCchPrintfW(line, _countof(line), L"FILETYPE        0x%lX\r\n", dwValue);
	}
#ifndef NO_CONSTANTS_DB
	else
	{
		str = g_db.DumpValue(L"VFT_", dwValue, true);
		StringCchPrintfW(line, _countof(line), L"FILETYPE        %s\r\n", str.c_str());
	}
#endif
	ret += line;

	// FILESUBTYPE
	dwValue = m_fixed.dwFileSubtype;
	if (dwValue)
	{
		if (g_settings.bHideID || NO_DB)
		{
			StringCchPrintfW(line, _countof(line), L"FILESUBTYPE     0x%lX\r\n", dwValue);
		}
#ifndef NO_CONSTANTS_DB
		else
		{
			if (m_fixed.dwFileType == VFT_DRV)
				str = g_db.DumpValue(L"VFT2_DRV_", dwValue, true);
			else if (m_fixed.dwFileType == VFT_FONT)
				str = g_db.DumpValue(L"VFT2_FONT_", dwValue, true);
			else
				str = g_db.DumpValue(L"VFT2_others", dwValue, true);
			StringCchPrintfW(line, _countof(line), L"FILESUBTYPE     %s\r\n", str.c_str());
		}
#endif
		ret += line;
	}

	if (m_fixed.dwFileFlagsMask | m_fixed.dwFileFlags)
	{
		// FILEFLAGSMASK
		dwValue = m_fixed.dwFileFlagsMask;
		if (g_settings.bHideID || NO_DB)
		{
			StringCchPrintfW(line, _countof(line), L"FILEFLAGSMASK   0x%lX\r\n", dwValue);
		}
#ifndef NO_CONSTANTS_DB
		else
		{
			str = g_db.DumpBitFieldOrZero(L"VS_FF_", dwValue);
			StringCchPrintfW(line, _countof(line), L"FILEFLAGSMASK   %s\r\n", str.c_str());
		}
#endif
		ret += line;

		// FILEFLAGS
		dwValue = m_fixed.dwFileFlags;
		if (g_settings.bHideID || NO_DB)
		{
			StringCchPrintfW(line, _countof(line), L"FILEFLAGS       0x%lX\r\n", dwValue);
		}
#ifndef NO_CONSTANTS_DB
		else
		{
			str = g_db.DumpBitFieldOrZero(L"VS_FF_", dwValue);
			StringCchPrintfW(line, _countof(line), L"FILEFLAGS       %s\r\n", str.c_str());
		}
#endif
		ret += line;
	}

	if (g_settings.bUseBeginEnd)
		ret += L"BEGIN\r\n";
	else
		ret += L"{\r\n";

	const std::vector<Var>& vars = m_vars[0].vars;
	for (auto& item : vars)
	{
		ret += DumpBlock(item, 1);
	}
	if (g_settings.bUseBeginEnd)
		ret += L"END\r\n";
	else
		ret += L"}\r\n";

	return ret;
}
