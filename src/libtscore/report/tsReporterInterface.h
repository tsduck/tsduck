//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Interface for classes using an associated Report object.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"

namespace ts {
    //!
    //! Interface for classes using an associated Report object.
    //! @ingroup libtscore log
    //!
    class TSCOREDLL ReporterInterface
    {
        TS_INTERFACE(ReporterInterface);
    public:
        //!
        //! Access the Report which is associated with this object.
        //! Can be called from another thread only if the Report object is thread-safe.
        //! @return A reference to the associated report.
        //!
        virtual Report& report() const = 0;
    };
}
