// VersionRes.hpp --- Version Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// License: GPL-3 or later

#pragma once

#ifndef _INC_WINDOWS
	#include <windows.h>
#endif
#include <cassert>
#include <vector>
#include <map>

#include "MByteStreamEx.hpp"
#include "MString.hpp"
#include "MIdOrString.hpp"

//////////////////////////////////////////////////////////////////////////////

typedef struct VarInfoHead
{
	WORD    wLength;
	WORD    wValueLength;
	WORD    wType;
} VarInfoHead;

struct Var
{
	VarInfoHead             head;
	MStringW                key;
	std::vector<BYTE>       value;
	std::vector<BYTE>       children;
	std::vector<Var>        vars;
};
typedef std::vector<Var> Vars;

//////////////////////////////////////////////////////////////////////////////

class VersionRes
{
public:
	VersionRes() = default;

	bool VarsFromStream(Vars& vars, const MByteStreamEx& stream);
	bool LoadFromData(const std::vector<BYTE>& data);
	MStringW Dump(const MIdOrString& name) const;

protected:
	std::vector<Var>        m_vars;
	VS_FIXEDFILEINFO        m_fixed;

	MStringW DumpValue(WORD wType, const Var& value, int depth = 0) const;
	MStringW DumpBlock(const Var& var, int depth = 0) const;
};
