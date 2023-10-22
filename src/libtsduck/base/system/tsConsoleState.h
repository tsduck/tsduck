//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Save and restore the state of the Windows console.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCerrReport.h"

namespace ts {
    //!
    //! A class to save and restore the state of the Windows console.
    //! @ingroup windows
    //!
    //! On Windows, the old DOS console and the PowerShell console use "code pages"
    //! for characters. By default, the current code page is some locale which is
    //! usually not compatible with UTF-8. As a consequence, outputs from TSDuck
    //! command line applications may appears garbled.
    //!
    //! The constructor of an instance of this class saves the state of the console
    //! and switches to UTF-8. The destructor restores the previous state. So, defining
    //! one instance of this class properly configures the console.
    //!
    //! Important: To restore the previous state, the destructor must be called.
    //! Be aware that a premature call to exit() for instance only calls the
    //! destructors of the static objects, not the destructors of objects in the
    //! stack frames at the time exit() was called. To make sure that the previous
    //! state of the console is properly restore, you should declare a static
    //! instance of this class, not an object in the main() function.
    //!
    //! Additional notes:
    //! - Other consoles on Windows such as mintty (Msys and Cygwin for instance)
    //!   do not have this problem. Msys and Cygwin start with UTF-8 as default.
    //! - Other operating systems such as Linux and macOS do not have this problem
    //!   either. This class is available on Linux and macOS but does nothing.
    //!
    class TSDUCKDLL ConsoleState
    {
        TS_NOCOPY(ConsoleState);
    public:
        //!
        //! Constructor.
        //! On Windows, it initializes the console code page to UTF-8. The previous state
        //! will be restored in the destructor.
        //! @param [in,out] report Where to report error messages. Use the standard error by default.
        //!
        ConsoleState(Report& report = CERR);

        //!
        //! Destructor.
        //! On Windows, it restores the original console code page.
        //!
        ~ConsoleState();

    private:
#if defined(TS_WINDOWS)
        ::UINT _input_cp = 0;
        ::UINT _output_cp = 0;
#endif
    };
}
