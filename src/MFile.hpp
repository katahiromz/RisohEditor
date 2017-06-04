////////////////////////////////////////////////////////////////////////////
// MFile.hpp -- Win32 file or pipe wrapper
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MFILE_HPP_
#define MZC4_MFILE_HPP_

////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <cassert>

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

#ifndef MzcFootmark
    #define MzcFootmark()
#endif
#ifndef MzcFootmarkThis
    #define MzcFootmarkThis()
#endif

////////////////////////////////////////////////////////////////////////////
// MFile -- Win32 file or pipe

class MFile
{
public:
    HANDLE m_hFile;

public:
    MFile();
    MFile(HANDLE hFile);
    MFile(LPCTSTR pszFileName, BOOL bOutput = FALSE,
          DWORD dwFILE_SHARE_ = FILE_SHARE_READ);
    virtual ~MFile();

    operator HANDLE() const;
    PHANDLE operator&();
    operator bool() const;
    bool operator!() const;
    bool operator==(HANDLE hFile) const;
    bool operator!=(HANDLE hFile) const;
    bool operator==(const MFile& file) const;
    bool operator!=(const MFile& file) const;

    MFile& operator=(HANDLE hFile);
    void Attach(HANDLE hFile);
    HANDLE Detach();
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
    void SeekToBegin();
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

private:
    // NOTE: MFile is not copyable
    MFile(const MFile& file);
    MFile& operator=(const MFile& file);
};

////////////////////////////////////////////////////////////////////////////
// path

BOOL MzcPathExists(LPCTSTR psz);
#define MzcGetShortPathName GetShortPathName
#ifdef GetLongPathName
    #define MzcGetLongPathName GetLongPathName
#endif
DWORD MzcGetFullPathName(
    LPCTSTR pszPath, DWORD cchBuff, LPTSTR pszFull, LPTSTR *pFilePart = NULL);
#define MzcGetTempPath GetTempPath
#define MzcGetTempFileName GetTempFileName

////////////////////////////////////////////////////////////////////////////
// directory

BOOL MzcDirExists(LPCTSTR psz);
#define MzcCreateDirectory CreateDirectory
#define MzcCreateDirectoryEx CreateDirectoryEx
BOOL MzcCreateDirectoryRecursive(LPCTSTR pszPath, BOOL fForce = FALSE);
#define MzcRemoveDirectory RemoveDirectory
BOOL MzcDeleteDirectory(LPCTSTR pszDir);
#define MzcGetCurrentDirectory GetCurrentDirectory
#define MzcSetCurrentDirectory SetCurrentDirectory

////////////////////////////////////////////////////////////////////////////
// file

BOOL MzcFileExists(LPCTSTR psz);
#define MzcGetFileAttributes GetFileAttributes
#define MzcSetFileAttributes SetFileAttributes
LPBYTE MzcGetFileContents(LPCTSTR pszFile, LPDWORD pdwSize = NULL);
BOOL MzcPutFileContents(LPCTSTR pszFile, LPCVOID pvContents, DWORD dwSize);
BOOL MzcPutFileContents(LPCTSTR pszFile, LPCTSTR psz);
#define MzcDeleteFile DeleteFile
BOOL MzcMoveFile(LPCTSTR pszExistingFile, LPCTSTR pszNewFile);
BOOL MzcMoveFileEx(LPCTSTR pszExistingFile, LPCTSTR pszNewFile, DWORD dwFlags);
BOOL MzcCopyFile(LPCTSTR pszExistingFile, LPCTSTR pszNewFile,
                 BOOL bFailIfExists = FALSE);

////////////////////////////////////////////////////////////////////////////
// file name extension

LPSTR MzcFindDotExtA(LPSTR pszPath);
LPWSTR MzcFindDotExtW(LPWSTR pszPath);
LPSTR MzcSetDotExtA(LPSTR pszPath, LPCSTR pszDotExt);
LPWSTR MzcSetDotExtW(LPWSTR pszPath, LPCWSTR pszDotExt);

////////////////////////////////////////////////////////////////////////////
// drive

#define MzcGetLogicalDrives GetLogicalDrives
#define MzcGetDiskFreeSpace GetDiskFreeSpace
#define MzcGetDiskFreeSpaceEx GetDiskFreeSpaceEx

#ifdef MZC_WRAP_SHLWAPI
    #define MzcAddBackslashA PathAddBackslashA
    #define MzcAddBackslashW PathAddBackslashW
    #define MzcRemoveBackslashA PathRemoveBackslashA
    #define MzcRemoveBackslashW PathRemoveBackslashW
    #define MzcFindFileNameA PathFindFileNameA
    #define MzcFindFileNameW PathFindFileNameW
#else
    void MzcAddBackslashA(LPSTR pszPath);
    void MzcAddBackslashW(LPWSTR pszPath);
    void MzcRemoveBackslashA(LPSTR pszPath);
    void MzcRemoveBackslashW(LPWSTR pszPath);
    LPSTR MzcFindFileNameA(LPSTR pszPath);
    LPWSTR MzcFindFileNameW(LPWSTR pszPath);
#endif

#ifdef UNICODE
    #define MzcAddBackslash MzcAddBackslashW
    #define MzcRemoveBackslash MzcRemoveBackslashW
    #define MzcFindFileName MzcFindFileNameW
    #define MzcFindDotExt MzcFindDotExtW
#else
    #define MzcAddBackslash MzcAddBackslashA
    #define MzcRemoveBackslash MzcRemoveBackslashA
    #define MzcFindFileName MzcFindFileNameA
    #define MzcFindDotExt MzcFindDotExtA
#endif

////////////////////////////////////////////////////////////////////////////

#ifndef MZC_NO_INLINING
    #undef MZC_INLINE
    #define MZC_INLINE inline
    #include "MFile_inl.hpp"
#endif

#endif  // ndef MZC4_MFILE_HPP_
