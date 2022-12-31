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
//!  Encapsulation of Windows Common Object Model (COM).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCerrReport.h"

namespace ts {
    //!
    //! A class to encapsulate the initialization of Windows COM.
    //! @ingroup windows
    //!
    //! On Windows, the applications which use the Common Object Model (COM) need
    //! to initialize the COM framework before using COM objects and correctly
    //! uninitialize COM when they are finished. The COM class does that as
    //! automatically as possible, regardless of the operating system.
    //!
    //! Each instance of COM initializes the COM frameword in the constructor
    //! and performs the corresponding uninitialize in the destructor.
    //!
    class TSDUCKDLL COM
    {
    public:
        //!
        //! Constructor.
        //! It initializes COM. The equivalent uninitialize will be performed in the destructor.
        //! @param [in,out] report Where to report error messages. Use the standard error by default.
        //!
        COM(Report& report = CERR);

        //!
        //! Destructor.
        //! It uninitializes COM, unless uninitialize() was already explicitly invoked.
        //!
        ~COM();

        //!
        //! Check if initialization was successful.
        //! @return True if initialization was successful and uninitialize() was not called.
        //!
        bool isInitialized() const {return _is_init;}

        //!
        //! Perform an early COM uninitialize (before destructor).
        //!
        void uninitialize();

    private:
        bool _is_init;
    };
}
