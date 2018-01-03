// MTextToText.hpp -- text encoding conversion                  -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MTEXTTOTEXT_HPP_
#define MZC4_MTEXTTOTEXT_HPP_       3       /* Version 3 */

class MAnsiToWide;
class MWideToAnsi;

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
    MAnsiToWide(INT nCodePage, const char *str);
    MAnsiToWide(INT nCodePage, const char *str, INT count);
    MAnsiToWide(INT nCodePage, const std::string& str);
    MAnsiToWide(INT nCodePage, const MAnsiToWide& str) : m_str(str) { }

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
    MWideToAnsi(INT nCodePage, const wchar_t *str);
    MWideToAnsi(INT nCodePage, const wchar_t *str, INT count);
    MWideToAnsi(INT nCodePage, const std::wstring& str);
    MWideToAnsi(INT nCodePage, const MWideToAnsi& str) : m_str(str) { }

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

#define MAnsiToAnsi(cp,ansi)   (ansi)
#define MWideToWide(cp,wide)   (wide)

#ifdef UNICODE
    #define MAnsiToText MAnsiToWide
    #define MTextToAnsi MWideToAnsi
    #define MWideToText MWideToWide
    #define MTextToWide MWideToWide
    #define MTextToText MWideToWide
#else
    #define MAnsiToText MAnsiToAnsi
    #define MTextToAnsi MAnsiToAnsi
    #define MWideToText MWideToAnsi
    #define MTextToWide MAnsiToWide
    #define MTextToText MAnsiToAnsi
#endif

////////////////////////////////////////////////////////////////////////////

inline MAnsiToWide::MAnsiToWide(INT nCodePage, const char *str)
{
    INT cch = ::MultiByteToWideChar(nCodePage, 0, str, -1, NULL, 0);
    m_str.resize(cch);
    if (!::MultiByteToWideChar(nCodePage, 0, str, -1, &m_str[0], cch + 1))
    {
        m_str.clear();
    }
}

inline MAnsiToWide::MAnsiToWide(INT nCodePage, const char *str, INT count)
{
    INT cch = ::MultiByteToWideChar(nCodePage, 0, str, count, NULL, 0);
    m_str.resize(cch);
    if (!::MultiByteToWideChar(nCodePage, 0, str, count, &m_str[0], cch + 1))
    {
        m_str.clear();
    }
}

inline MAnsiToWide::MAnsiToWide(INT nCodePage, const std::string& str)
{
    INT cch = ::MultiByteToWideChar(nCodePage, 0, str.c_str(), INT(str.size()),
                                    NULL, 0);
    m_str.resize(cch);
    if (!::MultiByteToWideChar(nCodePage, 0, str.c_str(), INT(str.size()),
                               &m_str[0], cch + 1))
    {
        m_str.clear();
    }
}

inline MWideToAnsi::MWideToAnsi(INT nCodePage, const wchar_t *str)
{
    INT cch = ::WideCharToMultiByte(nCodePage, 0, str, -1, NULL, 0, NULL, NULL);
    m_str.resize(cch);
    if (!::WideCharToMultiByte(nCodePage, 0, str, -1, &m_str[0], cch + 1,
                               NULL, NULL))
    {
        m_str.clear();
    }
}

inline MWideToAnsi::MWideToAnsi(INT nCodePage, const wchar_t *str, INT count)
{
    INT cch = ::WideCharToMultiByte(nCodePage, 0, str, count, NULL, 0, NULL, NULL);
    m_str.resize(cch);
    if (!::WideCharToMultiByte(nCodePage, 0, str, count, &m_str[0], cch + 1,
                               NULL, NULL))
    {
        m_str.clear();
    }
}

inline MWideToAnsi::MWideToAnsi(INT nCodePage, const std::wstring& str)
{
    INT cch = ::WideCharToMultiByte(nCodePage, 0, str.c_str(), INT(str.size()),
                                    NULL, 0, NULL, NULL);
    m_str.resize(cch);
    if (!::WideCharToMultiByte(nCodePage, 0, str.c_str(), INT(str.size()),
                               &m_str[0], cch + 1, NULL, NULL))
    {
        m_str.clear();
    }
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MTEXTTOTEXT_HPP_
