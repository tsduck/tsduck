//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Temporarily modify the Report of a ReporterBase object.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"

namespace ts {
    //!
    //! Temporarily modify the Report of a ReporterBase object.
    //! This "guard" class uses the RAII pattern (Resource Acquisition Is Initialization) to replace
    //! the Report of a ReporterBase object during its initialization and restores the previous Report in
    //! its destructor. Somehow, ReporterGuard acts on ReporterBase as std::lock_guard acts on std::mutex.
    //! @ingroup libtscore log
    //!
    class TSCOREDLL ReporterGuard
    {
        TS_NOBUILD_NOCOPY(ReporterGuard);
    public:
        //!
        //! Constructor.
        //! @param [in] base ReporterBase object for which the report is modified.
        //! @param [in] replacement Temporary Report to set on @a base. Use nullptr to drop reported messages.
        //!
        ReporterGuard(ReporterBase& base, Report* replacement);

        //!
        //! Destructor.
        //! The Report that was used in @a base before the constructor of this object is restored.
        //!
        ~ReporterGuard();

    private:
        ReporterBase& _base;
        Report*       _previous;
    };
}
