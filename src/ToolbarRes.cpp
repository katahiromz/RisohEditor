// ToolbarRes.cpp --- TOOLBAR Resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#include "ToolbarRes.hpp"
#include "ConstantsDB.hpp"

bool ToolbarRes::LoadFromStream(const MByteStreamEx& stream)
{
    m_items.clear();

    WORD wVer;
    if (!stream.PeekWord(wVer))
        return false;

    if (wVer == 1)
    {
        WORD wWidth, wHeight, wCount;
        if (!stream.ReadWord(wVer) ||
            !stream.ReadWord(wWidth) ||
            !stream.ReadWord(wHeight) ||
            !stream.ReadWord(wCount))
        {
            return false;
        }

        if (wVer != 1 || wWidth == 0 || wHeight == 0)
            return false;

        for (DWORD i = 0; i < wCount; ++i)
        {
            WORD item;
            if (!stream.ReadWord(item))
            {
                m_items.clear();
                return false;
            }
            m_items.push_back(item);
        }

        m_width = wWidth;
        m_height = wHeight;
        return true;
    }
    else if (wVer >= 3)
    {
        uint32_t wWidth, wHeight, dwCount;
        if (!stream.ReadDword(wWidth) ||
            !stream.ReadDword(wHeight) ||
            !stream.ReadDword(dwCount))
        {
            return false;
        }

        if (wWidth == 0 || wHeight == 0)
            return false;

        for (DWORD i = 0; i < dwCount; ++i)
        {
            uint32_t item;
            if (!stream.ReadDword(item))
            {
                m_items.clear();
                return false;
            }
            m_items.push_back(item);
        }

        m_width = wWidth;
        m_height = wHeight;
        return true;
    }
    else
    {
        return false;
    }
}

bool ToolbarRes::SaveToStream(MByteStreamEx& stream) const
{
    if (!stream.WriteWord(WORD(1)) ||
        !stream.WriteWord(WORD(m_width)) ||
        !stream.WriteWord(WORD(m_height)) ||
        !stream.WriteWord(WORD(m_items.size())))
    {
        return false;
    }

    for (auto item : m_items)
    {
        if (!stream.WriteWord(WORD(item)))
            return false;
    }

    return true;
}

MStringW ToolbarRes::Dump(const MIdOrString& id_or_str) const
{
    MStringW ret;

    if (id_or_str.is_str())
    {
        ret += id_or_str.str();
    }
    else
    {
        ret += g_db.GetNameOfResID(IDTYPE_BITMAP, id_or_str.m_id);
    }

    ret += L" TOOLBAR ";
    ret += std::to_wstring(m_width);
    ret += L", ";
    ret += std::to_wstring(m_height);
    ret += L"\r\n";

    if (g_settings.bUseBeginEnd)
        ret += L"BEGIN\r\n";
    else
        ret += L"{\r\n";

    for (auto item : m_items)
    {
        if (item == 0)
        {
            ret += L"    SEPARATOR\r\n";
        }
        else
        {
            ret += L"    BUTTON ";
            ret += g_db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, item, true);
            ret += L"\r\n";
        }
    }

    if (g_settings.bUseBeginEnd)
        ret += L"END\r\n";
    else
        ret += L"}\r\n";

    return ret;
}

std::vector<BYTE> ToolbarRes::data() const
{
    MByteStreamEx stream;
    SaveToStream(stream);
    return stream.data();
}
