////////////////////////////////////////////////////////////////////////////
// MProcessMaker_inl.hpp -- Win32 process maker
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

MZC_INLINE MProcessMaker::MProcessMaker()
{
    Init();
}

MZC_INLINE /*virtual*/ MProcessMaker::~MProcessMaker()
{
    CloseAll();
}

MZC_INLINE MProcessMaker::operator HANDLE() const
{
    return m_pi.hProcess;
}

MZC_INLINE BOOL MProcessMaker::TerminateProcess(UINT uExitCode)
{
    return ::TerminateProcess(m_pi.hProcess, uExitCode);
}

MZC_INLINE void MProcessMaker::SetStdInput(HANDLE hStdIn)
{
    m_si.hStdInput = hStdIn;
    if (hStdIn != NULL)
        m_si.dwFlags |= STARTF_USESTDHANDLES;
    else
        m_si.dwFlags &= ~STARTF_USESTDHANDLES;
}

MZC_INLINE void MProcessMaker::SetStdOutput(HANDLE hStdOut)
{
    m_si.hStdOutput = hStdOut;
    if (hStdOut != NULL)
        m_si.dwFlags |= STARTF_USESTDHANDLES;
    else
        m_si.dwFlags &= ~STARTF_USESTDHANDLES;
}

MZC_INLINE void MProcessMaker::SetStdError(HANDLE hStdErr)
{
    m_si.hStdError = hStdErr;
    if (hStdErr != NULL)
        m_si.dwFlags |= STARTF_USESTDHANDLES;
    else
        m_si.dwFlags &= ~STARTF_USESTDHANDLES;
}

MZC_INLINE void MProcessMaker::SetShowWindow(int nCmdShow/* = SW_HIDE*/)
{
    m_si.wShowWindow = static_cast<WORD>(nCmdShow);
    m_si.dwFlags |= STARTF_USESHOWWINDOW;
}

MZC_INLINE void MProcessMaker::SetCreationFlags(
    DWORD dwFlags/* = CREATE_NEW_CONSOLE*/)
{
    m_dwCreationFlags = dwFlags;
}

MZC_INLINE void MProcessMaker::SetCurrentDirectory(LPCTSTR pszCurDir)
{
    m_pszCurDir = pszCurDir;
}

MZC_INLINE void MProcessMaker::SetDesktop(LPTSTR lpDesktop)
{
    m_si.lpDesktop = lpDesktop;
}

MZC_INLINE void MProcessMaker::SetTitle(LPTSTR lpTitle)
{
    m_si.lpTitle = lpTitle;
}

MZC_INLINE void MProcessMaker::SetPosition(DWORD dwX, DWORD dwY)
{
    m_si.dwX = dwX;
    m_si.dwY = dwY;
    m_si.dwFlags |= STARTF_USEPOSITION;
}

MZC_INLINE void MProcessMaker::SetSize(DWORD dwXSize, DWORD dwYSize)
{
    m_si.dwXSize = dwXSize;
    m_si.dwYSize = dwYSize;
    m_si.dwFlags |= STARTF_USESIZE;
}

MZC_INLINE void MProcessMaker::SetCountChars(
    DWORD dwXCountChars, DWORD dwYCountChars)
{
    m_si.dwXCountChars = dwXCountChars;
    m_si.dwYCountChars = dwYCountChars;
    m_si.dwFlags |= STARTF_USECOUNTCHARS;
}

MZC_INLINE void MProcessMaker::SetFillAttirbutes(DWORD dwFillAttribute)
{
    m_si.dwFillAttribute = dwFillAttribute;
    m_si.dwFlags |= STARTF_USEFILLATTRIBUTE;
}

MZC_INLINE HANDLE MProcessMaker::GetProcessHandle() const
{
    return m_pi.hProcess;
}

MZC_INLINE HANDLE MProcessMaker::GetThreadHandle() const
{
    return m_pi.hThread;
}

MZC_INLINE DWORD MProcessMaker::GetExitCode() const
{
    assert(m_pi.hProcess != NULL);
    DWORD dwExitCode;
    ::GetExitCodeProcess(m_pi.hProcess, &dwExitCode);
    return dwExitCode;
}

MZC_INLINE DWORD MProcessMaker::WaitForSingleObject(DWORD dwTimeout/* = INFINITE*/)
{
    assert(m_pi.hProcess != NULL);
    return ::WaitForSingleObject(m_pi.hProcess, dwTimeout);
}

MZC_INLINE DWORD MProcessMaker::WaitForSingleObjectEx(
    DWORD dwTimeout/* = INFINITE*/, BOOL bAlertable/* = TRUE*/)
{
    assert(m_pi.hProcess != NULL);
    return ::WaitForSingleObjectEx(m_pi.hProcess, dwTimeout, bAlertable);
}

MZC_INLINE BOOL MProcessMaker::IsRunning() const
{
    return (m_pi.hProcess != NULL &&
        ::WaitForSingleObject(m_pi.hProcess, 0) == WAIT_TIMEOUT);
}

MZC_INLINE MProcessMaker::operator bool() const
{
    return m_pi.hProcess != NULL;
}

MZC_INLINE bool MProcessMaker::operator!() const
{
    return m_pi.hProcess == NULL;
}

MZC_INLINE PROCESS_INFORMATION& MProcessMaker::ProcessInfo()
{
    return m_pi;
}

MZC_INLINE STARTUPINFO& MProcessMaker::StartupInfo()
{
    return m_si;
}

MZC_INLINE const PROCESS_INFORMATION& MProcessMaker::ProcessInfo() const
{
    return m_pi;
}

MZC_INLINE const STARTUPINFO& MProcessMaker::StartupInfo() const
{
    return m_si;
}

MZC_INLINE void MProcessMaker::CloseProcessHandle()
{
    if (m_pi.hProcess != NULL)
    {
        ::CloseHandle(m_pi.hProcess);
        m_pi.hProcess = NULL;
    }
}

MZC_INLINE void MProcessMaker::CloseThreadHandle()
{
    if (m_pi.hThread != NULL)
    {
        ::CloseHandle(m_pi.hThread);
        m_pi.hThread = NULL;
    }
}

MZC_INLINE void MProcessMaker::CloseStdInput()
{
    HANDLE hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
    if (m_si.hStdInput != hStdInput)
    {
        ::CloseHandle(m_si.hStdInput);
        m_si.hStdInput = hStdInput;
    }
}

MZC_INLINE void MProcessMaker::CloseStdOutput()
{
    HANDLE hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (m_si.hStdOutput != hStdOutput)
    {
        ::CloseHandle(m_si.hStdOutput);
        m_si.hStdOutput = hStdOutput;
    }
}

MZC_INLINE void MProcessMaker::CloseStdError()
{
    HANDLE hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
    if (m_si.hStdError != hStdError)
    {
        ::CloseHandle(m_si.hStdError);
        m_si.hStdError = hStdError;
    }
}

MZC_INLINE void MProcessMaker::CloseAll()
{
    CloseProcessHandle();
    CloseThreadHandle();
    CloseStdInput();
    CloseStdOutput();
    CloseStdError();
}

MZC_INLINE BOOL MProcessMaker::PrepareForRedirect(
    PHANDLE phInputWrite, PHANDLE phOutputRead, PSECURITY_ATTRIBUTES psa)
{
    return PrepareForRedirect(phInputWrite, phOutputRead, phOutputRead, psa);
}
