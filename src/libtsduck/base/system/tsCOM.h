//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_NOCOPY(COM);
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
        bool isInitialized() const { return _is_init; }

        //!
        //! Perform an early COM uninitialize (before destructor).
        //!
        void uninitialize();

    private:
        bool _is_init = false;
    };
}
