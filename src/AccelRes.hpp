#ifndef ACCEL_RES_HPP_
#define ACCEL_RES_HPP_

#include <windows.h>
#include <cassert>
#include <vector>

#include "ByteStream.hpp"
#include "Text.hpp"

struct AccelTableEntry
{
    WORD   fFlags;
    WORD   wAscii;
    WORD   wId;
    WORD   padding;
};

class AccelRes
{
public:
    typedef AccelTableEntry             entry_type;
    typedef std::vector<entry_type>     entries_type;
    AccelRes() { }

    BOOL LoadFromStream(const ByteStream& stream)
    {
        m_entries.clear();
        if (stream.size() < sizeof(entry_type))
            return FALSE;

        entry_type entry;
        size_t i, count = stream.size() / sizeof(entry_type);
        for (i = 0; i < count; ++i)
        {
            if (!stream.ReadRaw(entry))
                return FALSE;

            m_entries.push_back(entry);

            if (entry.fFlags & 0x80)
                break;
        }

        return TRUE;
    }

    BOOL CreateSample()
    {
        static const entry_type entries[] =
        {
            { FVIRTKEY | FCONTROL, 'C', 1 },
            { FVIRTKEY | FCONTROL, 'V', 2 },
        };
        ByteStream stream(entries, sizeof(entries));
        return LoadFromStream(stream);
    }

    BOOL SaveToStream(ByteStream& stream) const
    {
        if (m_entries.empty())
            return TRUE;

        size_t i, count = m_entries.size();
        for (i = 0; i < count; ++i)
        {
            entry_type entry = m_entries[i];

            if (i + 1 == count)
                entry.fFlags |= 0x80;
            else
                entry.fFlags &= ~0x80;

            if (!stream.WriteRaw(entry))
                return FALSE;
        }

        return TRUE;
    }

    std::wstring Dump(const ID_OR_STRING &id_or_str) const
    {
        std::wstring ret;

        ret += id_or_str.wstr();
        ret += L" ";
        ret += L"ACCELERATORS\r\n";
        ret += L"{\r\n";

        entries_type::const_iterator it, end = m_entries.end();
        for (it = m_entries.begin(); it != end; ++it)
        {
            BOOL VIRTKEY = (it->fFlags & FVIRTKEY);
            BOOL NOINVERT = (it->fFlags & FNOINVERT);
            BOOL SHIFT = (it->fFlags & FSHIFT);
            BOOL CONTROL = (it->fFlags & FCONTROL);
            BOOL ALT = (it->fFlags & FALT);

            ret += L"    ";
            ret += virtkey(it->wAscii);
            ret += L", ";
            ret += deci(it->wId);

            if (NOINVERT)
                ret += L", NOINVERT";
            if (ALT)
                ret += L", ALT";
            if (CONTROL)
                ret += L", CONTROL";
            if (SHIFT)
                ret += L", SHIFT";
            if (VIRTKEY)
                ret += L", VIRTKEY";

            ret += L"\r\n";
        }

        ret += L"}\r\n";
        return ret;
    }

protected:
    entries_type    m_entries;
};

#endif  // ndef ACCEL_RES_HPP_
