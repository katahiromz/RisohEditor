// MWaitCursor.hpp --- Win32API wait cursor manager             -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MWAITCURSOR_HPP_
#define MZC4_MWAITCURSOR_HPP_       2   /* Version 2 */

class MWaitCursor;

////////////////////////////////////////////////////////////////////////////

#ifndef _INC_WINDOWS
    #include <windows.h>    // Win32API
#endif
#include <cassert>          // assert

////////////////////////////////////////////////////////////////////////////

class MWaitCursor
{
public:
    MWaitCursor();
    virtual ~MWaitCursor();
    VOID Restore();

public:
    static VOID DoWaitCursor(INT nCode);
};

////////////////////////////////////////////////////////////////////////////

inline VOID MWaitCursor::DoWaitCursor(INT nCode)
{
    static LONG     s_nCount = 0;
    static HCURSOR  s_hcurRestore = NULL;

    assert(nCode == 0 || nCode == 1 || nCode == -1);

    switch (nCode)
    {
    case -1:
        InterlockedDecrement(&s_nCount);
        break;

    case 1:
        InterlockedIncrement(&s_nCount);
        break;
    }

    if (s_nCount > 0)
    {
        HCURSOR hcurPrev = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
        if (nCode > 0 && s_nCount == 1)
            s_hcurRestore = hcurPrev;
    }
    else
    {
        s_nCount = 0;
        ::SetCursor(s_hcurRestore);
    }
}

inline MWaitCursor::MWaitCursor()
{
    MWaitCursor::DoWaitCursor(1);
}

inline /*virtual*/ MWaitCursor::~MWaitCursor()
{
    MWaitCursor::DoWaitCursor(-1);
}

inline VOID MWaitCursor::Restore()
{
    MWaitCursor::DoWaitCursor(0);
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MWAITCURSOR_HPP_
