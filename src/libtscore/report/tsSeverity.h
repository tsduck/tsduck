//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Message severity
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class UString;
    class Names;

    //!
    //! Message severity.
    //! @ingroup log
    //!
    //! Positive values are debug levels. The typical default reporting level is @c Info.
    //! All messages with a higher level (@c Verbose and all debug levels) are not reported by default.
    //!
    //! The @c struct is here just to add a naming level.
    //!
    struct TSCOREDLL Severity {

        static constexpr int Fatal   = -5;  //!< Fatal error, typically aborts the application.
        static constexpr int Severe  = -4;  //!< Severe errror.
        static constexpr int Error   = -3;  //!< Regular error.
        static constexpr int Warning = -2;  //!< Warning message.
        static constexpr int Info    = -1;  //!< Information message.
        static constexpr int Verbose = 0;   //!< Verbose information.
        static constexpr int Debug   = 1;   //!< First debug level.

        //!
        //! Formatted line prefix header for a severity
        //! @param [in] severity Severity value.
        //! @return A string to prepend to messages. Empty for Info and Verbose levels.
        //!
        static UString Header(int severity);

        //!
        //! An enumeration to use severity values on the command line for instance.
        //! @return A constant reference to the enumeration data.
        //!
        static const Names& Enums();
    };
}
