//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Synchronous message report with notification to a Python class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"

namespace ts {
    namespace py {
        //!
        //! Synchronous message report with notification to a Python class.
        //! @ingroup python
        //!
        class TSDUCKDLL SyncReport : public ts::Report
        {
            TS_NOBUILD_NOCOPY(SyncReport);
        public:
            //!
            //! Profile of a Python callback which receives log messages.
            //!
            using LogCallback = void* (*)(int severity, const UChar* message, size_t message_bytes);

            //!
            //! Constructor.
            //! @param [in] log_callback Python callback to receive log messages.
            //! @param [in] max_severity Set initial level report to that level.
            //!
            SyncReport(LogCallback log_callback, int max_severity);

            //!
            //! Destructor.
            //!
            virtual ~SyncReport() override;

        private:
            // Inherited from ts::Report:
            virtual void writeLog(int severity, const UString& msg) override;

            LogCallback _log_callback;
        };
    }
}
