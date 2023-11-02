//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A singleton implementing Report which drops all messages.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsSingleton.h"

namespace ts {
    //!
    //! A singleton implementing Report which drops all messages.
    //! @ingroup log
    //!
    class TSDUCKDLL NullReport : public Report
    {
        TS_DECLARE_SINGLETON(NullReport);
    protected:
        // String interface implementation
        virtual void writeLog(int severity, const UString& msg) override;
    };
}

//!
//! Macro for fast access to the ts::NullReport singleton.
//!
#define NULLREP (ts::NullReport::Instance())
