/* MFileAPI.h -- filesystem manipulator                       -*- C++ -*-
 * This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
 */

#ifndef MZC4_MFILEAPI_H_
#define MZC4_MFILEAPI_H_        11  /* Version 11 */

/*
 * MPath_... functions
 * MDir_... functions
 * MFile_... functions
 */

/* TODO: support MSDOS, link, shortcut */
/**************************************************************************/

/* C99 and C++11 */
#if __STDC_VERSION__ >= 199901L && !defined(C99)
    #define C99             1
#endif
#if __cplusplus >= 201103L && !defined(CXX11)
    #define CXX11           1
#endif

/* <stdbool.h> and <stdint.h> */
#ifdef __cplusplus
    #ifdef CXX11
        #include <cstdint>
    #else
        #include "pstdint.h"
    #endif
#else
    #if C99
        #include <stdbool.h>
        #include <stdint.h>
    #else
        #include "pstdbool.h"
        #include "pstdint.h"
    #endif
#endif

#ifndef USING_NAMESPACE_STD
    #ifdef __cplusplus
        #define USING_NAMESPACE_STD     using namespace std
    #else
        #define USING_NAMESPACE_STD     /*empty*/
    #endif
#endif

#ifdef _WIN32
    #ifndef _INC_WINDOWS
        #include <windows.h>    /* Win32API */
    #endif
    #include <mbstring.h>       /* _mbsrchr */
    #include <tchar.h>          /* generic text mapping */

    #ifndef MChar
        typedef TCHAR MChar;
        #define MChar MChar
    #endif
#else
    #ifdef __cplusplus
        #include <cstdlib>
        #include <cstdio>
    #else
        #include <stdlib.h>
        #include <stdio.h>
    #endif

    #include <sys/stat.h>
    #include <sys/types.h>

    #ifdef MSDOS
        #include <direct.h>
    #else
        #include <unistd.h>
        #include <dirent.h>     /* opendir, ... */
    #endif

    #ifndef MChar
        typedef char MChar;
        #define MChar MChar
    #endif

    #ifndef MAX_PATH
        #define MAX_PATH 256
    #endif

    #ifndef TEXT
        #define TEXT(x)     x
    #endif
#endif

#ifdef __cplusplus
    #include <cassert>          /* assert */
    #include <cstring>          /* strlen, wcslen */
#else
    #include <assert.h>
    #include <string.h>
#endif

#ifndef optional_
    #ifdef __cplusplus
        #define optional_(def) = def
    #else
        #define optional_(def)      /*empty*/
    #endif
#endif

/**************************************************************************/
/* pathname */

bool MPath_Exists(const MChar *pathname);
MChar *MPath_GetFull(MChar *full, const MChar *relative);
MChar *MPath_AddSep(MChar *pathname);
MChar *MPath_RemoveSep(MChar *pathname);
MChar *MPath_FindTitle(MChar *pathname);
MChar *MPath_SetTitle(MChar *pathname, const MChar *title);
MChar *MPath_FindDotExt(MChar *pathname);
MChar *MPath_SetDotExt(MChar *pathname, const MChar *dot_ext);
bool MPath_IsDots(const MChar *name);
void MPath_BackslashToSlash(MChar *pathname);
void MPath_SlashToBackslash(MChar *pathname);

/**************************************************************************/
/* directory */

bool MDir_Exists(const MChar *pathname);
bool MDir_Create(const MChar *pathname);
bool MDir_Remove(const MChar *pathname);
bool MDir_Get(MChar *pathname, size_t maxbuf optional_(MAX_PATH));
bool MDir_Set(const MChar *pathname);
bool MDir_CreateRecurse(const MChar *pathname, bool fForce optional_(false));

#ifndef MSDOS
    #ifdef _WIN32
        typedef WIN32_FIND_DATA     MZC_DIR_INFO;
        typedef HANDLE              MZC_DIR_P;
    #else
        typedef struct MZC_DIR_INFO
        {
            MChar   cFileName[MAX_PATH];
        } MZC_DIR_INFO;
        typedef DIR *MZC_DIR_P;
    #endif

    MZC_DIR_P MDir_FindFirstItem(const MChar *pathname, MZC_DIR_INFO *info);
    bool      MDir_FindNextItem(MZC_DIR_P dirp, MZC_DIR_INFO *info);
    bool      MDir_FindClose(MZC_DIR_P dirp);

    bool MDir_Delete(const MChar *dir);
#endif  /* ndef MSDOS */

/**************************************************************************/
/* file */

bool MFile_Exists(const MChar *filename);

uint8_t *
MFile_GetContents(const MChar *filename, size_t *psize optional_(NULL));
bool
MFile_PutContents(const MChar *filename, const void *pvContents, size_t size);

bool MFile_PutText(const MChar *filename, const MChar *psz);

bool MFile_Move(const MChar *existing_file, const MChar *new_file);
bool MFile_Copy(const MChar *existing_file, const MChar *new_file,
                bool bFailIfExists optional_(false));
bool MFile_Delete(const MChar *filename);

/**************************************************************************/

inline bool MPath_Exists(const MChar *pathname)
{
    USING_NAMESPACE_STD;
    assert(pathname);
#ifdef _WIN32
    DWORD attrs;
#else
    struct stat sbuf;
#endif

    assert(pathname);
#ifdef _WIN32
    attrs = GetFileAttributes(pathname);
    return (attrs != 0xFFFFFFFF);
#else
    return (stat(pathname, &sbuf) == 0);
#endif
}

inline bool MFile_Exists(const MChar *filename)
{
    USING_NAMESPACE_STD;
    assert(filename);
#ifdef _WIN32
    DWORD attrs;
#else
    struct stat sbuf;
#endif

    assert(filename);
#ifdef _WIN32
    attrs = GetFileAttributes(filename);
    return (attrs != 0xFFFFFFFF) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    if (stat(filename, &sbuf) == 0)
    {
        if ((sbuf.st_mode & S_IFMT) == S_IFREG)
            return true;
    }
    return false;
#endif
}

inline bool MDir_Exists(const MChar *pathname)
{
    USING_NAMESPACE_STD;
    assert(pathname);
#ifdef _WIN32
    DWORD attrs;
#else
    struct stat sbuf;
#endif

    assert(pathname);
#ifdef _WIN32
    attrs = GetFileAttributes(pathname);
    return (attrs != 0xFFFFFFFF) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    if (stat(pathname, &sbuf) == 0)
    {
        if ((sbuf.st_mode & S_IFMT) == S_IFDIR)
            return true;
    }
    return false;
#endif
}

inline MChar *MPath_GetFull(MChar *full, const MChar *relative)
{
    USING_NAMESPACE_STD;
    assert(full);
    assert(relative);
#ifdef _WIN32
    GetFullPathName(relative, MAX_PATH, full, NULL);
    return full;
#else
    return realpath(relative, full);
#endif
}

inline bool MFile_Delete(const MChar *filename)
{
    USING_NAMESPACE_STD;
    assert(filename);
#ifdef _WIN32
    return !!::DeleteFile(filename);
#elif defined(MSDOS)
    return remove(filename) == 0;
#else
    return unlink(filename) == 0;
#endif
}

inline uint8_t *
MFile_GetContents(const MChar *filename, size_t *psize)
{
    USING_NAMESPACE_STD;
    uint8_t *pb = NULL;

#ifdef _WIN32
    HANDLE hFile;
    DWORD cbFile, cbRead;

    assert(filename);

    if (psize)
        *psize = 0;

    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        cbFile = GetFileSize(hFile, NULL);
        if (cbFile != 0xFFFFFFFF)
        {
            LPVOID pv = malloc((cbFile + 1) * sizeof(char));
            pb = reinterpret_cast<uint8_t *>(pv);
            if (pb)
            {
                if (::ReadFile(hFile, pb, cbFile, &cbRead, NULL) &&
                    cbFile == cbRead)
                {
                    pb[cbFile] = 0;
                    if (psize)
                        *psize = cbFile;
                }
                else
                {
                    free(pb);
                    pb = NULL;
                }
            }
        }
        CloseHandle(hFile);
    }
#else
    FILE *fp;
    struct stat sbuf;

    assert(filename);

    if (psize)
        *psize = 0;

    fp = fopen(filename, "rb");
    if (fp == NULL)
        return NULL;

    if (stat(filename, &sbuf) == 0)
    {
        pb = (uint8_t *)calloc(1, sbuf.st_size + 1);
        if (pb)
        {
            if (fread(pb, sbuf.st_size, 1, fp))
            {
                if (psize)
                    *psize = sbuf.st_size;
            }
            else
            {
                free(pb);
                pb = NULL;
            }
        }
    }
    fclose(fp);
#endif

    return pb;
}

inline bool MFile_PutText(const MChar *filename, const MChar *str)
{
    USING_NAMESPACE_STD;
    return MFile_PutContents(filename, str, _tcslen(str) * sizeof(MChar));
}

inline bool
MFile_PutContents(const MChar *filename, const void *pvContents, size_t size)
{
    USING_NAMESPACE_STD;
    bool bOK = false;

#ifdef _WIN32
    HANDLE hFile;
    DWORD cbWritten, dwSize = (DWORD)size;

    assert(filename);
    assert(pvContents || size == 0);

    hFile = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL,
        CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (::WriteFile(hFile, pvContents, dwSize, &cbWritten, NULL) &&
            dwSize == cbWritten)
        {
            if (::CloseHandle(hFile))
                bOK = true;
        }
        else
        {
            CloseHandle(hFile);
        }

        if (!bOK)
            DeleteFile(filename);
    }
#else
    FILE *fp;

    assert(filename);
    assert(pvContents || size == 0);

    fp = fopen(filename, "wb");
    if (fp)
    {
        if (fwrite(pvContents, size, 1, fp))
        {
            fclose(fp);
            bOK = true;
        }
        else
        {
            fclose(fp);
            MFile_Delete(filename);
        }
    }
#endif

    return bOK;
}

inline bool MDir_Create(const MChar *pathname)
{
    USING_NAMESPACE_STD;
#ifdef _WIN32
    return !!CreateDirectory(pathname, NULL);
#elif defined(MSDOS)
    return mkdir(pathname) == 0;
#else
    return mkdir(pathname, 0755) == 0;
#endif
}

inline bool
MFile_Move(const MChar *existing_file, const MChar *new_file)
{
    USING_NAMESPACE_STD;
    assert(existing_file);
    assert(new_file);
#ifdef _WIN32
    return ::MoveFile(existing_file, new_file);
#else
    return rename(existing_file, new_file) == 0;
#endif
}

inline bool
MFile_Copy(const MChar *existing_file, const MChar *new_file,
          bool bFailIfExists)
{
    USING_NAMESPACE_STD;
#ifdef _WIN32
    return ::CopyFile(existing_file, new_file, bFailIfExists);
#else
    size_t size;
    void *ptr = MFile_GetContents(existing_file, &size);
    return MFile_PutContents(new_file, ptr, size);
#endif
}

inline MChar *MPath_SetDotExt(MChar *pathname, const MChar *dot_ext)
{
    USING_NAMESPACE_STD;
    assert(pathname);
    assert(dot_ext);
#ifdef _WIN32
    lstrcpy(MPath_FindDotExt(pathname), dot_ext);
#else
    strcpy(MPath_FindDotExt(pathname), dot_ext);
#endif
    return pathname;
}

inline MChar *MPath_AddSep(MChar *pathname)
{
    USING_NAMESPACE_STD;
    size_t path_len;

#ifdef _WIN32
    MChar *prev;

    assert(pathname);
    path_len = _tcslen(pathname);
    if (path_len == 0)
        return pathname;

    prev = CharPrev(pathname, pathname + path_len);
    if (*prev != TEXT('\\') && *prev != TEXT('/'))
    {
        pathname[path_len++] = TEXT('\\');
        pathname[path_len] = 0;
    }
#else
    char *prev;

    assert(pathname);
    path_len = strlen(pathname);
    if (path_len == 0)
        return pathname;

    prev = &pathname[path_len - 1];
    if (*prev != '/')
    {
        pathname[path_len++] = '/';
        pathname[path_len] = 0;
    }
#endif

    return pathname;
}

inline MChar *MPath_FindDotExt(MChar *pathname)
{
    USING_NAMESPACE_STD;
    MChar *title, *dot_ext;
    assert(pathname);
    title = MPath_FindTitle(pathname);
#ifdef _WIN32
    dot_ext = _tcsrchr(title, TEXT('.'));
    if (dot_ext)
        return dot_ext;
    return pathname + _tcslen(pathname);
#else
    dot_ext = strrchr(title, '.');
    if (dot_ext)
        return dot_ext;
    return pathname + strlen(pathname);
#endif
}

inline MChar *MPath_RemoveSep(MChar *pathname)
{
    USING_NAMESPACE_STD;
    size_t path_len;
    MChar *prev;

    assert(pathname);
#ifdef _WIN32
    path_len = _tcslen(pathname);
#else
    path_len = strlen(pathname);
#endif
    if (path_len == 0)
        return pathname;

#ifdef _WIN32
    #ifdef UNICODE
        prev = pathname + path_len - 1;
        if (*prev == L'\\' || *prev == L'/')
            *prev = 0;
    #else
        prev = CharPrev(pathname, pathname + path_len);
        if (*prev == '\\' || *prev == '/')
            *prev = 0;
    #endif
#else
    prev = pathname + path_len - 1;
    if (*prev == '/')
        *prev = 0;
#endif

    return pathname;
}

inline MChar *MPath_FindTitle(MChar *pathname)
{
    USING_NAMESPACE_STD;
#ifdef _WIN32
    MChar *pch1 = _tcsrchr(pathname, _T('\\'));
    MChar *pch2 = _tcsrchr(pathname, _T('/'));
    if (pch1 == NULL && pch2 == NULL)
        return pathname;
    if (pch1 == NULL)
        return pch2 + 1;
    if (pch2 == NULL)
        return pch1 + 1;
    if (pch1 < pch2)
        return pch2 + 1;
    else
        return pch1 + 1;
#else
    MChar *pch = strrchr(pathname, '/');
    if (pch == NULL)
        return pathname;
    return pch + 1;
#endif
}

inline MChar *MPath_SetTitle(MChar *pathname, const MChar *title)
{
    USING_NAMESPACE_STD;
#ifdef _WIN32
    lstrcpy(MPath_FindTitle(pathname), title);
#else
    strcpy(MPath_FindTitle(pathname), title);
#endif
    return pathname;
}

inline bool MDir_Remove(const MChar *pathname)
{
    USING_NAMESPACE_STD;
#ifdef _WIN32
    return !!::RemoveDirectory(pathname);
#else
    return rmdir(pathname) == 0;
#endif
}

#ifndef MSDOS
    inline MZC_DIR_P MDir_FindFirstItem(const MChar *pathname, MZC_DIR_INFO *info)
    {
        USING_NAMESPACE_STD;
    #ifdef _WIN32
        MChar spec[MAX_PATH];
        MZC_DIR_P dirp;

        assert(pathname);
        assert(info);

        lstrcpy(spec, pathname);
        MPath_AddSep(spec);
        lstrcat(spec, TEXT("*"));

        dirp = FindFirstFile(spec, info);
        if (dirp == INVALID_HANDLE_VALUE)
            dirp = NULL;
        return dirp;
    #else
        DIR *dirp;
        struct dirent *ent;

        assert(pathname);
        assert(info);

        dirp = opendir(pathname);
        if (dirp == NULL)
            return NULL;

        ent = readdir(dirp);
        if (!ent)
        {
            closedir(dirp);
            return NULL;
        }

        strcpy(info->cFileName, ent->d_name);
        return dirp;
    #endif
    }

    inline bool MDir_FindNextItem(MZC_DIR_P dirp, MZC_DIR_INFO *info)
    {
        USING_NAMESPACE_STD;
    #ifdef _WIN32
        assert(dirp);
        assert(info);

        if (dirp == NULL)
            return false;

        return FindNextFile(dirp, info);
    #else
        struct dirent *ent;

        assert(dirp);
        assert(info);

        if (dirp == NULL)
            return false;

        ent = readdir(dirp);
        if (ent == NULL)
            return false;

        strcpy(info->cFileName, ent->d_name);
        return true;
    #endif
    }

    inline bool MDir_FindClose(MZC_DIR_P dirp)
    {
        USING_NAMESPACE_STD;
        assert(dirp);

    #ifdef _WIN32
        return !!FindClose(dirp);
    #else
        return closedir(dirp) == 0;
    #endif
    }

    inline bool MDir_Delete(const MChar *dir)
    {
    #ifdef _WIN32
        TCHAR dir_old[MAX_PATH];
    #else
        char dir_old[MAX_PATH];
    #endif
        MZC_DIR_INFO info;
        MZC_DIR_P dirp;

        assert(dir);

        if (!MDir_Exists(dir))
            return false;

        MDir_Get(dir_old, MAX_PATH);
        if (!MDir_Set(dir))
            return false;

        dirp = MDir_FindFirstItem(TEXT("."), &info);
        if (dirp)
        {
            do
            {
                if (!MPath_IsDots(info.cFileName))
                {
                    if (MDir_Exists(info.cFileName))
                        MDir_Delete(info.cFileName);
                    else
                        MFile_Delete(info.cFileName);
                }
            } while(MDir_FindNextItem(dirp, &info));
            MDir_FindClose(dirp);
        }
        MDir_Set(dir_old);

        return MDir_Remove(dir);
    }
#endif  /* ndef MSDOS */

inline bool MDir_Get(MChar *pathname, size_t maxbuf)
{
    USING_NAMESPACE_STD;
#ifdef _WIN32
    return !!GetCurrentDirectory((DWORD)maxbuf, pathname);
#else
    MChar *pch = getcwd(NULL, maxbuf);
    if (pch)
    {
        if (maxbuf)
        {
            strncpy(pathname, pch, maxbuf);
            pathname[maxbuf - 1] = 0;
            free(pch);
            return true;
        }
        free(pch);
    }
    return false;
#endif
}

inline bool MDir_Set(const MChar *pathname)
{
    USING_NAMESPACE_STD;
#ifdef _WIN32
    return !!SetCurrentDirectory(pathname);
#else
    return chdir(pathname) == 0;
#endif
}

inline bool MPath_IsDots(const MChar *name)
{
    return name[0] == TEXT('.') && (
        name[1] == 0 || (name[1] == TEXT('.') && name[2] == 0)
    );
}

inline bool
MDir_CreateRecurse(const MChar *path, bool fForce)
{
    MChar *new_path, *pch, *last_sep;

    assert(path);
#ifdef _WIN32
    new_path = _tcsdup(path);
#else
    new_path = strdup(path);
#endif
    if (!new_path)
        return false;

    MPath_RemoveSep(new_path);

    for (;;)
    {
        if (!MPath_Exists(new_path))
        {
            last_sep = NULL;
#ifdef _WIN32
            for (pch = new_path; *pch; pch = CharNext(pch))
            {
                if (*pch == TEXT('\\') || *pch == TEXT('/'))
                    last_sep = pch;
            }
#else
            for (pch = new_path; *pch; ++pch)
            {
                if (*pch == '/')
                    last_sep = pch;
            }
#endif
            if (last_sep)
            {
                *last_sep = 0;
                if (MDir_CreateRecurse(new_path, fForce))
                {
                    free(new_path);
                    return MDir_Create(path);
                }
            }
            else
            {
                if (MDir_Create(new_path))
                {
                    return true;
                }
            }

            free(new_path);
            return false;
        }

        if (!MDir_Exists(new_path))
        {
            if (!fForce)
                return false;

            if (!MFile_Delete(new_path))
                return false;

            continue;
        }

        break;
    }

    free(new_path);
    return true;
}

inline void MPath_BackslashToSlash(MChar *pathname)
{
    MChar *pch = pathname;
#ifdef _WIN32
    while (*pch)
    {
        if (*pch == TEXT('\\'))
        {
            *pch = TEXT('/');
        }
        pch = CharNext(pch);
    }
#else
    while (*pch)
    {
        if (*pch == '\\')
        {
            *pch = '/';
        }
        ++pch;
    }
#endif
}

inline void MPath_SlashToBackslash(MChar *pathname)
{
    MChar *pch = pathname;
#ifdef _WIN32
    while (*pch)
    {
        if (*pch == TEXT('/'))
        {
            *pch = TEXT('\\');
        }
        pch = CharNext(pch);
    }
#else
    while (*pch)
    {
        if (*pch == '/')
        {
            *pch = '\\';
        }
        ++pch;
    }
#endif
}

/**************************************************************************/

#endif  /* ndef MZC4_MFILEAPI_H_ */
