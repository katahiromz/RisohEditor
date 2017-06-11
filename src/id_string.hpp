// is_string.hpp --- ID and Strings
//////////////////////////////////////////////////////////////////////////////

#ifndef ID_STRING_HPP_
#define ID_STRING_HPP_

#include "Text.hpp"

//////////////////////////////////////////////////////////////////////////////

struct ID_OR_STRING
{
    WORD m_ID;
    std::wstring m_Str;

    ID_OR_STRING() : m_ID(0)
    {
    }

    ID_OR_STRING(WORD ID) : m_ID(ID)
    {
    }

    ID_OR_STRING(LPCWSTR Str)
    {
        if (IS_INTRESOURCE(Str))
        {
            m_ID = LOWORD(Str);
        }
        else
        {
            m_ID = 0;
            m_Str = Str;
        }
    }

    const WCHAR *Ptr() const
    {
        if (m_ID)
            return MAKEINTRESOURCEW(m_ID);
        return m_Str.c_str();
    }

    bool is_zero() const
    {
        return m_ID == 0 && m_Str.empty();
    }

    bool is_null() const
    {
        return is_zero();
    }

    bool empty() const
    {
        return is_zero();
    }

    bool is_str() const
    {
        return (!m_ID && !m_Str.empty());
    }

    bool is_int() const
    {
        return !is_str();
    }

    void clear()
    {
        m_ID = 0;
        m_Str.clear();
    }

    ID_OR_STRING& operator=(WORD ID)
    {
        m_ID = ID;
        m_Str.clear();
        return *this;
    }

    ID_OR_STRING& operator=(const WCHAR *Str)
    {
        if (IS_INTRESOURCE(Str))
        {
            m_ID = LOWORD(Str);
            m_Str.clear();
        }
        else
        {
            m_ID = 0;
            m_Str = Str;
        }
        return *this;
    }

    bool operator==(const ID_OR_STRING& id_or_str) const
    {
        if (id_or_str.m_ID != 0)
        {
            if (m_ID != 0)
                return id_or_str.m_ID == m_ID;
        }
        else
        {
            if (m_ID == 0)
                return m_Str == id_or_str.m_Str;
        }
        return false;
    }
    bool operator==(const WCHAR *psz) const
    {
        if (IS_INTRESOURCE(psz))
        {
            if (m_ID != 0)
                return LOWORD(psz) == m_ID;
        }
        else
        {
            if (m_ID == 0)
                return m_Str == psz;
        }
        return false;
    }
    bool operator==(const std::wstring& Str) const
    {
        return *this == Str.c_str();
    }
    bool operator==(WORD w) const
    {
        return m_ID == w;
    }

    bool operator!=(const ID_OR_STRING& id_or_str) const
    {
        return !(*this == id_or_str);
    }
    bool operator!=(const WCHAR *psz) const
    {
        return !(*this == psz);
    }
    bool operator!=(const std::wstring& Str) const
    {
        return !(*this == Str);
    }
    bool operator!=(WORD w) const
    {
        return m_ID != w;
    }

    std::wstring& str() const
    {
        static std::wstring s_str;
        if (m_ID == 0)
        {
            if (m_Str.size())
            {
                s_str = m_Str;
                return s_str;
            }
        }
        s_str = str_dec(m_ID);
        return s_str;
    }

    std::wstring& str_or_empty() const
    {
        static std::wstring s_str;
        if (m_ID == 0)
        {
            if (m_Str.size())
            {
                s_str = m_Str;
            }
            else
            {
                s_str.clear();
            }
        }
        else
        {
            s_str = str_dec(m_ID);
        }
        return s_str;
    }

    LPCWSTR c_str() const
    {
        return str().c_str();
    }

    LPCWSTR c_str_or_empty() const
    {
        return str_or_empty().c_str();
    }

    std::wstring quoted_wstr() const
    {
        std::wstring ret;
        if (m_ID == 0)
        {
            if (m_Str.size())
            {
                ret += L"\"";
                ret += str_escape(m_Str);
                ret += L"\"";
            }
            else
            {
                ret = L"\"\"";
            }
        }
        else
        {
            ret = str_dec(m_ID);
        }
        return ret;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef ID_STRING_HPP_

//////////////////////////////////////////////////////////////////////////////
