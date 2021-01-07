#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2021, Thierry Lelegard
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

##
# Base class for TSDuck report classes.
# @ingroup python
#
class Report:
    # Severity levels, same values as C++ counterparts.
    ## Fatal error, typically aborts the application.
    Fatal   = -5;
    ## Severe errror.
    Severe  = -4;
    ## Regular error.
    Error   = -3;
    ## Warning message.
    Warning = -2;
    ## Information message.
    Info    = -1;
    ## Verbose information.
    Verbose =  0;
    ## First debug level.
    Debug   =  1;

    ##
    # Constructor with the address of a C++ Report object.
    # Used only by subclasses, do not not instantiate the base class from applictions.
    # @param report Address of the C++ Report object.
    # @param to_be_deleted A boolean, when true the C++ object is deallocated on finalization.
    #
    def __init__(self, report, to_be_deleted):
        self._report_addr = ctypes.c_void_p(report)
        self._to_be_deleted = to_be_deleted

    ##
    # Finalizer.
    # Deallocate the associated C++ object.
    #
    def __del__(self):
        if self._to_be_deleted:
            lib.tspyDeleteReport(self._report_addr)
            self._report_addr = ctypes.c_void_p(0)
            self._to_be_deleted = False

    ##
    # Set the maximum severity of the report.
    # @param severity Severity level.
    # @return None.
    #
    def setMaxSeverity(self, severity):
        lib.tspySetMaxSeverity(self._report_addr, severity)

    ##
    # Log a message to the report.
    # @param severity Severity level of the message.
    # @param message Message to report.
    # @return None.
    #
    def log(self, severity, message):
        buf = lib.InByteBuffer(message)
        lib.tspyLogReport(self._report_addr, severity, buf.data_ptr(), buf.size())

    ##
    # Log a messages at error level.
    # @param message Message to report.
    # @return None.
    #
    def error(self, message):
        self.log(Report.Error, message)

    ##
    # Log a messages at warning level.
    # @param message Message to report.
    # @return None.
    #
    def warning(self, message):
        self.log(Report.Warning, message)

    ##
    # Log a messages at info level.
    # @param message Message to report.
    # @return None.
    #
    def info(self, message):
        self.log(Report.Info, message)

    ##
    # Log a messages at verbose level.
    # @param message Message to report.
    # @return None.
    #
    def verbose(self, message):
        self.log(Report.Verbose, message)

    ##
    # Log a messages at debug level.
    # @param message Message to report.
    # @return None.
    #
    def debug(self, message):
        self.log(Report.Debug, message)

##
# A wrapper class for C++ NullReport.
# @ingroup python
#
class NullReport(Report):

    ##
    # Constructor.
    #
    def __init__(self):
        super().__init__(lib.tspyNullReport(), False)

##
# A wrapper class for C++ CerrReport.
# @ingroup python
#
class StdErrReport(Report):
    
    ##
    # Constructor.
    #
    def __init__(self):
        super().__init__(lib.tspyStdErrReport(), False)

##
# A wrapper class for C++ AsyncReport.
# @ingroup python
#
class AsyncReport(Report):

    ##
    # Constructor, starts the async log thread.
    # @param severity Initial severity.
    # @param sync_log Synchronous log.
    # @param timed_log Add time stamps in log messages.
    # @param log_msg_count Maximum buffered log messages.
    #
    def __init__(self, severity = Report.Info, sync_log = False, timed_log = False, log_msg_count = 0):
        super().__init__(lib.tspyNewAsyncReport(severity, sync_log, timed_log, log_msg_count), True)

    ##
    # Synchronously terminates the async log thread.
    # @return None.
    #
    def terminate(self):
        lib.tspyTerminateAsyncReport(self._report_addr)

    ##
    # Finalizer.
    # Deallocate the associated C++ object.
    #
    def __del__(self):
        super().__del__()
