#include <windows.h>
#include <stdio.h>

void DumpBinary(LPCVOID pv, DWORD Size, const char *VarName)
{
    const BYTE *pb = (const BYTE *)pv;

    printf("static const BYTE %s[] = {\n", VarName);
    for (DWORD i = 0; i < Size; ++i)
    {
        printf("0x%02X, ", pb[i]);
        if (i % 13 == 12)
            printf("\n");
    }
    printf("\n");
    printf("};\n");
}

void DumpResource(HINSTANCE hInst, LPCTSTR Type, LPCTSTR Name, const char *VarName)
{
    HRSRC hRsrc = FindResourceEx(hInst, Type, Name, 0);
    DWORD Size = SizeofResource(hInst, hRsrc);
    HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
    LPVOID pv = LockResource(hGlobal);
    DumpBinary(pv, Size, VarName);
    UnlockResource(hGlobal);
}

int main(void)
{
    HINSTANCE hInst = GetModuleHandle(NULL);

    DumpResource(hInst, RT_ACCELERATOR, MAKEINTRESOURCE(1), "abAccel");
    DumpResource(hInst, RT_DIALOG, MAKEINTRESOURCE(1), "abDialog");
    DumpResource(hInst, RT_MENU, MAKEINTRESOURCE(1), "abMenu");
    DumpResource(hInst, RT_STRING, MAKEINTRESOURCE(1), "abString");
    DumpResource(hInst, RT_VERSION, MAKEINTRESOURCE(1), "abVersion");

    return 0;
}
