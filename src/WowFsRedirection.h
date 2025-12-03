
#pragma once

typedef BOOL (WINAPI *FN_Wow64DisableWow64FsRedirection)(PVOID *OldValue);
typedef BOOL (WINAPI *FN_Wow64RevertWow64FsRedirection)(PVOID OldValue);

static inline BOOL DisableWow64FsRedirection(PVOID *OldValue)
{
	HINSTANCE hKernel32 = GetModuleHandleA("kernel32");
	FN_Wow64DisableWow64FsRedirection pWow64DisableWow64FsRedirection;
	FARPROC fn = GetProcAddress(hKernel32, "Wow64DisableWow64FsRedirection");
	if (!fn)
		return FALSE;
	CopyMemory(&pWow64DisableWow64FsRedirection, &fn, sizeof(fn));
	return (*pWow64DisableWow64FsRedirection)(OldValue);
}

static inline BOOL RevertWow64FsRedirection(PVOID OldValue)
{
	HINSTANCE hKernel32 = GetModuleHandleA("kernel32");
	FN_Wow64RevertWow64FsRedirection pWow64RevertWow64FsRedirection;
	FARPROC fn = GetProcAddress(hKernel32, "Wow64RevertWow64FsRedirection");
	if (!fn)
		return FALSE;
	CopyMemory(&pWow64RevertWow64FsRedirection, &fn, sizeof(fn));
	return (*pWow64RevertWow64FsRedirection)(OldValue);
}
