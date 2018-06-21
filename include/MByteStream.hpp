// MByteStream.hpp -- MZC4 byte stream                          -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MBYTESTREAM_HPP_
#define MZC4_MBYTESTREAM_HPP_       9       /* Version 9 */

class MByteStream;

//////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32) && !defined(WONVER)
    #ifndef _INC_WINDOWS
        #include <windows.h>
    #endif
#else
    #include "wondef.h"         // Wonders API
    #define _unlink     unlink
#endif

#if __cplusplus >= 201103L          /* C++11 */
    #include <cstdint>
#else
    #include "pstdint.h"
#endif

#ifndef _WIN32
    #include <unistd.h>     // for unlink
#endif

#include "MString.hpp"

#include <cstdlib>          // C standard library
#include <cstdio>           // C standard I/O
#include <cstring>          // C string
#include <cassert>          // assert
#include <vector>           // for std::vector
#include <string>           // for std::string and std::wstring

//////////////////////////////////////////////////////////////////////////////

class MByteStream
{
public:
    typedef std::vector<uint8_t> data_type;
    typedef std::size_t size_type;

    MByteStream() : m_pos(0)
    {
    }

    MByteStream(size_t size) : m_pos(0)
    {
        m_data.resize(size);
    }

    MByteStream(const data_type& data) : m_pos(0), m_data(data)
    {
    }

    MByteStream(const void *ptr, size_t size) :
        m_pos(0), m_data((const uint8_t *)ptr, (const uint8_t *)ptr + size)
    {
    }

    virtual ~MByteStream()
    {
    }

    void assign(const void *ptr, size_t size)
    {
        m_data.assign((const uint8_t *)ptr, (const uint8_t *)ptr + size);
    }
    void assign(const void *ptr1, const void *ptr2)
    {
        m_data.assign((const uint8_t *)ptr1, (const uint8_t *)ptr2);
    }
    void assign(const data_type& data)
    {
        m_pos = 0;
        m_data = data;
    }

    void clear()
    {
        m_pos = 0;
        m_data.clear();
    }

    data_type& data()
    {
        return m_data;
    }

    const data_type& data() const
    {
        return m_data;
    }

    void *ptr(size_t index = 0)
    {
        return &m_data[index];
    }
    const void *ptr(size_t index = 0) const
    {
        return &m_data[index];
    }

    size_t size() const
    {
        return m_data.size();
    }

    size_t remainder() const
    {
        if (m_pos <= size())
            return size() - m_pos;
        return 0;
    }

    size_t pos() const
    {
        return m_pos;
    }
    void pos(size_t pos_) const
    {
        m_pos = pos_;
    }

    bool seek(int16_t delta) const
    {
        if (delta > 0)
        {
            if (m_pos + delta <= size())
            {
                m_pos += delta;
                return true;
            }
        }
        else if (delta < 0)
        {
            if (m_pos + delta >= 0)
            {
                m_pos += delta;
                return true;
            }
        }
        return false;
    }

    template <typename T>
    bool WriteRaw(const T& data)
    {
        return WriteData(&data, sizeof(T));
    }

    bool WriteData(const void *data, size_t nSize)
    {
        if (data && nSize)
        {
            size_t old_size = size();
            m_data.resize(old_size + nSize);
            memcpy(&m_data[old_size], data, nSize);
        }
        return true;
    }

    bool WriteByte(uint8_t value)
    {
        uint8_t b = value;
        return WriteData(&b, sizeof(b));
    }

    bool WriteWord(uint16_t value)
    {
        uint16_t w = value;
        return WriteData(&w, sizeof(w));
    }

    void WriteWordAlignment()
    {
        if (m_data.size() & 1)
            m_data.resize(m_data.size() + 1);
    }

    bool WriteDword(uint32_t value)
    {
        uint32_t dw = value;
        return WriteData(&dw, sizeof(dw));
    }

    void WriteDwordAlignment()
    {
        size_t n = (m_data.size() & 3);
        if (n)
        {
            m_data.resize(m_data.size() + (4 - n));
        }
    }

    template <typename T>
    bool ReadRaw(T& value) const
    {
        return ReadData(&value, sizeof(T));
    }

    bool ReadData(void *data, size_t nSize) const
    {
        if (m_pos + nSize <= size())
        {
            if (nSize)
            {
                memcpy(data, &m_data[m_pos], nSize);
                m_pos += nSize;
            }
            return true;
        }
        return false;
    }

    bool ReadByte(uint8_t& b) const
    {
        return ReadData(&b, sizeof(b));
    }

    bool ReadByte(char& b) const
    {
        return ReadData(&b, sizeof(b));
    }

    bool ReadWord(uint16_t& w) const
    {
        return ReadData(&w, sizeof(w));
    }

    bool ReadWord(int16_t& w) const
    {
        return ReadData(&w, sizeof(w));
    }

    bool ReadDword(uint32_t& dw) const
    {
        return ReadData(&dw, sizeof(dw));
    }

    bool ReadDword(int32_t& n) const
    {
        return ReadData(&n, sizeof(n));
    }

    bool PeekWord(uint16_t& w) const
    {
        size_t nSize = sizeof(uint16_t);
        if (m_pos + nSize <= size())
        {
            memcpy(&w, &m_data[m_pos], nSize);
            return true;
        }
        return false;
    }

    bool PeekByte(uint8_t& b) const
    {
        size_t nSize = sizeof(uint8_t);
        if (m_pos + nSize <= size())
        {
            memcpy(&b, &m_data[m_pos], nSize);
            return true;
        }
        return false;
    }

    void ReadWordAlignment() const
    {
        if (m_pos & 1)
            ++m_pos;
    }

    void ReadDwordAlignment() const
    {
        size_t mod = (m_pos & 3);
        if (mod)
            m_pos += 4 - mod;
    }

    bool LoadFromFile(const TCHAR *FileName)
    {
        m_pos = 0;
        m_data.clear();

        FILE *fp;
#ifdef UNICODE
        fp = _wfopen(FileName, L"rb");
#else
        fp = fopen(FileName, "rb");
#endif
        if (!fp)
            return false;

        bool ok = true;
        uint8_t buf[512];
        for (;;)
        {
            size_t size = fread(buf, 1, 512, fp);
            if (size == 0)
            {
                if (ferror(fp))
                    ok = false;
                break;
            }

            m_data.insert(m_data.end(), &buf[0], &buf[size]);
        }

        fclose(fp);

        return ok;
    }

    bool SaveToFile(const TCHAR *FileName) const
    {
        FILE *fp;
#ifdef UNICODE
        fp = _wfopen(FileName, L"wb");
#else
        fp = fopen(FileName, "wb");
#endif
        if (!fp)
            return false;

        size_t n = fwrite(&m_data[0], m_data.size(), 1, fp);
        fclose(fp);

        if (!n)
        {
#ifdef UNICODE
            _wremove(FileName);
#else
            _unlink(FileName);
#endif
        }

        return n != 0;
    }

    uint8_t& operator[](size_t index)
    {
        return m_data[index];
    }
    const uint8_t& operator[](size_t index) const
    {
        return m_data[index];
    }

    bool ReadSz(MStringA& str) const
    {
        str.clear();
        uint8_t b;
        while (ReadByte(b))
        {
            if (b == 0)
                return true;
            str += b;
        }
        return false;
    }
    bool WriteSz(const MStringA& str)
    {
        return WriteData(&str[0], (str.size() + 1) * sizeof(char));
    }

    bool ReadSz(MStringW& str) const
    {
        str.clear();
        uint16_t w;
        while (ReadWord(w))
        {
            if (w == 0)
                return true;
            str += (WCHAR)w;
        }
        return false;
    }
    bool WriteSz(const MStringW& str)
    {
        return WriteData(&str[0], (str.size() + 1) * sizeof(WCHAR));
    }

protected:
    mutable size_type   m_pos;
    data_type           m_data;
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MBYTESTREAM_HPP_
