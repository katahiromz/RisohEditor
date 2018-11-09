/* pstdint.h -- portable standard integers                      -*- C++ -*- */
/* This file is part of MZC4.  See file "ReadMe.txt" and "License.txt". */
/****************************************************************************/

#ifndef MZC4_PSTDINT_H_
#define MZC4_PSTDINT_H_     18   /* Version 18 */

#if __cplusplus >= 201103L
    #include <cstdint>
#elif __STDC_VERSION__ >= 199901L
    #include <stdint.h>
#elif defined(_WIN32) && !defined(WONVER)
    #ifndef _INC_WINDOWS
        #include <windows.h>
    #endif
    typedef signed char int8_t;
    typedef SHORT       int16_t;
    typedef INT         int32_t;
    typedef LONGLONG    int64_t;
    typedef BYTE        uint8_t;
    typedef WORD        uint16_t;
    typedef UINT        uint32_t;
    typedef DWORDLONG   uint64_t;
    typedef INT_PTR     intptr_t;
    typedef UINT_PTR    uintptr_t;
#else
    #ifdef __cplusplus
        #include <cstddef>
        #include <climits>
    #else
        #include <stddef.h>
        #include <limits.h>
    #endif
    #ifndef INT8_MIN
        #define INT8_MIN (-128)
        #define INT8_MAX 127
        #define UINT8_MAX 0xFF
    #endif
    #ifndef INT16_MIN
        #define INT16_MIN (-32768)
        #define INT16_MAX 32767
        #define UINT16_MAX 0xFFFF
    #endif
    #ifndef INT32_MIN
        #define INT32_MIN (-2147483647 - 1)
        #define INT32_MAX 2147483647
        #define UINT32_MAX 0xFFFFFFFF
    #endif
    typedef signed char                 int8_t;
    typedef unsigned char               uint8_t;
    typedef short                       int16_t;
    typedef unsigned short              uint16_t;
    #ifdef MSDOS
        typedef long                    int32_t;
        typedef unsigned long           uint32_t;
        typedef int                     intptr_t;
        typedef unsigned int            uintptr_t;
    #else
        typedef int                     int32_t;
        typedef unsigned int            uint32_t;
        #ifndef INT64_MAX
            #ifdef _I64_MAX
                #define INT64_MIN _I64_MIN
                #define INT64_MAX _I64_MAX
                #define UINT64_MAX _UI64_MAX
                typedef __int64             int64_t;
                typedef unsigned __int64    uint64_t;
                typedef __int64             intptr_t;
                typedef unsigned __int64    uintptr_t;
            #else
                #if defined(__LP64__) && !defined(__APPLE__)
                    #define INT64_MIN (-9223372036854775807L - 1)
                    #define INT64_MAX 9223372036854775807L
                    #define UINT64_MAX 0xFFFFFFFFFFFFFFFFL
                    typedef long           int64_t;
                    typedef unsigned long  uint64_t;
                #else
                    #define INT64_MIN (-9223372036854775807LL - 1)
                    #define INT64_MAX 9223372036854775807LL
                    #define UINT64_MAX 0xFFFFFFFFFFFFFFFFLL
                    typedef long long           int64_t;
                    typedef unsigned long long  uint64_t;
                #endif
                typedef long           intptr_t;
                typedef unsigned long  uintptr_t;
            #endif
        #endif
    #endif
#endif

typedef char MZC4_PSTDINT_TEST_01_[(sizeof(int8_t) == 1) ? 1 : -1];
typedef char MZC4_PSTDINT_TEST_02_[(sizeof(uint8_t) == 1) ? 1 : -1];
typedef char MZC4_PSTDINT_TEST_03_[(sizeof(int16_t) == 2) ? 1 : -1];
typedef char MZC4_PSTDINT_TEST_04_[(sizeof(uint16_t) == 2) ? 1 : -1];
typedef char MZC4_PSTDINT_TEST_05_[(sizeof(int32_t) == 4) ? 1 : -1];
typedef char MZC4_PSTDINT_TEST_06_[(sizeof(uint32_t) == 4) ? 1 : -1];
#ifndef MSDOS
    typedef char MZC4_PSTDINT_TEST_07_[(sizeof(int64_t) == 8) ? 1 : -1];
    typedef char MZC4_PSTDINT_TEST_08_[(sizeof(uint64_t) == 8) ? 1 : -1];
#endif
typedef char MZC4_PSTDINT_TEST_09_[(sizeof(intptr_t) == sizeof(void *)) ? 1 : -1];
typedef char MZC4_PSTDINT_TEST_10_[(sizeof(uintptr_t) == sizeof(void *)) ? 1 : -1];

/****************************************************************************/

#endif  /* ndef MZC4_PSTDINT_H_ */
