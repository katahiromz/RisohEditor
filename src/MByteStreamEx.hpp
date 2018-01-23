// MByteStream.hpp -- MZC4 byte stream                          -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MBYTESTREAMEX_HPP_
#define MZC4_MBYTESTREAMEX_HPP_     3       /* Version 3 */

class MByteStreamEx;

////////////////////////////////////////////////////////////////////////////

#include "MByteStream.hpp"

////////////////////////////////////////////////////////////////////////////

class MByteStreamEx : public MByteStream
{
public:
    MByteStreamEx()
    {
    }

    MByteStreamEx(size_t size) : MByteStream(size)
    {
        m_data.resize(size);
    }

    MByteStreamEx(const data_type& data) : MByteStream(data)
    {
    }

    MByteStreamEx(const void *ptr, size_t size) : MByteStream(ptr, size)
    {
    }

    BOOL ReadID(MIdOrString& id_or_str) const
    {
        WORD w;
        if (!PeekWord(w))
            return FALSE;

        if (w == 0xFFFF)
        {
            ReadWord(w);
            if (ReadWord(w))
            {
                id_or_str = w;
                return TRUE;
            }
            return FALSE;
        }

        id_or_str.m_id = 0;
        return ReadSz(id_or_str.m_str);
    }

    BOOL WriteID(const MIdOrString& id_or_str)
    {
        if (id_or_str.is_str())
        {
            return WriteSz(id_or_str.m_str);
        }
        return WriteWord(0xFFFF) && WriteWord(id_or_str.m_id);
    }

    BOOL ReadString(MIdOrString& id_or_str) const
    {
        WORD w;
        if (!PeekWord(w))
            return FALSE;

        if (w == 0)
        {
            ReadWord(w);
            id_or_str.clear();
            return TRUE;
        }

        if (w == 0xFFFF)
        {
            WORD w;
            if (!ReadWord(w) || !ReadWord(w))
                return FALSE;
            id_or_str = w;
            return TRUE;
        }

        id_or_str.m_id = 0;
        return ReadSz(id_or_str.m_str);
    }

    BOOL WriteString(LPCWSTR psz)
    {
        if (psz == NULL)
        {
            return WriteWord(0);
        }
        if (IS_INTRESOURCE(psz))
        {
            WORD aw[2];
            aw[0] = 0xFFFF;
            aw[1] = LOWORD(psz);
            return WriteRaw(aw);
        }
        return WriteData(psz, (lstrlenW(psz) + 1) * sizeof(WCHAR));
    }
};

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MBYTESTREAMEX_HPP_
