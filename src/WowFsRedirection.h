
#pragma once

typedef BOOL (WINAPI *FN_Wow64DisableWow64FsRedirection)(PVOID *OldValue);
typedef BOOL (WINAPI *FN_Wow64RevertWow64FsRedirection)(PVOID OldValue);

static inline BOOL DisableWow64FsRedirection(PVOID *OldValue)
{
    HINSTANCE hKernel32 = GetModuleHandleA("kernel32");
    FN_Wow64DisableWow64FsRedirection pWow64DisableWow64FsRedirection;
    pWow64DisableWow64FsRedirection =
        (FN_Wow64DisableWow64FsRedirection)GetProcAddress(hKernel32, "Wow64DisableWow64FsRedirection");
    if (pWow64DisableWow64FsRedirection)
        return (*pWow64DisableWow64FsRedirection)(OldValue);
    return FALSE;
}

static inline BOOL RevertWow64FsRedirection(PVOID OldValue)
{
    HINSTANCE hKernel32 = GetModuleHandleA("kernel32");
    FN_Wow64RevertWow64FsRedirection pWow64RevertWow64FsRedirection;
    pWow64RevertWow64FsRedirection =
        (FN_Wow64RevertWow64FsRedirection)GetProcAddress(hKernel32, "Wow64RevertWow64FsRedirection");
    if (pWow64RevertWow64FsRedirection)
        return (*pWow64RevertWow64FsRedirection)(OldValue);
    return FALSE;
}
