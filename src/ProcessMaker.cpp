////////////////////////////////////////////////////////////////////////////
// ProcessMaker.cpp -- Win32 process maker
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#include "stdafx.hpp"

#ifdef MZC_NO_INLINING
    #undef MZC_INLINE
    #define MZC_INLINE  /*empty*/
    #include "ProcessMaker_inl.hpp"
#endif

using namespace std;

#if defined(__BORLANDC__) && (__BORLANDC__ <= 0x551)
    #define _strdup strdup
    #define _wcsdup wcsdup
#endif

////////////////////////////////////////////////////////////////////////////
// MProcessMaker --- Win32 process maker

void MProcessMaker::Init()
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

MProcessMaker::MProcessMaker(
    LPCTSTR pszAppName, LPCTSTR pszCommandLine/* = NULL*/,
    LPCTSTR pszzEnvironment/* = NULL*/, BOOL bInherit/* = TRUE*/,
    LPSECURITY_ATTRIBUTES lpProcessAttributes/* = NULL*/,
    LPSECURITY_ATTRIBUTES lpThreadAttributes/* = NULL*/)
{
    Init();
    CreateProcess(pszAppName, pszCommandLine, pszzEnvironment,
        bInherit, lpProcessAttributes, lpThreadAttributes);
}

BOOL MProcessMaker::CreateProcess(
    LPCTSTR pszAppName, LPCTSTR pszCommandLine/* = NULL*/,
    LPCTSTR pszzEnvironment/* = NULL*/, BOOL bInherit/* = TRUE*/,
    LPSECURITY_ATTRIBUTES lpProcessAttributes/* = NULL*/,
    LPSECURITY_ATTRIBUTES lpThreadAttributes/* = NULL*/)
{
    BOOL b;
    LPCVOID pcEnv = reinterpret_cast<LPCVOID>(pszzEnvironment);
    LPVOID pEnv = const_cast<LPVOID>(pcEnv);
    if (pszCommandLine)
    {
        LPTSTR pszCmdLine = const_cast<LPTSTR>(pszCommandLine);
        #ifdef UNICODE
            b = ::CreateProcess(pszAppName, pszCmdLine, 
                lpProcessAttributes, lpThreadAttributes,
                bInherit, m_dwCreationFlags | CREATE_UNICODE_ENVIRONMENT,
                pEnv, m_pszCurDir, &m_si, &m_pi);
        #else
            b = ::CreateProcess(pszAppName, pszCmdLine, 
                lpProcessAttributes, lpThreadAttributes,
                bInherit, m_dwCreationFlags, pEnv,
                m_pszCurDir, &m_si, &m_pi);
        #endif
    }
    else
    {
        #ifdef UNICODE
            b = ::CreateProcess(pszAppName, NULL,
                lpProcessAttributes, lpThreadAttributes,
                bInherit, m_dwCreationFlags | CREATE_UNICODE_ENVIRONMENT,
                pEnv, m_pszCurDir, &m_si, &m_pi);
        #else
            b = ::CreateProcess(pszAppName, NULL,
                lpProcessAttributes, lpThreadAttributes,
                bInherit, m_dwCreationFlags, pEnv,
                m_pszCurDir, &m_si, &m_pi);
        #endif
    }
    return b;
}

BOOL MProcessMaker::CreateProcessAsUser(
    HANDLE hToken, LPCTSTR pszAppName, LPCTSTR pszCommandLine/* = NULL*/,
    LPCTSTR pszzEnvironment/* = NULL*/, BOOL bInherit/* = TRUE*/,
    LPSECURITY_ATTRIBUTES lpProcessAttributes/* = NULL*/,
    LPSECURITY_ATTRIBUTES lpThreadAttributes/* = NULL*/)
{
    BOOL b;
    LPCVOID pcEnv = reinterpret_cast<LPCVOID>(pszzEnvironment);
    LPVOID pEnv = const_cast<LPVOID>(pcEnv);
    if (pszCommandLine)
    {
        LPTSTR pszCmdLine = const_cast<LPTSTR>(pszCommandLine);
        #ifdef UNICODE
            b = ::CreateProcessAsUser(hToken, pszAppName, pszCmdLine,
                lpProcessAttributes, lpThreadAttributes,
                bInherit, m_dwCreationFlags | CREATE_UNICODE_ENVIRONMENT,
                pEnv, m_pszCurDir, &m_si, &m_pi);
        #else
            b = ::CreateProcessAsUser(hToken, pszAppName, pszCmdLine,
                lpProcessAttributes, lpThreadAttributes,
                bInherit, m_dwCreationFlags, pEnv,
                m_pszCurDir, &m_si, &m_pi);
        #endif
    }
    else
    {
        #ifdef UNICODE
            b = ::CreateProcessAsUser(hToken, pszAppName, NULL, 
                lpProcessAttributes, lpThreadAttributes,
                bInherit, m_dwCreationFlags | CREATE_UNICODE_ENVIRONMENT,
                pEnv, m_pszCurDir, &m_si, &m_pi);
        #else
            b = ::CreateProcessAsUser(hToken, pszAppName, NULL, 
                lpProcessAttributes, lpThreadAttributes,
                bInherit, m_dwCreationFlags, pEnv,
                m_pszCurDir, &m_si, &m_pi);
        #endif
    }
    return b;
}

BOOL MProcessMaker::PrepareForRedirect(
    PHANDLE phInputWrite, PHANDLE phOutputRead, PHANDLE phErrorRead,
    PSECURITY_ATTRIBUTES psa)
{
    MFile hInputRead, hInputWriteTmp;
    MFile hOutputReadTmp, hOutputWrite;
    MFile hErrorReadTmp, hErrorWrite;

    if (phInputWrite != NULL)
    {
        if (::CreatePipe(&hInputRead, &hInputWriteTmp, psa, 0))
        {
            if (!hInputWriteTmp.DuplicateHandle(phInputWrite, FALSE))
                return FALSE;
            hInputWriteTmp.CloseHandle();
        }
        else
            return FALSE;
    }

    if (phOutputRead != NULL)
    {
        if (::CreatePipe(&hOutputReadTmp, &hOutputWrite, psa, 0))
        {
            if (!hOutputReadTmp.DuplicateHandle(phOutputRead, FALSE))
                return FALSE;
            hOutputReadTmp.CloseHandle();
        }
        else
            return FALSE;
    }

    if (phOutputRead != NULL && phOutputRead == phErrorRead)
    {
        if (!hOutputWrite.DuplicateHandle(&hErrorWrite, TRUE))
            return FALSE;
    }
    else if (phErrorRead != NULL)
    {
        if (::CreatePipe(&hErrorReadTmp, &hErrorWrite, psa, 0))
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

#ifdef UNITTEST
    // unit test and example
    #include <cstdio>
    int main(void)
    {
        MProcessMaker pmaker;
        MFile hInputWrite, hOutputRead;
        if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
            pmaker.CreateProcess(NULL, TEXT("cmd.exe /C dir")))
        {
            DWORD cbAvail;
            while (hOutputRead.PeekNamedPipe(NULL, 0, NULL, &cbAvail))
            {
                if (cbAvail == 0)
                {
                    if (!pmaker.IsRunning())
                        break;

                    pmaker.WaitForSingleObject(500);
                    continue;
                }

                CHAR szBuf[256];
                DWORD cbRead;
                if (cbAvail > sizeof(szBuf))
                    cbAvail = sizeof(szBuf);
                else  if (cbAvail == 0)
                    continue;

                if (hOutputRead.ReadFile(szBuf, cbAvail, &cbRead))
                {
                    if (cbRead == 0)
                        continue;

                    fwrite(szBuf, 1, cbRead, stdout);
                }
            }
        }
        return 0;
    }
#endif

////////////////////////////////////////////////////////////////////////////
