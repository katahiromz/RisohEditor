// DlgInitRes.cpp --- DLGINIT Resource
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#include "DlgInitRes.hpp"
#include "ConstantsDB.hpp"

bool DlgInitRes::LoadFromStream(const MByteStreamEx& stream)
{
    m_entries.clear();

    WORD wCtrl;
    while (stream.ReadWord(wCtrl) && wCtrl)
    {
        DlgInitEntry entry;
        entry.wCtrl = wCtrl;

        int32_t dwLen;
        if (!stream.ReadWord(entry.wMsg) || !stream.ReadDword(dwLen))
            return false;

        assert(dwLen);
        if (dwLen)
        {
            entry.strText.resize(dwLen - 1);
            if (!stream.ReadData(&entry.strText[0], dwLen))
                return false;
        }

        m_entries.push_back(entry);
    }

    return true;
}

bool DlgInitRes::SaveToStream(MByteStreamEx& stream) const
{
    for (auto& entry : m_entries)
    {
        DWORD dwLen = DWORD(entry.strText.size() + 1);
        if (!stream.WriteWord(entry.wCtrl) ||
            !stream.WriteWord(entry.wMsg) ||
            !stream.WriteDword(dwLen) ||
            !stream.WriteData(entry.strText.c_str(), dwLen))
        {
            return false;
        }
    }

    return stream.WriteWord(0);
}

MStringW DlgInitRes::Dump(const MIdOrString& id_or_str) const
{
    MStringW ret;

    if (id_or_str.is_str())
    {
        ret += id_or_str.str();
    }
    else
    {
        ret += g_db.GetNameOfResID(IDTYPE_DIALOG, id_or_str.m_id);
    }

    ret += L" 240\r\n";
    if (g_settings.bUseBeginEnd)
        ret += L"BEGIN\r\n";
    else
        ret += L"{\r\n";

    if (m_entries.size() == 0)
    {
        ret += L"    0\r\n";
        if (g_settings.bUseBeginEnd)
            ret += L"END\r\n";
        else
            ret += L"}\r\n";
        return ret;
    }

    for (auto& entry : m_entries)
    {
        ret += L"    ";
        ret += g_db.GetCtrlOrCmdName(entry.wCtrl);
        ret += L", ";

// Win16 messages
#define WIN16_LB_ADDSTRING  0x0401
#define WIN16_CB_ADDSTRING  0x0403
#define AFX_CB_ADDSTRING    0x1234

        switch (entry.wMsg)
        {
        case WIN16_LB_ADDSTRING:
        case LB_ADDSTRING:
            ret += mstr_hex_word(LB_ADDSTRING);
            break;
        case WIN16_CB_ADDSTRING:
        case CB_ADDSTRING:
            ret += mstr_hex_word(CB_ADDSTRING);
            break;
        case AFX_CB_ADDSTRING:
        case CBEM_INSERTITEM:
            ret += mstr_hex_word(CBEM_INSERTITEM);
            break;
        default:
            ret += mstr_hex_word(entry.wMsg);
        }

        ret += L", ";
        ret += mstr_hex_word(WORD(entry.strText.size() + 1));
        ret += L", 0";

        auto pw = reinterpret_cast<const UNALIGNED WORD *>(entry.strText.c_str());
        size_t len = (entry.strText.size() + 1) / 2;
        for (size_t k = 0; k < len; ++k)
        {
            ret += L", ";
            ret += mstr_hex_word(pw[k]);
        }
        if (entry.strText.size() % 2 == 0)
        {
            ret += L", \"\\000\"";
        }
        ret += L", ";

        ret += L"\r\n";
    }

    ret += L"    0\r\n";
    if (g_settings.bUseBeginEnd)
        ret += L"END\r\n";
    else
        ret += L"}\r\n";

    return ret;
}

std::vector<BYTE> DlgInitRes::data() const
{
    MByteStreamEx stream;
    SaveToStream(stream);
    return stream.data();
}
