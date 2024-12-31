//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Asynchronous message report with notification to a Python class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAsyncReport.h"

namespace ts {
    namespace py {
        //!
        //! Asynchronous message report with notification to a Python class.
        //! @ingroup python
        //!
        class TSDUCKDLL AsyncReport : public ts::AsyncReport
        {
            TS_NOBUILD_NOCOPY(AsyncReport);
        public:
            //!
            //! Profile of a Python callback which receives log messages.
            //!
            using LogCallback = void* (*)(int severity, const UChar* message, size_t message_bytes);

            //!
            //! Constructor.
            //! @param [in] log_callback Python callback to receive log messages.
            //! @param [in] max_severity Set initial level report to that level.
            //! @param [in] args Initial parameters.
            //!
            AsyncReport(LogCallback log_callback, int max_severity, const AsyncReportArgs& args = AsyncReportArgs());

            //!
            //! Destructor.
            //!
            virtual ~AsyncReport() override;

        private:
            // Inherited from ts::AsyncReport:
            virtual void asyncThreadLog(int severity, const UString& message) override;

            LogCallback _log_callback;
        };
    }
}
