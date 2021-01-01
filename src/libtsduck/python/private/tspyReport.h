//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2021, Thierry Lelegard
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
//!  @ingroup python
//!  TSDuck Python bindings: encapsulates Report objects for Python.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tspy.h"

//!
//! Get the TSDuck CERR report instance.
//! @return CERR report instance.
//!
TSDUCKPY void* tspyStdErrReport();

//!
//! Get the TSDuck NULLREP instance.
//! @return NULLREP instance.
//!
TSDUCKPY void* tspyNullReport();

//!
//! Create a new instance of AsyncReport.
//! @param [in] severity Initial severity.
//! @param [in] sync_log Synchronous log.
//! @param [in] timed_log Add time stamps in log messages.
//! @param [in] log_msg_count Maximum buffered log messages.
//! @return A new AsyncReport instance.
//!
TSDUCKPY void* tspyNewAsyncReport(int severity, bool sync_log, bool timed_log, size_t log_msg_count);

//!
//! Synchronously terminate an AsyncReport.
//! @param [in] report A previously allocated instance of Report.
//!
TSDUCKPY void tspyTerminateAsyncReport(void* report);

//!
//! Delete a previously allocated instance of Report.
//! @param [in] report A previously allocated instance of Report.
//!
TSDUCKPY void tspyDeleteReport(void* report);

//!
//! Set the maximum severity of an instance of Report.
//! @param [in] report Address of an instance of Report.
//! @param [in] severity Message severity.
//!
TSDUCKPY void tspySetMaxSeverity(void* report, int severity);

//!
//! Log a message on an instance of Report.
//! @param [in] report Address of an instance of Report.
//! @param [in] severity Message severity.
//! @param [in] buffer Address of a buffer containing a UTF-16 string.
//! @param [in] size Size in bytes of the buffer.
//!
TSDUCKPY void tspyLogReport(void* report, int severity, const uint8_t* buffer, size_t size);
