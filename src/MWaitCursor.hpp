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

	// True while at least one MWaitCursor is alive, i.e. the wait cursor
	// should be showing. Exposed so a WM_SETCURSOR handler can keep
	// reasserting the wait cursor: SetCursor() only lasts until the next
	// WM_SETCURSOR message, so any code that pumps messages while an
	// MWaitCursor is alive (e.g. a long operation that keeps repainting
	// instead of just blocking) will otherwise see the cursor silently
	// reset to the window class's cursor the moment the mouse moves.
	static BOOL IsActive()
	{
		return s_count() > 0;
	}

public:
	static VOID DoWaitCursor(INT nCode);

private:
	// A function-local static gives this counter exactly one definition
	// across every translation unit that includes this header, without
	// needing a separate .cpp to hold an out-of-line static data member.
	static LONG& s_count()
	{
		static LONG s_nCount = 0;
		return s_nCount;
	}
};

////////////////////////////////////////////////////////////////////////////

inline VOID MWaitCursor::DoWaitCursor(INT nCode)
{
	static HCURSOR  s_hcurRestore = NULL;
	LONG& s_nCount = s_count();

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
