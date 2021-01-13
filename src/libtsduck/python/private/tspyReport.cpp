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
//
//  TSDuck Python bindings: encapsulates Report objects for Python.
//
//----------------------------------------------------------------------------

#include "tspy.h"
#include "tspyAsyncReport.h"
#include "tsCerrReport.h"
#include "tsNullReport.h"
TSDUCK_SOURCE;

//----------------------------------------------------------------------------
// Interface of native methods.
//----------------------------------------------------------------------------

//!
//! Build a report header from a severity.
//! @param [in] severity Severity level.
//! @param [out] buffer Where to write the UTF-16 header string.
//! @param [in,out] buffer_size Buffer size in bytes. Updated with the string length in bytes.
//!
TSDUCKPY void tspyReportHeader(int severity, uint8_t* buffer, size_t* buffer_size);

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
//! Create a new instance of ts::AsyncReport.
//! @param [in] severity Initial severity.
//! @param [in] sync_log Synchronous log.
//! @param [in] timed_log Add time stamps in log messages.
//! @param [in] log_msg_count Maximum buffered log messages.
//! @return A new AsyncReport instance.
//!
TSDUCKPY void* tspyNewAsyncReport(int severity, bool sync_log, bool timed_log, size_t log_msg_count);

//!
//! Create a new instance of ts::py::AsyncReport.
//! @param [in] log Python callback to log messages.
//! @param [in] severity Initial severity.
//! @param [in] sync_log Synchronous log.
//! @param [in] log_msg_count Maximum buffered log messages.
//! @return A new AsyncReport instance.
//!
TSDUCKPY void* tspyNewPyAsyncReport(ts::py::AsyncReport::LogCallback log, int severity, bool sync_log, size_t log_msg_count);

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

//-----------------------------------------------------------------------------
// Build a report header from a severity.
//-----------------------------------------------------------------------------

void tspyReportHeader(int severity, uint8_t* buffer, size_t* buffer_size)
{
    if (buffer != nullptr && buffer_size != nullptr) {
        const ts::UString str(ts::Severity::Header(severity));
        *buffer_size = 2 * std::min(*buffer_size / 2, str.size());
        ::memcpy(buffer, str.data(), *buffer_size);
    }
}

//-----------------------------------------------------------------------------
// Get static report instances.
//-----------------------------------------------------------------------------

void* tspyStdErrReport()
{
    return ts::CerrReport::Instance();
}

void* tspyNullReport()
{
    return ts::NullReport::Instance();
}

//-----------------------------------------------------------------------------
// Interface to ts::AsyncReport.
//-----------------------------------------------------------------------------

void* tspyNewAsyncReport(int severity, bool sync_log, bool timed_log, size_t log_msg_count)
{
    ts::AsyncReportArgs args;
    args.sync_log = sync_log;
    args.timed_log = timed_log;
    args.log_msg_count = log_msg_count > 0 ? log_msg_count : ts::AsyncReportArgs::MAX_LOG_MESSAGES;
    return new ts::AsyncReport(severity, args);
}

void tspyTerminateAsyncReport(void* report)
{
    ts::AsyncReport* rep = reinterpret_cast<ts::AsyncReport*>(report);
    if (rep != nullptr) {
        rep->terminate();
    }
}

//-----------------------------------------------------------------------------
// Interface to ts::py::AsyncReport.
//-----------------------------------------------------------------------------

void* tspyNewPyAsyncReport(ts::py::AsyncReport::LogCallback log, int severity, bool sync_log, size_t log_msg_count)
{
    ts::AsyncReportArgs args;
    args.sync_log = sync_log;
    args.log_msg_count = log_msg_count > 0 ? log_msg_count : ts::AsyncReportArgs::MAX_LOG_MESSAGES;
    return new ts::py::AsyncReport(log, severity, args);
}

//-----------------------------------------------------------------------------
// Delete a previously allocated instance of Report.
//-----------------------------------------------------------------------------

void tspyDeleteReport(void* report)
{
    delete reinterpret_cast<ts::Report*>(report);
}

//-----------------------------------------------------------------------------
// Set the maximum severity of an instance of Report.
//-----------------------------------------------------------------------------

void tspySetMaxSeverity(void* report, int severity)
{
    ts::Report* rep = reinterpret_cast<ts::Report*>(report);
    if (rep != nullptr) {
        rep->setMaxSeverity(severity);
    }
}

//-----------------------------------------------------------------------------
// Log a message on an instance of Report.
//-----------------------------------------------------------------------------

void tspyLogReport(void* report, int severity, const uint8_t* buffer, size_t size)
{
    ts::Report* rep = reinterpret_cast<ts::Report*>(report);
    if (rep != nullptr) {
        rep->log(severity, ts::py::ToString(buffer, size));
    }
}
