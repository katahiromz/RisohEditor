// MTextToText.hpp -- text encoding conversion                  -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MTEXTTOTEXT_HPP_
#define MZC4_MTEXTTOTEXT_HPP_       2       /* Version 2 */

class MAnsiToWide;
class MWideToAnsi;
class MUtf8ToWide;
class MWideToUtf8;

////////////////////////////////////////////////////////////////////////////

#ifndef _INC_WINDOWS
    #include <windows.h>    // Win32API
#endif

#include <cassert>          // assert
#include <string>           // for std::string and std::wstring
#include <cstring>          // for std::strlen and std::wcslen

////////////////////////////////////////////////////////////////////////////

class MAnsiToWide
{
public:
    MAnsiToWide() { }
    MAnsiToWide(const char *str);
    MAnsiToWide(const char *str, INT count);
    MAnsiToWide(const std::string& str);
    MAnsiToWide(const MAnsiToWide& str) : m_str(str) { }

    MAnsiToWide& operator=(const MAnsiToWide& str)
    {
        m_str = str;
        return *this;
    }

    bool empty() const
    {
        return m_str.empty();
    }

    size_t size() const
    {
        return m_str.size();
    }

    wchar_t *data()
    {
        return &m_str[0];
    }

    const wchar_t *c_str() const
    {
        return m_str.c_str();
    }

    operator const wchar_t *() const
    {
        return c_str();
    }

protected:
    std::wstring m_str;
};

////////////////////////////////////////////////////////////////////////////

class MWideToAnsi
{
public:
    MWideToAnsi() { }
    MWideToAnsi(const wchar_t *str);
    MWideToAnsi(const wchar_t *str, INT count);
    MWideToAnsi(const std::wstring& str);
    MWideToAnsi(const MWideToAnsi& str) : m_str(str) { }

    MWideToAnsi& operator=(const MWideToAnsi& str)
    {
        m_str = str;
        return *this;
    }

    bool empty() const
    {
        return m_str.empty();
    }

    size_t size() const
    {
        return m_str.size();
    }

    char *data()
    {
        return &m_str[0];
    }

    const char *c_str() const
    {
        return m_str.c_str();
    }

    operator const char *() const
    {
        return c_str();
    }

protected:
    std::string m_str;
};

////////////////////////////////////////////////////////////////////////////

class MUtf8ToWide
{
public:
    MUtf8ToWide() { }
    MUtf8ToWide(const char *str);
    MUtf8ToWide(const char *str, INT count);
    MUtf8ToWide(const std::string& str);
    MUtf8ToWide(const MUtf8ToWide& str) : m_str(str) { }

    MUtf8ToWide& operator=(const MUtf8ToWide& str)
    {
        m_str = str;
        return *this;
    }

    bool empty() const
    {
        return m_str.empty();
    }

    size_t size() const
    {
        return m_str.size();
    }

    wchar_t *data()
    {
        return &m_str[0];
    }

    const wchar_t *c_str() const
    {
        return m_str.c_str();
    }

    operator const wchar_t *() const
    {
        return c_str();
    }

protected:
    std::wstring m_str;
};

////////////////////////////////////////////////////////////////////////////

class MWideToUtf8
{
public:
    MWideToUtf8() { }
    MWideToUtf8(const wchar_t *str);
    MWideToUtf8(const wchar_t *str, INT count);
    MWideToUtf8(const std::wstring& str);
    MWideToUtf8(const MWideToUtf8& str) : m_str(str) { }

    MWideToUtf8& operator=(const MWideToUtf8& str)
    {
        m_str = str;
        return *this;
    }

    bool empty() const
    {
        return m_str.empty();
    }

    size_t size() const
    {
        return m_str.size();
    }

    char *data()
    {
        return &m_str[0];
    }

    const char *c_str() const
    {
        return m_str.c_str();
    }

    operator const char *() const
    {
        return c_str();
    }

protected:
    std::string m_str;
};

////////////////////////////////////////////////////////////////////////////

#define MAnsiToAnsi(ansi)   (ansi)
#define MWideToWide(wide)   (wide)
#define MUtf8ToUtf8(utf8)   (utf8)

#define MAnsiToUtf8(ansi)   MWideToUtf8(MAnsiToWide(ansi))
#define MUtf8ToAnsi(utf8)   MWideToAnsi(MUtf8ToWide(utf8))

#ifdef UNICODE
    #define MAnsiToText MAnsiToWide
    #define MTextToAnsi MWideToAnsi
    #define MWideToText MWideToWide
    #define MTextToWide MWideToWide
    #define MUtf8ToText MUtf8ToWide
    #define MTextToUtf8 MWideToUtf8
    #define MTextToText MWideToWide
#else
    #define MAnsiToText MAnsiToAnsi
    #define MTextToAnsi MAnsiToAnsi
    #define MWideToText MWideToAnsi
    #define MTextToWide MAnsiToWide
    #define MUtf8ToText MUtf8ToAnsi
    #define MTextToUtf8 MAnsiToUtf8
    #define MTextToText MAnsiToAnsi
#endif

////////////////////////////////////////////////////////////////////////////

inline MAnsiToWide::MAnsiToWide(const char *str)
{
    INT cch = ::MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    m_str.resize(cch);
    if (!::MultiByteToWideChar(CP_ACP, 0, str, -1, &m_str[0], cch + 1))
    {
        m_str.clear();
    }
}

inline MAnsiToWide::MAnsiToWide(const char *str, INT count)
{
    INT cch = ::MultiByteToWideChar(CP_ACP, 0, str, count, NULL, 0);
    m_str.resize(cch);
    if (!::MultiByteToWideChar(CP_ACP, 0, str, count, &m_str[0], cch + 1))
    {
        m_str.clear();
    }
}

inline MAnsiToWide::MAnsiToWide(const std::string& str)
{
    INT cch = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), INT(str.size()),
                                    NULL, 0);
    m_str.resize(cch);
    if (!::MultiByteToWideChar(CP_ACP, 0, str.c_str(), INT(str.size()),
                               &m_str[0], cch + 1))
    {
        m_str.clear();
    }
}

inline MWideToAnsi::MWideToAnsi(const wchar_t *str)
{
    INT cch = ::WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    m_str.resize(cch);
    if (!::WideCharToMultiByte(CP_ACP, 0, str, -1, &m_str[0], cch + 1,
                               NULL, NULL))
    {
        m_str.clear();
    }
}

inline MWideToAnsi::MWideToAnsi(const wchar_t *str, INT count)
{
    INT cch = ::WideCharToMultiByte(CP_ACP, 0, str, count, NULL, 0, NULL, NULL);
    m_str.resize(cch);
    if (!::WideCharToMultiByte(CP_ACP, 0, str, count, &m_str[0], cch + 1,
                               NULL, NULL))
    {
        m_str.clear();
    }
}

inline MWideToAnsi::MWideToAnsi(const std::wstring& str)
{
    INT cch = ::WideCharToMultiByte(CP_ACP, 0, str.c_str(), INT(str.size()),
                                    NULL, 0, NULL, NULL);
    m_str.resize(cch);
    if (!::WideCharToMultiByte(CP_ACP, 0, str.c_str(), INT(str.size()),
                               &m_str[0], cch + 1, NULL, NULL))
    {
        m_str.clear();
    }
}

inline MUtf8ToWide::MUtf8ToWide(const char *str)
{
    INT cch = ::MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    m_str.resize(cch);
    if (!::MultiByteToWideChar(CP_UTF8, 0, str, -1, &m_str[0], cch + 1))
    {
        m_str.clear();
    }
}

inline MUtf8ToWide::MUtf8ToWide(const char *str, INT count)
{
    INT cch = ::MultiByteToWideChar(CP_UTF8, 0, str, count, NULL, 0);
    m_str.resize(cch);
    if (!::MultiByteToWideChar(CP_UTF8, 0, str, count, &m_str[0], cch + 1))
    {
        m_str.clear();
    }
}

inline MUtf8ToWide::MUtf8ToWide(const std::string& str)
{
    INT cch = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), INT(str.size()),
                                    NULL, 0);
    m_str.resize(cch);
    if (!::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), INT(str.size()),
                               &m_str[0], cch + 1))
    {
        m_str.clear();
    }
}

inline MWideToUtf8::MWideToUtf8(const wchar_t *str)
{
    INT cch = ::WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    m_str.resize(cch);
    if (!::WideCharToMultiByte(CP_UTF8, 0, str, -1, &m_str[0], cch + 1,
                               NULL, NULL))
    {
        m_str.clear();
    }
}

inline MWideToUtf8::MWideToUtf8(const wchar_t *str, INT count)
{
    INT cch = ::WideCharToMultiByte(CP_UTF8, 0, str, count, NULL, 0, NULL, NULL);
    m_str.resize(cch);
    if (!::WideCharToMultiByte(CP_UTF8, 0, str, count, &m_str[0], cch + 1,
                               NULL, NULL))
    {
        m_str.clear();
    }
}

inline MWideToUtf8::MWideToUtf8(const std::wstring& str)
{
    INT cch = ::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), INT(str.size()),
                                    NULL, 0, NULL, NULL);
    m_str.resize(cch);
    if (!::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), INT(str.size()),
                               &m_str[0], cch + 1, NULL, NULL))
    {
        m_str.clear();
    }
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MTEXTTOTEXT_HPP_
