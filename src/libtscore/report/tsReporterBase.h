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
#include "tsReport.h"

namespace ts {
    //!
    //! Base class for classes using a Report object.
    //! @ingroup libtscore log
    //!
    class TSCOREDLL ReporterBase
    {
        TS_DEFAULT_COPY_MOVE(ReporterBase);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //!
        explicit ReporterBase(Report* report = nullptr) : _report(report) {}

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //!
        explicit ReporterBase(ReporterBase* delegate) : _delegate(delegate) {}

        //!
        //! Destructor.
        //!
        virtual ~ReporterBase();

        //!
        //! Access the Report which is associated with this object.
        //! Can be called from another thread only if the Report object is thread-safe.
        //! @return A reference to the associated report.
        //!
        Report& report() const;

        //!
        //! Associate this object with another Report to log errors.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @return The address of the previous Report object or a null pointer if there was none.
        //!
        Report* setReport(Report* report);

        //!
        //! Associate this object with another ReporterBase to log errors.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, the previous explicit Report is used..
        //! @return The address of the previous ReporterBase object or a null pointer if there was none.
        //!
        ReporterBase* setReport(ReporterBase* delegate);

        //!
        //! Temporarily mute the associated report.
        //! @param [in] mute It true, report() will return a null report (log messages are discarded),
        //! until muteReport() is invoked again with @a mute set to false.
        //! @return Previous state of the mute field.
        //!
        bool muteReport(bool mute);

    protected:
        //!
        //! Compute a log severity level from a "silent" parameter.
        //! Some subclass methods have a "silent" parameter to avoid reporting errors which may be insignificant,
        //! typically when closing a device after an error, in which case the close operation may produce other
        //! errors if the previous error left the device in an inconsistent state. While those errors should not
        //! be displayed as errors, we still display them at debug level.
        //! @param [in] silent If true, do not report errors, report debug messages instead.
        //! @return Error when @a silent is false, Debug otherwise..
        //!
        //!
        static inline int SilentLevel(bool silent) { return silent ? Severity::Debug : Severity::Error; }

    private:
        ReporterBase* _delegate = nullptr;
        Report*       _report = nullptr;
        bool          _mute = false;
    };
}
