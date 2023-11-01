//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  An encapsulation of Report with a message prefix.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsUString.h"

namespace ts {
    //!
    //! An encapsulation of Report with a message prefix.
    //! @ingroup log
    //!
    //! This class encapsulates another instance of Report and
    //! prepend all messages with a prefix.
    //!
    class TSDUCKDLL ReportWithPrefix : public Report
    {
        TS_NOBUILD_NOCOPY(ReportWithPrefix);
    public:
        //!
        //! Constructor.
        //! @param [in] report The actual object which is used to report.
        //! @param [in] prefix The prefix to prepend to all messages.
        //!
        explicit ReportWithPrefix(Report& report, const UString& prefix = UString());

        //!
        //! Get the current prefix to display.
        //! @return The current prefix to display.
        //!
        UString prefix() const { return _prefix; }

        //!
        //! Set the prefix to display.
        //! @param [in] prefix The prefix to prepend to all messages.
        //!
        void setPrefix(const UString& prefix) { _prefix = prefix; }

        // Inherited methods.
        virtual void setMaxSeverity(int level) override;

    protected:
        // Inherited methods.
        virtual void writeLog(int severity, const UString& msg) override;

    private:
        Report& _report;
        UString _prefix {};
    };
}
