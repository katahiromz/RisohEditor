////////////////////////////////////////////////////////////////////////////
// File.cpp -- Win32 file or pipe wrapper
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#include "stdafx.hpp"

#ifdef MZC_NO_INLINING
    #undef MZC_INLINE
    #define MZC_INLINE  /*empty*/
    #include "File_inl.hpp"
#endif

using namespace std;

////////////////////////////////////////////////////////////////////////////

#ifndef MZC_WRAP_SHLWAPI
    void MzcAddBackslashA(LPSTR pszPath)
    {
        assert(pszPath);
        std::size_t cchPath = strlen(pszPath);
        if (cchPath == 0)
            return;
        LPSTR pchPrev = CharPrevA(pszPath, pszPath + cchPath);
        if (*pchPrev != '\\' && *pchPrev != '/')
        {
            pszPath[cchPath++] = '\\';
            pszPath[cchPath] = '\0';
        }
    }

    void MzcAddBackslashW(LPWSTR pszPath)
    {
        assert(pszPath);
        std::size_t cchPath = wcslen(pszPath);
        if (cchPath == 0)
            return;
        LPWSTR pchPrev = pszPath + cchPath - 1;
        if (*pchPrev != L'\\' && *pchPrev != L'/')
        {
            pszPath[cchPath++] = L'\\';
            pszPath[cchPath] = L'\0';
        }
    }

    void MzcRemoveBackslashA(LPSTR pszPath)
    {
        assert(pszPath);
        std::size_t cchPath = strlen(pszPath);
        if (cchPath == 0)
            return;
        LPSTR pchPrev = CharPrevA(pszPath, pszPath + cchPath);
        if (*pchPrev == '\\' || *pchPrev == '/')
            *pchPrev = '\0';
    }

    void MzcRemoveBackslashW(LPWSTR pszPath)
    {
        assert(pszPath);
        std::size_t cchPath = wcslen(pszPath);
        if (cchPath == 0)
            return;
        LPWSTR pchPrev = pszPath + cchPath - 1;
        if (*pchPrev == L'\\' || *pchPrev == L'/')
            *pchPrev = L'\0';
    }

    LPSTR MzcFindFileNameA(LPSTR pszPath)
    {
        assert(pszPath);
        #ifdef ANSI
            LPSTR pch1 = strrchr(pszPath, '\\');
            LPSTR pch2 = strrchr(pszPath, '/');
        #else
            LPSTR pch1 = reinterpret_cast<LPSTR>(
                _mbsrchr(reinterpret_cast<LPBYTE>(pszPath), '\\'));
            LPSTR pch2 = reinterpret_cast<LPSTR>(
                _mbsrchr(reinterpret_cast<LPBYTE>(pszPath), '/'));
        #endif
        if (pch1 == NULL && pch2 == NULL)
            return pszPath;
        if (pch1 == NULL)
            return pch2 + 1;
        if (pch2 == NULL)
            return pch1 + 1;
        if (pch1 < pch2)
            return pch2 + 1;
        else
            return pch1 + 1;
    }

    LPWSTR MzcFindFileNameW(LPWSTR pszPath)
    {
        assert(pszPath);
        LPWSTR pch1 = wcsrchr(pszPath, L'\\');
        LPWSTR pch2 = wcsrchr(pszPath, L'/');
        if (pch1 == NULL && pch2 == NULL)
            return pszPath;
        if (pch1 == NULL)
            return pch2 + 1;
        if (pch2 == NULL)
            return pch1 + 1;
        if (pch1 < pch2)
            return pch2 + 1;
        else
            return pch1 + 1;
    }
#endif  // ndef MZC_WRAP_SHLWAPI

LPSTR MzcFindDotExtA(LPSTR pszPath)
{
    assert(pszPath);
    pszPath = MzcFindFileNameA(pszPath);
    #ifdef ANSI
        LPSTR pch = strrchr(pszPath, '.');
    #else
        LPSTR pch = reinterpret_cast<LPSTR>(
            _mbsrchr(reinterpret_cast<LPBYTE>(pszPath), '.'));
    #endif
    if (pch)
        return pch;
    else
        return pszPath + strlen(pszPath);
}

LPWSTR MzcFindDotExtW(LPWSTR pszPath)
{
    assert(pszPath);
    pszPath = MzcFindFileNameW(pszPath);
    LPWSTR pch = wcsrchr(pszPath, L'.');
    if (pch)
        return pch;
    else
        return pszPath + lstrlenW(pszPath);
}

////////////////////////////////////////////////////////////////////////////

LPBYTE MzcGetFileContents(LPCTSTR pszFile, LPDWORD pdwSize/* = NULL*/)
{
    assert(pszFile);

    HANDLE hFile;
    DWORD cbFile, cbRead, dwLastError;
    LPBYTE pb = NULL;

    if (pdwSize)
        *pdwSize = 0;

    hFile = ::CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        cbFile = ::GetFileSize(hFile, NULL);
        if (cbFile != 0xFFFFFFFF)
        {
            LPVOID pv = malloc((cbFile + 1) * sizeof(char));
            pb = reinterpret_cast<LPBYTE>(pv);
            if (pb)
            {
                if (::ReadFile(hFile, pb, cbFile, &cbRead, NULL) &&
                    cbFile == cbRead)
                {
                    pb[cbFile] = 0;
                    if (pdwSize)
                        *pdwSize = cbFile;
                }
                else
                {
                    dwLastError = ::GetLastError();
                    free(pb);
                    pb = NULL;
                }
            }
            else
                dwLastError = ERROR_OUTOFMEMORY;
        }
        else
            dwLastError = ::GetLastError();

        ::CloseHandle(hFile);
    }
    else
        dwLastError = ::GetLastError();

    if (pb == NULL)
    {
        MzcFootmark();
        ::SetLastError(dwLastError);
    }

    return pb;
}

BOOL MzcPutFileContents(LPCTSTR pszFile, LPCVOID pvContents, DWORD dwSize)
{
    assert(pszFile);
    assert(pvContents || dwSize == 0);

    HANDLE hFile;
    DWORD cbWritten, dwLastError;
    BOOL bOK = FALSE;

    hFile = ::CreateFile(pszFile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
        CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (::WriteFile(hFile, pvContents, dwSize, &cbWritten, NULL) &&
            dwSize == cbWritten)
        {
            if (::CloseHandle(hFile))
                bOK = TRUE;
            else
                dwLastError = ::GetLastError();
        }
        else
        {
            dwLastError = ::GetLastError();
            ::CloseHandle(hFile);
        }

        if (!bOK)
            ::DeleteFile(pszFile);
    }
    else
        dwLastError = ::GetLastError();

    if (!bOK)
    {
        MzcFootmark();
        ::SetLastError(dwLastError);
    }

    return bOK;
}

////////////////////////////////////////////////////////////////////////////

BOOL MzcCreateDirectoryRecursive(LPCTSTR pszPath, BOOL fForce/* = FALSE*/)
{
    assert(pszPath);

    // check length
    if (lstrlen(pszPath) >= MAX_PATH)
    {
        ::SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;   // failure
    }

    // save it local to recurse
    TCHAR szPath[MAX_PATH];
    lstrcpyn(szPath, pszPath, MAX_PATH);
    MzcRemoveBackslash(szPath);

    // does it exists?
    DWORD attrs = ::GetFileAttributes(szPath);
    if (attrs == 0xFFFFFFFF)
    {
retry:;
        // no, it doesn't. find the last separator
        LPTSTR pchLastSep = NULL;
        for(LPTSTR pch = szPath; *pch; pch = CharNext(pch))
        {
            if (*pch == _T('\\') || *pch == _T('/'))
                pchLastSep = pch;
        }

        if (pchLastSep != NULL)
        {
            // found sep; recurse the parent directory
            *pchLastSep = 0;
            if (MzcCreateDirectoryRecursive(szPath))
                return ::CreateDirectory(pszPath, NULL);

            return FALSE;   // failure
        }
        else
        {
            // no sep; a create
            return ::CreateDirectory(szPath, NULL);
        }
    }

    if (!(attrs & FILE_ATTRIBUTE_DIRECTORY))
    {
        // shit, same name non-directory file exists
        if (!fForce)
        {
            ::SetLastError(ERROR_ALREADY_EXISTS);
            return FALSE;
        }

        // don't disturb us or delete you by force of arms!
        if (!::DeleteFile(szPath))
            return FALSE;   // OMG, failure!

        goto retry;
    }

    return TRUE;    // the directory exists. success.
}

inline BOOL MzcIsDots_(LPCTSTR psz)
{
    assert(psz);
    return (psz[0] == _T('.') &&
        (psz[1] == _T('\0') || (psz[1] == _T('.') && psz[2] == _T('\0')))
    );
}

BOOL MzcDeleteDirectory(LPCTSTR pszDir)
{
    assert(pszDir);

    DWORD attrs;
    TCHAR szDirOld[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATA find;

    attrs = ::GetFileAttributes(pszDir);
    if (attrs == 0xFFFFFFFF || !(attrs & FILE_ATTRIBUTE_DIRECTORY))
        return FALSE;

    ::GetCurrentDirectory(MAX_PATH, szDirOld);
    if (!::SetCurrentDirectory(pszDir))
        return FALSE;

    hFind = ::FindFirstFile(_T("*"), &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!MzcIsDots_(find.cFileName))
            {
                if (find.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                {
                    // ignore
                }
                else
                {
                    if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        MzcDeleteDirectory(find.cFileName);
                    else
                        ::DeleteFile(find.cFileName);
                }
            }
        } while(::FindNextFile(hFind, &find));
        ::FindClose(hFind);
    }
    ::SetCurrentDirectory(szDirOld);

    return ::RemoveDirectory(pszDir);
}

////////////////////////////////////////////////////////////////////////////
// MFile -- Win32 file or pipe

BOOL MFile::DuplicateHandle(PHANDLE phFile, BOOL bInherit)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    HANDLE hProcess = ::GetCurrentProcess();
    return ::DuplicateHandle(hProcess, m_hFile, hProcess, phFile, 0,
        bInherit, DUPLICATE_SAME_ACCESS);
}

BOOL MFile::DuplicateHandle(
    PHANDLE phFile, BOOL bInherit, DWORD dwDesiredAccess)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    HANDLE hProcess = ::GetCurrentProcess();
    return ::DuplicateHandle(hProcess, m_hFile, hProcess, phFile,
        dwDesiredAccess, bInherit, 0);
}

BOOL __cdecl MFile::WriteFormatA(LPCSTR pszFormat, ...)
{
    assert(pszFormat);
    assert(strlen(pszFormat) < 1024);
    va_list argList;
    CHAR sz[1024];
    va_start(argList, pszFormat);
    wvsprintfA(sz, pszFormat, argList);
    BOOL b = WriteSzA(sz);
    va_end(argList);
    return b;
}

BOOL __cdecl MFile::WriteFormatW(LPCWSTR pszFormat, ...)
{
    assert(pszFormat);
    assert(wcslen(pszFormat) < 1024);
    va_list argList;
    WCHAR sz[1024];
    va_start(argList, pszFormat);
    wvsprintfW(sz, pszFormat, argList);
    BOOL b = WriteSzW(sz);
    va_end(argList);
    return b;
}

BOOL __cdecl MFile::WriteFormat(LPCTSTR pszFormat, ...)
{
    assert(pszFormat);
    assert(lstrlen(pszFormat) < 1024);
    va_list argList;
    TCHAR sz[1024];
    va_start(argList, pszFormat);
    wvsprintf(sz, pszFormat, argList);
    BOOL b = WriteSz(sz);
    va_end(argList);
    return b;
}

BOOL MFile::OpenFileForAppend(
    LPCTSTR pszFileName, DWORD dwFILE_SHARE_/* = FILE_SHARE_READ*/)
{
    assert(pszFileName);
    BOOL bExisted = (::GetFileAttributes(pszFileName) != 0xFFFFFFFF);
    if (!MFile::CreateFile(pszFileName, GENERIC_READ | GENERIC_WRITE,
        dwFILE_SHARE_, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL))
        return FALSE;
    if (SetFilePointer(0, NULL, FILE_END) == 0xFFFFFFFF)
    {
        assert(false);
        CloseHandle();
        if (!bExisted)
            ::DeleteFile(pszFileName);
        return FALSE;
    }
    return TRUE;
}

BOOL MFile::WriteBinary(LPCVOID pv, DWORD cb)
{
    assert(pv || cb == 0);
    const BYTE *pb = reinterpret_cast<const BYTE *>(pv);
    DWORD cbWritten;
    while (cb != 0)
    {
        if (WriteFile(pb, cb, &cbWritten))
        {
            cb -= cbWritten;
            pb += cbWritten;
        }
        else
            break;
    }
    return (cb == 0) && FlushFileBuffers();
}

////////////////////////////////////////////////////////////////////////////

#ifdef UNITTEST
    #include <cstdio>
    using namespace std;
    int main(void)
    {
        {
            MFile file(TEXT("a.txt"), TRUE);
            file.WriteSz("TEST");
            printf("size: %lu\n", file.GetFileSize());
        }
        if (MzcDeleteFile(TEXT("a.txt")))
            printf("success\n");
        else
            printf("failure\n");
        return 0;
    }
#endif

////////////////////////////////////////////////////////////////////////////
