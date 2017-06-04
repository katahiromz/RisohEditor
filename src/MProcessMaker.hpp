////////////////////////////////////////////////////////////////////////////
// MProcessMaker.hpp -- Win32 process maker
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_PROCESSMAKER_HPP_
#define MZC4_PROCESSMAKER_HPP_

////////////////////////////////////////////////////////////////////////////
// MProcessMaker --- Win32 process maker

class MProcessMaker
{
public:
    MProcessMaker();
    MProcessMaker(LPCTSTR pszAppName, LPCTSTR pszCommandLine = NULL,
                  LPCTSTR pszzEnvironment = NULL, BOOL bInherit = TRUE,
                  LPSECURITY_ATTRIBUTES lpProcessAttributes = NULL,
                  LPSECURITY_ATTRIBUTES lpThreadAttributes = NULL);
    virtual ~MProcessMaker();

    operator bool() const;
    bool operator!() const;
    operator HANDLE() const;

    HANDLE  GetProcessHandle() const;
    HANDLE  GetThreadHandle() const;
    DWORD   GetExitCode() const;

    // set attributes for child process
    void SetShowWindow(int nCmdShow = SW_HIDE);
    void SetCreationFlags(DWORD dwFlags = CREATE_NEW_CONSOLE);
    void SetCurrentDirectory(LPCTSTR pszCurDir);
    void SetDesktop(LPTSTR lpDesktop);
    void SetTitle(LPTSTR lpTitle);
    void SetPosition(DWORD dwX, DWORD dwY);
    void SetSize(DWORD dwXSize, DWORD dwYSize);
    void SetCountChars(DWORD dwXCountChars, DWORD dwYCountChars);
    void SetFillAttirbutes(DWORD dwFillAttribute);

    void SetStdInput(HANDLE hStdIn);
    void SetStdOutput(HANDLE hStdOut);
    void SetStdError(HANDLE hStdErr);

    BOOL PrepareForRedirect(PHANDLE phInputWrite, PHANDLE phOutputRead,
                            PSECURITY_ATTRIBUTES psa = NULL);
    BOOL PrepareForRedirect(PHANDLE phInputWrite, PHANDLE phOutputRead,
                            PHANDLE phErrorRead,
                            PSECURITY_ATTRIBUTES psa = NULL);

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

    void CloseProcessHandle();
    void CloseThreadHandle();
    void CloseStdInput();
    void CloseStdOutput();
    void CloseStdError();

    void CloseAll();

          PROCESS_INFORMATION& ProcessInfo();
    const PROCESS_INFORMATION& ProcessInfo() const;
          STARTUPINFO& StartupInfo();
    const STARTUPINFO& StartupInfo() const;

protected:
    PROCESS_INFORMATION     m_pi;
    STARTUPINFO             m_si;
    DWORD                   m_dwCreationFlags;
    LPCTSTR                 m_pszCurDir;

    void Init();

private:
    // NOTE: MProcessMaker is not copyable.
    MProcessMaker(const MProcessMaker&);
    MProcessMaker& operator=(const MProcessMaker&);
};

////////////////////////////////////////////////////////////////////////////

#ifndef MZC_NO_INLINING
    #undef MZC_INLINE
    #define MZC_INLINE inline
    #include "MProcessMaker_inl.hpp"
#endif

#endif  // ndef MZC4_PROCESSMAKER_HPP_
