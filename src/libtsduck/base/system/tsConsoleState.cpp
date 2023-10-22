//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsConsoleState.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructor: save console state and configure it.
//----------------------------------------------------------------------------

ts::ConsoleState::ConsoleState(Report& report)
{
#if defined(TS_WINDOWS)
    _input_cp = ::GetConsoleCP();
    _output_cp = ::GetConsoleOutputCP();
    report.debug(u"previous code pages: input: %d, output: %d", {_input_cp, _output_cp});

    // Set Windows console input and output to UTF-8.
    if (::SetConsoleCP(CP_UTF8) == 0) {
        report.error(u"SetConsoleCP error: %s", {SysErrorCodeMessage()});
    }
    if (::SetConsoleOutputCP(CP_UTF8) == 0) {
        report.error(u"SetConsoleOutputCP error: %s", {SysErrorCodeMessage()});
    }

    report.debug(u"new code pages: input: %d, output: %d", {::GetConsoleCP(), ::GetConsoleOutputCP()});
#endif
}


//----------------------------------------------------------------------------
// Destructor: restore console state.
//----------------------------------------------------------------------------

ts::ConsoleState::~ConsoleState()
{
    // Restore initial console state.
#if defined(TS_WINDOWS)
    ::SetConsoleCP(_input_cp);
    ::SetConsoleOutputCP(_output_cp);
#endif
}
