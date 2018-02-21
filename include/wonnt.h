/* wonnt.h --- Wonders API by katahiromz */
/**************************************************************************/

#ifndef WONNT_H
#define WONNT_H     14  /* Version 14 */

#if defined(_WIN32) && !defined(WONVER)
    #include <windows.h>
#else

/**************************************************************************/

#ifndef WINAPI
    #define WINAPI      /*empty*/
    #define WINAPIV     /*empty*/
#endif

#ifndef NTAPI
    #define NTAPI       /*empty*/
#endif

/**************************************************************************/

#if __cplusplus >= 201103L
    #include <cstdint>
#elif __STDC_VERSION__ >= 199901L
    #include <stdint.h>
#else
    #include "pstdint.h"
#endif

typedef char CHAR;
typedef signed char SCHAR;
typedef uint8_t BYTE, UCHAR;
typedef int16_t SHORT;
typedef uint16_t WORD, USHORT;
typedef int32_t LONG;
typedef uint32_t DWORD, ULONG;
typedef int32_t INT, BOOL;
typedef uint32_t UINT;
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG, DWORDLONG;

/* WCHAR */
#ifndef __WCHAR_DEFINED
    #define __WCHAR_DEFINED
    #ifdef _WIN32
        typedef wchar_t WCHAR;
    #else
        #if __cplusplus >= 201103L
            typedef char16_t WCHAR;
        #else
            typedef uint16_t WCHAR;
        #endif
    #endif
#endif

#ifdef _WIN64
    typedef int64_t LONG_PTR;
    typedef uint64_t ULONG_PTR, DWORD_PTR;
#else
    typedef LONG LONG_PTR;
    typedef ULONG ULONG_PTR, DWORD_PTR;
#endif

typedef BYTE BOOLEAN;

#ifdef UNICODE
    typedef WCHAR TCHAR;
#else
    typedef char TCHAR;
#endif

typedef void *HANDLE;
typedef INT HFILE;

#define C_ASSERT(x)  typedef char WONNT_STATIC_ASSERT_##__LINE__[(x) ? 1 : -1]

C_ASSERT(sizeof(CHAR) == 1);
C_ASSERT(sizeof(SCHAR) == 1);
C_ASSERT(sizeof(UCHAR) == 1);
C_ASSERT(sizeof(BYTE) == 1);

C_ASSERT(sizeof(SHORT) == 2);
C_ASSERT(sizeof(USHORT) == 2);
C_ASSERT(sizeof(WORD) == 2);

C_ASSERT(sizeof(LONG) == 4);
C_ASSERT(sizeof(ULONG) == 4);
C_ASSERT(sizeof(DWORD) == 4);

C_ASSERT(sizeof(LONGLONG) == 8);
C_ASSERT(sizeof(ULONGLONG) == 8);
C_ASSERT(sizeof(DWORDLONG) == 8);

C_ASSERT(sizeof(INT) == sizeof(int));
C_ASSERT(sizeof(UINT) == sizeof(unsigned int));

C_ASSERT(sizeof(BOOL) == 4);
C_ASSERT(sizeof(BOOLEAN) == 1);

C_ASSERT(sizeof(HANDLE) == sizeof(void *));

C_ASSERT(sizeof(WCHAR) == sizeof(wchar_t));

typedef WORD LANGID;

#define MAKELANGID(p, s)       ((((WORD)(s)) << 10) | (WORD)(p))
#define PRIMARYLANGID(lgid)    ((WORD)(lgid) & 0x3ff)
#define SUBLANGID(lgid)        ((WORD)(lgid) >> 10)

/**************************************************************************/

#define IMAGE_DOS_SIGNATURE 0x5A4D
typedef struct {
    WORD e_magic;
    WORD e_cblp;
    WORD e_cp;
    WORD e_crlc;
    WORD e_cparhdr;
    WORD e_minalloc;
    WORD e_maxalloc;
    WORD e_ss;
    WORD e_sp;
    WORD e_csum;
    WORD e_ip;
    WORD e_cs;
    WORD e_lfarlc;
    WORD e_ovno;
    WORD e_res[4];
    WORD e_oemid;
    WORD e_oeminfo;
    WORD e_res2[10];
    LONG e_lfanew;
} IMAGE_DOS_HEADER;

typedef struct {
    WORD Machine;
    WORD NumberOfSections;
    DWORD TimeDateStamp;
    DWORD PointerToSymbolTable;
    DWORD NumberOfSymbols;
    WORD SizeOfOptionalHeader;
    WORD Characteristics;
} IMAGE_FILE_HEADER;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct {
    DWORD VirtualAddress;
    DWORD Size;
} IMAGE_DATA_DIRECTORY;

typedef struct {
    WORD Magic;
    BYTE MajorLinkerVersion;
    BYTE MinorLinkerVersion;
    DWORD SizeOfCode;
    DWORD SizeOfInitializedData;
    DWORD SizeOfUninitializedData;
    DWORD AddressOfEntryPoint;
    DWORD BaseOfCode;
    DWORD BaseOfData;
    DWORD ImageBase;
    DWORD SectionAlignment;
    DWORD FileAlignment;
    WORD MajorOperatingSystemVersion;
    WORD MinorOperatingSystemVersion;
    WORD MajorImageVersion;
    WORD MinorImageVersion;
    WORD MajorSubsystemVersion;
    WORD MinorSubsystemVersion;
    DWORD Win32VersionValue;
    DWORD SizeOfImage;
    DWORD SizeOfHeaders;
    DWORD CheckSum;
    WORD Subsystem;
    WORD DllCharacteristics;
    DWORD SizeOfStackReserve;
    DWORD SizeOfStackCommit;
    DWORD SizeOfHeapReserve;
    DWORD SizeOfHeapCommit;
    DWORD LoaderFlags;
    DWORD NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32;

typedef struct {
    WORD Magic;
    BYTE MajorLinkerVersion;
    BYTE MinorLinkerVersion;
    DWORD SizeOfCode;
    DWORD SizeOfInitializedData;
    DWORD SizeOfUninitializedData;
    DWORD AddressOfEntryPoint;
    DWORD BaseOfCode;
    ULONGLONG ImageBase;
    DWORD SectionAlignment;
    DWORD FileAlignment;
    WORD MajorOperatingSystemVersion;
    WORD MinorOperatingSystemVersion;
    WORD MajorImageVersion;
    WORD MinorImageVersion;
    WORD MajorSubsystemVersion;
    WORD MinorSubsystemVersion;
    DWORD Win32VersionValue;
    DWORD SizeOfImage;
    DWORD SizeOfHeaders;
    DWORD CheckSum;
    WORD Subsystem;
    WORD DllCharacteristics;
    ULONGLONG SizeOfStackReserve;
    ULONGLONG SizeOfStackCommit;
    ULONGLONG SizeOfHeapReserve;
    ULONGLONG SizeOfHeapCommit;
    DWORD LoaderFlags;
    DWORD NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64;

#define IMAGE_NT_SIGNATURE 0x00004550
typedef struct {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32;
typedef struct {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64;

#ifdef _WIN64
    typedef IMAGE_OPTIONAL_HEADER64 IMAGE_OPTIONAL_HEADER;
    typedef IMAGE_NT_HEADERS64 IMAGE_NT_HEADERS;
#else
    typedef IMAGE_OPTIONAL_HEADER32 IMAGE_OPTIONAL_HEADER;
    typedef IMAGE_NT_HEADERS32 IMAGE_NT_HEADERS;
#endif

#define IMAGE_DIRECTORY_ENTRY_EXPORT 0
#define IMAGE_DIRECTORY_ENTRY_IMPORT 1
#define IMAGE_DIRECTORY_ENTRY_RESOURCE 2
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION 3
#define IMAGE_DIRECTORY_ENTRY_SECURITY 4
#define IMAGE_DIRECTORY_ENTRY_BASERELOC 5
#define IMAGE_DIRECTORY_ENTRY_DEBUG 6
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE 7
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR 8
#define IMAGE_DIRECTORY_ENTRY_TLS 9
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG 10
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT 11
#define IMAGE_DIRECTORY_ENTRY_IAT 12
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT 13
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14

typedef struct {
    union {
        DWORD Characteristics;
        DWORD OriginalFirstThunk;
    };
    DWORD TimeDateStamp;

    DWORD ForwarderChain;
    DWORD Name;
    DWORD FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR;

typedef struct {
    DWORD Characteristics;
    DWORD TimeDateStamp;
    WORD MajorVersion;
    WORD MinorVersion;
    DWORD Name;
    DWORD Base;
    DWORD NumberOfFunctions;
    DWORD NumberOfNames;
    DWORD AddressOfFunctions;
    DWORD AddressOfNames;
    DWORD AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY;

#define IMAGE_SIZEOF_SHORT_NAME 8

typedef struct {
    BYTE Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
        DWORD PhysicalAddress;
        DWORD VirtualSize;
    } Misc;
    DWORD VirtualAddress;
    DWORD SizeOfRawData;
    DWORD PointerToRawData;
    DWORD PointerToRelocations;
    DWORD PointerToLinenumbers;
    WORD NumberOfRelocations;
    WORD NumberOfLinenumbers;
    DWORD Characteristics;
} IMAGE_SECTION_HEADER;

#define IMAGE_ORDINAL_FLAG64 0x8000000000000000ull
#define IMAGE_ORDINAL_FLAG32 0x80000000
#define IMAGE_ORDINAL64(Ordinal) (Ordinal & 0xffffull)
#define IMAGE_ORDINAL32(Ordinal) (Ordinal & 0xffff)
#define IMAGE_SNAP_BY_ORDINAL64(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG64)!=0)
#define IMAGE_SNAP_BY_ORDINAL32(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG32)!=0)

typedef struct {
    WORD Hint;
    BYTE Name[1];
} IMAGE_IMPORT_BY_NAME;

#define IMAGE_RESOURCE_NAME_IS_STRING        0x80000000
#define IMAGE_RESOURCE_DATA_IS_DIRECTORY     0x80000000

typedef struct {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    WORD    NumberOfNamedEntries;
    WORD    NumberOfIdEntries;
} IMAGE_RESOURCE_DIRECTORY;

typedef struct {
    union {
        struct {
            DWORD NameOffset:31;
            DWORD NameIsString:1;
        };
        DWORD   Name;
        WORD    Id;
    };
    union {
        DWORD   OffsetToData;
        struct {
            DWORD   OffsetToDirectory:31;
            DWORD   DataIsDirectory:1;
        };
    };
} IMAGE_RESOURCE_DIRECTORY_ENTRY;

/**************************************************************************/

#ifndef FIELD_OFFSET
    #define FIELD_OFFSET(type, field) \
        ((LONG)(LONG_PTR)&(((type *)0)->field))
#endif

#ifndef RTL_FIELD_SIZE
    #define RTL_FIELD_SIZE(type, field) \
        (sizeof(((type *)0)->field))
#endif

#ifndef RTL_SIZEOF_THROUGH_FIELD
    #define RTL_SIZEOF_THROUGH_FIELD(type, field) \
        (FIELD_OFFSET(type, field) + RTL_FIELD_SIZE(type, field))
#endif

#ifndef IMAGE_FIRST_SECTION
    #define IMAGE_FIRST_SECTION(nt) ((IMAGE_SECTION_HEADER *)             \
        ((ULONG_PTR)nt + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + \
         ((IMAGE_NT_HEADERS *)(nt))->FileHeader.SizeOfOptionalHeader))
#endif

/**************************************************************************/

#endif  /* !(defined(_WIN32) && !defined(_WONVER)) */
#endif  /* ndef WONNT_H */
