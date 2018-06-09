// DlgInitRes.hpp --- DLGINIT Resource
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

#ifndef DLGINIT_RES_HPP_
#define DLGINIT_RES_HPP_

#include <windows.h>
#include <cassert>
#include <vector>

#include "MByteStreamEx.hpp"
#include "MString.hpp"
#include "ConstantsDB.hpp"
#include "DlgInit.h"

//////////////////////////////////////////////////////////////////////////////

struct DLGINIT_ENTRY
{
    WCHAR sz0[64];
    WCHAR sz1[64];
    WCHAR sz2[256];
};

struct DlgInitEntry
{
    WORD        wCtrl;
    WORD        wMsg;
    MStringA    strText;
};

//////////////////////////////////////////////////////////////////////////////

class DlgInitRes
{
public:
    typedef DlgInitEntry                entry_type;
    typedef std::vector<entry_type>     entries_type;

    DlgInitRes() { }

    bool LoadFromStream(const MByteStreamEx& stream)
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

    bool SaveToStream(MByteStreamEx& stream) const
    {
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            const DlgInitEntry& entry = m_entries[i];
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

    MStringW Dump(const MIdOrString& id_or_str) const
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

        ret += L" DLGINIT\r\n";
        ret += L"{\r\n";

        if (m_entries.size() == 0)
        {
            ret += L"    0\r\n";
            ret += L"}\r\n";
            return ret;
        }

        entries_type::const_iterator it, end = m_entries.end();
        for (it = m_entries.begin(); it != end; ++it)
        {
            ret += L"    ";
            ret += g_db.GetCtrlOrCmdName(it->wCtrl);
            ret += L", ";

// Win16 messages
#define WIN16_LB_ADDSTRING  0x0401
#define WIN16_CB_ADDSTRING  0x0403
#define AFX_CB_ADDSTRING    0x1234

            switch (it->wMsg)
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
                ret += mstr_hex_word(it->wMsg);
            }

            ret += L", ";
            ret += mstr_hex_word(it->strText.size() + 9);
            ret += L", 0";

            const WORD *pw = reinterpret_cast<const WORD *>(it->strText.c_str());
            size_t len = (it->strText.size() + 1) / 2;
            for (size_t k = 0; k < len; ++k)
            {
                ret += L", ";
                ret += mstr_hex_word(pw[k]);
            }
            if (it->strText.size() % 2 == 0)
            {
                ret += L", \"\\000\"";
            }
            ret += L", ";

            ret += L"\r\n";
        }

        ret += L"    0\r\n";
        ret += L"}\r\n";

        return ret;
    }

    entries_type& entries()
    {
        return m_entries;
    }
    const entries_type& entries() const
    {
        return m_entries;
    }

    bool empty() const
    {
        return size() == 0;
    }
    size_t size() const
    {
        return m_entries.size();
    }
    entry_type& operator[](size_t i)
    {
        return m_entries[i];
    }
    const entry_type& operator[](size_t i) const
    {
        return m_entries[i];
    }
    void push_back(const DlgInitEntry& entry)
    {
        m_entries.push_back(entry);
    }
    void clear()
    {
        m_entries.clear();
    }

    std::vector<BYTE> data() const
    {
        MByteStreamEx stream;
        SaveToStream(stream);
        return stream.data();
    }

    void ReplaceInvalid(WORD wCtrl)
    {
        for (size_t i = 0; i < size(); ++i)
        {
            if (m_entries[i].wCtrl == WORD(-1))
            {
                m_entries[i].wCtrl = wCtrl;
            }
        }
    }
    void ReplaceMsg(WORD wCtrl, WORD wMsg)
    {
        for (size_t i = 0; i < size(); ++i)
        {
            if (m_entries[i].wCtrl == wCtrl)
            {
                m_entries[i].wMsg = wMsg;
            }
        }
    }
    void Filter(DlgInitRes& destination, WORD wCtrl)
    {
        for (size_t i = 0; i < size(); ++i)
        {
            if (m_entries[i].wCtrl == wCtrl)
            {
                destination.push_back(m_entries[i]);
            }
        }
    }
    void Erase(WORD wCtrl)
    {
        for (size_t i = 0; i < size(); ++i)
        {
            if (m_entries[i].wCtrl == wCtrl)
            {
                m_entries.erase(m_entries.begin() + i);
                --i;
            }
        }
    }
    void EraseInvalid()
    {
        Erase(WORD(-1));
    }
    void Union(const DlgInitRes& another)
    {
        for (size_t i = 0; i < another.size(); ++i)
        {
            push_back(another[i]);
        }
    }

protected:
    entries_type    m_entries;
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef DLGINIT_RES_HPP_
