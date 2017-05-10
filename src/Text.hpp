#ifndef TEXT_HPP_
#define TEXT_HPP_

#include <windows.h>
#include <string>
#include <vector>
#include <cstring>

inline std::wstring AnsiToWide(const std::string str)
{
    std::wstring ret;
    int src_len = int(str.size());
    int dest_len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), src_len, NULL, 0);
    ret.resize(dest_len);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), src_len, &ret[0], dest_len);
    return ret;
}
inline std::string WideToAnsi(const std::wstring str)
{
    std::string ret;
    int src_len = int(str.size());
    int dest_len = WideCharToMultiByte(CP_ACP, 0, str.c_str(), src_len,
                                       NULL, 0, NULL, NULL);
    ret.resize(dest_len);
    WideCharToMultiByte(CP_ACP, 0, str.c_str(), src_len, &ret[0], dest_len,
                        NULL, NULL);
    return ret;
}
inline std::wstring Utf8ToWide(const std::string str)
{
    std::wstring ret;
    int src_len = int(str.size());
    int dest_len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), src_len, NULL, 0);
    ret.resize(dest_len);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), src_len, &ret[0], dest_len);
    return ret;
}
inline std::string WideToUtf8(const std::wstring str)
{
    std::string ret;
    int src_len = int(str.size());
    int dest_len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), src_len,
                                       NULL, 0, NULL, NULL);
    ret.resize(dest_len);
    WideCharToMultiByte(CP_UTF8, 0, str.c_str(), src_len, &ret[0], dest_len,
                        NULL, NULL);
    return ret;
}
#define AnsiToAnsi(str)         std::string(str)
#define WideToWide(str)         std::wstring(str)
#ifdef UNICODE
    #define AnsiToText(str)     AnsiToWide(str)
    #define WideToText(str)     WideToWide(str)
    #define TextToAnsi(str)     WideToAnsi(str)
    #define TextToWide(str)     WideToWide(str)
    #define TextToText(str)     WideToWide(str)
#else
    #define AnsiToText(str)     AnsiToAnsi(str)
    #define WideToText(str)     WideToAnsi(str)
    #define TextToAnsi(str)     AnsiToAnsi(str)
    #define TextToWide(str)     AnsiToWide(str)
    #define TextToText(str)     AnsiToAnsi(str)
#endif

inline void
swap_endian(void *ptr, DWORD len)
{
    BYTE *pb = (BYTE *)ptr;
    len /= 2;
    while (--len)
    {
        BYTE b = pb[0];
        pb[0] = pb[1];
        pb[1] = b;
        ++pb;
        ++pb;
    }
}

inline void str_trim(std::string& str, const char *spaces = " \t\r\n")
{
    size_t i = str.find_first_not_of(spaces);
    size_t j = str.find_last_not_of(spaces);
    if ((i == std::string::npos) || (j == std::string::npos))
    {
        str.clear();
    }
    else
    {
        str = str.substr(i, j - i + 1);
    }
}

inline void str_trim(std::wstring& str, const wchar_t *spaces = L" \t\r\n")
{
    size_t i = str.find_first_not_of(spaces);
    size_t j = str.find_last_not_of(spaces);
    if ((i == std::wstring::npos) || (j == std::wstring::npos))
    {
        str.clear();
    }
    else
    {
        str = str.substr(i, j - i + 1);
    }
}

inline void str_trim(char *str, const char *spaces = " \t\r\n")
{
    std::string s = str;
    str_trim(s, spaces);
    std::strcpy(str, s.c_str());
}

inline void str_trim(wchar_t *str, const wchar_t *spaces = L" \t\r\n")
{
    std::wstring s = str;
    str_trim(s, spaces);
    std::wcscpy(str, s.c_str());
}

inline const char *skip_space(const char *pch)
{
    using namespace std;
    while (*pch && isspace(*pch))
    {
        ++pch;
    }
    return pch;
}

inline const wchar_t *skip_space(const wchar_t *pch)
{
    using namespace std;
    while (*pch && iswspace(*pch))
    {
        ++pch;
    }
    return pch;
}

inline const char *skip_word(const char *pch)
{
    using namespace std;
    while (*pch && isalpha(*pch) || isdigit(*pch) || *pch == '_')
    {
        ++pch;
    }
    return pch;
}

inline const wchar_t *skip_word(const wchar_t *pch)
{
    using namespace std;
    while (*pch && iswalpha(*pch) || iswdigit(*pch) || *pch == L'_')
    {
        ++pch;
    }
    return pch;
}

inline std::string str_escape(const std::string& str)
{
    std::string ret;

    for (size_t i = 0; i < str.size(); ++i)
    {
        char ch = str[i];
        switch (ch)
        {
        case '\"': ret += "\"\""; break;
        case '\\': ret += "\\\\"; break;
        case '\0': ret += "\\0"; break;
        case '\a': ret += "\\a"; break;
        case '\b': ret += "\\b"; break;
        case '\f': ret += "\\f"; break;
        case '\n': ret += "\\n"; break;
        case '\r': ret += "\\r"; break;
        case '\t': ret += "\\t"; break;
        case '\v': ret += "\\v"; break;
        default:
            if (ch < 0x20)
            {
                using namespace std;
                char sz[32];
                sprintf(sz, "\\x%02X", ch);
                ret += sz;
            }
            else
            {
                ret += ch;
            }
        }
    }

    return ret;
}

inline std::wstring str_escape(const std::wstring& str)
{
    std::wstring ret;

    for (size_t i = 0; i < str.size(); ++i)
    {
        wchar_t ch = str[i];
        switch (ch)
        {
        case L'\"': ret += L"\"\""; break;
        case L'\\': ret += L"\\\\"; break;
        case L'\0': ret += L"\\0"; break;
        case L'\a': ret += L"\\a"; break;
        case L'\b': ret += L"\\b"; break;
        case L'\f': ret += L"\\f"; break;
        case L'\n': ret += L"\\n"; break;
        case L'\r': ret += L"\\r"; break;
        case L'\t': ret += L"\\t"; break;
        case L'\v': ret += L"\\v"; break;
        default:
            if (ch < 0x20)
            {
                using namespace std;
                wchar_t sz[32];
                wsprintfW(sz, L"\\x%02X", (BYTE)ch);
                ret += sz;
            }
            else
            {
                ret += ch;
            }
        }
    }

    return ret;
}

inline bool guts_escape(std::string& str, const char*& pch)
{
    using namespace std;
    switch (*pch)
    {
    case '\\': str += '\\'; ++pch; break;
    case '"': str += '\"'; ++pch; break;
    case 'a': str += '\a'; ++pch; break;
    case 'b': str += '\b'; ++pch; break;
    case 'f': str += '\f'; ++pch; break;
    case 'n': str += '\n'; ++pch; break;
    case 'r': str += '\r'; ++pch; break;
    case 't': str += '\t'; ++pch; break;
    case 'v': str += '\v'; ++pch; break;
    case 'x':
        {
            ++pch;
            std::string strNum;
            if (isxdigit(*pch))
            {
                strNum += *pch;
                ++pch;
                if (isxdigit(*pch))
                {
                    strNum += *pch;
                    ++pch;
                }
            }
            str += (char)strtoul(strNum.c_str(), NULL, 16);
        }
        break;
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
        {
            std::string strNum;
            if ('0' <= *pch && *pch <= '7')
            {
                strNum += *pch;
                ++pch;
                if ('0' <= *pch && *pch <= '7')
                {
                    strNum += *pch;
                    ++pch;
                    if ('0' <= *pch && *pch <= '7')
                    {
                        strNum += *pch;
                        ++pch;
                    }
                }
            }
            str += (char)strtoul(strNum.c_str(), NULL, 8);
        }
        break;
    default:
        str += *pch;
        ++pch;
        return false;
    }
    return true;
}

inline bool guts_escape(std::wstring& str, const wchar_t*& pch)
{
    using namespace std;
    switch (*pch)
    {
    case L'\\': str += L'\\'; ++pch; break;
    case L'"': str += L'\"'; ++pch; break;
    case L'a': str += L'\a'; ++pch; break;
    case L'b': str += L'\b'; ++pch; break;
    case L'f': str += L'\f'; ++pch; break;
    case L'n': str += L'\n'; ++pch; break;
    case L'r': str += L'\r'; ++pch; break;
    case L't': str += L'\t'; ++pch; break;
    case L'v': str += L'\v'; ++pch; break;
    case L'x':
        {
            ++pch;
            std::wstring strNum;
            if (iswxdigit(*pch))
            {
                strNum += *pch;
                ++pch;
                if (iswxdigit(*pch))
                {
                    strNum += *pch;
                    ++pch;
                }
            }
            str += (wchar_t)wcstoul(strNum.c_str(), NULL, 16);
        }
        break;
    case L'0': case L'1': case L'2': case L'3':
    case L'4': case L'5': case L'6': case L'7':
        {
            std::wstring strNum;
            if (L'0' <= *pch && *pch <= L'7')
            {
                strNum += *pch;
                ++pch;
                if (L'0' <= *pch && *pch <= L'7')
                {
                    strNum += *pch;
                    ++pch;
                    if (L'0' <= *pch && *pch <= L'7')
                    {
                        strNum += *pch;
                        ++pch;
                    }
                }
            }
            str += (wchar_t)wcstoul(strNum.c_str(), NULL, 8);
        }
        break;
    case 'u':
        {
            ++pch;
            std::wstring strNum;
            if (iswxdigit(*pch))
            {
                strNum += *pch;
                ++pch;
                if (iswxdigit(*pch))
                {
                    strNum += *pch;
                    ++pch;
                    if (iswxdigit(*pch))
                    {
                        strNum += *pch;
                        ++pch;
                        if (iswxdigit(*pch))
                        {
                            strNum += *pch;
                            ++pch;
                        }
                    }
                }
            }
            str += (wchar_t)wcstoul(strNum.c_str(), NULL, 16);
        }
        break;
    default:
        str += *pch;
        ++pch;
        return false;
    }
    return true;
}

inline bool guts_quote(std::string& str, const char*& pch)
{
    using namespace std;
    str.clear();

    pch = skip_space(pch);
    if (*pch != L'\"')
        return false;

    for (++pch; *pch; ++pch)
    {
        if (*pch == L'\\')
        {
            ++pch;
            guts_escape(str, pch);
            --pch;
        }
        else if (*pch == L'\"')
        {
            ++pch;
            if (*pch == L'\"')
            {
                str += L'\"';
            }
            else
            {
                break;
            }
        }
        else
        {
            str += *pch;
        }
    }

    return true;
}

inline bool guts_quote(std::wstring& str, const wchar_t*& pch)
{
    using namespace std;
    str.clear();

    pch = skip_space(pch);
    if (*pch != L'\"')
        return false;

    for (++pch; *pch; ++pch)
    {
        if (*pch == L'\\')
        {
            ++pch;
            guts_escape(str, pch);
            --pch;
        }
        else if (*pch == L'\"')
        {
            ++pch;
            if (*pch == L'\"')
            {
                str += L'\"';
            }
            else
            {
                break;
            }
        }
        else
        {
            str += *pch;
        }
    }

    return true;
}

inline std::wstring str_dec(UINT nID)
{
    using namespace std;
    wchar_t sz[32];
    wsprintfW(sz, L"%u", nID);
    std::wstring ret(sz);
    return ret;
}

inline std::wstring str_hex(UINT nID)
{
    std::wstring ret;
    if (nID == 0)
    {
        ret = L"0";
    }
    else
    {
        using namespace std;
        wchar_t sz[32];
        wsprintfW(sz, L"0x%X", nID);
        ret = sz;
    }
    return ret;
}

template <typename T_STR>
inline void
str_replace_all(T_STR& str, const T_STR& from, const T_STR& to)
{
    size_t i = 0;
    for (;;) {
        i = str.find(from, i);
        if (i == T_STR::npos)
            break;
        str.replace(i, from.size(), to);
        i += to.size();
    }
}
template <typename T_STR>
inline void
str_replace_all(T_STR& str,
                const typename T_STR::value_type *from,
                const typename T_STR::value_type *to)
{
    str_replace_all(str, T_STR(from), T_STR(to));
}

template <typename T_STR>
inline bool is_ascii(const T_STR& str)
{
    size_t i, count = str.size();
    for (i = 0; i < count; ++i)
    {
        typename T_STR::value_type ch = str[i];
        if (ch < 0x20 || 0x7F < ch)
            return false;
    }
    return true;
}

inline std::wstring
BinaryToText(const std::vector<BYTE>& Data)
{
    std::wstring ret;
    if (Data.size() >= 2 && memcmp(&Data[0], "\xFF\xFE", 2) == 0)
    {
        // UTF-16 LE
        ret.assign((const WCHAR *)&Data[0], Data.size() / sizeof(WCHAR));
    }
    else if (Data.size() >= 2 && memcmp(&Data[0], "\xFE\xFF", 2) == 0)
    {
        // UTF-16 BE
        ret.assign((const WCHAR *)&Data[0], Data.size() / sizeof(WCHAR));
        swap_endian(&ret[0], ret.size() * sizeof(WCHAR));
    }
    else if (Data.size() >= 3 && memcmp(&Data[0], "\xEF\xBB\xBF", 3) == 0)
    {
        // UTF-8
        std::string str((const char *)&Data[3], Data.size() - 3);
        ret = Utf8ToWide(str);
    }
    else
    {
        // ANSI
        std::string str((const char *)&Data[0], Data.size());
        ret = AnsiToWide(str);
    }
    str_replace_all(ret, L"\r\n", L"\n");
    str_replace_all(ret, L"\r", L"\n");
    str_replace_all(ret, L"\n", L"\r\n");
    return ret;
}

inline std::string str_quote(const std::string& str)
{
    std::string ret;
    ret += "\"";
    ret += str_escape(str);
    ret += "\"";
    return ret;
}

inline std::wstring str_quote(const std::wstring& str)
{
    std::wstring ret;
    ret += L"\"";
    ret += str_escape(str);
    ret += L"\"";
    return ret;
}

#endif  // ndef TEXT_HPP_
