//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A singleton implementing Report on std::cerr without synchronization
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsSingleton.h"

namespace ts {
    //!
    //! A singleton implementing Report on std::cerr without synchronization.
    //! @ingroup log
    //!
    //! If the environment variable TS_CERR_DEBUG_LEVEL is set to some integer
    //! value, it is used as initial maximum severity level for this object.
    //!
    //! This class is a singleton. Use static Instance() method to access the single instance.
    //!
    class TSDUCKDLL CerrReport : public Report
    {
        TS_DECLARE_SINGLETON(CerrReport);
    protected:
        // Report implementation
        virtual void writeLog(int severity, const UString& msg) override;
    };
}

//!
//! Macro for fast access to the ts::CerrReport singleton.
//!
#define CERR (ts::CerrReport::Instance())
