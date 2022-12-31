//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
        const ::UINT _input_cp;
        const ::UINT _output_cp;
#endif
    };
}
