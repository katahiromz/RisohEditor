// VersionRes.hpp --- Version Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
