// MProcessMaker.hpp -- Win32API process maker                  -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MPROCESSMAKER_HPP_
#define MZC4_MPROCESSMAKER_HPP_     7   /* Version 7 */

#include "MFile.hpp"
#include "MSecurityAttributes.hpp"
#include <tchar.h>
#include <cstring>

////////////////////////////////////////////////////////////////////////////

class MProcessMaker
{
public:
    MProcessMaker();
    MProcessMaker(LPCTSTR pszAppName, LPCTSTR pszCommandLine = NULL,
                  LPCTSTR pszzEnvironment = NULL, BOOL bInherit = TRUE,
                  LPSECURITY_ATTRIBUTES lpProcessAttributes = NULL,
                  LPSECURITY_ATTRIBUTES lpThreadAttributes = NULL);
    virtual ~MProcessMaker();

    bool operator!() const;
    operator HANDLE() const;
    HANDLE Handle() const;

    HANDLE  GetProcessHandle() const;
    HANDLE  GetThreadHandle() const;
    DWORD   GetExitCode() const;

    // set attributes for child process
    VOID SetShowWindow(INT nCmdShow = SW_HIDE);
    VOID SetCreationFlags(DWORD dwFlags = CREATE_NEW_CONSOLE);
    VOID SetCurrentDirectory(LPCTSTR pszCurDir);
    VOID SetDesktop(LPTSTR lpDesktop);
    VOID SetTitle(LPTSTR lpTitle);
    VOID SetPosition(DWORD dwX, DWORD dwY);
    VOID SetSize(DWORD dwXSize, DWORD dwYSize);
    VOID SetCountChars(DWORD dwXCountChars, DWORD dwYCountChars);
    VOID SetFillAttirbutes(DWORD dwFillAttribute);

    VOID SetStdInput(HANDLE hStdIn);
    VOID SetStdOutput(HANDLE hStdOut);
    VOID SetStdError(HANDLE hStdErr);

    BOOL PrepareForRedirect(PHANDLE phInputWrite, PHANDLE phOutputRead);
    BOOL PrepareForRedirect(PHANDLE phInputWrite, PHANDLE phOutputRead,
                            PHANDLE phErrorRead);

    BOOL CreateProcess(LPCTSTR pszAppName, LPCTSTR pszCommandLine = NULL,
                       LPCTSTR pszzEnvironment = NULL, BOOL bInherit = TRUE,
                       LPSECURITY_ATTRIBUTES lpProcessAttributes = NULL,
                       LPSECURITY_ATTRIBUTES lpThreadAttributes = NULL);
    BOOL CreateProcessAsUser(HANDLE hToken, LPCTSTR pszAppName,
                             LPCTSTR pszCommandLine = NULL,
                             LPCTSTR pszzEnvironment = NULL,
                             BOOL bInherit = TRUE,
                             LPSECURITY_ATTRIBUTES lpProcessAttributes = NULL,
                             LPSECURITY_ATTRIBUTES lpThreadAttributes = NULL);
    DWORD WaitForSingleObject(DWORD dwTimeout = INFINITE);
    DWORD WaitForSingleObjectEx(DWORD dwTimeout = INFINITE,
                                BOOL bAlertable = TRUE);
    BOOL TerminateProcess(UINT uExitCode);
    BOOL IsRunning() const;

    VOID CloseProcessHandle();
    VOID CloseThreadHandle();
    VOID CloseStdInput();
    VOID CloseStdOutput();
    VOID CloseStdError();

    VOID CloseAll();

          PROCESS_INFORMATION& ProcessInfo();
    const PROCESS_INFORMATION& ProcessInfo() const;
          STARTUPINFO& StartupInfo();
    const STARTUPINFO& StartupInfo() const;

protected:
    PROCESS_INFORMATION     m_pi;
    STARTUPINFO             m_si;
    DWORD                   m_dwCreationFlags;
    LPCTSTR                 m_pszCurDir;

    VOID Init();

private:
    // NOTE: MProcessMaker is not copyable.
    MProcessMaker(const MProcessMaker&);
    MProcessMaker& operator=(const MProcessMaker&);
};

////////////////////////////////////////////////////////////////////////////

inline MProcessMaker::MProcessMaker()
{
    Init();
}

inline /*virtual*/ MProcessMaker::~MProcessMaker()
{
    CloseAll();
}

inline MProcessMaker::operator HANDLE() const
{
    return Handle();
}

inline HANDLE MProcessMaker::Handle() const
{
    return (this ? m_pi.hProcess : NULL);
}

inline BOOL MProcessMaker::TerminateProcess(UINT uExitCode)
{
    return ::TerminateProcess(m_pi.hProcess, uExitCode);
}

inline VOID MProcessMaker::SetStdInput(HANDLE hStdIn)
{
    m_si.hStdInput = hStdIn;
    if (hStdIn)
        m_si.dwFlags |= STARTF_USESTDHANDLES;
}

inline VOID MProcessMaker::SetStdOutput(HANDLE hStdOut)
{
    m_si.hStdOutput = hStdOut;
    if (hStdOut)
        m_si.dwFlags |= STARTF_USESTDHANDLES;
}

inline VOID MProcessMaker::SetStdError(HANDLE hStdErr)
{
    m_si.hStdError = hStdErr;
    if (hStdErr)
        m_si.dwFlags |= STARTF_USESTDHANDLES;
}

inline VOID MProcessMaker::SetShowWindow(INT nCmdShow/* = SW_HIDE*/)
{
    m_si.wShowWindow = static_cast<WORD>(nCmdShow);
    m_si.dwFlags |= STARTF_USESHOWWINDOW;
}

inline VOID MProcessMaker::SetCreationFlags(
    DWORD dwFlags/* = CREATE_NEW_CONSOLE*/)
{
    m_dwCreationFlags = dwFlags;
}

inline VOID MProcessMaker::SetCurrentDirectory(LPCTSTR pszCurDir)
{
    m_pszCurDir = pszCurDir;
}

inline VOID MProcessMaker::SetDesktop(LPTSTR lpDesktop)
{
    m_si.lpDesktop = lpDesktop;
}

inline VOID MProcessMaker::SetTitle(LPTSTR lpTitle)
{
    m_si.lpTitle = lpTitle;
}

inline VOID MProcessMaker::SetPosition(DWORD dwX, DWORD dwY)
{
    m_si.dwX = dwX;
    m_si.dwY = dwY;
    m_si.dwFlags |= STARTF_USEPOSITION;
}

inline VOID MProcessMaker::SetSize(DWORD dwXSize, DWORD dwYSize)
{
    m_si.dwXSize = dwXSize;
    m_si.dwYSize = dwYSize;
    m_si.dwFlags |= STARTF_USESIZE;
}

inline VOID MProcessMaker::SetCountChars(
    DWORD dwXCountChars, DWORD dwYCountChars)
{
    m_si.dwXCountChars = dwXCountChars;
    m_si.dwYCountChars = dwYCountChars;
    m_si.dwFlags |= STARTF_USECOUNTCHARS;
}

inline VOID MProcessMaker::SetFillAttirbutes(DWORD dwFillAttribute)
{
    m_si.dwFillAttribute = dwFillAttribute;
    m_si.dwFlags |= STARTF_USEFILLATTRIBUTE;
}

inline HANDLE MProcessMaker::GetProcessHandle() const
{
    return (this ? m_pi.hProcess : NULL);
}

inline HANDLE MProcessMaker::GetThreadHandle() const
{
    return (this ? m_pi.hThread : NULL);
}

inline DWORD MProcessMaker::GetExitCode() const
{
    assert(m_pi.hProcess);
    DWORD dwExitCode;
    ::GetExitCodeProcess(m_pi.hProcess, &dwExitCode);
    return dwExitCode;
}

inline DWORD MProcessMaker::WaitForSingleObject(DWORD dwTimeout/* = INFINITE*/)
{
    assert(m_pi.hProcess);
    return ::WaitForSingleObject(m_pi.hProcess, dwTimeout);
}

inline DWORD MProcessMaker::WaitForSingleObjectEx(
    DWORD dwTimeout/* = INFINITE*/, BOOL bAlertable/* = TRUE*/)
{
    assert(m_pi.hProcess);
    return ::WaitForSingleObjectEx(m_pi.hProcess, dwTimeout, bAlertable);
}

inline BOOL MProcessMaker::IsRunning() const
{
    return (GetProcessHandle() &&
            ::WaitForSingleObject(m_pi.hProcess, 0) == WAIT_TIMEOUT);
}

inline bool MProcessMaker::operator!() const
{
    return Handle() == NULL;
}

inline PROCESS_INFORMATION& MProcessMaker::ProcessInfo()
{
    return m_pi;
}

inline STARTUPINFO& MProcessMaker::StartupInfo()
{
    return m_si;
}

inline const PROCESS_INFORMATION& MProcessMaker::ProcessInfo() const
{
    return m_pi;
}

inline const STARTUPINFO& MProcessMaker::StartupInfo() const
{
    return m_si;
}

inline VOID MProcessMaker::CloseProcessHandle()
{
    if (m_pi.hProcess != NULL)
    {
        ::CloseHandle(m_pi.hProcess);
        m_pi.hProcess = NULL;
    }
}

inline VOID MProcessMaker::CloseThreadHandle()
{
    if (m_pi.hThread != NULL)
    {
        ::CloseHandle(m_pi.hThread);
        m_pi.hThread = NULL;
    }
}

inline VOID MProcessMaker::CloseStdInput()
{
    HANDLE hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
    if (m_si.hStdInput != hStdInput)
    {
        ::CloseHandle(m_si.hStdInput);
        m_si.hStdInput = hStdInput;
    }
}

inline VOID MProcessMaker::CloseStdOutput()
{
    HANDLE hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (m_si.hStdOutput != hStdOutput)
    {
        ::CloseHandle(m_si.hStdOutput);
        m_si.hStdOutput = hStdOutput;
    }
}

inline VOID MProcessMaker::CloseStdError()
{
    HANDLE hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
    if (m_si.hStdError != hStdError)
    {
        ::CloseHandle(m_si.hStdError);
        m_si.hStdError = hStdError;
    }
}

inline VOID MProcessMaker::CloseAll()
{
    CloseProcessHandle();
    CloseThreadHandle();
    CloseStdInput();
    CloseStdOutput();
    CloseStdError();
}

inline BOOL MProcessMaker::PrepareForRedirect(
    PHANDLE phInputWrite, PHANDLE phOutputRead)
{
    return PrepareForRedirect(phInputWrite, phOutputRead, phOutputRead);
}

////////////////////////////////////////////////////////////////////////////

inline VOID MProcessMaker::Init()
{
    ZeroMemory(&m_si, sizeof(m_si));
    m_si.cb = sizeof(STARTUPINFO);

    ZeroMemory(&m_pi, sizeof(m_pi));
    m_dwCreationFlags = 0;
    m_pszCurDir = NULL;
    m_si.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
    m_si.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
    m_si.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
}

inline MProcessMaker::MProcessMaker(
    LPCTSTR pszAppName, LPCTSTR pszCommandLine/* = NULL*/,
    LPCTSTR pszzEnvironment/* = NULL*/, BOOL bInherit/* = TRUE*/,
    LPSECURITY_ATTRIBUTES lpProcessAttributes/* = NULL*/,
    LPSECURITY_ATTRIBUTES lpThreadAttributes/* = NULL*/)
{
    Init();
    CreateProcess(pszAppName, pszCommandLine, pszzEnvironment,
        bInherit, lpProcessAttributes, lpThreadAttributes);
}

inline BOOL MProcessMaker::CreateProcess(
    LPCTSTR pszAppName, LPCTSTR pszCommandLine/* = NULL*/,
    LPCTSTR pszzEnvironment/* = NULL*/, BOOL bInherit/* = TRUE*/,
    LPSECURITY_ATTRIBUTES lpProcessAttributes/* = NULL*/,
    LPSECURITY_ATTRIBUTES lpThreadAttributes/* = NULL*/)
{
    using namespace std;
    BOOL b;
    LPTSTR pszCmdLine = _tcsdup(pszCommandLine);
    LPCVOID pcEnv = reinterpret_cast<LPCVOID>(pszzEnvironment);
    LPVOID pEnv = const_cast<LPVOID>(pcEnv);
    DWORD dwCreationFlags = m_dwCreationFlags;
    if (pszCmdLine)
    {
        #ifdef UNICODE
            if (pEnv)
                dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
            b = ::CreateProcess(pszAppName, pszCmdLine, 
                lpProcessAttributes, lpThreadAttributes,
                bInherit, dwCreationFlags, pEnv, m_pszCurDir, &m_si, &m_pi);
        #else
            b = ::CreateProcess(pszAppName, pszCmdLine, 
                lpProcessAttributes, lpThreadAttributes,
                bInherit, dwCreationFlags, pEnv,
                m_pszCurDir, &m_si, &m_pi);
        #endif
        free(pszCmdLine);
    }
    else
    {
        #ifdef UNICODE
            if (pEnv)
                dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
            b = ::CreateProcess(pszAppName, NULL,
                lpProcessAttributes, lpThreadAttributes,
                bInherit, dwCreationFlags,
                pEnv, m_pszCurDir, &m_si, &m_pi);
        #else
            b = ::CreateProcess(pszAppName, NULL,
                lpProcessAttributes, lpThreadAttributes,
                bInherit, dwCreationFlags, pEnv,
                m_pszCurDir, &m_si, &m_pi);
        #endif
    }
    return b;
}

inline BOOL MProcessMaker::CreateProcessAsUser(
    HANDLE hToken, LPCTSTR pszAppName, LPCTSTR pszCommandLine/* = NULL*/,
    LPCTSTR pszzEnvironment/* = NULL*/, BOOL bInherit/* = TRUE*/,
    LPSECURITY_ATTRIBUTES lpProcessAttributes/* = NULL*/,
    LPSECURITY_ATTRIBUTES lpThreadAttributes/* = NULL*/)
{
    using namespace std;
    BOOL b;
    LPTSTR pszCmdLine = _tcsdup(pszCommandLine);
    LPCVOID pcEnv = reinterpret_cast<LPCVOID>(pszzEnvironment);
    LPVOID pEnv = const_cast<LPVOID>(pcEnv);
    DWORD dwCreationFlags = m_dwCreationFlags;
    if (pszCmdLine)
    {
        LPTSTR pszCmdLine = const_cast<LPTSTR>(pszCommandLine);
        #ifdef UNICODE
            if (pEnv)
                dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
            b = ::CreateProcessAsUser(hToken, pszAppName, pszCmdLine,
                lpProcessAttributes, lpThreadAttributes,
                bInherit, dwCreationFlags, pEnv, m_pszCurDir, &m_si, &m_pi);
        #else
            b = ::CreateProcessAsUser(hToken, pszAppName, pszCmdLine,
                lpProcessAttributes, lpThreadAttributes,
                bInherit, dwCreationFlags, pEnv, m_pszCurDir, &m_si, &m_pi);
        #endif
        free(pszCmdLine);
    }
    else
    {
        #ifdef UNICODE
            if (pEnv)
                dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
            b = ::CreateProcessAsUser(hToken, pszAppName, NULL, 
                lpProcessAttributes, lpThreadAttributes,
                bInherit, dwCreationFlags, pEnv, m_pszCurDir, &m_si, &m_pi);
        #else
            b = ::CreateProcessAsUser(hToken, pszAppName, NULL, 
                lpProcessAttributes, lpThreadAttributes,
                bInherit, dwCreationFlags, pEnv, m_pszCurDir, &m_si, &m_pi);
        #endif
    }
    return b;
}

inline BOOL MProcessMaker::PrepareForRedirect(
    PHANDLE phInputWrite, PHANDLE phOutputRead, PHANDLE phErrorRead)
{
    MSecurityAttributes sa;

    MFile hInputRead, hInputWriteTmp;
    MFile hOutputReadTmp, hOutputWrite;
    MFile hErrorReadTmp, hErrorWrite;

    if (phInputWrite)
    {
        if (::CreatePipe(&hInputRead, &hInputWriteTmp, &sa, 0))
        {
            if (!hInputWriteTmp.DuplicateHandle(phInputWrite, FALSE))
                return FALSE;
            hInputWriteTmp.CloseHandle();
        }
        else
            return FALSE;
    }

    if (phOutputRead)
    {
        if (::CreatePipe(&hOutputReadTmp, &hOutputWrite, &sa, 0))
        {
            if (!hOutputReadTmp.DuplicateHandle(phOutputRead, FALSE))
                return FALSE;
            hOutputReadTmp.CloseHandle();
        }
        else
            return FALSE;
    }

    if (phOutputRead && phOutputRead == phErrorRead)
    {
        if (!hOutputWrite.DuplicateHandle(&hErrorWrite, TRUE))
            return FALSE;
    }
    else if (phErrorRead)
    {
        if (::CreatePipe(&hErrorReadTmp, &hErrorWrite, &sa, 0))
        {
            if (!hErrorReadTmp.DuplicateHandle(phErrorRead, FALSE))
                return FALSE;
            hErrorReadTmp.CloseHandle();
        }
        else
            return FALSE;
    }

    if (phInputWrite != NULL)
        SetStdInput(hInputRead.Detach());
    if (phOutputRead != NULL)
        SetStdOutput(hOutputWrite.Detach());
    if (phErrorRead != NULL)
        SetStdError(hErrorWrite.Detach());

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MPROCESSMAKER_HPP_
