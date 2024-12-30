//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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

namespace ts {
    //!
    //! A singleton implementing Report which drops all messages.
    //! @ingroup log
    //!
    class TSDUCKDLL NullReport : public Report
    {
        TS_NOCOPY(NullReport);
    public:
        //!
        //! Get the instance of the CerrReport singleton.
        //! @return A reference to the CerrReport singleton.
        //!
        static NullReport& Instance();

    protected:
        // String interface implementation
        virtual void writeLog(int severity, const UString& msg) override;

    private:
        // Prevent direct construction.
        NullReport() = default;
    };
}

//!
//! Macro for fast access to the ts::NullReport singleton.
//!
#define NULLREP (ts::NullReport::Instance())
