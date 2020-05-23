// MRegKeyPortablePortable.hpp -- *.ini file manipulator        -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MREGKEYPORTABLE_HPP_
#define MZC4_MREGKEYPORTABLE_HPP_       7   /* Version 7 */

#ifndef HKCR
    #define HKCR    HKEY_CLASSES_ROOT
    #define HKCU    HKEY_CURRENT_USER
    #define HKLM    HKEY_LOCAL_MACHINE
    #define HKU     HKEY_USERS
    #define HKPD    HKEY_PERFORMANCE_DATA
    #define HKCC    HKEY_CURRENT_CONFIG
    #define HKDD    HKEY_DYN_DATA
#endif

class MRegKeyPortable;

////////////////////////////////////////////////////////////////////////////

#ifndef _INC_WINDOWS
    #include <windows.h>    // Win32API
#endif
#include <shlwapi.h>
#include <cassert>          // assert
#include <new>              // std::nothrow
#include <string>           // std::wstring

#ifdef UNICODE
    typedef std::wstring tstring;
#else
    typedef std::string tstring;
#endif

////////////////////////////////////////////////////////////////////////////

class MRegKeyPortable
{
public:
    MRegKeyPortable(LPCWSTR pszAppName, LPCWSTR pszIniFileName = NULL);
    virtual ~MRegKeyPortable();

    LONG RegCloseKey();

    LONG RegQueryValueEx(LPCTSTR pszValueName = NULL, 
                         LPDWORD lpReserved = NULL, LPDWORD lpType = NULL, 
                         LPBYTE lpData = NULL, LPDWORD lpcbData = NULL);

    LONG QueryBinary(LPCTSTR pszValueName, LPVOID pvValue, DWORD cb);
    LONG QueryDword(LPCTSTR pszValueName, DWORD& dw);
    LONG QueryDwordLE(LPCTSTR pszValueName, DWORD& dw);
    LONG QueryDwordBE(LPCTSTR pszValueName, DWORD& dw);
    LONG QuerySz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValue);
    LONG QueryExpandSz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValue);
    template <typename T_STRUCT>
    LONG QueryStruct(LPCTSTR pszValueName, T_STRUCT& data);

    template <typename T_STRING>
    LONG QuerySz(LPCTSTR pszValueName, T_STRING& strValue);
    template <typename T_STRING>
    LONG QueryExpandSz(LPCTSTR pszValueName, T_STRING& strValue);

    LONG RegSetValueEx(LPCTSTR pszValueName, DWORD dwReserved, 
        DWORD dwType, CONST BYTE *lpData, DWORD cbData);

    LONG SetBinary(LPCTSTR pszValueName, LPCVOID pvValue, DWORD cb);
    LONG SetDword(LPCTSTR pszValueName, DWORD dw);
    LONG SetDwordLE(LPCTSTR pszValueName, DWORD dw);
    LONG SetDwordBE(LPCTSTR pszValueName, DWORD dw);
    LONG SetSz(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD cchValue);
    LONG SetSz(LPCTSTR pszValueName, LPCTSTR pszValue);
    LONG SetExpandSz(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD cchValue);
    LONG SetExpandSz(LPCTSTR pszValueName, LPCTSTR pszValue);
    template <typename T_STRUCT>
    LONG SetStruct(LPCTSTR pszValueName, const T_STRUCT& data);

    LONG RegDeleteValue(LPCTSTR pszValueName);
    LONG RegFlushKey();

protected:
    tstring m_strIniFileName;
    tstring m_strAppName;
};

////////////////////////////////////////////////////////////////////////////

inline
MRegKeyPortable::MRegKeyPortable(LPCWSTR pszAppName, LPCWSTR pszIniFileName)
    : m_strAppName(pszAppName)
{
    if (pszIniFileName)
    {
        m_strIniFileName = pszIniFileName;
    }
    else
    {
        TCHAR szPath[MAX_PATH];
        GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
        LPTSTR pchDotExt = PathFindExtension(szPath);
        if (pchDotExt)
            *pchDotExt = 0;
        m_strIniFileName = szPath;
        m_strIniFileName += TEXT(".ini");
    }
}

template <typename T_STRUCT>
inline LONG MRegKeyPortable::QueryStruct(LPCTSTR pszValueName, T_STRUCT& data)
{
    DWORD cbData = sizeof(data);
    return MRegKeyPortable::RegQueryValueEx(pszValueName, NULL, NULL, (LPBYTE)&data, &cbData);
}

template <typename T_STRUCT>
inline LONG MRegKeyPortable::SetStruct(LPCTSTR pszValueName, const T_STRUCT& data)
{
    return MRegKeyPortable::RegSetValueEx(pszValueName, 0, REG_BINARY,
                                          (CONST BYTE *)&data, sizeof(data));
}

template <typename T_STRING>
LONG MRegKeyPortable::QuerySz(LPCTSTR pszValueName, T_STRING& strValue)
{
    LONG result;
    strValue.clear();

    DWORD cbData;
    result = RegQueryValueEx(pszValueName, NULL, NULL, NULL, &cbData);
    if (result != ERROR_SUCCESS)
        return result;

    LPTSTR psz = new(std::nothrow) TCHAR[cbData / sizeof(TCHAR) + 1];
    assert(psz);
    if (psz)
    {
        result = RegQueryValueEx(pszValueName, NULL, NULL, 
                                 reinterpret_cast<LPBYTE>(psz), &cbData);
        if (result != ERROR_SUCCESS)
        {
            strValue = psz;
        }
        delete[] psz;
    }
    return result;
}

template <typename T_STRING>
LONG MRegKeyPortable::QueryExpandSz(LPCTSTR pszValueName, T_STRING& strValue)
{
    LONG result;
    strValue.clear();

    DWORD cbData;
    result = RegQueryValueEx(pszValueName, NULL, NULL, NULL, &cbData);
    if (result != ERROR_SUCCESS)
        return result;

    LPTSTR psz = new(std::nothrow) TCHAR[cbData / sizeof(TCHAR) + 1];
    assert(psz);
    if (psz)
    {
        result = RegQueryValueEx(pszValueName, NULL, NULL, 
                                 reinterpret_cast<LPBYTE>(psz), &cbData);
        if (result != ERROR_SUCCESS)
        {
            strValue = psz;
        }
        delete[] psz;
    }
    return result;
}

inline /*virtual*/ MRegKeyPortable::~MRegKeyPortable()
{
    RegCloseKey();
}

inline LONG MRegKeyPortable::RegCloseKey()
{
    return MRegKeyPortable::RegFlushKey();
}

inline LONG MRegKeyPortable::RegQueryValueEx(LPCTSTR pszValueName/* = NULL*/, 
    LPDWORD lpReserved/* = NULL*/, LPDWORD lpType/* = NULL*/, 
    LPBYTE lpData/* = NULL*/, LPDWORD lpcbData/* = NULL*/)
{
    if (pszValueName == NULL)
        pszValueName = TEXT(".DEFAULT");

    static TCHAR s_szText[512];
    char sz[3];
    BOOL bOK;
    bOK = GetPrivateProfileString(m_strAppName.c_str(), pszValueName, TEXT(""),
                                  s_szText, ARRAYSIZE(s_szText), m_strIniFileName.c_str());
    if (!bOK || s_szText[0] == 0)
        return ERROR_ACCESS_DENIED;

    StrTrim(s_szText, TEXT(" \t\r\n"));

    DWORD cchText = lstrlen(s_szText);
    DWORD cbValue = (cchText - 1) / 2;

    if (lpcbData)
    {
        if (*lpcbData < cbValue)
        {
            *lpcbData = cbValue;
            return ERROR_MORE_DATA;
        }
        *lpcbData = cbValue;
    }

    if (lpType)
    {
        sz[0] = (char)s_szText[2 * cbValue];
        sz[1] = 0;
        *lpType = strtol(sz, NULL, 16);
    }

    if (lpData == NULL)
    {
        return ERROR_SUCCESS;
    }

    for (DWORD i = 0; i < cbValue; ++i)
    {
        sz[0] = (char)s_szText[2 * i + 0];
        sz[1] = (char)s_szText[2 * i + 1];
        sz[2] = 0;
        lpData[i] = (BYTE)strtol(sz, NULL, 16);
    }

    return ERROR_SUCCESS;
}

inline LONG MRegKeyPortable::QueryBinary(
    LPCTSTR pszValueName, LPVOID pvValue, DWORD cb)
{
    DWORD cbData = cb;
    return RegQueryValueEx(pszValueName, NULL, NULL, 
                           reinterpret_cast<LPBYTE>(pvValue), 
                           &cbData);
}

inline LONG MRegKeyPortable::QueryDword(LPCTSTR pszValueName, DWORD& dw)
{
    DWORD cbData = sizeof(DWORD);
    return RegQueryValueEx(pszValueName, NULL, NULL, 
                           reinterpret_cast<LPBYTE>(&dw), &cbData);
}

inline LONG MRegKeyPortable::QueryDwordLE(LPCTSTR pszValueName, DWORD& dw)
{
    DWORD cbData = sizeof(DWORD);
    return RegQueryValueEx(pszValueName, NULL, NULL, 
                           reinterpret_cast<LPBYTE>(&dw), &cbData);
}

inline LONG MRegKeyPortable::QueryDwordBE(LPCTSTR pszValueName, DWORD& dw)
{
    DWORD cbData = sizeof(DWORD);
    return RegQueryValueEx(pszValueName, NULL, NULL, 
                           reinterpret_cast<LPBYTE>(&dw), &cbData);
}

inline LONG
MRegKeyPortable::QuerySz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValue)
{
    DWORD cbData = cchValue * sizeof(TCHAR);
    return RegQueryValueEx(pszValueName, NULL, NULL, 
                           reinterpret_cast<LPBYTE>(pszValue), &cbData);
}

inline LONG MRegKeyPortable::QueryExpandSz(
    LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValue)
{
    DWORD cbData = cchValue * sizeof(TCHAR);
    return RegQueryValueEx(pszValueName, NULL, NULL, 
                           reinterpret_cast<LPBYTE>(pszValue), &cbData);
}

inline LONG MRegKeyPortable::RegSetValueEx(LPCTSTR pszValueName, DWORD dwReserved, 
    DWORD dwType, CONST BYTE *lpData, DWORD cbData)
{
    static TCHAR s_szText[512];
    static const TCHAR s_szHex[] = TEXT("0123456789ABCDEF");

    if (pszValueName == NULL)
        pszValueName = TEXT(".DEFAULT");

    if (cbData * 2 + 1 > ARRAYSIZE(s_szText))
        return ERROR_ACCESS_DENIED;

    DWORD i;
    for (i = 0; i < cbData; ++i)
    {
        s_szText[2 * i + 0] = s_szHex[(lpData[i] >> 4) & 0x0F];
        s_szText[2 * i + 1] = s_szHex[lpData[i] & 0x0F];
    }
    s_szText[2 * i + 0] = s_szHex[dwType & 0xF];
    s_szText[2 * i + 1] = 0;

    BOOL bOK = WritePrivateProfileString(m_strAppName.c_str(), pszValueName, s_szText,
                                         m_strIniFileName.c_str());
    return bOK ? ERROR_SUCCESS : ERROR_ACCESS_DENIED;
}

inline LONG MRegKeyPortable::SetBinary(LPCTSTR pszValueName, LPCVOID pvValue, DWORD cb)
{
    return RegSetValueEx(pszValueName, 0, REG_BINARY, 
        reinterpret_cast<const BYTE *>(pvValue), cb);
}

inline LONG MRegKeyPortable::SetDword(LPCTSTR pszValueName, DWORD dw)
{
    DWORD dwValue = dw;
    return RegSetValueEx(pszValueName, 0, REG_DWORD, 
        reinterpret_cast<const BYTE *>(&dwValue), sizeof(DWORD));
}

inline LONG MRegKeyPortable::SetDwordLE(LPCTSTR pszValueName, DWORD dw)
{
    DWORD dwValue = dw;
    return RegSetValueEx(pszValueName, 0, REG_DWORD_LITTLE_ENDIAN, 
        reinterpret_cast<const BYTE *>(&dwValue), sizeof(DWORD));
}

inline LONG MRegKeyPortable::SetDwordBE(LPCTSTR pszValueName, DWORD dw)
{
    DWORD dwValue = dw;
    return RegSetValueEx(pszValueName, 0, REG_DWORD_BIG_ENDIAN, 
        reinterpret_cast<const BYTE *>(&dwValue), sizeof(DWORD));
}

inline LONG
MRegKeyPortable::SetSz(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD cchValue)
{
    return RegSetValueEx(pszValueName, 0, REG_SZ, 
        reinterpret_cast<const BYTE *>(pszValue), cchValue * sizeof(TCHAR));
}

inline LONG
MRegKeyPortable::SetExpandSz(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD cchValue)
{
    return RegSetValueEx(pszValueName, 0, REG_EXPAND_SZ, 
        reinterpret_cast<const BYTE *>(pszValue), cchValue * sizeof(TCHAR));
}

inline LONG MRegKeyPortable::RegDeleteValue(LPCTSTR pszValueName)
{
    BOOL bOK = WritePrivateProfileString(m_strAppName.c_str(), pszValueName, NULL,
                                         m_strIniFileName.c_str());
    return (bOK ? ERROR_SUCCESS : ERROR_ACCESS_DENIED);
}

inline LONG MRegKeyPortable::RegFlushKey()
{
    ::WritePrivateProfileString(NULL, NULL, NULL, m_strIniFileName.c_str());
    return ERROR_SUCCESS;
}

inline LONG MRegKeyPortable::SetSz(LPCTSTR pszValueName, LPCTSTR pszValue)
{
    return SetSz(pszValueName, pszValue, lstrlen(pszValue) + 1);
}

inline LONG MRegKeyPortable::SetExpandSz(LPCTSTR pszValueName, LPCTSTR pszValue)
{
    return SetExpandSz(pszValueName, pszValue, lstrlen(pszValue) + 1);
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MREGKEYPORTABLE_HPP_
