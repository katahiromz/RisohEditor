// MTextToText.hpp -- text encoding conversion                  -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MTEXTTOTEXT_HPP_
#define MZC4_MTEXTTOTEXT_HPP_       4       /* Version 4 */

class MAnsiToWide;
class MWideToAnsi;

////////////////////////////////////////////////////////////////////////////

#include "MString.hpp"

#include <cassert>

////////////////////////////////////////////////////////////////////////////

class MAnsiToWide
{
public:
    MAnsiToWide()
    {
    }
    MAnsiToWide(int codepage, const char *str)
    {
        do_it(codepage, str, mstrlen(str));
    }
    MAnsiToWide(int codepage, const char *str, size_t count)
    {
        do_it(codepage, str, count);
    }
    MAnsiToWide(int codepage, const MStringA& str)
    {
        do_it(codepage, str.c_str(), str.size());
    }
    MAnsiToWide(int codepage, const MAnsiToWide& str) : m_str(str.m_str) { }

    MAnsiToWide& operator=(const MAnsiToWide& str)
    {
        m_str = str.m_str;
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

    WCHAR *data()
    {
        return &m_str[0];
    }

    const WCHAR *c_str() const
    {
        return m_str.c_str();
    }

    operator const WCHAR *() const
    {
        return c_str();
    }

protected:
    MStringW m_str;
    void do_it(int codepage, const char *str, size_t count);
};

////////////////////////////////////////////////////////////////////////////

class MWideToAnsi
{
public:
    MWideToAnsi()
    {
    }
    MWideToAnsi(int codepage, const WCHAR *str)
    {
        do_it(codepage, str, mstrlen(str));
    }
    MWideToAnsi(int codepage, const WCHAR *str, size_t count)
    {
        do_it(codepage, str, count);
    }
    MWideToAnsi(int codepage, const MStringW& str)
    {
        do_it(codepage, str.c_str(), str.size());
    }
    MWideToAnsi(int codepage, const MWideToAnsi& str) : m_str(str.m_str) { }

    MWideToAnsi& operator=(const MWideToAnsi& str)
    {
        m_str = str.m_str;
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
    MStringA m_str;
    void do_it(int codepage, const WCHAR *str, size_t count);
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

#if defined(_WIN32) && !defined(WONVER)
    inline void
    MAnsiToWide::do_it(int codepage, const char *str, size_t count)
    {
        int len = int(count);
        int cch = ::MultiByteToWideChar(codepage, 0, str, len, NULL, 0);
        m_str.resize(cch);
        if (!::MultiByteToWideChar(codepage, 0, str, len, &m_str[0], cch + 1))
        {
            m_str.clear();
        }
    }

    inline void
    MWideToAnsi::do_it(int codepage, const WCHAR *str, size_t count)
    {
        int len = int(count);
        int cch = ::WideCharToMultiByte(codepage, 0, str, len, NULL, 0, NULL, NULL);
        m_str.resize(cch);
        if (!::WideCharToMultiByte(codepage, 0, str, len, &m_str[0], cch + 1,
                                   NULL, NULL))
        {
            m_str.clear();
        }
    }
#else
    #include <iconv.h>

    #ifndef WonGetACP
        #define WonGetACP()     1252
    #endif
    #ifndef WonGetOEMCP
        #define WonGetOEMCP()   437
    #endif

    #ifndef CP_ACP
        #define CP_ACP          WonGetACP()
    #endif
    #ifndef CP_OEMCP
        #define CP_OEMCP        WonGetOEMCP()
    #endif
    #ifndef CP_UTF8
        #define CP_UTF8         65001   // UTF-8
    #endif

    namespace text2text
    {
        inline const char *get_wide_encoding(void)
        {
            switch (sizeof(WCHAR))
            {
            case 1:
                return "UTF-8";
            case 2:
                return "UCS-2LE";
            case 4:
                return "UCS-4LE";
            default:
                assert(0);
                return NULL;
            }
        }

        inline std::string encoding_from_cp(int codepage)
        {
            if (codepage == CP_ACP)
                codepage = WonGetACP();
            else if (codepage == CP_OEMCP)
                codepage = WonGetOEMCP();

            std::string ret;
            switch (codepage)
            {
            case CP_UTF8:
                ret = "UTF-8";
                break;
            case 932:
                ret = "SHIFT_JIS";
                break;
            case 1252:
                ret = "ISO-8859-1";
                break;
            default:
                mstr_to_dec(ret, codepage, false);
                ret = "CP" + ret;
            }
            return ret;
        }
    }

    inline void
    MAnsiToWide::do_it(int codepage, const char *str, size_t count)
    {
        m_str.clear();
        if (*str == 0)
            return;

        iconv_t ic = iconv_open(text2text::get_wide_encoding(),
                                text2text::encoding_from_cp(codepage).c_str());
        if ((iconv_t)-1 == ic)
            return;

        size_t ansi_len = count * sizeof(char);
        #ifdef ICONV_SECOND_ARGUMENT_IS_CONST
            const char *ansi_ptr  = str;
        #else
            char *ansi_ptr  = const_cast<char *>(str);
        #endif

        size_t buf_len = (count + 1) * sizeof(WCHAR);
        if (char *buf = (char *)malloc(buf_len))
        {
            size_t wide_len = buf_len;
            char *wide_ptr = buf;
            if (buf && (size_t)-1 != iconv(ic, &ansi_ptr, &ansi_len, &wide_ptr, &wide_len))
            {
                wide_len = buf_len - wide_len;
                m_str.assign(reinterpret_cast<WCHAR *>(buf), wide_len / sizeof(WCHAR));
            }
            free(buf);
        }

        iconv_close(ic);
    }

    inline void
    MWideToAnsi::do_it(int codepage, const WCHAR *str, size_t count)
    {
        m_str.clear();
        if (*str == 0)
            return;

        iconv_t ic = iconv_open(text2text::encoding_from_cp(codepage).c_str(),
                                text2text::get_wide_encoding());
        if ((iconv_t)-1 == ic)
            return;

        size_t wide_len = count * sizeof(WCHAR);
        #ifdef ICONV_SECOND_ARGUMENT_IS_CONST
            const char *wide_ptr  = reinterpret_cast<const char *>(str);
        #else
            char *wide_ptr  = reinterpret_cast<char *>(const_cast<WCHAR *>(str));
        #endif

        size_t buf_len = (count + 1) * 3;
        if (char *buf = (char *)malloc(buf_len))
        {
            size_t ansi_len = buf_len;
            char *ansi_ptr = buf;
            if (buf && (size_t)-1 != iconv(ic, &wide_ptr, &wide_len, &ansi_ptr, &ansi_len))
            {
                ansi_len = buf_len - ansi_len;
                m_str.assign(buf, ansi_len / sizeof(char));
            }
            free(buf);
        }

        iconv_close(ic);
    }
#endif

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MTEXTTOTEXT_HPP_
