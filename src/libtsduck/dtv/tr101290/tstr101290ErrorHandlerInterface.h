//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract interface to receive ETSI TR 101 290 errors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstr101290.h"
#include "tsTS.h"

namespace ts::tr101290 {

    class Analyzer;

    //!
    //! Abstract interface to receive ETSI TR 101 290 errors.
    //! @ingroup libtsduck mpeg
    //!
    //! This abstract interface must be implemented by classes which need to be
    //! notified of errors from a ts::tr101290::Analyzer.
    //!
    class TSDUCKDLL ErrorHandlerInterface
    {
        TS_INTERFACE(ErrorHandlerInterface);
    public:
        //!
        //! This hook is invoked when an error is detected.
        //! @param [in,out] analyzer The analyzer instance which detected the error.
        //! @param [in] error The error code as an enum value.
        //! Use ts::tr101290::GetCounterDescription() if more details are needed.
        //! @param [in] reference A reference string which indicates the rule which triggered the error.
        //! @param [in] context The context of the error, for instance a table name or PID number.
        //! @param [in] pid The PID on which the error occurred, PID_NULL if not PID is relevant.
        //! @see ts::tr101290::GetCounterDescription()
        //!
        virtual void handleTR101290Error(Analyzer& analyzer, ErrorCounter error, const UString& reference, const UString& context, PID pid) = 0;
    };
}
