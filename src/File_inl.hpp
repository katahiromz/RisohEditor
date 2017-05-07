////////////////////////////////////////////////////////////////////////////
// File_inl.hpp -- Win32 file or pipe wrapper
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

MZC_INLINE MFile::MFile() : m_hFile(INVALID_HANDLE_VALUE)
{
}

MZC_INLINE MFile::MFile(HANDLE hFile) : m_hFile(hFile)
{
}

MZC_INLINE MFile::MFile(LPCTSTR pszFileName, BOOL bOutput/* = FALSE*/,
                        DWORD dwFILE_SHARE_/* = FILE_SHARE_READ*/)
    : m_hFile(INVALID_HANDLE_VALUE)
{
    if (bOutput)
        OpenFileForOutput(pszFileName, dwFILE_SHARE_);
    else
        OpenFileForInput(pszFileName, dwFILE_SHARE_);
}

MZC_INLINE /*virtual*/ MFile::~MFile()
{
    if (m_hFile != INVALID_HANDLE_VALUE && m_hFile != NULL)
        CloseHandle();
}

MZC_INLINE MFile::operator HANDLE() const
{
    return m_hFile;
}

MZC_INLINE PHANDLE MFile::operator&()
{
    return &m_hFile;
}

MZC_INLINE MFile::operator bool() const
{
    return m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE;
}

MZC_INLINE bool MFile::operator!() const
{
    return m_hFile == NULL || m_hFile == INVALID_HANDLE_VALUE;
}

MZC_INLINE bool MFile::operator==(HANDLE hFile) const
{
    return m_hFile == hFile;
}

MZC_INLINE bool MFile::operator!=(HANDLE hFile) const
{
    return m_hFile != hFile;
}

MZC_INLINE bool MFile::operator==(const MFile& file) const
{
    return m_hFile == file.m_hFile;
}

MZC_INLINE bool MFile::operator!=(const MFile& file) const
{
    return m_hFile != file.m_hFile;
}

MZC_INLINE BOOL MFile::OpenFileForInput(
    LPCTSTR pszFileName, DWORD dwFILE_SHARE_/* = FILE_SHARE_READ*/)
{
    return MFile::CreateFile(pszFileName, GENERIC_READ,
        dwFILE_SHARE_, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

MZC_INLINE BOOL MFile::OpenFileForOutput(
    LPCTSTR pszFileName, DWORD dwFILE_SHARE_/* = FILE_SHARE_READ*/)
{
    return MFile::CreateFile(pszFileName, GENERIC_WRITE,
        dwFILE_SHARE_, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

MZC_INLINE BOOL MFile::OpenFileForRandom(
    LPCTSTR pszFileName, DWORD dwFILE_SHARE_/* = FILE_SHARE_READ*/)
{
    return MFile::CreateFile(pszFileName,
        GENERIC_READ | GENERIC_WRITE, dwFILE_SHARE_, NULL, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
}

MZC_INLINE DWORD MFile::WaitForSingleObject(
    DWORD dwTimeout/* = INFINITE*/)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::WaitForSingleObject(m_hFile, dwTimeout);
}

MZC_INLINE MFile& MFile::operator=(HANDLE hFile)
{
#ifdef _DEBUG
    BY_HANDLE_FILE_INFORMATION info;
    assert(hFile == INVALID_HANDLE_VALUE ||
        ::GetFileInformationByHandle(hFile, &info));
#endif
    if (m_hFile != hFile)
        Attach(hFile);
    return *this;
}

MZC_INLINE void MFile::Attach(HANDLE hFile)
{
    if (m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE)
        CloseHandle();
    assert(hFile != NULL && hFile != INVALID_HANDLE_VALUE);
    assert(m_hFile == NULL || m_hFile == INVALID_HANDLE_VALUE);
#ifdef _DEBUG
    BY_HANDLE_FILE_INFORMATION info;
    assert(::GetFileInformationByHandle(hFile, &info));
#endif
    m_hFile = hFile;
}

MZC_INLINE HANDLE MFile::Detach()
{
    HANDLE hFile = m_hFile;
    m_hFile = NULL;
    return hFile;
}

MZC_INLINE BOOL MFile::CloseHandle()
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    BOOL b = ::CloseHandle(m_hFile);
    m_hFile = INVALID_HANDLE_VALUE;
    return b;
}

MZC_INLINE BOOL MFile::PeekNamedPipe(
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

MZC_INLINE BOOL MFile::ReadFile(LPVOID pBuffer, DWORD cbToRead,
    LPDWORD pcbRead, LPOVERLAPPED pOverlapped/* = NULL*/)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::ReadFile(m_hFile, pBuffer, cbToRead, pcbRead, pOverlapped);
}

MZC_INLINE BOOL MFile::WriteFile(LPCVOID pBuffer, DWORD cbToWrite,
    LPDWORD pcbWritten, LPOVERLAPPED pOverlapped/* = NULL*/)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::WriteFile(
        m_hFile, pBuffer, cbToWrite, pcbWritten, pOverlapped);
}

MZC_INLINE BOOL MFile::WriteSzA(LPCSTR psz,
    LPDWORD pcbWritten, LPOVERLAPPED pOverlapped/* = NULL*/)
{
    using namespace std;
    SIZE_T size = strlen(psz) * sizeof(CHAR);
#ifdef _WIN64
    if (size > 0x7FFFFFFF)
        MzcFootmarkThis();
#endif
    return WriteFile(psz, (DWORD)size, pcbWritten, pOverlapped);
}

MZC_INLINE BOOL MFile::WriteSzW(LPCWSTR psz,
    LPDWORD pcbWritten, LPOVERLAPPED pOverlapped/* = NULL*/)
{
    using namespace std;
    SIZE_T size = wcslen(psz) * sizeof(WCHAR);
#ifdef _WIN64
    if (size > 0x7FFFFFFF)
        MzcFootmarkThis();
#endif
    return WriteFile(psz, (DWORD)size, pcbWritten, pOverlapped);
}

MZC_INLINE BOOL MFile::WriteSz(LPCTSTR psz,
    LPDWORD pcbWritten, LPOVERLAPPED pOverlapped/* = NULL*/)
{
    return WriteFile(psz, (DWORD)(lstrlen(psz) * sizeof(TCHAR)), pcbWritten, pOverlapped);
}

MZC_INLINE BOOL MFile::CreateFile(LPCTSTR pszFileName,
    DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES pSA, DWORD dwCreationDistribution,
    DWORD dwFlagsAndAttributes/* = FILE_ATTRIBUTE_NORMAL*/,
    HANDLE hTemplateFile/* = NULL*/)
{
    assert(m_hFile == NULL || m_hFile == INVALID_HANDLE_VALUE);
    Attach(::CreateFile(pszFileName, dwDesiredAccess, dwShareMode,
        pSA, dwCreationDistribution, dwFlagsAndAttributes, hTemplateFile));
    return (m_hFile != INVALID_HANDLE_VALUE);
}

MZC_INLINE DWORD MFile::SetFilePointer(
    LONG nDeltaLow,
    PLONG pnDeltaHigh/* = NULL*/,
    DWORD dwOrigin/* = FILE_BEGIN*/)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::SetFilePointer(m_hFile, nDeltaLow, pnDeltaHigh, dwOrigin);
}

MZC_INLINE DWORD MFile::SeekToEnd()
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return SetFilePointer(0, NULL, FILE_END);
}

MZC_INLINE void MFile::SeekToBegin()
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    SetFilePointer(0, NULL, FILE_BEGIN);
}

MZC_INLINE DWORD MFile::GetFileSize(LPDWORD pdwHighPart/* = NULL*/) const
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::GetFileSize(m_hFile, pdwHighPart);
}

MZC_INLINE DWORDLONG MFile::GetFileSize64() const
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    DWORD dwLow, dwHigh;
    dwLow = ::GetFileSize(m_hFile, &dwHigh);
    if (dwLow == 0xFFFFFFFF && ::GetLastError() != NO_ERROR)
        return 0;
    else
        return MAKELONGLONG(dwLow, dwHigh);
}

MZC_INLINE BOOL MFile::SetEndOfFile()
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::SetEndOfFile(m_hFile);
}

MZC_INLINE BOOL MFile::FlushFileBuffers()
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::FlushFileBuffers(m_hFile);
}

MZC_INLINE BOOL MFile::WriteSzA(LPCSTR psz)
{
    assert(psz);
    int cb = lstrlenA(psz);
    return WriteBinary(psz, (DWORD) cb);
}

MZC_INLINE BOOL MFile::WriteSzW(LPCWSTR psz)
{
    assert(psz);
    int cb = lstrlenW(psz) * sizeof(WCHAR);
    return WriteBinary(psz, (DWORD) cb);
}

MZC_INLINE BOOL MFile::WriteSz(LPCTSTR psz)
{
    assert(psz);
    int cb = lstrlen(psz) * sizeof(TCHAR);
    return WriteBinary(psz, (DWORD) cb);
}

MZC_INLINE BOOL MFile::GetFileTime(
    LPFILETIME pftCreate/* = NULL*/,
    LPFILETIME pftLastAccess/* = NULL*/,
    LPFILETIME pftLastWrite/* = NULL*/) const
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::GetFileTime(m_hFile, pftCreate, pftLastAccess, pftLastWrite);
}

MZC_INLINE DWORD MFile::GetFileType() const
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::GetFileType(m_hFile);
}

MZC_INLINE BOOL MFile::GetFileInformationByHandle(LPBY_HANDLE_FILE_INFORMATION info)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::GetFileInformationByHandle(m_hFile, info);
}

MZC_INLINE BOOL MFile::LockFile(DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
    DWORD dwNumberOfBytesToLockLow, DWORD dwNumberOfBytesToLockHigh)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::LockFile(m_hFile, dwFileOffsetLow, dwFileOffsetHigh,
        dwNumberOfBytesToLockLow, dwNumberOfBytesToLockHigh);
}

MZC_INLINE BOOL MFile::LockFile(DWORDLONG dwlFileOffset, DWORDLONG dwlNumberOfBytesToLock)
{
    return MFile::LockFile(LOLONG(dwlFileOffset), HILONG(dwlFileOffset),
        LOLONG(dwlNumberOfBytesToLock), HILONG(dwlNumberOfBytesToLock));
}

MZC_INLINE BOOL MFile::LockFileEx(DWORD dwFlags, DWORD dwReserved,
    DWORD dwNumberOfBytesToLockLow, DWORD dwNumberOfBytesToLockHigh,
    LPOVERLAPPED lpOverlapped)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::LockFileEx(m_hFile, dwFlags, dwReserved,
        dwNumberOfBytesToLockLow, dwNumberOfBytesToLockHigh, lpOverlapped);
}

MZC_INLINE BOOL MFile::LockFileEx(DWORD dwFlags, DWORD dwReserved,
    DWORDLONG dwlNumberOfBytesToLock, LPOVERLAPPED lpOverlapped)
{
    return MFile::LockFileEx(dwFlags, dwReserved,
        LOLONG(dwlNumberOfBytesToLock), HILONG(dwlNumberOfBytesToLock),
        lpOverlapped);
}

MZC_INLINE BOOL MFile::UnlockFile(DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
    DWORD dwNumberOfBytesToUnlockLow, DWORD dwNumberOfBytesToUnlockHigh)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::UnlockFile(m_hFile, dwFileOffsetLow, dwFileOffsetHigh,
        dwNumberOfBytesToUnlockLow, dwNumberOfBytesToUnlockHigh);
}

MZC_INLINE BOOL MFile::UnlockFile(DWORDLONG dwFileOffset, DWORDLONG dwNumberOfBytesToUnlock)
{
    return MFile::UnlockFile(LOLONG(dwFileOffset), HILONG(dwFileOffset),
        LOLONG(dwNumberOfBytesToUnlock), HILONG(dwNumberOfBytesToUnlock));
}

MZC_INLINE BOOL MFile::UnlockFileEx(DWORD dwReserved, DWORD dwNumberOfBytesToUnlockLow,
    DWORD dwNumberOfBytesToUnlockHigh, LPOVERLAPPED lpOverlapped)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::UnlockFileEx(m_hFile, dwReserved, dwNumberOfBytesToUnlockLow,
        dwNumberOfBytesToUnlockHigh, lpOverlapped);
}

MZC_INLINE BOOL MFile::UnlockFileEx(DWORD dwReserved, DWORDLONG dwlNumberOfBytesToUnlock,
    LPOVERLAPPED lpOverlapped)
{
    return MFile::UnlockFileEx(dwReserved, LOLONG(dwlNumberOfBytesToUnlock),
        HILONG(dwlNumberOfBytesToUnlock), lpOverlapped);
}

MZC_INLINE BOOL MFile::ReadFileEx(LPVOID pBuffer, DWORD cbToRead, 
    LPOVERLAPPED pOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::ReadFileEx(m_hFile, pBuffer, cbToRead, pOverlapped, lpCompletionRoutine);
}

MZC_INLINE BOOL MFile::WriteFileEx(LPCVOID pBuffer, DWORD cbToWrite, 
    LPOVERLAPPED pOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    assert(m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
    return ::WriteFileEx(m_hFile, pBuffer, cbToWrite, pOverlapped, lpCompletionRoutine);
}


MZC_INLINE BOOL MFile::GetStdHandle(DWORD dwSTD_)
{
    Attach(::GetStdHandle(dwSTD_));
    return (m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
}

MZC_INLINE BOOL MFile::GetStdIn()
{
    Attach(::GetStdHandle(STD_INPUT_HANDLE));
    return (m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
}

MZC_INLINE BOOL MFile::GetStdOut()
{
    Attach(::GetStdHandle(STD_OUTPUT_HANDLE));
    return (m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
}

MZC_INLINE BOOL MFile::GetStdErr()
{
    Attach(::GetStdHandle(STD_ERROR_HANDLE));
    return (m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE);
}

MZC_INLINE BOOL MFile::SetStdHandle(DWORD dwSTD_) const
{
    return ::SetStdHandle(dwSTD_, m_hFile);
}

MZC_INLINE BOOL MFile::SetStdIn() const
{
    return ::SetStdHandle(STD_INPUT_HANDLE, m_hFile);
}

MZC_INLINE BOOL MFile::SetStdOut() const
{
    return ::SetStdHandle(STD_OUTPUT_HANDLE, m_hFile);
}

MZC_INLINE BOOL MFile::SetStdErr() const
{
    return ::SetStdHandle(STD_ERROR_HANDLE, m_hFile);
}

////////////////////////////////////////////////////////////////////////////

MZC_INLINE BOOL MzcPathExists(LPCTSTR psz)
{
    assert(psz);
    DWORD attrs = ::GetFileAttributes(psz);
    return attrs != 0xFFFFFFFF;
}

MZC_INLINE BOOL MzcFileExists(LPCTSTR psz)
{
    assert(psz);
    DWORD attrs = ::GetFileAttributes(psz);
    return attrs != 0xFFFFFFFF && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

MZC_INLINE BOOL MzcDirExists(LPCTSTR psz)
{
    assert(psz);
    DWORD attrs = ::GetFileAttributes(psz);
    return attrs != 0xFFFFFFFF && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

MZC_INLINE LPSTR MzcSetDotExtA(LPSTR pszPath, LPCSTR pszDotExt)
{
    assert(pszPath);
    assert(pszDotExt);
    lstrcpyA(MzcFindDotExtA(pszPath), pszDotExt);
    return pszPath;
}

MZC_INLINE LPWSTR MzcSetDotExtW(LPWSTR pszPath, LPCWSTR pszDotExt)
{
    assert(pszPath);
    assert(pszDotExt);
    lstrcpyW(MzcFindDotExtW(pszPath), pszDotExt);
    return pszPath;
}

MZC_INLINE BOOL MzcCopyFile(LPCTSTR pszExistingFile, LPCTSTR pszNewFile,
                            BOOL bFailIfExists/* = FALSE*/)
{
    assert(pszExistingFile);
    assert(pszNewFile);
    return ::CopyFile(pszExistingFile, pszNewFile, bFailIfExists);
}

MZC_INLINE DWORD MzcGetFullPathName(
    LPCTSTR pszPath, DWORD cchBuff, LPTSTR pszFull, LPTSTR *pFilePart/* = NULL*/)
{
    assert(pszPath);
    assert(pszFull);
    return ::GetFullPathName(pszPath, cchBuff, pszFull, pFilePart);
}

MZC_INLINE BOOL MzcMoveFile(LPCTSTR pszExistingFile, LPCTSTR pszNewFile)
{
    assert(pszExistingFile);
    assert(pszNewFile);
    return ::MoveFile(pszExistingFile, pszNewFile);
}

MZC_INLINE BOOL MzcMoveFileEx(LPCTSTR pszExistingFile, LPCTSTR pszNewFile, DWORD dwFlags)
{
    assert(pszExistingFile);
    assert(pszNewFile);
    return ::MoveFileEx(pszExistingFile, pszNewFile, dwFlags);
}

MZC_INLINE BOOL MzcPutFileContents(LPCTSTR pszFile, LPCTSTR psz)
{
    return MzcPutFileContents(pszFile, psz, lstrlen(psz) * sizeof(TCHAR));
}
