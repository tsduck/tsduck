//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for classes using a Report object.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNullReport.h"

namespace ts {
    //!
    //! Base class for classes using a Report object.
    //! @ingroup libtscore log
    //!
    class TSCOREDLL ReporterBase
    {
        TS_DEFAULT_COPY_MOVE(ReporterBase);
    private:
        ReporterBase() = delete;
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //!
        explicit ReporterBase(Report* report) : _report(report) {}

        //!
        //! Destructor.
        //!
        virtual ~ReporterBase();

        //!
        //! Access the Report which is associated with this object.
        //! Can be called from another thread only if the Report object is thread-safe.
        //! @return A reference to the associated report.
        //!
        Report& report() const { return _mute || _report == nullptr ? NULLREP : *_report; }

        //!
        //! Associate this object with another Report to log errors.
        //! @param [in,out] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @return The address of the previous Report object or a null pointer if there was none.
        //!
        Report* setReport(Report* report);

        //!
        //! Temporarily mute the associated report.
        //! @param [in] mute It true, report() will return a null report (log messages are discarded),
        //! until muteReport() is invoked again with @a mute set to false.
        //! @return Previous state of the mute field.
        //!
        bool muteReport(bool mute);

    private:
        Report* _report = nullptr;
        bool    _mute = false;
    };
}
