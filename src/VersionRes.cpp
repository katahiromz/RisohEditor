// VersionRes.cpp --- Version Resources
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

#include "VersionRes.hpp"
#include "ConstantsDB.hpp"

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
        if (var.head.wType == 1)
            dwSize *= 2;
        var.value.resize(dwSize);
        if (!stream.ReadData(&var.value[0], dwSize))
            return false;
    }
    stream.ReadDwordAlignment();

    while (stream.pos() < pos1)
    {
        VarsFromStream(var.vars, stream);
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
    ret += mstr_quote(value.key);

    if (value.value.size() >= 2)
    {
        if (wType == 0)
        {
            LPWORD pw = LPWORD(&value.value[0]);
            WCHAR buf[MAX_PATH];
            for (size_t i = 0; i < value.value.size(); i += 2)
            {
                StringCchPrintfW(buf, _countof(buf), L", 0x%04X", *pw++);
                ret += buf;
            }
        }
        else
        {
            WCHAR *pch = reinterpret_cast<WCHAR *>(&value.value[0]);
            MStringW str(pch, value.value.size() / 2);
            ret += L", ";
            ret += mstr_quote(str);
        }
    }
    else
    {
        ret += L", \"\"";
    }

    ret += L"\r\n";
    return ret;
}

MStringW
VersionRes::DumpBlock(const Var& var, int depth) const
{
    MStringW ret;

    ret += MStringW(depth * 4, L' ');
    ret += L"BLOCK \"";
    ret += var.key;
    ret += L"\"\r\n";
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
    MStringW ret;
    WCHAR line[MAX_PATH];

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

    StringCchPrintfW(line, _countof(line), L"FILEOS          0x%04lX\r\n", m_fixed.dwFileOS);
    ret += line;

    StringCchPrintfW(line, _countof(line), L"FILETYPE        0x%X\r\n", m_fixed.dwFileType);
    ret += line;

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
