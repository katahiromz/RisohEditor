// MFile.hpp -- Win32API file or pipe wrapper                  -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MFILE_HPP_
#define MZC4_MFILE_HPP_     12      /* Version 12 */

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#include <tchar.h>
#include <cassert>
#ifndef NO_STRSAFE
    #include <strsafe.h>
#endif

class MFile;

////////////////////////////////////////////////////////////////////////////

#ifndef LOLONG
    #define LOLONG(dwl) static_cast<DWORD>(dwl)
#endif
#ifndef HILONG
    #define HILONG(dwl) static_cast<DWORD>(((dwl) >> 32) & 0xFFFFFFFF)
#endif
#ifndef MAKELONGLONG
    #define MAKELONGLONG(lo,hi) \
        (static_cast<DWORD>(lo) | \
            (static_cast<DWORDLONG>(static_cast<DWORD>(hi)) << 32))
#endif

////////////////////////////////////////////////////////////////////////////

class MFile
{
public:
    HANDLE m_hFile;

public:
    MFile();
    MFile(HANDLE hFile);
    MFile(const MFile& file);
    MFile& operator=(HANDLE hFile);
    MFile& operator=(const MFile& file);
    MFile(LPCTSTR pszFileName, BOOL bOutput = FALSE,
          DWORD dwFILE_SHARE_ = FILE_SHARE_READ);
    virtual ~MFile();

    operator HANDLE() const;
    PHANDLE operator&();

    bool operator!() const;
    bool operator==(HANDLE hFile) const;
    bool operator!=(HANDLE hFile) const;
    bool operator==(const MFile& file) const;
    bool operator!=(const MFile& file) const;

    BOOL Attach(HANDLE hFile);
    HANDLE Detach();
    HANDLE Handle() const;
    BOOL CloseHandle();

    BOOL DuplicateHandle(PHANDLE phFile, BOOL bInherit);
    BOOL DuplicateHandle(PHANDLE phFile, BOOL bInherit,
                         DWORD dwDesiredAccess);
    DWORD WaitForSingleObject(DWORD dwTimeout = INFINITE);

    BOOL PeekNamedPipe(LPVOID pBuffer = NULL, DWORD cbBuffer = 0,
                       LPDWORD pcbRead = NULL, LPDWORD pcbAvail = NULL,
                       LPDWORD pBytesLeft = NULL);
    BOOL ReadFile(LPVOID pBuffer, DWORD cbToRead, LPDWORD pcbRead,
                  LPOVERLAPPED pOverlapped = NULL);
    BOOL WriteFile(LPCVOID pBuffer, DWORD cbToWrite, LPDWORD pcbWritten,
                   LPOVERLAPPED pOverlapped = NULL);
    BOOL WriteSzA(LPCSTR psz, LPDWORD pcbWritten,
                  LPOVERLAPPED pOverlapped = NULL);
    BOOL WriteSzW(LPCWSTR psz, LPDWORD pcbWritten,
                  LPOVERLAPPED pOverlapped = NULL);
    BOOL WriteSz(LPCTSTR psz, LPDWORD pcbWritten,
                 LPOVERLAPPED pOverlapped = NULL);

    BOOL         WriteBinary(LPCVOID pv, DWORD cb);
    BOOL         WriteSzA(LPCSTR psz);
    BOOL         WriteSzW(LPCWSTR psz);
    BOOL         WriteSz(LPCTSTR psz);
    BOOL __cdecl WriteFormatA(LPCSTR pszFormat, ...);
    BOOL __cdecl WriteFormatW(LPCWSTR pszFormat, ...);
    BOOL __cdecl WriteFormat(LPCTSTR pszFormat, ...);

    BOOL GetStdHandle(DWORD dwSTD_);
    BOOL GetStdIn();
    BOOL GetStdOut();
    BOOL GetStdErr();
    BOOL SetStdHandle(DWORD dwSTD_) const;
    BOOL SetStdIn() const;
    BOOL SetStdOut() const;
    BOOL SetStdErr() const;

    BOOL OpenFileForInput(LPCTSTR pszFileName,
                          DWORD dwFILE_SHARE_ = FILE_SHARE_READ);
    BOOL OpenFileForOutput(LPCTSTR pszFileName,
                           DWORD dwFILE_SHARE_ = FILE_SHARE_READ);
    BOOL OpenFileForRandom(LPCTSTR pszFileName,
                           DWORD dwFILE_SHARE_ = FILE_SHARE_READ);
    BOOL OpenFileForAppend(LPCTSTR pszFileName,
                           DWORD dwFILE_SHARE_ = FILE_SHARE_READ);

    BOOL CreateFile(LPCTSTR pszFileName, DWORD dwDesiredAccess,
                    DWORD dwShareMode, LPSECURITY_ATTRIBUTES pSA,
                    DWORD dwCreationDistribution,
                    DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL,
                    HANDLE hTemplateFile = NULL);
    DWORD GetFileSize(LPDWORD pdwHighPart = NULL) const;
    DWORDLONG GetFileSize64() const;
    BOOL SetEndOfFile();
    DWORD SetFilePointer(LONG nDeltaLow, PLONG pnDeltaHigh = NULL,
                         DWORD dwOrigin = FILE_BEGIN);
    VOID SeekToBegin();
    DWORD SeekToEnd();
    BOOL FlushFileBuffers();
    BOOL GetFileTime(LPFILETIME pftCreate = NULL,
                     LPFILETIME pftLastAccess = NULL,
                     LPFILETIME pftLastWrite = NULL) const;

    BOOL GetFileInformationByHandle(LPBY_HANDLE_FILE_INFORMATION info);
    DWORD GetFileType() const;

    BOOL LockFile(DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                  DWORD dwNumberOfBytesToLockLow,
                  DWORD dwNumberOfBytesToLockHigh);
    BOOL LockFile(DWORDLONG dwlFileOffset, DWORDLONG dwlNumberOfBytesToLock);

    BOOL LockFileEx(DWORD dwFlags, DWORD dwReserved,
                    DWORD dwNumberOfBytesToLockLow,
                    DWORD dwNumberOfBytesToLockHigh,
                    LPOVERLAPPED lpOverlapped);
    BOOL LockFileEx(DWORD dwFlags, DWORD dwReserved,
                    DWORDLONG dwlNumberOfBytesToLock,
                    LPOVERLAPPED lpOverlapped);

    BOOL UnlockFile(DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                    DWORD dwNumberOfBytesToUnlockLow,
                    DWORD dwNumberOfBytesToUnlockHigh);
    BOOL UnlockFile(DWORDLONG dwFileOffset, DWORDLONG dwNumberOfBytesToUnlock);

    BOOL UnlockFileEx(DWORD dwReserved, DWORD dwNumberOfBytesToUnlockLow,
                      DWORD dwNumberOfBytesToUnlockHigh,
                      LPOVERLAPPED lpOverlapped);
    BOOL UnlockFileEx(DWORD dwReserved, DWORDLONG dwlNumberOfBytesToUnlock,
                      LPOVERLAPPED lpOverlapped);

    BOOL ReadFileEx(LPVOID pBuffer, DWORD cbToRead, LPOVERLAPPED pOverlapped,
                    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
    BOOL WriteFileEx(LPCVOID pBuffer, DWORD cbToWrite, LPOVERLAPPED pOverlapped,
                     LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

    static HANDLE CloneHandleDx(HANDLE hFile);
};

////////////////////////////////////////////////////////////////////////////

inline MFile::MFile() : m_hFile(INVALID_HANDLE_VALUE)
{
}

inline MFile::MFile(HANDLE hFile) : m_hFile(hFile)
{
}

inline MFile::MFile(const MFile& file)
    : m_hFile(CloneHandleDx(file.m_hFile))
{
}

inline MFile& MFile::operator=(const MFile& file)
{
    if (Handle() != file.m_hFile)
    {
        HANDLE hFile = CloneHandleDx(file.m_hFile);
        Attach(hFile);
    }
    return *this;
}

inline MFile::MFile(LPCTSTR pszFileName, BOOL bOutput/* = FALSE*/,
                        DWORD dwFILE_SHARE_/* = FILE_SHARE_READ*/)
    : m_hFile(INVALID_HANDLE_VALUE)
{
    if (bOutput)
        OpenFileForOutput(pszFileName, dwFILE_SHARE_);
    else
        OpenFileForInput(pszFileName, dwFILE_SHARE_);
}

inline /*virtual*/ MFile::~MFile()
{
    CloseHandle();
}

inline HANDLE MFile::Handle() const
{
    if (this == NULL)
        return INVALID_HANDLE_VALUE;
    if (m_hFile == NULL)
        return INVALID_HANDLE_VALUE;
    return m_hFile;
}

inline MFile::operator HANDLE() const
{
    return Handle();
}

inline PHANDLE MFile::operator&()
{
    return &m_hFile;
}

inline bool MFile::operator!() const
{
    HANDLE hFile = Handle();
    return hFile == INVALID_HANDLE_VALUE || hFile == NULL;
}

inline bool MFile::operator==(HANDLE hFile) const
{
    return Handle() == hFile;
}

inline bool MFile::operator!=(HANDLE hFile) const
{
    return Handle() != hFile;
}

inline bool MFile::operator==(const MFile& file) const
{
    return Handle() == file.Handle();
}

inline bool MFile::operator!=(const MFile& file) const
{
    return Handle() != file.Handle();
}

inline BOOL MFile::OpenFileForInput(
    LPCTSTR pszFileName, DWORD dwFILE_SHARE_/* = FILE_SHARE_READ*/)
{
    return MFile::CreateFile(pszFileName, GENERIC_READ,
        dwFILE_SHARE_, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

inline BOOL MFile::OpenFileForOutput(
    LPCTSTR pszFileName, DWORD dwFILE_SHARE_/* = FILE_SHARE_READ*/)
{
    return MFile::CreateFile(pszFileName, GENERIC_WRITE,
        dwFILE_SHARE_, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

inline BOOL MFile::OpenFileForRandom(
    LPCTSTR pszFileName, DWORD dwFILE_SHARE_/* = FILE_SHARE_READ*/)
{
    return MFile::CreateFile(pszFileName,
        GENERIC_READ | GENERIC_WRITE, dwFILE_SHARE_, NULL, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
}

inline DWORD MFile::WaitForSingleObject(
    DWORD dwTimeout/* = INFINITE*/)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::WaitForSingleObject(m_hFile, dwTimeout);
}

inline MFile& MFile::operator=(HANDLE hFile)
{
#ifndef NDEBUG
    BY_HANDLE_FILE_INFORMATION info;
    assert(hFile == INVALID_HANDLE_VALUE ||
        ::GetFileInformationByHandle(hFile, &info));
#endif
    if (Handle() != hFile)
    {
        Attach(hFile);
    }
    return *this;
}

inline BOOL MFile::Attach(HANDLE hFile)
{
    CloseHandle();
    assert(hFile != NULL && hFile != INVALID_HANDLE_VALUE);
    assert(m_hFile == NULL || m_hFile == INVALID_HANDLE_VALUE);
#ifndef NDEBUG
    BY_HANDLE_FILE_INFORMATION info;
    assert(::GetFileInformationByHandle(hFile, &info));
#endif
    m_hFile = hFile;
    return m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE;
}

inline HANDLE MFile::Detach()
{
    HANDLE hFile = m_hFile;
    m_hFile = NULL;
    return hFile;
}

inline BOOL MFile::CloseHandle()
{
    if (Handle() != NULL && Handle() != INVALID_HANDLE_VALUE)
    {
        BOOL bOK = ::CloseHandle(Detach());
        return bOK;
    }
    return FALSE;
}

inline BOOL MFile::PeekNamedPipe(
    LPVOID pBuffer/* = NULL*/,
    DWORD cbBuffer/* = 0*/,
    LPDWORD pcbRead/* = NULL*/,
    LPDWORD pcbAvail/* = NULL*/,
    LPDWORD pBytesLeft/* = NULL*/)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::PeekNamedPipe(m_hFile, pBuffer, cbBuffer,
        pcbRead, pcbAvail, pBytesLeft);
}

inline BOOL MFile::ReadFile(LPVOID pBuffer, DWORD cbToRead,
    LPDWORD pcbRead, LPOVERLAPPED pOverlapped/* = NULL*/)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::ReadFile(m_hFile, pBuffer, cbToRead, pcbRead, pOverlapped);
}

inline BOOL MFile::WriteFile(LPCVOID pBuffer, DWORD cbToWrite,
    LPDWORD pcbWritten, LPOVERLAPPED pOverlapped/* = NULL*/)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::WriteFile(
        m_hFile, pBuffer, cbToWrite, pcbWritten, pOverlapped);
}

inline BOOL MFile::WriteSzA(LPCSTR psz,
    LPDWORD pcbWritten, LPOVERLAPPED pOverlapped/* = NULL*/)
{
    using namespace std;
    SIZE_T size = strlen(psz) * sizeof(CHAR);
    return WriteFile(psz, (DWORD)size, pcbWritten, pOverlapped);
}

inline BOOL MFile::WriteSzW(LPCWSTR psz,
    LPDWORD pcbWritten, LPOVERLAPPED pOverlapped/* = NULL*/)
{
    using namespace std;
    SIZE_T size = wcslen(psz) * sizeof(WCHAR);
    return WriteFile(psz, (DWORD)size, pcbWritten, pOverlapped);
}

inline BOOL MFile::WriteSz(LPCTSTR psz,
    LPDWORD pcbWritten, LPOVERLAPPED pOverlapped/* = NULL*/)
{
    return WriteFile(psz, (DWORD)(lstrlen(psz) * sizeof(TCHAR)), pcbWritten, pOverlapped);
}

inline BOOL MFile::CreateFile(LPCTSTR pszFileName,
    DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES pSA, DWORD dwCreationDistribution,
    DWORD dwFlagsAndAttributes/* = FILE_ATTRIBUTE_NORMAL*/,
    HANDLE hTemplateFile/* = NULL*/)
{
    return Attach(::CreateFile(pszFileName, dwDesiredAccess, dwShareMode,
                  pSA, dwCreationDistribution, dwFlagsAndAttributes, hTemplateFile));
}

inline DWORD MFile::SetFilePointer(
    LONG nDeltaLow,
    PLONG pnDeltaHigh/* = NULL*/,
    DWORD dwOrigin/* = FILE_BEGIN*/)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::SetFilePointer(m_hFile, nDeltaLow, pnDeltaHigh, dwOrigin);
}

inline DWORD MFile::SeekToEnd()
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return SetFilePointer(0, NULL, FILE_END);
}

inline VOID MFile::SeekToBegin()
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    SetFilePointer(0, NULL, FILE_BEGIN);
}

inline DWORD MFile::GetFileSize(LPDWORD pdwHighPart/* = NULL*/) const
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::GetFileSize(m_hFile, pdwHighPart);
}

inline DWORDLONG MFile::GetFileSize64() const
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    DWORD dwLow, dwHigh;
    dwLow = ::GetFileSize(m_hFile, &dwHigh);
    if (dwLow == 0xFFFFFFFF && ::GetLastError() != NO_ERROR)
        return 0;
    else
        return MAKELONGLONG(dwLow, dwHigh);
}

inline BOOL MFile::SetEndOfFile()
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::SetEndOfFile(m_hFile);
}

inline BOOL MFile::FlushFileBuffers()
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::FlushFileBuffers(m_hFile);
}

inline BOOL MFile::WriteSzA(LPCSTR psz)
{
    assert(psz);
    INT cb = lstrlenA(psz);
    return WriteBinary(psz, (DWORD) cb);
}

inline BOOL MFile::WriteSzW(LPCWSTR psz)
{
    assert(psz);
    INT cb = lstrlenW(psz) * sizeof(WCHAR);
    return WriteBinary(psz, (DWORD) cb);
}

inline BOOL MFile::WriteSz(LPCTSTR psz)
{
    assert(psz);
    INT cb = lstrlen(psz) * sizeof(TCHAR);
    return WriteBinary(psz, (DWORD) cb);
}

inline BOOL MFile::GetFileTime(
    LPFILETIME pftCreate/* = NULL*/,
    LPFILETIME pftLastAccess/* = NULL*/,
    LPFILETIME pftLastWrite/* = NULL*/) const
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::GetFileTime(m_hFile, pftCreate, pftLastAccess, pftLastWrite);
}

inline DWORD MFile::GetFileType() const
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::GetFileType(m_hFile);
}

inline BOOL MFile::GetFileInformationByHandle(LPBY_HANDLE_FILE_INFORMATION info)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::GetFileInformationByHandle(m_hFile, info);
}

inline BOOL MFile::LockFile(DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
    DWORD dwNumberOfBytesToLockLow, DWORD dwNumberOfBytesToLockHigh)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::LockFile(m_hFile, dwFileOffsetLow, dwFileOffsetHigh,
        dwNumberOfBytesToLockLow, dwNumberOfBytesToLockHigh);
}

inline BOOL MFile::LockFile(DWORDLONG dwlFileOffset, DWORDLONG dwlNumberOfBytesToLock)
{
    return MFile::LockFile(LOLONG(dwlFileOffset), HILONG(dwlFileOffset),
        LOLONG(dwlNumberOfBytesToLock), HILONG(dwlNumberOfBytesToLock));
}

inline BOOL MFile::LockFileEx(DWORD dwFlags, DWORD dwReserved,
    DWORD dwNumberOfBytesToLockLow, DWORD dwNumberOfBytesToLockHigh,
    LPOVERLAPPED lpOverlapped)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::LockFileEx(m_hFile, dwFlags, dwReserved,
        dwNumberOfBytesToLockLow, dwNumberOfBytesToLockHigh, lpOverlapped);
}

inline BOOL MFile::LockFileEx(DWORD dwFlags, DWORD dwReserved,
    DWORDLONG dwlNumberOfBytesToLock, LPOVERLAPPED lpOverlapped)
{
    return MFile::LockFileEx(dwFlags, dwReserved,
        LOLONG(dwlNumberOfBytesToLock), HILONG(dwlNumberOfBytesToLock),
        lpOverlapped);
}

inline BOOL MFile::UnlockFile(DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
    DWORD dwNumberOfBytesToUnlockLow, DWORD dwNumberOfBytesToUnlockHigh)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::UnlockFile(m_hFile, dwFileOffsetLow, dwFileOffsetHigh,
        dwNumberOfBytesToUnlockLow, dwNumberOfBytesToUnlockHigh);
}

inline BOOL MFile::UnlockFile(DWORDLONG dwFileOffset, DWORDLONG dwNumberOfBytesToUnlock)
{
    return MFile::UnlockFile(LOLONG(dwFileOffset), HILONG(dwFileOffset),
        LOLONG(dwNumberOfBytesToUnlock), HILONG(dwNumberOfBytesToUnlock));
}

inline BOOL MFile::UnlockFileEx(DWORD dwReserved, DWORD dwNumberOfBytesToUnlockLow,
    DWORD dwNumberOfBytesToUnlockHigh, LPOVERLAPPED lpOverlapped)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::UnlockFileEx(m_hFile, dwReserved, dwNumberOfBytesToUnlockLow,
        dwNumberOfBytesToUnlockHigh, lpOverlapped);
}

inline BOOL MFile::UnlockFileEx(DWORD dwReserved, DWORDLONG dwlNumberOfBytesToUnlock,
    LPOVERLAPPED lpOverlapped)
{
    return MFile::UnlockFileEx(dwReserved, LOLONG(dwlNumberOfBytesToUnlock),
        HILONG(dwlNumberOfBytesToUnlock), lpOverlapped);
}

inline BOOL MFile::ReadFileEx(LPVOID pBuffer, DWORD cbToRead, 
    LPOVERLAPPED pOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::ReadFileEx(m_hFile, pBuffer, cbToRead, pOverlapped, lpCompletionRoutine);
}

inline BOOL MFile::WriteFileEx(LPCVOID pBuffer, DWORD cbToWrite, 
    LPOVERLAPPED pOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::WriteFileEx(m_hFile, pBuffer, cbToWrite, pOverlapped, lpCompletionRoutine);
}


inline BOOL MFile::GetStdHandle(DWORD dwSTD_)
{
    return Attach(::GetStdHandle(dwSTD_));
}

inline BOOL MFile::GetStdIn()
{
    return Attach(::GetStdHandle(STD_INPUT_HANDLE));
}

inline BOOL MFile::GetStdOut()
{
    return Attach(::GetStdHandle(STD_OUTPUT_HANDLE));
}

inline BOOL MFile::GetStdErr()
{
    return Attach(::GetStdHandle(STD_ERROR_HANDLE));
}

inline BOOL MFile::SetStdHandle(DWORD dwSTD_) const
{
    return ::SetStdHandle(dwSTD_, m_hFile);
}

inline BOOL MFile::SetStdIn() const
{
    return ::SetStdHandle(STD_INPUT_HANDLE, m_hFile);
}

inline BOOL MFile::SetStdOut() const
{
    return ::SetStdHandle(STD_OUTPUT_HANDLE, m_hFile);
}

inline BOOL MFile::SetStdErr() const
{
    return ::SetStdHandle(STD_ERROR_HANDLE, m_hFile);
}

////////////////////////////////////////////////////////////////////////////

inline BOOL MFile::DuplicateHandle(PHANDLE phFile, BOOL bInherit)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    HANDLE hProcess = ::GetCurrentProcess();
    return ::DuplicateHandle(hProcess, m_hFile, hProcess, phFile, 0,
        bInherit, DUPLICATE_SAME_ACCESS);
}

inline BOOL MFile::DuplicateHandle(
    PHANDLE phFile, BOOL bInherit, DWORD dwDesiredAccess)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    HANDLE hProcess = ::GetCurrentProcess();
    return ::DuplicateHandle(hProcess, m_hFile, hProcess, phFile,
        dwDesiredAccess, bInherit, 0);
}

inline BOOL __cdecl MFile::WriteFormatA(LPCSTR pszFormat, ...)
{
    assert(pszFormat);
    assert(strlen(pszFormat) < 1024);
    va_list argList;
    CHAR sz[1024];
    va_start(argList, pszFormat);
#ifndef NO_STRSAFE
    StringCchVPrintfA(sz, _countof(sz), pszFormat, argList);
#else
    wvsprintfA(sz, pszFormat, argList);
#endif
    BOOL b = WriteSzA(sz);
    va_end(argList);
    return b;
}

inline BOOL __cdecl MFile::WriteFormatW(LPCWSTR pszFormat, ...)
{
    assert(pszFormat);
    assert(wcslen(pszFormat) < 1024);
    va_list argList;
    WCHAR sz[1024];
    va_start(argList, pszFormat);
#ifndef NO_STRSAFE
    StringCchVPrintfW(sz, _countof(sz), pszFormat, argList);
#else
    wvsprintfW(sz, pszFormat, argList);
#endif
    BOOL b = WriteSzW(sz);
    va_end(argList);
    return b;
}

inline BOOL __cdecl MFile::WriteFormat(LPCTSTR pszFormat, ...)
{
    assert(pszFormat);
    assert(lstrlen(pszFormat) < 1024);
    va_list argList;
    TCHAR sz[1024];
    va_start(argList, pszFormat);
#ifndef NO_STRSAFE
    StringCchVPrintf(sz, _countof(sz), pszFormat, argList);
#else
    wvsprintf(sz, pszFormat, argList);
#endif
    BOOL b = WriteSz(sz);
    va_end(argList);
    return b;
}

inline BOOL MFile::OpenFileForAppend(
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

inline BOOL MFile::WriteBinary(LPCVOID pv, DWORD cb)
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

inline /*static*/ HANDLE MFile::CloneHandleDx(HANDLE hFile)
{
    if (hFile == INVALID_HANDLE_VALUE || hFile == NULL)
        return INVALID_HANDLE_VALUE;

    HANDLE hProcess = ::GetCurrentProcess();
    HANDLE hDup = INVALID_HANDLE_VALUE;
    ::DuplicateHandle(hProcess, hFile, hProcess, &hDup, 0,
                      FALSE, DUPLICATE_SAME_ACCESS);
    return hDup;
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MFILE_HPP_
