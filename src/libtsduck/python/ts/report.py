#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2020, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------
#
#  TSDuck Python bindings to reporting classes.
#
#-----------------------------------------------------------------------------

from . import lib
import ctypes

# Report base class.
class Report:
    # Severity levels, same values as C++ counterparts.
    FATAL   = -5;
    SEVERE  = -4;
    ERROR   = -3;
    WARNING = -2;
    INFO    = -1;
    VERBOSE =  0;
    DEBUG   =  1;

    # Constructor with the address of a C++ Report object.
    def __init__(self, report, to_be_deleted):
        self._report_addr = ctypes.c_void_p(report)
        self._to_be_deleted = to_be_deleted

    # Finalizer.
    def __del__(self):
        if self._to_be_deleted:
            lib.tspyDeleteReport(self._report_addr)
            self._report_addr = ctypes.c_void_p(0)
            self._to_be_deleted = False

    # Set the maximum severity of the report.
    def setMaxSeverity(self, severity):
        lib.tspySetMaxSeverity(self._report_addr, severity)

    # Log a message to the report.
    def log(self, severity, message):
        buf = lib.InByteBuffer(message)
        lib.tspyLogReport(self._report_addr, severity, buf.data_ptr(), buf.size())

    # Log messages at predefined levels.
    def error(self, message):
        self.log(Report.ERROR, message)
    def warning(self, message):
        self.log(Report.WARNING, message)
    def info(self, message):
        self.log(Report.INFO, message)
    def verbose(self, message):
        self.log(Report.VERBOSE, message)
    def debug(self, message):
        self.log(Report.DEBUG, message)

# A wrapper for C++ NullReport.
class NullReport(Report):
    def __init__(self):
        super().__init__(lib.tspyNullReport(), False)

# A wrapper for C++ CerrReport.
class StdErrReport(Report):
    def __init__(self):
        super().__init__(lib.tspyStdErrReport(), False)

# A wrapper for C++ AsyncReport.
class AsyncReport(Report):
    # Constructor, starts the async log thread.
    def __init__(self, severity = Report.INFO, sync_log = False, timed_log = False, log_msg_count = 0):
        super().__init__(lib.tspyNewAsyncReport(severity, sync_log, timed_log, log_msg_count), True)

    # Synchronously terminates the async log thread.
    def terminate(self):
        lib.tspyTerminateAsyncReport(self._report_addr)

    # Finalizer.
    def __del__(self):
        super().__del__()
