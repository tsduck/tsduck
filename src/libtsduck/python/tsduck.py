#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
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
#  TSDuck Python bindings
#
#-----------------------------------------------------------------------------

import os
import platform
import ctypes
import ctypes.util


#-----------------------------------------------------------------------------
# Load the TSDuck library
#-----------------------------------------------------------------------------

## @cond nodoxygen

# This internal function searches the TSDuck shared library.
def _searchLibTSDuck():
    if os.name == 'nt':
        base = 'tsduck.dll'
        search = [os.getenv('TSDUCK','')]
        search.extend(os.getenv('Path','').split(os.pathsep))
    elif platform.system() == 'Darwin':
        base = 'libtsduck.dylib'
        # On macOS, LD_LIBRARY_PATH is not passed to shell-scripts for security reasons.
        # A backup version LD_LIBRARY_PATH2 is defined to test development versions.
        search = os.getenv('LD_LIBRARY_PATH2','').split(os.pathsep)
        search.extend(os.getenv('LD_LIBRARY_PATH','').split(os.pathsep))
    else:
        base = 'libtsduck.so'
        search = os.getenv('LD_LIBRARY_PATH','').split(os.pathsep)

    # Search the TSDuck library.
    path = ''
    for dir in (dir for dir in search if dir != ''):
        file = dir + os.sep + base
        if os.path.exists(file):
            return file

    # If not found in various explicit paths, try system search.
    return ctypes.util.find_library('tsduck')

# Load the TSDuck library.
_lib = ctypes.CDLL(_searchLibTSDuck())

# Definition of some C/C++ pointer types.
_c_uint8_p = ctypes.POINTER(ctypes.c_uint8)
_c_size_p = ctypes.POINTER(ctypes.c_size_t)

## @endcond


#-----------------------------------------------------------------------------
# Utility class to pass a string to C++.
# The C++ function uses (const uint8_t* buffer, size_t size) parameters.
#-----------------------------------------------------------------------------

## @cond nodoxygen

class _InByteBuffer:
    # Constructor with string value.
    def __init__(self, string):
        if isinstance(string, str):
            self._data = bytearray(string.encode("utf-16"))
        else:
            self._data = bytearray()

    # Append with the content of a string or list of strings.
    # Strings are separated with an FFFF sequence.
    def extend(self, strings):
        if isinstance(strings, list):
            for s in strings:
                self.extend(s)
        elif isinstance(strings, str):
            if len(self._data) > 0:
                self._data.extend(b'\xFF\xFF')
            self._data.extend(strings.encode("utf-16"))

    # "uint8_t* buffer" parameter for the C++ function.
    def data_ptr(self):
        carray_type = ctypes.c_uint8 * len(self._data)
        return ctypes.cast(carray_type.from_buffer(self._data), _c_uint8_p)

    # "size_t size" parameter for the C++ function.
    def size(self):
        return ctypes.c_size_t(len(self._data))

## @endcond


#-----------------------------------------------------------------------------
# Utility class to encapsulate a binary buffer returning data from C++.
# The C++ function typically uses (uint8_t* buffer, size_t* size) parameters.
# The size parameter contains the maximum buffer size on input and the actual
# returned data size on output.
#-----------------------------------------------------------------------------

## @cond nodoxygen

class _OutByteBuffer:
    # Constructor with maximum buffer size in bytes.
    def __init__(self, size):
        self._size = ctypes.c_size_t(size)
        self._data = bytearray(self._size.value)

    # "uint8_t* buffer" parameter for the C++ function.
    def data_ptr(self):
        carray_type = ctypes.c_uint8 * self._size.value
        return ctypes.cast(carray_type.from_buffer(self._data), _c_uint8_p)

    # "size_t* size" parameter for the C++ function.
    def size_ptr(self):
        return ctypes.byref(self._size)

    # Get the returned data as a byte array.
    def to_bytearray(self):
        return self._data[:self._size.value]

    # Get the returned data as a UTF-16 string.
    def to_string(self):
        return self._data[:self._size.value].decode("utf-16")

## @endcond


#-----------------------------------------------------------------------------
# General-purpose transport stream constants
#-----------------------------------------------------------------------------

## MPEG TS packet size in bytes.
PKT_SIZE = 188

## MPEG TS packet size in bits.
PKT_SIZE_BITS = 8 * PKT_SIZE

## Size in bytes of a Reed-Solomon outer FEC.
RS_SIZE = 16

## Size in bytes of a TS packet with trailing Reed-Solomon outer FEC.
PKT_RS_SIZE = PKT_SIZE + RS_SIZE

## Size in bytes of a timestamp preceeding a TS packet in M2TS files (Blu-ray disc).
M2TS_HEADER_SIZE = 4

## Size in bytes of an TS packet in M2TS files (Blu-ray disc).
PKT_M2TS_SIZE = M2TS_HEADER_SIZE + PKT_SIZE


#-----------------------------------------------------------------------------
# TSDuck library general information.
#-----------------------------------------------------------------------------

##
# TSDuck version as an integer.
# @ingroup python
# @return TSDuck version as an integer, suitable for comparison between versions.
#
def intVersion():
    # uint32_t tspyVersionInteger()
    cfunc = _lib.tspyVersionInteger
    cfunc.restype = ctypes.c_uint32
    cfunc.argtypes = []
    return int(cfunc())

##
# TSDuck version as a string.
# @ingroup python
# @return TSDuck version as a string.
#
def version():
    # void tspyVersionString(uint8_t* buffer, size_t* size)
    cfunc = _lib.tspyVersionString
    cfunc.restype = None
    cfunc.argtypes = [_c_uint8_p, _c_size_p]
    buf = _OutByteBuffer(64)
    cfunc(buf.data_ptr(), buf.size_ptr())
    return buf.to_string()


#-----------------------------------------------------------------------------
# NativeObject: Base class of all Python classes with an associated C++ object
#-----------------------------------------------------------------------------

##
# This base class is derived by all TSDuck classes which are backed by a C++ object.
# @ingroup python
#
# There is an inherent problem when garbage-collected languages such as Python are interfaced
# with languages using explicit memory management. When a Python class encapsulates a native
# C++ object of the corresponding C++ class, when should the C++ object be deleted?
# This is a problem which has been discussed many times on the Internet and the answer
# is disappointing: there is no good solution.
#
#  1. A naive approach would be to override __del__(self) and perform the C++ deallocation here.
#     It is well known that __del__() creates more issues than it solves. Specifically, we cannot
#     guarantee the order of finalization of objects, which could lead to crashes when C++ objects
#     reference each other is a predetermined order.
#
#  2. Never delete C++ objects and let them accumulate. This can be acceptable if a
#     guaranteed maximum number of C++ objects are allocated during the life of the
#     application and the corresponding memory usage is acceptable.
#
#  3. Expose a public method in all Python classes which deletes, frees, deallocates, you name it,
#     the encapsulated C++ object. It is then the responsibility of the application to call
#     this method on time. This is counter-intuitive to both Python and C++ programmers but
#     this is the price to pay when you want to use them together.
#
# In the Python TSDuck bindings, all classes which encapsulate a C++ object derive from the
# base class NativeObject which provides the delete() method to explicitly delete the C++
# object. In practice, users have the choice between solutions 2 or 3.
#
class NativeObject:

    ##
    # Constructor for subclasses.
    #
    def __init__(self):
        # Storage of a pointer to the C++ object.
        # Subclasses constructors should initialize it using the result of some native function.
        self._native_object = ctypes.c_void_p(0)

    ##
    # Explicitly free the underlying C++ object.
    # After this call, the object becomes unusable.
    # Most usages are unpredictable but most likely will do nothing.
    # @return None.
    #
    def delete(self):
        # Subclasses constructors should free it using some native function
        # and then call super().delete() to cleanup the field.
        self._native_object = ctypes.c_void_p(0)


#-----------------------------------------------------------------------------
# Report: Base class for TSDuck report classes
#-----------------------------------------------------------------------------

##
# Base class for TSDuck report classes.
# @ingroup python
#
class Report(NativeObject):

    # Severity levels, same values as C++ counterparts.
    ## Fatal error, typically aborts the application.
    Fatal   = -5
    ## Severe errror.
    Severe  = -4
    ## Regular error.
    Error   = -3
    ## Warning message.
    Warning = -2
    ## Information message.
    Info    = -1
    ## Verbose information.
    Verbose =  0
    ## First debug level.
    Debug   =  1

    ##
    # Formatted line prefix header for a severity.
    # @param severity Severity value.
    # @return A string to prepend to messages. Empty for Info and Verbose levels.
    #
    @staticmethod
    def header(severity):
        # void tspyReportHeader(int severity, uint8_t* buffer, size_t* buffer_size)
        cfunc = _lib.tspyReportHeader
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_int, _c_uint8_p, _c_size_p]
        buf = _OutByteBuffer(64)
        cfunc(severity, buf.data_ptr(), buf.size_ptr())
        return buf.to_string()

    ##
    # Constructor for subclasses.
    #
    def __init__(self):
        super().__init__()

    ##
    # Set the maximum severity of the report.
    # @param severity Severity level.
    # @return None.
    #
    def setMaxSeverity(self, severity):
        # void tspySetMaxSeverity(void* report, int severity)
        cfunc = _lib.tspySetMaxSeverity
        cfunc.restype = None
        tspySetMaxSeveritycfunc.argtypes = [ctypes.c_void_p, ctypes.c_int]
        cfunc(self._native_object, severity)

    ##
    # Log a message to the report.
    # @param severity Severity level of the message.
    # @param message Message to report.
    # @return None.
    #
    def log(self, severity, message):
        # void tspyLogReport(void* report, int severity, const uint8_t* buffer, size_t size)
        cfunc = _lib.tspyLogReport
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p, ctypes.c_int, _c_uint8_p, ctypes.c_size_t]
        buf = _InByteBuffer(message)
        cfunc(self._native_object, severity, buf.data_ptr(), buf.size())

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


#-----------------------------------------------------------------------------
# NullReport: Drop report messages (singleton)
#-----------------------------------------------------------------------------

##
# A wrapper class for C++ NullReport.
# @ingroup python
#
class NullReport(Report):

    ##
    # Constructor.
    #
    def __init__(self):
        super().__init__()
        # void* tspyNullReport()
        cfunc = _lib.tspyNullReport
        cfunc.restype = ctypes.c_void_p
        cfunc.argtypes = []
        self._native_object = cfunc()
        # Not to be deleted, this is a singleton.


#-----------------------------------------------------------------------------
# StdErrReport: Send report messages to standard error (singleton)
#-----------------------------------------------------------------------------

##
# A wrapper class for C++ CerrReport.
# @ingroup python
#
class StdErrReport(Report):

    ##
    # Constructor.
    #
    def __init__(self):
        super().__init__()
        # void* tspyStdErrReport()
        cfunc = _lib.tspyStdErrReport
        cfunc.restype = ctypes.c_void_p
        cfunc.argtypes = []
        self._native_object = cfunc()
        # Not to be deleted, this is a singleton.


#-----------------------------------------------------------------------------
# AsyncReport: A wrapper class for C++ AsyncReport
#-----------------------------------------------------------------------------

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
        super().__init__()
        # void* tspyNewAsyncReport(int severity, bool sync_log, bool timed_log, size_t log_msg_count)
        cfunc = _lib.tspyNewAsyncReport
        cfunc.restype = ctypes.c_void_p
        cfunc.argtypes = [ctypes.c_int, ctypes.c_bool, ctypes.c_bool, ctypes.c_size_t]
        self._native_object = cfunc(severity, sync_log, timed_log, log_msg_count)

    # Explicitly free the underlying C++ object (inherited).
    def delete(self):
        # void tspyDeleteReport(void* report)
        cfunc = _lib.tspyDeleteReport
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)
        super().delete()

    ##
    # Synchronously terminates the async log thread.
    # @return None.
    #
    def terminate(self):
        # void tspyTerminateAsyncReport(void* report)
        cfunc = _lib.tspyTerminateAsyncReport
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)


#-----------------------------------------------------------------------------
# AbstractSyncReport: Python synchronous report class.
#-----------------------------------------------------------------------------

##
# An abstract Report class which can be derived by applications to get log messages.
# This class synchronously logs messages and is not thread-safe.
# @ingroup python
#
class AbstractSyncReport(AsyncReport):

    ##
    # Constructor.
    # @param severity Initial severity.
    #
    def __init__(self, severity = Report.Info):
        super().__init__()

        # An internal callback, called from the C++ class.
        def log_callback(sev, buf, len):
            self.log(sev, ctypes.string_at(buf, len).decode('utf-16'))

        # Keep a reference on the callback in the object instance.
        callback = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_void_p, ctypes.c_size_t)
        self._cb = callback(log_callback)

        # Finally create the native object.
        # void* tspyNewPySyncReport(ts::py::SyncReport::LogCallback log, int severity)
        cfunc = _lib.tspyNewPySyncReport
        cfunc.restype = ctypes.c_void_p
        # Don't know which type to use for ctypes.CFUNCTYPE() as first parameter.
        # cfunc.argtypes = [???, ctypes.c_int]
        self._native_object = cfunc(self._cb, severity)


#-----------------------------------------------------------------------------
# AbstractAsyncReport: Python asynchronous report class.
#-----------------------------------------------------------------------------

##
# An abstract Report class which can be derived by applications to get log messages.
# This class uses the C++ class ts::AsyncReport which asynchronously logs messages
# in a separate thread. The Python callback is invoked in a common C++ thread.
# @ingroup python
#
class AbstractAsyncReport(AsyncReport):

    ##
    # Constructor, starts the async log thread.
    # @param severity Initial severity.
    # @param sync_log Synchronous log.
    # @param log_msg_count Maximum buffered log messages.
    #
    def __init__(self, severity = Report.Info, sync_log = False, log_msg_count = 0):
        super().__init__()

        # An internal callback, called from the C++ class.
        def log_callback(sev, buf, len):
            self.log(sev, ctypes.string_at(buf, len).decode('utf-16'))

        # Keep a reference on the callback in the object instance.
        callback = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_void_p, ctypes.c_size_t)
        self._cb = callback(log_callback)

        # Finally create the native object.
        # void* tspyNewPyAsyncReport(ts::py::AsyncReport::LogCallback log, int severity, bool sync_log, size_t log_msg_count)
        cfunc = _lib.tspyNewPyAsyncReport
        cfunc.restype = ctypes.c_void_p
        # Don't know which type to use for ctypes.CFUNCTYPE() as first parameter.
        # cfunc.argtypes = [???, ctypes.c_int, ctypes.c_bool, ctypes.c_size_t]
        self._native_object = cfunc(self._cb, severity, sync_log, log_msg_count)


#-----------------------------------------------------------------------------
# DuckContext: TSDuck execution context
#-----------------------------------------------------------------------------

##
# A wrapper class for C++ DuckContext.
# @ingroup python
#
class DuckContext(NativeObject):

    # Bit masks for standards, used to qualify the signalization, same values as C++ counterparts.
    ## No known standard
    NONE  = 0x00;
    ## Defined by MPEG, common to all standards
    MPEG  = 0x01;
    ## Defined by ETSI/DVB.
    DVB   = 0x02;
    ## Defined by ANSI/SCTE.
    SCTE  = 0x04;
    ## Defined by ATSC.
    ATSC  = 0x08;
    ## Defined by ISDB.
    ISDB  = 0x10;
    ## Defined in Japan only (typically in addition to ISDB).
    JAPAN = 0x20;
    ## Defined by ABNT (Brazil, typically in addition to ISDB).
    ABNT  = 0x40;

    ##
    # Constructor.
    # @param report The tsduck.Report object to use.
    #
    def __init__(self, report):
        super().__init__()
        self._report = report
        # void* tspyNewDuckContext(void* report)
        cfunc = _lib.tspyNewDuckContext
        cfunc.restype = ctypes.c_void_p
        cfunc.argtypes = [ctypes.c_void_p]
        self._native_object = cfunc(self._report._native_object)

    # Explicitly free the underlying C++ object (inherited).
    def delete(self):
        # void tspyDeleteDuckContext(void* duck)
        cfunc = _lib.tspyDeleteDuckContext
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)
        super().delete()

    ##
    # Set the default character set for strings.
    # The default should be the DVB superset of ISO/IEC 6937 as defined in ETSI EN 300 468.
    # Use another default in the context of an operator using an incorrect signalization,
    # assuming another default character set (usually from its own country) or in the
    # context of mixed standards (ISBD/DVB for instance).
    # @param charset The new default character set name or an empty string to revert to the default.
    # @return True on success, False if @a charset is invalid.
    #
    def setDefaultCharset(self, charset):
        # bool tspyDuckContextSetDefaultCharset(void* duck_ptr, const uint8_t* name, size_t name_size)
        cfunc = _lib.tspyDuckContextSetDefaultCharset
        cfunc.restype = ctypes.c_bool
        cfunc.argtypes = [ctypes.c_void_p, _c_uint8_p, ctypes.c_size_t]
        buf = _InByteBuffer(charset)
        return bool(cfunc(self._native_object, buf.data_ptr(), buf.size()))

    ##
    # Set the default CAS id to use.
    # @param cas Default CAS id to be used when the CAS is unknown.
    # @return None.
    #
    def setDefaultCASId(self, cas):
        # void tspyDuckContextSetDefaultCASId(void* duck_ptr, uint16_t cas)
        cfunc = _lib.tspyDuckContextSetDefaultCASId
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p, ctypes.c_uint16]
        cfunc(self._native_object, cas)

    ##
    # Set the default private data specifier to use in the absence of explicit private_data_specifier_descriptor.
    # @param pds Default PDS. Use zero to revert to no default.
    # @return None.
    #
    def setDefaultPDS(self, pds):
        # void tspyDuckContextSetDefaultPDS(void* duck_ptr, uint32_t pds)
        cfunc = _lib.tspyDuckContextSetDefaultPDS
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
        cfunc(self._native_object, pds)

    ##
    # Add a list of standards which are present in the transport stream or context.
    # @param mask A bit mask of standards.
    # @return None.
    #
    def addStandards(self, mask):
        # void tspyDuckContextAddStandards(void* duck_ptr, uint32_t mask)
        cfunc = _lib.tspyDuckContextAddStandards
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
        cfunc(self._native_object, mask)

    ##
    # Get the list of standards which are present in the transport stream or context.
    # @return A bit mask of standards.
    #
    def standards(self):
        # uint32_t tspyDuckContextStandards(void* duck_ptr)
        cfunc = _lib.tspyDuckContextStandards
        cfunc.restype = ctypes.c_uint32
        cfunc.argtypes = [ctypes.c_void_p]
        return int(cfunc(self._native_object))

    ##
    # Reset the list of standards which are present in the transport stream or context.
    # @param mask A bit mask of standards.
    # @return None.
    #
    def resetStandards(self, mask = NONE):
        # void tspyDuckContextResetStandards(void* duck_ptr, uint32_t mask)
        cfunc = _lib.tspyDuckContextResetStandards
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
        cfunc(self._native_object, mask)

    ##
    # Set a non-standard time reference offset.
    # In DVB SI, reference times are UTC. These SI can be reused in non-standard ways
    # where the stored times use another reference. This is the case with ARIB and ABNT
    # variants of ISDB which reuse TOT, TDT and EIT but with another local time reference.
    # @param offset Offset from UTC in milli-seconds. Can be positive or negative.
    # The default offset is zero, meaning plain UTC time.
    # @return None.
    #
    def setTimeReferenceOffset(self, offset):
        # void tspyDuckContextSetTimeReferenceOffset(void* duck_ptr, int64_t offset)
        cfunc = _lib.tspyDuckContextSetTimeReferenceOffset
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p, ctypes.c_int64]
        cfunc(self._native_object, offset)

    ##
    # Set a non-standard time reference offset using a name.
    # @param name Time reference name. The non-standard time reference offset is computed
    # from this name which can be "JST" or "UTC[[+|-]hh[:mm]]".
    # @return True on success, False if @a name is invalid.
    #
    def setTimeReference(self, name):
        # bool tspyDuckContextSetTimeReference(void* duck_ptr, const uint8_t* name, size_t name_size)
        cfunc = _lib.tspyDuckContextSetTimeReference
        cfunc.restype = ctypes.c_bool
        cfunc.argtypes = [ctypes.c_void_p, _c_uint8_p, ctypes.c_size_t]
        buf = _InByteBuffer(name)
        return bool(cfunc(self._native_object, buf.data_ptr(), buf.size()))


#-----------------------------------------------------------------------------
# PluginEventContext: Context of a plugin event.
#-----------------------------------------------------------------------------

##
# Context of a plugin event.
# Each time a plugin signals an event for the application, a PluginEventContext
# instance is built and passed to all registered event handlers for that event.
# @ingroup python
#
class PluginEventContext:

    ##
    # Constructor.
    #
    def __init__(self):
        ## A plugin-defined 32-bit code describing the event type.
        self.event_code = 0
        ## Plugin name as found in the plugin registry.
        self.plugin_name = ""
        ## Plugin index in the chain.
        self.plugin_index = 0
        ## Total number N of plugins in the chain.
        self.plugin_count = 0
        ## Known bitrate in the context of the plugin at the time of the event.
        self.bitrate = 0
        ## Number of packets which passed through the plugin at the time of the event.
        self.plugin_packets = 0
        ## Total number of packets which passed through the plugin thread at the time of the event.
        self.total_packets = 0
        ## Indicate if the event data are read-only or if they can be updated.
        self.read_only_data = True
        ## Maximum returned data size in bytes (if they can be modified).
        self.max_data_size = 0


#-----------------------------------------------------------------------------
# AbstractPluginEventHandler: Base class for plugin event handlers
#-----------------------------------------------------------------------------

##
# An abstract class which can be derived by applications to get plugin events.
# @ingroup python
#
class AbstractPluginEventHandler(NativeObject):

    ##
    # Constructor.
    #
    def __init__(self):
        super().__init__()

        # Profile of the Python callback!
        callback = ctypes.CFUNCTYPE(ctypes.c_bool, # return type
                                    ctypes.c_uint32, ctypes.c_void_p, ctypes.c_size_t,
                                    ctypes.c_size_t, ctypes.c_size_t, ctypes.c_size_t,
                                    ctypes.c_size_t, ctypes.c_size_t,
                                    ctypes.POINTER(ctypes.c_ubyte), ctypes.c_size_t, ctypes.c_size_t,
                                    ctypes.c_bool, ctypes.c_void_p)

        #-- The internal callback with profile above, called from the C++ class.
        def event_callback(event_code, name_addr, name_size,
                           plugin_index, plugin_count, bitrate,
                           plugin_packets, total_packets,
                           data_addr, data_size, data_max_size,
                           data_read_only, event_data_obj):

            # Build a PluginEventContext from individual fields.
            context = PluginEventContext()
            context.event_code = event_code
            context.plugin_name = ctypes.string_at(name_addr, name_size).decode('utf-16')
            context.plugin_index = plugin_index
            context.plugin_count = plugin_count
            context.bitrate = bitrate
            context.plugin_packets = plugin_packets
            context.total_packets = total_packets
            context.read_only_data = bool(data_read_only)
            context.max_data_size = 0 if data_read_only else data_max_size

            # Build the input binary data of the event.
            event_data = bytes(ctypes.string_at(data_addr, data_size))

            # Call the public Python callback.
            ret = self.handlePluginEvent(context, event_data)

            # Analyze the result: bool, bytearray or tuple of both.
            success = True
            outdata = None
            if type(ret) is bool:
                success = ret
            elif type(ret) is bytearray or type(ret) is bytes:
                outdata = ret
            elif type(ret) is tuple:
                for elem in ret:
                    if type(elem) is bool:
                        success = elem
                    elif type(elem) is bytearray or type(elem) is bytes:
                        outdata = elem

            # If output data is a non-mutable bytes field, do not know how to get its address in ctypes.
            # So, convert it to a mutable bytearray first. This is very inefficient and deserves improvement.
            if type(outdata) is bytes:
                outdata = bytearray(outdata)

            # Copy back output data if there are any.
            if type(outdata) is bytearray:
                # void tspyPyPluginEventHandlerUpdateData(void* obj, void* data, size_t size)
                cfunc = _lib.tspyPyPluginEventHandlerUpdateData
                cfunc.restype = None
                cfunc.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
                carray_type = ctypes.c_uint8 * len(outdata)
                carray = ctypes.cast(carray_type.from_buffer(outdata), _c_uint8_p)
                cfunc(event_data_obj, carray, ctypes.c_size_t(len(outdata)))

            return success

        # -- back to __init__():
        # Keep a reference on the callback in the object instance.
        self._cb = callback(event_callback)

        # Finally create the native object.
        # void* tspyNewPyPluginEventHandler(ts::py::PluginEventHandler::PyCallback callback)
        cfunc = _lib.tspyNewPyPluginEventHandler
        cfunc.restype = ctypes.c_void_p
        cfunc.argtypes = [ctypes.c_void_p]
        # Don't know which type to use for ctypes.CFUNCTYPE() as first parameter.
        # cfunc.argtypes = [???]
        self._native_object = cfunc(self._cb)

    # Explicitly free the underlying C++ object (inherited).
    def delete(self):
        # void tspyDeletePyPluginEventHandler(void* obj)
        cfunc = _lib.tspyDeletePyPluginEventHandler
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)
        super().delete()

    ##
    # This handler is invoked when a plugin signals an event for which this object is registered.
    # The application should override it to collect the event.
    #
    # The associated input event data is passed in @a data. If @a context.read_only_data is
    # False, it is possible to update the data. This is typically the case with the @e memory
    # input plugin which signals events with empty input data and expects TS packets as
    # returned data. The updated data, if any, should be returned by the function as a bytearray.
    # The size of the returned data shall not exceed @a context.max_data_size. Otherwise, it will
    # be ignored.
    #
    # It is also possible to signal an error state by returning False.
    #
    # The return value of this function can consequently be a bool, a bytearray or a tuple of both.
    # The bool is True on success or False to set the error indicator of the event. The bytearray
    # is the updated output event data (if the even data is not read-only). The default is no error,
    # no data if the function returns nothing.
    #
    # Example: error, no data:
    # @code
    #   return False
    # @endcode
    #
    # Example: no error, 10-byte data:
    # @code
    #   return bytearray(b'0123456789')
    # @endcode
    #
    # Example: error but also with 10-byte data:
    # @code
    #   return False, bytearray(b'0123456789')
    # @endcode
    # or
    # @code
    #   return bytearray(b'0123456789'), False
    # @endcode
    #
    # @param context An instance of PluginEventContext containing the details of the event.
    # @param data A bytes object containing the data of the event. This is a read-only
    # sequence of bytes. There is no way to return data from Python to the plugin.
    # @return A bool, a bytearray or a tuple of both.
    #
    def handlePluginEvent(self, context, data):
        pass


#-----------------------------------------------------------------------------
# SectionFile: A container for binary sections.
#-----------------------------------------------------------------------------

##
# A wrapper class for C++ SectionFile.
# @ingroup python
#
class SectionFile(NativeObject):

    # CRC32 validation methods, used when loading binary MPEG sections, same values as C++ counterparts.
    ## Ignore the section CRC32 when loading a binary section. This is the default.
    CRC32_IGNORE = 0
    ## Check that the value of the CRC32 of the section is correct and fail if it isn't.
    CRC32_CHECK = 1
    ## Recompute a fresh new CRC32 value based on the content of the section.
    CRC32_COMPUTE = 2

    ##
    # Constructor.
    # @param duck The tsduck.DuckContext object to use.
    #
    def __init__(self, duck):
        super().__init__()
        self._duck = duck
        # void* tspyNewSectionFile(void* report)
        cfunc = _lib.tspyNewSectionFile
        cfunc.restype = ctypes.c_void_p
        cfunc.argtypes = [ctypes.c_void_p]
        self._native_object = cfunc(self._duck._native_object)

    # Explicitly free the underlying C++ object (inherited).
    def delete(self):
        # void tspyDeleteSectionFile(void* sf)
        cfunc = _lib.tspyDeleteSectionFile
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)
        super().delete()

    ##
    # Clear the content of the SectionFile, erase all sections.
    # @return None.
    #
    def clear(self):
        # void tspySectionFileClear(void* sf)
        cfunc = _lib.tspySectionFileClear
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)

    ##
    # Get the size in bytes of all sections.
    # This would be the size of the corresponding binary file.
    # @return The size in bytes of all sections.
    #
    def binarySize(self):
        # size_t tspySectionFileBinarySize(void* sf)
        cfunc = _lib.tspySectionFileBinarySize
        cfunc.restype = ctypes.c_size_t
        cfunc.argtypes = [ctypes.c_void_p]
        return int(cfunc(self._native_object))

    ##
    # Get the total number of sections in the file.
    # @return The total number of sections in the file.
    #
    def sectionsCount(self):
        # size_t tspySectionFileSectionsCount(void* sf)
        cfunc = _lib.tspySectionFileSectionsCount
        cfunc.restype = ctypes.c_size_t
        cfunc.argtypes = [ctypes.c_void_p]
        return int(cfunc(self._native_object))

    ##
    # Get the total number of full tables in the file.
    # Orphan sections are not included.
    # @return The total number of full tables in the file.
    #
    def tablesCount(self):
        # size_t tspySectionFileTablesCount(void* sf)
        cfunc = _lib.tspySectionFileTablesCount
        cfunc.restype = ctypes.c_size_t
        cfunc.argtypes = [ctypes.c_void_p]
        return int(cfunc(self._native_object))

    ##
    # Set the CRC32 processing mode when loading binary sections.
    # @param mode For binary files, how to process the CRC32 of the input sections.
    # Must be one of the CRC32_* values.
    # @return None
    #
    def setCRCValidation(self, mode):
        # void tspySectionFileSetCRCValidation(void* sf, int mode)
        cfunc = _lib.tspySectionFileSetCRCValidation
        cfunc.restype = None
        tspySectionFileSetCRCValidationcfunc.argtypes = [ctypes.c_void_p, ctypes.c_int]
        cfunc(self._native_object, mode)

    ##
    # Load a binary section file from a memory buffer.
    # The loaded sections are added to the content of this object.
    # @param data A bytearray containing the binary data to load.
    # @return True on success, False if some sections were incorrect or truncated.
    #
    def fromBinary(self, data):
        # bool tspySectionLoadBuffer(void* sf, const uint8_t* buffer, size_t size)
        cfunc = _lib.tspySectionLoadBuffer
        cfunc.restype = ctypes.c_bool
        cfunc.argtypes = [ctypes.c_void_p, _c_uint8_p, ctypes.c_size_t]
        size = ctypes.c_size_t(len(data))
        carray_type = ctypes.c_uint8 * size.value
        return bool(cfunc(self._native_object, ctypes.cast(carray_type.from_buffer(data), _c_uint8_p), size))

    ##
    # Get the binary content of a section file.
    # @return A bytearray containing the binary sections.
    #
    def toBinary(self):
        # void tspySectionSaveBuffer(void* sf, uint8_t* buffer, size_t* size)
        cfunc = _lib.tspySectionSaveBuffer
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p, _c_uint8_p, _c_size_p]
        size = ctypes.c_size_t(self.binarySize())
        data = bytearray(size.value)
        carray_type = ctypes.c_uint8 * size.value
        cfunc(self._native_object, ctypes.cast(carray_type.from_buffer(data), _c_uint8_p), ctypes.byref(size))
        return data[:size.value]

    ##
    # Load a binary section file.
    # The loaded sections are added to the content of this object.
    # @param file Binary file name.
    # If the file name is empty or "-", the standard input is used.
    # @return True on success, False on error.
    #
    def loadBinary(self, file):
        # bool tspySectionFileLoadBinary(void* sf, const uint8_t* name, size_t name_size)
        cfunc = _lib.tspySectionFileLoadBinary
        cfunc.restype = ctypes.c_bool
        cfunc.argtypes = [ctypes.c_void_p, _c_uint8_p, ctypes.c_size_t]
        name = _InByteBuffer(file)
        return bool(cfunc(self._native_object, name.data_ptr(), name.size()))

    ##
    # Save a binary section file.
    # @param [in] file Binary file name.
    # If the file name is empty or "-", the standard output is used.
    # @return True on success, False on error.
    #
    def saveBinary(self, file):
        # bool tspySectionFileSaveBinary(void* sf, const uint8_t* name, size_t name_size)
        cfunc = _lib.tspySectionFileSaveBinary
        cfunc.restype = ctypes.c_bool
        cfunc.argtypes = [ctypes.c_void_p, _c_uint8_p, ctypes.c_size_t]
        name = _InByteBuffer(file)
        return bool(cfunc(self._native_object, name.data_ptr(), name.size()))

    ##
    # Load an XML file.
    # The loaded tables are added to the content of this object.
    # @param file XML file name.
    # If the file name starts with "<?xml", this is considered as "inline XML content".
    # If the file name is empty or "-", the standard input is used.
    # @return True on success, False on error.
    #
    def loadXML(self, file):
        # bool tspySectionFileLoadXML(void* sf, const uint8_t* name, size_t name_size)
        cfunc = _lib.tspySectionFileLoadXML
        cfunc.restype = ctypes.c_bool
        cfunc.argtypes = [ctypes.c_void_p, _c_uint8_p, ctypes.c_size_t]
        name = _InByteBuffer(file)
        return bool(cfunc(self._native_object, name.data_ptr(), name.size()))

    ##
    # Save an XML file.
    # @param [in] file XML file name.
    # If the file name is empty or "-", the standard output is used.
    # @return True on success, False on error.
    #
    def saveXML(self, file):
        # bool tspySectionFileSaveXML(void* sf, const uint8_t* name, size_t name_size)
        cfunc = _lib.tspySectionFileSaveXML
        cfunc.restype = ctypes.c_bool
        cfunc.argtypes = [ctypes.c_void_p, _c_uint8_p, ctypes.c_size_t]
        name = _InByteBuffer(file)
        return bool(cfunc(self._native_object, name.data_ptr(), name.size()))

    ##
    # Save a JSON file after automated XML-to-JSON conversion.
    # @param [in] file JSON file name.
    # If the file name is empty or "-", the standard output is used.
    # @return True on success, False on error.
    #
    def saveJSON(self, file):
        # bool tspySectionFileSaveJSON(void* sf, const uint8_t* name, size_t name_size)
        cfunc = _lib.tspySectionFileSaveJSON
        cfunc.restype = ctypes.c_bool
        cfunc.argtypes = [ctypes.c_void_p, _c_uint8_p, ctypes.c_size_t]
        name = _InByteBuffer(file)
        return bool(cfunc(self._native_object, name.data_ptr(), name.size()))

    ##
    # Serialize as XML text.
    # @return Complete XML document text, empty on error.
    #
    def toXML(self):
        # size_t tspySectionFileToXML(void* sf, uint8_t* buffer, size_t* size)
        cfunc = _lib.tspySectionFileToXML
        cfunc.restype = ctypes.c_size_t
        cfunc.argtypes = [ctypes.c_void_p, _c_uint8_p, _c_size_p]
        buf = _OutByteBuffer(2048)
        len = cfunc(self._native_object, buf.data_ptr(), buf.size_ptr())
        if len > 2048:
            # First try was too short
            buf = _OutByteBuffer(len)
            len = cfunc(self._native_object, buf.data_ptr(), buf.size_ptr())
        return buf.to_string()

    ##
    # Serialize as JSON text.
    # @return Complete JSON document text, empty on error.
    #
    def toJSON(self):
        # size_t tspySectionFileToJSON(void* sf, uint8_t* buffer, size_t* size)
        cfunc = _lib.tspySectionFileToJSON
        cfunc.restype = ctypes.c_size_t
        cfunc.argtypes = [ctypes.c_void_p, _c_uint8_p, _c_size_p]
        buf = _OutByteBuffer(2048)
        len = cfunc(self._native_object, buf.data_ptr(), buf.size_ptr())
        if len > 2048:
            # First try was too short
            buf = _OutByteBuffer(len)
            len = cfunc(self._native_object, buf.data_ptr(), buf.size_ptr())
        return buf.to_string()

    ##
    # Reorganize all EIT sections according to ETSI TS 101 211.
    #
    # Only one EITp/f subtable is kept per service. It is split in two sections if two
    # events (present and following) are specified. All EIT schedule are kept. But they
    # are completely reorganized. All events are extracted and spread over new EIT
    # sections according to ETSI TS 101 211 rules.
    #
    # The "last midnight" according to which EIT segments are assigned is derived from
    # parameters @a year, @a month and @a day. If any of them is out or range, the start
    # time of the oldest event in the section file is used as "reference date".
    #
    # @param year Year of the reference time for EIT schedule.
    # This is the "last midnight" according to which EIT segments are assigned.
    # @param month Month (1..12) of the reference time for EIT schedule.
    # @param day Day (1..31) of the reference time for EIT schedule.
    # @return None.
    # @see ETSI TS 101 211, section 4.1.4
    #
    def reorganizeEITs(self, year = 0, month = 0, day = 0):
        # void tspySectionFileReorganizeEITs(void* sf, int year, int month, int day)
        cfunc = _lib.tspySectionFileReorganizeEITs
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        cfunc(self._native_object, year, month, day)


#-----------------------------------------------------------------------------
# SystemMonitor: A wrapper class for C++ SystemMonitor
#-----------------------------------------------------------------------------

##
# A wrapper class for C++ SystemMonitor.
# @ingroup python
#
class SystemMonitor(NativeObject):

    ##
    # Constructor.
    # Create the monitoring object but do not start the monitoring thread yet.
    # @param report The tsduck.Report object to use.
    # @param config The monitoring configuration file name, if different from the default.
    #
    def __init__(self, report, config = None):
        super().__init__()
        self._report = report
        # void* tspyNewSystemMonitor(void* report, const uint8_t* config, size_t config_size)
        cfunc = _lib.tspyNewSystemMonitor
        cfunc.restype = ctypes.c_void_p
        cfunc.argtypes = [ctypes.c_void_p, _c_uint8_p, ctypes.c_size_t]
        buf = _InByteBuffer(config)
        self._native_object = cfunc(self._report._native_object, buf.data_ptr(), buf.size())

    # Explicitly free the underlying C++ object (inherited).
    def delete(self):
        # void tspyDeleteReport(void* report)
        cfunc = _lib.tspyDeleteReport
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)
        super().delete()

    ##
    # Start the monitoring thread.
    # @return None.
    #
    def start(self):
        # void tspyStartSystemMonitor(void* pymon)
        cfunc = _lib.tspyStartSystemMonitor
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)

    ##
    # Stop the monitoring thread.
    # The monitoring thread is requested to stop. This method returns immediately,
    # use waitForTermination() to synchronously wait for its termination.
    # @return None.
    #
    def stop(self):
        # void tspyStopSystemMonitor(void* pymon)
        cfunc = _lib.tspyStopSystemMonitor
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)

    ##
    # Synchronously wait for the monitoring thread to stop.
    # @return None.
    #
    def waitForTermination(self):
        # void tspyWaitSystemMonitor(void* pymon)
        cfunc = _lib.tspyWaitSystemMonitor
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)


#-----------------------------------------------------------------------------
# PluginEventHandlerRegistry: Base class for plugin processors
#-----------------------------------------------------------------------------

##
# A wrapper class for C++ PluginEventHandlerRegistry.
# @ingroup python
#
class PluginEventHandlerRegistry(NativeObject):

    ##
    # Constructor.
    #
    def __init__(self):
        super().__init__()

    ##
    # Register an event handler by event code.
    # @param handler An instance of AbstractPluginEventHandler.
    # @param event_code The code of the events to handle.
    # @return None.
    #
    def registerEventHandler(self, handler, event_code):
        # void tspyPluginEventHandlerRegister(void* tsp, ts::PluginEventHandlerInterface* handler, uint32_t event_code)
        cfunc = _lib.tspyPluginEventHandlerRegister
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32]
        cfunc(self._native_object, handler._native_object, ctypes.c_uint32(event_code))

    ##
    # Register an event handler for all events from the input plugin.
    # @param handler An instance of AbstractPluginEventHandler.
    # @return None.
    #
    def registerInputEventHandler(self, handler):
        # void tspyPluginEventHandlerRegisterInput(void* tsp, ts::PluginEventHandlerInterface* handler)
        cfunc = _lib.tspyPluginEventHandlerRegisterInput
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        cfunc(self._native_object, handler._native_object)

    ##
    # Register an event handler for all events from the output plugin.
    # @param handler An instance of AbstractPluginEventHandler.
    # @return None.
    #
    def registerOutputEventHandler(self, handler):
        # void tspyPluginEventHandlerRegisterOutput(void* tsp, ts::PluginEventHandlerInterface* handler)
        cfunc = _lib.tspyPluginEventHandlerRegisterOutput
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        cfunc(self._native_object, handler._native_object)



#-----------------------------------------------------------------------------
# TSPStartError: Exception class for start error in TSProcessor
#-----------------------------------------------------------------------------

##
# A Python exception class which is thrown in case of start error in TSProcessor.
# @ingroup python
#
class TSPStartError(Exception):
    pass


#-----------------------------------------------------------------------------
# TSProcessor: Python bindings to "tsp" command
#-----------------------------------------------------------------------------

##
# A wrapper class for C++ TSProcessor.
# @ingroup python
#
class TSProcessor(PluginEventHandlerRegistry):

    ## @cond nodoxygen
    # Internal class to pass TSProcessorArgs to the native library
    class _tspyTSProcessorArgs(ctypes.Structure):         # struct tspyTSProcessorArgs {...}
        _fields_ = [
            ("ignore_joint_termination", ctypes.c_long),  # Ignore "joint termination" options in plugins (bool).
            ("buffer_size", ctypes.c_long),               # Size in bytes of the global TS packet buffer.
            ("max_flushed_packets", ctypes.c_long),       # Max processed packets before flush.
            ("max_input_packets", ctypes.c_long),         # Max packets per input operation.
            ("max_output_packets", ctypes.c_long),        # Max packets per output operation.
            ("initial_input_packets", ctypes.c_long),     # Initial number of input packets to read before starting the processing.
            ("add_input_stuffing_0", ctypes.c_long),      # Add input stuffing: add instuff_nullpkt null packets ...
            ("add_input_stuffing_1", ctypes.c_long),      # ...  every @a instuff_inpkt input packets.
            ("add_start_stuffing", ctypes.c_long),        # Add null packets before actual input.
            ("add_stop_stuffing", ctypes.c_long),         # Add null packets after end of actual input.
            ("bitrate", ctypes.c_long),                   # Fixed input bitrate (user-specified).
            ("bitrate_adjust_interval", ctypes.c_long),   # Bitrate adjust interval (in milliseconds).
            ("receive_timeout", ctypes.c_long),           # Timeout on input operations (in milliseconds).
            ("log_plugin_index", ctypes.c_long),          # Log plugin index with plugin name (bool).
            ("plugins", _c_uint8_p),                      # Address of UTF-16 multi-strings buffer for plugins.
            ("plugins_size", ctypes.c_size_t),            # Size in bytes of plugins multi-strings buffer.
        ]
    ## @endcond

    ##
    # Constructor.
    # @param report The tsduck.Report object to use.
    #
    def __init__(self, report):
        super().__init__()
        self._report = report

        # Create the native object.
        # void* tspyNewTSProcessor(void* report)
        cfunc = _lib.tspyNewTSProcessor
        cfunc.restype = ctypes.c_void_p
        cfunc.argtypes = [ctypes.c_void_p]
        self._native_object = cfunc(self._report._native_object)

        # Publicly customizable tsp options:
        ## Option -\-ignore-joint-termination.
        self.ignore_joint_termination = False
        ## Option -\-log-plugin-index.
        self.log_plugin_index = False
        ## Option -\-buffer-size-mb (in bytes here).
        self.buffer_size = 16 * 1024 * 1024
        ## Option -\-max-flushed-packets (zero means default).
        self.max_flushed_packets = 0
        ## Option -\-max-input-packets (zero means default).
        self.max_input_packets = 0
        ## Option -\-max-output-packets (zero means unlimited).
        self.max_output_packets = 0
        ## Option -\-initial-input-packets (zero means default).
        self.initial_input_packets = 0
        ## Option -\-add-input-stuffing nullpkt/inpkt (two values).
        self.add_input_stuffing = [0, 0]
        ## Option -\-add-start-stuffing.
        self.add_start_stuffing = 0
        ## Option -\-add-stop-stuffing.
        self.add_stop_stuffing = 0
        ## Option -\-bitrate.
        self.bitrate = 0
        ## Option -\-bitrate-adjust-interval (in milliseconds).
        self.bitrate_adjust_interval = 5000
        ## Option -\-receive-timeout.
        self.receive_timeout = 0
        ## Application name, for help messages.
        self.app_name = ""
        ## Input plugin name and arguments (list of strings).
        self.input = []
        ## Packet processor plugins names and arguments (list of lists of strings).
        self.plugins = []
        ## Output plugin name and arguments (list of strings).
        self.output = []

    # Explicitly free the underlying C++ object (inherited).
    def delete(self):
        # void tspyDeleteTSProcessor(void* tsp)
        cfunc = _lib.tspyDeleteTSProcessor
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)
        super().delete()

    ##
    # Start the TS processor.
    # All properties shall have been set before calling this method.
    # @return None.
    #
    def start(self):
        # Build global options.
        args = self._tspyTSProcessorArgs()
        args.ignore_joint_termination = ctypes.c_long(self.ignore_joint_termination)
        args.buffer_size = ctypes.c_long(self.buffer_size)
        args.max_flushed_packets = ctypes.c_long(self.max_flushed_packets)
        args.max_input_packets = ctypes.c_long(self.max_input_packets)
        args.max_output_packets = ctypes.c_long(self.max_output_packets)
        args.initial_input_packets = ctypes.c_long(self.initial_input_packets)
        args.add_input_stuffing_0 = ctypes.c_long(self.add_input_stuffing[0])
        args.add_input_stuffing_1 = ctypes.c_long(self.add_input_stuffing[1])
        args.add_start_stuffing = ctypes.c_long(self.add_start_stuffing)
        args.add_stop_stuffing = ctypes.c_long(self.add_stop_stuffing)
        args.bitrate = ctypes.c_long(self.bitrate)
        args.bitrate_adjust_interval = ctypes.c_long(self.bitrate_adjust_interval)
        args.receive_timeout = ctypes.c_long(self.receive_timeout)
        args.log_plugin_index = ctypes.c_long(self.log_plugin_index)

        # Build UTF-16 buffer with application names and plugins.
        plugins = _InByteBuffer(self.app_name)
        if len(self.input) > 0:
            plugins.extend('-I')
            plugins.extend(self.input)
        for pl in self.plugins:
            plugins.extend('-P')
            plugins.extend(pl)
        if len(self.output) > 0:
            plugins.extend('-O')
            plugins.extend(self.output)
        args.plugins = plugins.data_ptr()
        args.plugins_size = plugins.size()

        # Start the processing.
        # bool tspyStartTSProcessor(void* tsp, const tspyTSProcessorArgs* args)
        cfunc = _lib.tspyStartTSProcessor
        cfunc.restype = ctypes.c_bool
        cfunc.argtypes = [ctypes.c_void_p, ctypes.POINTER(self._tspyTSProcessorArgs)]
        if not cfunc(self._native_object, ctypes.byref(args)):
            raise TSPStartError("Error starting TS processor")

    ##
    # Abort the TSProcessor.
    # @return None.
    #
    def abort(self):
        # void tspyAbortTSProcessor(void* tsp)
        cfunc = _lib.tspyAbortTSProcessor
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)

    ##
    # Suspend the calling thread until TS processing is completed.
    # @return None.
    #
    def waitForTermination(self):
        # void tspyWaitTSProcessor(void* tsp)
        cfunc = _lib.tspyWaitTSProcessor
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)


#-----------------------------------------------------------------------------
# SwitchStartError: Exception class for start error in InputSwitcher
#-----------------------------------------------------------------------------

##
# A Python exception class which is thrown in case of start error in InputSwitcher.
# @ingroup python
#
class SwitchStartError(Exception):
    pass


#-----------------------------------------------------------------------------
# InputSwitcher: Python bindings to "tsswitch" command
#-----------------------------------------------------------------------------

##
# A wrapper class for C++ InputSwitcher.
# @ingroup python
#
class InputSwitcher(PluginEventHandlerRegistry):

    ## @cond nodoxygen
    # Internal class to pass TSProcessorArgs to the native library
    class _tspyInputSwitcherArgs(ctypes.Structure): # struct tspyInputSwitcherArgs {...}
        _fields_ = [
            ("fast_switch", ctypes.c_long),           # Fast switch between input plugins (bool).
            ("delayed_switch", ctypes.c_long),        # Delayed switch between input plugins (bool).
            ("terminate", ctypes.c_long),             # Terminate when one input plugin completes (bool).
            ("reuse_port", ctypes.c_long),            # Reuse-port socket option (bool).
            ("first_input", ctypes.c_long),           # Index of first input plugin.
            ("primary_input", ctypes.c_long),         # Index of primary input plugin, negative if there is none.
            ("cycle_count", ctypes.c_long),           # Number of input cycles to execute (0;
            ("buffered_packets", ctypes.c_long),      # Input buffer size in packets (0=default).
            ("max_input_packets", ctypes.c_long),     # Maximum input packets to read at a time (0=default).
            ("max_output_packets", ctypes.c_long),    # Maximum input packets to send at a time (0=default).
            ("sock_buffer", ctypes.c_long),           # Socket buffer size (0=default).
            ("remote_server_port", ctypes.c_long),    # UDP server port for remote control (0=none).
            ("receive_timeout", ctypes.c_long),       # Receive timeout before switch (0=none).
            ("plugins", _c_uint8_p),                  # Address of UTF-16 multi-strings buffer for plugins.
            ("plugins_size", ctypes.c_size_t),        # Size in bytes of plugins multi-strings buffer.
            ("event_command", _c_uint8_p),            # Address of UTF-16 multi-strings buffer for event command.
            ("event_command_size", ctypes.c_size_t),  # Size in bytes of event_command.
            ("event_udp_addr", _c_uint8_p),           # Address of UTF-16 multi-strings buffer for event UDP IP addresds.
            ("event_udp_addr_size", ctypes.c_size_t), # Size in bytes of event_udp_addr.
            ("event_udp_port", ctypes.c_long),        # Associated UDP port number.
            ("local_addr", _c_uint8_p),               # Address of UTF-16 multi-strings buffer for event UDP outgoing interface.
            ("local_addr_size", ctypes.c_size_t),     # Size in bytes of local_addr.
            ("event_ttl", ctypes.c_long),             # Time-to-live socket option for event UDP.
        ]
    ## @endcond

    ##
    # Constructor.
    # @param report The tsduck.Report object to use.
    #
    def __init__(self, report):
        super().__init__()
        self._report = report

        # Create the native object.
        # void* tspyNewInputSwitcher(void* report)
        cfunc = _lib.tspyNewInputSwitcher
        cfunc.restype = ctypes.c_void_p
        cfunc.argtypes = [ctypes.c_void_p]
        self._native_object = cfunc(self._report._native_object)

        # Publicly customizable input switcher options:
        ## Fast switch between input plugins.
        self.fast_switch = False
        ## Delayed switch between input plugins.
        self.delayed_switch = False
        ## Terminate when one input plugin completes.
        self.terminate = False
        ## Reuse-port socket option.
        self.reuse_port = False
        ## Index of first input plugin.
        self.first_input = 0
        ## Index of primary input plugin, negative if there is none.
        self.primary_input = -1
        ## Number of input cycles to execute (0 = infinite).
        self.cycle_count = 1
        ## Input buffer size in packets (0=default).
        self.buffered_packets = 0
        ## Maximum input packets to read at a time (0=default).
        self.max_input_packets = 0
        ## Maximum input packets to send at a time (0=default).
        self.max_output_packets = 0
        ## Socket buffer size (0=default).
        self.sock_buffer = 0
        ## UDP server port for remote control (0=none).
        self.remote_server_port = 0
        ## Receive timeout before switch (0=none).
        self.receive_timeout = 0
        ## External shell command to run on a switching event.
        self.event_command = ''
        ## Remote IPv4 address or host name to receive switching event JSON description.
        self.event_udp_address = ''
        ## Remote UDP port to receive switching event JSON description.
        self.event_udp_port = 0
        ## Outgoing local interface for UDP event description.
        self.event_local_address = ''
        ## Time-to-live socket option for UDP event description.
        self.event_ttl = 0

        ## Application name, for help messages.
        self.app_name = ""
        ## Input plugins name and arguments (list of lists of strings).
        self.inputs = []
        ## Output plugin name and arguments (list of strings)
        self.output = []

    # Explicitly free the underlying C++ object (inherited).
    def delete(self):
        # void tspyDeleteInputSwitcher(void* pyobj)
        cfunc = _lib.tspyDeleteInputSwitcher
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)
        super().delete()

    ##
    # Start the input switcher.
    # All properties shall have been set before calling this method.
    # @return None.
    #
    def start(self):
        # Build global options.
        args = self._tspyInputSwitcherArgs()
        args.fast_switch = ctypes.c_long(self.fast_switch)
        args.delayed_switch = ctypes.c_long(self.delayed_switch)
        args.terminate = ctypes.c_long(self.terminate)
        args.reuse_port = ctypes.c_long(self.reuse_port)
        args.first_input = ctypes.c_long(self.first_input)
        args.primary_input = ctypes.c_long(self.primary_input)
        args.cycle_count = ctypes.c_long(self.cycle_count)
        args.buffered_packets = ctypes.c_long(self.buffered_packets)
        args.max_input_packets = ctypes.c_long(self.max_input_packets)
        args.max_output_packets = ctypes.c_long(self.max_output_packets)
        args.sock_buffer = ctypes.c_long(self.sock_buffer)
        args.remote_server_port = ctypes.c_long(self.remote_server_port)
        args.receive_timeout = ctypes.c_long(self.receive_timeout)
        evcmd_buf = _InByteBuffer(self.event_command)
        args.event_command = evcmd_buf.data_ptr()
        args.event_command_size = evcmd_buf.size()
        evudp_buf = _InByteBuffer(self.event_udp_address)
        args.event_udp_addr = evudp_buf.data_ptr()
        args.event_udp_addr_size = evudp_buf.size()
        args.event_udp_port = ctypes.c_long(self.event_udp_port)
        evloc_buf = _InByteBuffer(self.event_local_address)
        args.local_addr = evloc_buf.data_ptr()
        args.local_addr_size = evloc_buf.size()
        args.event_ttl = ctypes.c_long(self.event_ttl)

        # Build UTF-16 buffer with application names and plugins.
        plugins = _InByteBuffer(self.app_name)
        for pl in self.inputs:
            plugins.extend('-I')
            plugins.extend(pl)
        if len(self.output) > 0:
            plugins.extend('-O')
            plugins.extend(self.output)
        args.plugins = plugins.data_ptr()
        args.plugins_size = plugins.size()

        # Start the processing.
        # bool tspyStartInputSwitcher(void* pyobj, const tspyInputSwitcherArgs* pyargs)
        cfunc = _lib.tspyStartInputSwitcher
        cfunc.restype = ctypes.c_bool
        cfunc.argtypes = [ctypes.c_void_p, ctypes.POINTER(self._tspyInputSwitcherArgs)]
        if not cfunc(self._native_object, ctypes.byref(args)):
            raise SwitchStartError("Error starting input switcher")

    ##
    # Switch to another input plugin.
    # @param plugin_index Index of the new input plugin.
    # @return None.
    #
    def setInput(self, plugin_index):
        # void tspyInputSwitcherSetInput(void* pyobj, size_t index)
        cfunc = _lib.tspyInputSwitcherSetInput
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        cfunc(self._native_object, plugin_index)

    ##
    # Switch to the next input plugin.
    # @return None.
    #
    def nextInput(self):
        # void tspyInputSwitcherNextInput(void* tsp)
        cfunc = _lib.tspyInputSwitcherNextInput
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)

    ##
    # Switch to the previous input plugin.
    # @return None.
    #
    def previousInput(self):
        # void tspyInputSwitcherPreviousInput(void* tsp)
        cfunc = _lib.tspyInputSwitcherPreviousInput
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)

    ##
    # Get the index of the current input plugin.
    # @return The index of the current input plugin.
    #
    def currentInput(self):
        # size_t tspyInputSwitcherCurrentInput(void* pyobj)
        cfunc = _lib.tspyInputSwitcherCurrentInput
        cfunc.restype = ctypes.c_size_t
        cfunc.argtypes = [ctypes.c_void_p]
        return int(cfunc(self._native_object))

    ##
    # Terminate the processing.
    # @return None.
    #
    def stop(self):
        # void tspyStopInputSwitcher(void* pyobj)
        cfunc = _lib.tspyStopInputSwitcher
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)

    ##
    # Suspend the calling thread until input switcher session is completed.
    # @return None.
    #
    def waitForTermination(self):
        # void tspyWaitInputSwitcher(void* tsp)
        cfunc = _lib.tspyWaitInputSwitcher
        cfunc.restype = None
        cfunc.argtypes = [ctypes.c_void_p]
        cfunc(self._native_object)


#-----------------------------------------------------------------------------
# VersionMismatch: Exception class for TSDuck version mismatch
#-----------------------------------------------------------------------------

##
# A Python exception class which is thrown in case of TSDuck version mismatch.
# There is a incompatibility between the TSDuck Python module and the TSDuck
# binary library, probably due to an installation error.
# @ingroup python
#
class VersionMismatch(Exception):
    pass


#-----------------------------------------------------------------------------
# Module initialization
#-----------------------------------------------------------------------------

## @cond nodoxygen

__author__ = 'Thierry Lelegard'
__copyright__ = '2005-2022, Thierry Lelegard'
__version__ = version()

# Minimum required version for TSDuck library. Must be incremented when
# binary incompatibilities are introduced between tsduck.py and the library.

__min_version__ = 32702383

if intVersion() < __min_version__:
    raise VersionMismatch("TSDuck version mismatch, requires %d.%d-%d, this one is %s" % (__min_version__ // 10000000, (__min_version__ // 100000) % 100, __min_version__ % 100000, __version__))

## @endcond
