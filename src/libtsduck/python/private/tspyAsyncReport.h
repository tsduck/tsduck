//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
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
            typedef void* (*LogCallback)(int severity, const UChar* message, size_t message_bytes);

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
