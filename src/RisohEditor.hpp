#ifndef RISOHEDITOR_HPP_
#define RISOHEDITOR_HPP_

#include "MWindowBase.hpp"
#include "Res.hpp"

LPWSTR MakeFilterDx(LPWSTR psz);
BOOL GetPathOfShortcutDx(HWND hwnd, LPCWSTR pszLnkFile, LPWSTR pszPath);
HBITMAP Create24BppBitmapDx(INT width, INT height);
BOOL DumpBinaryFileDx(const WCHAR *filename, LPCVOID pv, DWORD size);
LPWSTR GetTempFileNameDx(LPCWSTR pszPrefix3Chars);
std::wstring str_vkey(WORD w);
HBITMAP CreateBitmapFromIconDx(HICON hIcon, INT width, INT height, BOOL bCursor);

HBITMAP
CreateBitmapFromIconOrPngDx(HWND hwnd, const ResEntry& Entry, BITMAP& bm);

HBITMAP
CreateBitmapFromIconsDx(HWND hwnd, ResEntries& Entries, const ResEntry& Entry);

HBITMAP
CreateBitmapFromCursorDx(HWND hwnd, const ResEntry& Entry, BITMAP& bm);

HBITMAP
CreateBitmapFromCursorsDx(HWND hwnd, ResEntries& Entries, const ResEntry& Entry);

BOOL DoAddCursor(HWND hwnd,
                 ResEntries& Entries,
                 const ID_OR_STRING& Name,
                 WORD Lang,
                 const std::wstring& CurFile);
BOOL DoReplaceCursor(HWND hwnd,
                     ResEntries& Entries,
                     const ID_OR_STRING& Name,
                     WORD Lang,
                     const std::wstring& CurFile);
BOOL DoAddIcon(HWND hwnd,
               ResEntries& Entries,
               const ID_OR_STRING& Name,
               WORD Lang,
               const std::wstring& IconFile);
BOOL DoReplaceIcon(HWND hwnd,
                   ResEntries& Entries,
                   const ID_OR_STRING& Name,
                   WORD Lang,
                   const std::wstring& IconFile);
BOOL DoAddBin(HWND hwnd,
              ResEntries& Entries,
              const ID_OR_STRING& Type,
              const ID_OR_STRING& Name,
              WORD Lang,
              const std::wstring& File);
BOOL DoReplaceBin(HWND hwnd,
                  ResEntries& Entries,
                  const ID_OR_STRING& Type,
                  const ID_OR_STRING& Name,
                  WORD Lang,
                  const std::wstring& File);
BOOL DoAddBitmap(HWND hwnd,
                 ResEntries& Entries,
                 const ID_OR_STRING& Name,
                 WORD Lang,
                 const std::wstring& BitmapFile);
BOOL DoReplaceBitmap(HWND hwnd,
                     ResEntries& Entries,
                     const ID_OR_STRING& Name,
                     WORD Lang,
                     const std::wstring& BitmapFile);

#endif  // ndef RISOHEDITOR_HPP_
