// AccelRes.hpp --- Accelerator-Table Resource
//////////////////////////////////////////////////////////////////////////////

#ifndef ACCEL_RES_HPP_
#define ACCEL_RES_HPP_

#include <windows.h>
#include <cassert>
#include <vector>

#include "MByteStreamEx.hpp"
#include "MString.hpp"
#include "ConstantsDB.hpp"

//////////////////////////////////////////////////////////////////////////////

struct ACCEL_ENTRY
{
    WCHAR sz0[128];
    WCHAR sz1[16];
    WCHAR sz2[128];
};

struct AccelTableEntry
{
    WORD   fFlags;
    WORD   wAscii;
    WORD   wId;
    WORD   padding;
};

//////////////////////////////////////////////////////////////////////////////

class AccelRes
{
public:
    typedef AccelTableEntry             entry_type;
    typedef std::vector<entry_type>     entries_type;
    ConstantsDB& m_ConstantsDB;
    AccelRes(ConstantsDB& db) : m_ConstantsDB(db) { }

    BOOL LoadFromStream(const MByteStreamEx& stream)
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

    void Update()
    {
        size_t i, count = m_entries.size();
        for (i = 0; i < count; ++i)
        {
            entry_type& entry = m_entries[i];

            if (i + 1 == count)
                entry.fFlags |= 0x80;
            else
                entry.fFlags &= ~0x80;
        }
    }

    std::vector<BYTE> data() const
    {
        DWORD size = m_entries.size() * sizeof(entry_type);
        const BYTE *pb = (const BYTE *)&m_entries[0];
        return std::vector<BYTE>(pb, pb + size);
    }

    std::wstring Dump(const MIdOrString &id_or_str) const
    {
        std::wstring ret;

        if (id_or_str.m_ID == 0)
        {
            ret += id_or_str.str();
        }
        else
        {
            ret += m_ConstantsDB.GetNameOfResID(IDTYPE_ACCEL, id_or_str.m_ID);
        }
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
            if (VIRTKEY)
            {
                ret += m_ConstantsDB.GetName(L"VIRTUALKEYS", it->wAscii);
            }
            else
            {
                std::string str;
                str += (char)it->wAscii;
                ret += MAnsiToWide(mstr_quote(str));
            }
            ret += L", ";
            if (0)
            {
                ret += mstr_dec(it->wId);
            }
            else
            {
                ret += m_ConstantsDB.GetNameOfResID(IDTYPE_COMMAND, it->wId);
            }

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
            else
                ret += L", ASCII";

            ret += L"\r\n";
        }

        ret += L"}\r\n";
        return ret;
    }

    entries_type& Entries()
    {
        return m_entries;
    }
    const entries_type& Entries() const
    {
        return m_entries;
    }

protected:
    entries_type    m_entries;
};

//////////////////////////////////////////////////////////////////////////////

inline std::wstring GetKeyFlags(WORD fFlags)
{
    std::wstring str;

    if (fFlags & FVIRTKEY)
        str += L"V ";

    if (fFlags & FNOINVERT)
        str += L"N ";

    if (fFlags & FCONTROL)
        str += L"C ";

    if (fFlags & FSHIFT)
        str += L"S ";

    if (fFlags & FALT)
        str += L"A ";

    return str;
}

inline void SetKeyFlags(WORD& fFlags, const std::wstring& str)
{
    fFlags = 0;
    if (str.find(L"V") != std::wstring::npos)
        fFlags |= FVIRTKEY;
    if (str.find(L"N") != std::wstring::npos)
        fFlags |= FNOINVERT;
    if (str.find(L"C") != std::wstring::npos)
        fFlags |= FCONTROL;
    if (str.find(L"S") != std::wstring::npos)
        fFlags |= FSHIFT;
    if (str.find(L"A") != std::wstring::npos)
        fFlags |= FALT;
}

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef ACCEL_RES_HPP_

//////////////////////////////////////////////////////////////////////////////
