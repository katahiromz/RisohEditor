#ifndef TEXT_HPP_
#define TEXT_HPP_

#include <windows.h>
#include <string>
#include <vector>

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

inline void trim(std::string& str, const char *spaces = " \t\r\n")
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

inline void trim(std::wstring& str, const wchar_t *spaces = L" \t\r\n")
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

inline std::string escape(const std::string& str)
{
    std::string ret;

    for (size_t i = 0; i < str.size(); ++i)
    {
        char ch = str[i];
        switch (ch)
        {
        case '\"': ret += "\"\""; break;
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

inline std::wstring escape(const std::wstring& str)
{
    std::wstring ret;

    for (size_t i = 0; i < str.size(); ++i)
    {
        wchar_t ch = str[i];
        switch (ch)
        {
        case L'\"': ret += L"\"\""; break;
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

inline std::wstring virtkey(WORD w)
{
    std::wstring ret;
    switch (w)
    {
    case VK_LBUTTON: ret = L"VK_LBUTTON"; break;
    case VK_RBUTTON: ret = L"VK_RBUTTON"; break;
    case VK_CANCEL: ret = L"VK_CANCEL"; break;
    case VK_MBUTTON: ret = L"VK_MBUTTON"; break;
    case VK_XBUTTON1: ret = L"VK_XBUTTON1"; break;
    case VK_XBUTTON2: ret = L"VK_XBUTTON2"; break;
    case VK_BACK: ret = L"VK_BACK"; break;
    case VK_TAB: ret = L"VK_TAB"; break;
    case VK_CLEAR: ret = L"VK_CLEAR"; break;
    case VK_RETURN: ret = L"VK_RETURN"; break;
    case VK_SHIFT: ret = L"VK_SHIFT"; break;
    case VK_CONTROL: ret = L"VK_CONTROL"; break;
    case VK_MENU: ret = L"VK_MENU"; break;
    case VK_PAUSE: ret = L"VK_PAUSE"; break;
    case VK_CAPITAL: ret = L"VK_CAPITAL"; break;
    case VK_FINAL: ret = L"VK_FINAL"; break;
    case VK_ESCAPE: ret = L"VK_ESCAPE"; break;
    case VK_CONVERT: ret = L"VK_CONVERT"; break;
    case VK_NONCONVERT: ret = L"VK_NONCONVERT"; break;
    case VK_ACCEPT: ret = L"VK_ACCEPT"; break;
    case VK_MODECHANGE: ret = L"VK_MODECHANGE"; break;
    case VK_SPACE: ret = L"VK_SPACE"; break;
    case VK_PRIOR: ret = L"VK_PRIOR"; break;
    case VK_NEXT: ret = L"VK_NEXT"; break;
    case VK_END: ret = L"VK_END"; break;
    case VK_HOME: ret = L"VK_HOME"; break;
    case VK_LEFT: ret = L"VK_LEFT"; break;
    case VK_UP: ret = L"VK_UP"; break;
    case VK_RIGHT: ret = L"VK_RIGHT"; break;
    case VK_DOWN: ret = L"VK_DOWN"; break;
    case VK_SELECT: ret = L"VK_SELECT"; break;
    case VK_PRINT: ret = L"VK_PRINT"; break;
    case VK_EXECUTE: ret = L"VK_EXECUTE"; break;
    case VK_SNAPSHOT: ret = L"VK_SNAPSHOT"; break;
    case VK_INSERT: ret = L"VK_INSERT"; break;
    case VK_DELETE: ret = L"VK_DELETE"; break;
    case VK_HELP: ret = L"VK_HELP"; break;
    case VK_LWIN: ret = L"VK_LWIN"; break;
    case VK_RWIN: ret = L"VK_RWIN"; break;
    case VK_APPS: ret = L"VK_APPS"; break;
    case VK_SLEEP: ret = L"VK_SLEEP"; break;
    case VK_NUMPAD0: ret = L"VK_NUMPAD0"; break;
    case VK_NUMPAD1: ret = L"VK_NUMPAD1"; break;
    case VK_NUMPAD2: ret = L"VK_NUMPAD2"; break;
    case VK_NUMPAD3: ret = L"VK_NUMPAD3"; break;
    case VK_NUMPAD4: ret = L"VK_NUMPAD4"; break;
    case VK_NUMPAD5: ret = L"VK_NUMPAD5"; break;
    case VK_NUMPAD6: ret = L"VK_NUMPAD6"; break;
    case VK_NUMPAD7: ret = L"VK_NUMPAD7"; break;
    case VK_NUMPAD8: ret = L"VK_NUMPAD8"; break;
    case VK_NUMPAD9: ret = L"VK_NUMPAD9"; break;
    case VK_MULTIPLY: ret = L"VK_MULTIPLY"; break;
    case VK_ADD: ret = L"VK_ADD"; break;
    case VK_SEPARATOR: ret = L"VK_SEPARATOR"; break;
    case VK_SUBTRACT: ret = L"VK_SUBTRACT"; break;
    case VK_DECIMAL: ret = L"VK_DECIMAL"; break;
    case VK_DIVIDE: ret = L"VK_DIVIDE"; break;
    case VK_F1: ret = L"VK_F1"; break;
    case VK_F2: ret = L"VK_F2"; break;
    case VK_F3: ret = L"VK_F3"; break;
    case VK_F4: ret = L"VK_F4"; break;
    case VK_F5: ret = L"VK_F5"; break;
    case VK_F6: ret = L"VK_F6"; break;
    case VK_F7: ret = L"VK_F7"; break;
    case VK_F8: ret = L"VK_F8"; break;
    case VK_F9: ret = L"VK_F9"; break;
    case VK_F10: ret = L"VK_F10"; break;
    case VK_F11: ret = L"VK_F11"; break;
    case VK_F12: ret = L"VK_F12"; break;
    case VK_F13: ret = L"VK_F13"; break;
    case VK_F14: ret = L"VK_F14"; break;
    case VK_F15: ret = L"VK_F15"; break;
    case VK_F16: ret = L"VK_F16"; break;
    case VK_F17: ret = L"VK_F17"; break;
    case VK_F18: ret = L"VK_F18"; break;
    case VK_F19: ret = L"VK_F19"; break;
    case VK_F20: ret = L"VK_F20"; break;
    case VK_F21: ret = L"VK_F21"; break;
    case VK_F22: ret = L"VK_F22"; break;
    case VK_F23: ret = L"VK_F23"; break;
    case VK_F24: ret = L"VK_F24"; break;
    case VK_NUMLOCK: ret = L"VK_NUMLOCK"; break;
    case VK_SCROLL: ret = L"VK_SCROLL"; break;
    case VK_OEM_FJ_MASSHOU: ret = L"VK_OEM_FJ_MASSHOU"; break;
    case VK_OEM_FJ_TOUROKU: ret = L"VK_OEM_FJ_TOUROKU"; break;
    case VK_OEM_FJ_LOYA: ret = L"VK_OEM_FJ_LOYA"; break;
    case VK_OEM_FJ_ROYA: ret = L"VK_OEM_FJ_ROYA"; break;
    case VK_LSHIFT: ret = L"VK_LSHIFT"; break;
    case VK_RSHIFT: ret = L"VK_RSHIFT"; break;
    case VK_LCONTROL: ret = L"VK_LCONTROL"; break;
    case VK_RCONTROL: ret = L"VK_RCONTROL"; break;
    case VK_LMENU: ret = L"VK_LMENU"; break;
    case VK_RMENU: ret = L"VK_RMENU"; break;
    case VK_BROWSER_BACK: ret = L"VK_BROWSER_BACK"; break;
    case VK_BROWSER_FORWARD: ret = L"VK_BROWSER_FORWARD"; break;
    case VK_BROWSER_REFRESH: ret = L"VK_BROWSER_REFRESH"; break;
    case VK_BROWSER_STOP: ret = L"VK_BROWSER_STOP"; break;
    case VK_BROWSER_SEARCH: ret = L"VK_BROWSER_SEARCH"; break;
    case VK_BROWSER_FAVORITES: ret = L"VK_BROWSER_FAVORITES"; break;
    case VK_BROWSER_HOME: ret = L"VK_BROWSER_HOME"; break;
    case VK_VOLUME_MUTE: ret = L"VK_VOLUME_MUTE"; break;
    case VK_VOLUME_DOWN: ret = L"VK_VOLUME_DOWN"; break;
    case VK_VOLUME_UP: ret = L"VK_VOLUME_UP"; break;
    case VK_MEDIA_NEXT_TRACK: ret = L"VK_MEDIA_NEXT_TRACK"; break;
    case VK_MEDIA_PREV_TRACK: ret = L"VK_MEDIA_PREV_TRACK"; break;
    case VK_MEDIA_STOP: ret = L"VK_MEDIA_STOP"; break;
    case VK_MEDIA_PLAY_PAUSE: ret = L"VK_MEDIA_PLAY_PAUSE"; break;
    case VK_LAUNCH_MAIL: ret = L"VK_LAUNCH_MAIL"; break;
    case VK_LAUNCH_MEDIA_SELECT: ret = L"VK_LAUNCH_MEDIA_SELECT"; break;
    case VK_LAUNCH_APP1: ret = L"VK_LAUNCH_APP1"; break;
    case VK_LAUNCH_APP2: ret = L"VK_LAUNCH_APP2"; break;
    case VK_OEM_1: ret = L"VK_OEM_1"; break;
    case VK_OEM_PLUS: ret = L"VK_OEM_PLUS"; break;
    case VK_OEM_COMMA: ret = L"VK_OEM_COMMA"; break;
    case VK_OEM_MINUS: ret = L"VK_OEM_MINUS"; break;
    case VK_OEM_PERIOD: ret = L"VK_OEM_PERIOD"; break;
    case VK_OEM_2: ret = L"VK_OEM_2"; break;
    case VK_OEM_3: ret = L"VK_OEM_3"; break;
    case VK_OEM_4: ret = L"VK_OEM_4"; break;
    case VK_OEM_5: ret = L"VK_OEM_5"; break;
    case VK_OEM_6: ret = L"VK_OEM_6"; break;
    case VK_OEM_7: ret = L"VK_OEM_7"; break;
    case VK_OEM_8: ret = L"VK_OEM_8"; break;
    case VK_OEM_AX: ret = L"VK_OEM_AX"; break;
    case VK_OEM_102: ret = L"VK_OEM_102"; break;
    case VK_ICO_HELP: ret = L"VK_ICO_HELP"; break;
    case VK_ICO_00: ret = L"VK_ICO_00"; break;
    case VK_PROCESSKEY: ret = L"VK_PROCESSKEY"; break;
    case VK_ICO_CLEAR: ret = L"VK_ICO_CLEAR"; break;
    case VK_PACKET: ret = L"VK_PACKET"; break;
    case VK_OEM_RESET: ret = L"VK_OEM_RESET"; break;
    case VK_OEM_JUMP: ret = L"VK_OEM_JUMP"; break;
    case VK_OEM_PA1: ret = L"VK_OEM_PA1"; break;
    case VK_OEM_PA2: ret = L"VK_OEM_PA2"; break;
    case VK_OEM_PA3: ret = L"VK_OEM_PA3"; break;
    case VK_OEM_WSCTRL: ret = L"VK_OEM_WSCTRL"; break;
    case VK_OEM_CUSEL: ret = L"VK_OEM_CUSEL"; break;
    case VK_OEM_ATTN: ret = L"VK_OEM_ATTN"; break;
    case VK_OEM_FINISH: ret = L"VK_OEM_FINISH"; break;
    case VK_OEM_COPY: ret = L"VK_OEM_COPY"; break;
    case VK_OEM_AUTO: ret = L"VK_OEM_AUTO"; break;
    case VK_OEM_ENLW: ret = L"VK_OEM_ENLW"; break;
    case VK_OEM_BACKTAB: ret = L"VK_OEM_BACKTAB"; break;
    case VK_ATTN: ret = L"VK_ATTN"; break;
    case VK_CRSEL: ret = L"VK_CRSEL"; break;
    case VK_EXSEL: ret = L"VK_EXSEL"; break;
    case VK_EREOF: ret = L"VK_EREOF"; break;
    case VK_PLAY: ret = L"VK_PLAY"; break;
    case VK_ZOOM: ret = L"VK_ZOOM"; break;
    case VK_NONAME: ret = L"VK_NONAME"; break;
    case VK_PA1: ret = L"VK_PA1"; break;
    case VK_OEM_CLEAR: ret = L"VK_OEM_CLEAR"; break;
    default:
        if ((L'A' <= w && w <= L'Z') || (L'0' <= w && w <= L'9'))
        {
            using namespace std;
            wchar_t sz[8];
            wsprintfW(sz, L"\"%c\"", w);
            ret = sz;
            break;
        }
        if (w == VK_KANA || w == VK_HANGUL)
        {
            if (GetACP() == 932)
            {
                ret = L"VK_KANA";
                break;
            }
            if (GetACP() == 949)
            {
                ret = L"VK_HANGEUL";
                break;
            }
        }
        if (w == VK_KANJI || w == VK_HANJA)
        {
            if (GetACP() == 932)
            {
                ret = L"VK_KANJI";
                break;
            }
            if (GetACP() == 949)
            {
                ret = L"VK_HANJA";
                break;
            }
        }
        if (w == VK_OEM_NEC_EQUAL || w == VK_OEM_FJ_JISHO)
        {
            ret == L"VK_OEM_NEC_EQUAL";
            break;
        }
        else
        {
            using namespace std;
            wchar_t sz[16];
            wsprintfW(sz, L"0x%04X", w);
            ret = sz;
        }
    }
    return ret;
}

inline std::wstring deci(UINT nID)
{
    using namespace std;
    wchar_t sz[32];
    wsprintfW(sz, L"%u", nID);
    std::wstring ret(sz);
    return ret;
}

inline std::wstring hexi(UINT nID)
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
replace_all(T_STR& str, const T_STR& from, const T_STR& to)
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
replace_all(T_STR& str,
            const typename T_STR::value_type *from,
            const typename T_STR::value_type *to)
{
    replace_all(str, T_STR(from), T_STR(to));
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
    replace_all(ret, L"\r\n", L"\n");
    replace_all(ret, L"\r", L"\n");
    replace_all(ret, L"\n", L"\r\n");
    return ret;
}

inline std::string quote(const std::string& str)
{
    std::string ret;
    ret += "\"";
    ret += escape(str);
    ret += "\"";
    return ret;
}

inline std::wstring quote(const std::wstring& str)
{
    std::wstring ret;
    ret += L"\"";
    ret += escape(str);
    ret += L"\"";
    return ret;
}

#endif  // ndef TEXT_HPP_
