// MSecurityAttributes.hpp -- security attributes wrapper       -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MSECURITYATTRIBUTES_HPP_
#define MZC4_MSECURITYATTRIBUTES_HPP_       2   /* Version 2 */

class MSecurityAttributes;

////////////////////////////////////////////////////////////////////////////

#ifndef _INC_WINDOWS
    #include <windows.h>    // Win32API
#endif
#include <cassert>          // assert

////////////////////////////////////////////////////////////////////////////

class MSecurityAttributes : public SECURITY_ATTRIBUTES
{
public:
    MSecurityAttributes(BOOL bInherit = TRUE,
                        LPVOID pSecurityDescriptor = NULL);
};

////////////////////////////////////////////////////////////////////////////

inline
MSecurityAttributes::MSecurityAttributes(
    BOOL bInherit/* = TRUE*/, LPVOID pSecurityDescriptor/* = NULL*/)
{
    nLength = sizeof(SECURITY_ATTRIBUTES);
    lpSecurityDescriptor = pSecurityDescriptor;
    bInheritHandle = bInherit;
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MSECURITYATTRIBUTES_HPP_
