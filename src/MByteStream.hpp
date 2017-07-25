// MByteStream.hpp -- MZC4 byte stream                          -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MBYTESTREAM_HPP_
#define MZC4_MBYTESTREAM_HPP_       2       /* Version 2 */

class MByteStream;

//////////////////////////////////////////////////////////////////////////////

#ifndef _INC_WINDOWS
    #include <windows.h>    // Win32API
#endif
#include <cassert>          // assert

#include <vector>           // for std::vector
#include <string>           // for std::string and std::wstring

//////////////////////////////////////////////////////////////////////////////

class MByteStream
{
public:
    typedef std::vector<BYTE> data_type;

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
        m_pos(0), m_data((const BYTE *)ptr, (const BYTE *)ptr + size)
    {
    }

    virtual ~MByteStream()
    {
    }

    void assign(const void *ptr, size_t size)
    {
        m_data.assign((const BYTE *)ptr, (const BYTE *)ptr + size);
    }

    void assign(const void *ptr1, const void *ptr2)
    {
        m_data.assign((const BYTE *)ptr1, (const BYTE *)ptr2);
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

    void *ptr(DWORD index = 0)
    {
        return &m_data[index];
    }
    const void *ptr(DWORD index = 0) const
    {
        return &m_data[index];
    }

    DWORD size() const
    {
        return DWORD(m_data.size());
    }

    DWORD remainder() const
    {
        if (m_pos <= size())
            return size() - m_pos;
        return 0;
    }

    DWORD pos() const
    {
        return m_pos;
    }
    void pos(DWORD pos_) const
    {
        m_pos = pos_;
    }

    bool seek(LONG delta) const
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
    BOOL WriteRaw(const T& data)
    {
        return WriteData(&data, sizeof(T));
    }

    BOOL WriteData(const void *data, DWORD Size)
    {
        if (data && Size)
        {
            DWORD old_size = size();
            m_data.resize(old_size + Size);
            memcpy(&m_data[old_size], data, Size);
        }
        return TRUE;
    }

    BOOL WriteByte(BYTE value)
    {
        BYTE b = value;
        return WriteData(&b, sizeof(b));
    }

    BOOL WriteWord(WORD value)
    {
        WORD w = value;
        return WriteData(&w, sizeof(w));
    }

    void WriteWordAlignment()
    {
        if (m_data.size() & 1)
            m_data.resize(m_data.size() + 1);
    }

    BOOL WriteDword(DWORD value)
    {
        DWORD dw = value;
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
    BOOL ReadRaw(T& value) const
    {
        return ReadData(&value, sizeof(T));
    }

    BOOL ReadData(void *data, DWORD Size) const
    {
        if (m_pos + Size <= size())
        {
            if (Size)
            {
                memcpy(data, &m_data[m_pos], Size);
                m_pos += Size;
            }
            return TRUE;
        }
        return FALSE;
    }

    BOOL ReadByte(BYTE& b) const
    {
        return ReadData(&b, sizeof(b));
    }

    BOOL ReadByte(char& b) const
    {
        return ReadData(&b, sizeof(b));
    }

    BOOL ReadWord(WORD& w) const
    {
        return ReadData(&w, sizeof(w));
    }

    BOOL ReadWord(short& w) const
    {
        return ReadData(&w, sizeof(w));
    }

    BOOL ReadDword(DWORD& dw) const
    {
        return ReadData(&dw, sizeof(dw));
    }

    BOOL ReadDword(LONG& n) const
    {
        return ReadData(&n, sizeof(n));
    }

    BOOL PeekWord(WORD& w) const
    {
        DWORD Size = sizeof(WORD);
        if (m_pos + Size <= size())
        {
            memcpy(&w, &m_data[m_pos], Size);
            return TRUE;
        }
        return FALSE;
    }

    BOOL PeekByte(BYTE& b) const
    {
        DWORD Size = sizeof(BYTE);
        if (m_pos + Size <= size())
        {
            memcpy(&b, &m_data[m_pos], Size);
            return TRUE;
        }
        return FALSE;
    }

    void ReadWordAlignment() const
    {
        if (m_pos & 1)
            ++m_pos;
    }

    void ReadDwordAlignment() const
    {
        DWORD mod = (m_pos & 3);
        if (mod)
            m_pos += 4 - mod;
    }

    BOOL LoadFromFile(LPCWSTR FileName)
    {
        HANDLE hFile = ::CreateFileW(FileName, GENERIC_READ,
            FILE_SHARE_READ, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            return FALSE;

        DWORD dwSize = ::GetFileSize(hFile, NULL);
        if (dwSize == 0xFFFFFFFF)
        {
            ::CloseHandle(hFile);
            return FALSE;
        }

        m_pos = 0;
        m_data.resize(dwSize);
        DWORD dwRead;
        if (!::ReadFile(hFile, &m_data[0], dwSize, &dwRead, NULL) ||
            dwRead != dwSize)
        {
            m_data.clear();
            ::CloseHandle(hFile);
            return FALSE;
        }
        return ::CloseHandle(hFile);
    }

    BOOL SaveToFile(LPCWSTR FileName) const
    {
        HANDLE hFile = ::CreateFileW(FileName, GENERIC_WRITE,
            FILE_SHARE_READ, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            return FALSE;

        DWORD dwWritten;
        if (!::WriteFile(hFile, &m_data[0], DWORD(m_data.size()),
                         &dwWritten, NULL))
        {
            ::CloseHandle(hFile);
            ::DeleteFileW(FileName);
            return FALSE;
        }

        if (!::CloseHandle(hFile))
        {
            ::DeleteFileW(FileName);
            return FALSE;
        }

        return TRUE;
    }

    BYTE& operator[](DWORD index)
    {
        return m_data[index];
    }
    const BYTE& operator[](DWORD index) const
    {
        return m_data[index];
    }

    BOOL ReadSz(std::wstring& str) const
    {
        str.clear();
        WORD w;
        while (ReadWord(w))
        {
            if (w == UNICODE_NULL)
                return TRUE;
            str += (WCHAR)w;
        }
        return FALSE;
    }
    BOOL WriteSz(const std::wstring& str)
    {
        return WriteData(&str[0], (str.size() + 1) * sizeof(WCHAR));
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

protected:
    mutable DWORD       m_pos;
    data_type           m_data;
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MBYTESTREAM_HPP_
