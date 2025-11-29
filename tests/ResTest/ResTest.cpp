// ResTest.cpp
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dlgs.h>
#include <tchar.h>
#include <cstdio>
#include "resource.h"
using std::FILE;
using std::fopen;
using std::fprintf;

#ifdef _MSC_VER
#pragma comment(linker,"/manifestdependency:\"type='win32' \
	name='Microsoft.Windows.Common-Controls' \
	version='6.0.0.0' \
	processorArchitecture='*' \
	publicKeyToken='6595b64144ccf1df' \
	language='*'\"")
#endif

void DumpBinary(FILE *fp, LPCVOID pv, DWORD Size, const char *VarName)
{
	const BYTE *pb = (const BYTE *)pv;

	fprintf(fp, "static const BYTE %s[] = {\n", VarName);
	for (DWORD i = 0; i < Size; ++i)
	{
		fprintf(fp, "0x%02X, ", pb[i]);
		if (i % 13 == 12)
			fprintf(fp, "\n");
	}
	fprintf(fp, "\n");
	fprintf(fp, "};\n");
}

void DumpResource(FILE *fp, HINSTANCE hInst, LPCTSTR Type, LPCTSTR Name, const char *VarName)
{
	HRSRC hRsrc = FindResourceEx(hInst, Type, Name, 0);
	DWORD Size = SizeofResource(hInst, hRsrc);
	HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
	LPVOID pv = LockResource(hGlobal);
	DumpBinary(fp, pv, Size, VarName);
	UnlockResource(hGlobal);
}

void DumpAll(FILE *fp)
{
	HINSTANCE hInst = GetModuleHandle(NULL);

	DumpResource(fp, hInst, RT_ACCELERATOR, MAKEINTRESOURCE(1), "abAccel");
	DumpResource(fp, hInst, RT_DIALOG, MAKEINTRESOURCE(1), "abDialog");
	DumpResource(fp, hInst, RT_MENU, MAKEINTRESOURCE(1), "abMenu");
	DumpResource(fp, hInst, RT_STRING, MAKEINTRESOURCE(1), "abString");
	DumpResource(fp, hInst, RT_VERSION, MAKEINTRESOURCE(1), "abVersion");

#ifndef RT_HTML
	#define RT_HTML         MAKEINTRESOURCE(23)
#endif
#ifndef RT_MANIFEST
	#define RT_MANIFEST     MAKEINTRESOURCE(24)
#endif
	DumpResource(fp, hInst, RT_HTML, MAKEINTRESOURCE(1), "abHtml");
	DumpResource(fp, hInst, RT_MANIFEST, MAKEINTRESOURCE(1), "abManifest");
}

//////////////////////////////////////////////////////////////////////////////

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	return TRUE;
}

void JustDoIt(HWND hwnd)
{
	FILE *fp = fopen("sample-resources.txt", "w");
	if (fp)
	{
		DumpAll(fp);
		fclose(fp);
	}
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
		EndDialog(hwnd, IDOK);
		break;
	case IDCANCEL:
		EndDialog(hwnd, IDCANCEL);
		break;
	case CMDID_JUSTDOIT:
		JustDoIt(hwnd);
		break;
	}
}

INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

extern "C" INT WINAPI
_tWinMain(HINSTANCE   hInstance,
		  HINSTANCE   hPrevInstance,
		  LPTSTR      lpCmdLine,
		  INT         nCmdShow)
{
	InitCommonControls();
	//DialogBox(hInstance, MAKEINTRESOURCE(2), NULL, DialogProc);
	DialogBox(hInstance, MAKEINTRESOURCE(4), NULL, DialogProc);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
