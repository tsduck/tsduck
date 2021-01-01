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
#  Internal interface to the C++ TSDuck library.
#
#-----------------------------------------------------------------------------

import os
import ctypes.util
from ctypes import *


#-----------------------------------------------------------------------------
# Load the TSDuck library
#-----------------------------------------------------------------------------

def search_libtsduck():
    if os.name == 'nt':
        base = 'tsduck.dll'
        search = [os.getenv('TSDUCK','')]
        search.extend(os.getenv('Path','').split(os.pathsep))
    else:
        base = 'libtsduck.so'
        # For macOS only: LD_LIBRARY_PATH is not passed to shell-scripts for security reasons.
        # A backup version is defined in build/setenv.sh to test development versions.
        search = os.getenv('LD_LIBRARY_PATH2','').split(os.pathsep)
        search.extend(os.getenv('LD_LIBRARY_PATH','').split(os.pathsep))

    # Search the TSDuck library.
    path = ''
    for dir in (dir for dir in search if dir != ''):
        file = dir + os.sep + base
        if os.path.exists(file):
            return file

    # If not found in various explicit paths, try system search.
    return ctypes.util.find_library(base)

# Load the TSDuck library.
_lib = CDLL(search_libtsduck())


#-----------------------------------------------------------------------------
# Utility class to pass a string to C++.
#-----------------------------------------------------------------------------

# The C++ function typically uses (const uint8_t* buffer, size_t size) parameters.

class InByteBuffer:
    # Constructor with string value.
    def __init__(self, string):
        self._data = bytearray(string.encode("utf-16"))

    # Append with the content of a string or list of strings.
    # Strings are separated with an FFFF sequence.
    def extend(self, strings):
        if isinstance(strings, list):
            for s in strings:
                self.extend(s)
        else:
            if len(self._data) > 0:
                self._data.extend(b'\xFF\xFF')
            self._data.extend(strings.encode("utf-16"))
        
    # "uint8_t* buffer" parameter for the C++ function.
    def data_ptr(self):
        carray_type = c_uint8 * len(self._data)
        return cast(carray_type.from_buffer(self._data), POINTER(c_uint8))

    # "size_t size" parameter for the C++ function.
    def size(self):
        return c_size_t(len(self._data))


#-----------------------------------------------------------------------------
# Utility class to encapsulate a binary buffer returning data from C++.
#-----------------------------------------------------------------------------

# The C++ function typically uses (uint8_t* buffer, size_t* size) parameters.
# The size parameter contains the maximum buffer size on input and the actual
# returned data size on output.

class OutByteBuffer:
    # Constructor with maximum buffer size in bytes.
    def __init__(self, size):
        self._size = c_size_t(size)
        self._data = bytearray(self._size.value)

    # "uint8_t* buffer" parameter for the C++ function.
    def data_ptr(self):
        carray_type = c_uint8 * self._size.value
        return cast(carray_type.from_buffer(self._data), POINTER(c_uint8))

    # "size_t* size" parameter for the C++ function.
    def size_ptr(self):
        return byref(self._size)

    # Get the returned data as a byte array.
    def to_bytearray(self):
        return self._data[:self._size.value]

    # Get the returned data as a UTF-16 string.
    def to_string(self):
        return self._data[:self._size.value].decode("utf-16")


#-----------------------------------------------------------------------------
# Bindings to C++ functions from the TSDuck library
#-----------------------------------------------------------------------------

# struct tspyTSProcessorArgs {...};
class tspyTSProcessorArgs(ctypes.Structure):
    _fields_ = [
        ("monitor", ctypes.c_long),                   # Run a resource monitoring thread (bool).
        ("ignore_joint_termination", ctypes.c_long),  # Ignore "joint termination" options in plugins (bool).
        ("buffer_size", ctypes.c_long),               # Size in bytes of the global TS packet buffer.
        ("max_flushed_packets", ctypes.c_long),       # Max processed packets before flush.
        ("max_input_packets", ctypes.c_long),         # Max packets per input operation.
        ("initial_input_packets", ctypes.c_long),     # Initial number of input packets to read before starting the processing.
        ("add_input_stuffing_0", ctypes.c_long),      # Add input stuffing: add instuff_nullpkt null packets ...
        ("add_input_stuffing_1", ctypes.c_long),      # ...  every @a instuff_inpkt input packets.
        ("add_start_stuffing", ctypes.c_long),        # Add null packets before actual input.
        ("add_stop_stuffing", ctypes.c_long),         # Add null packets after end of actual input.
        ("bitrate", ctypes.c_long),                   # Fixed input bitrate (user-specified).
        ("bitrate_adjust_interval", ctypes.c_long),   # Bitrate adjust interval (in milliseconds).
        ("receive_timeout", ctypes.c_long),           # Timeout on input operations (in milliseconds).
        ("log_plugin_index", ctypes.c_long),          # Log plugin index with plugin name (bool).
    ]

# void tspyAbortTSProcessor(void* tsp);

tspyAbortTSProcessor = _lib.tspyAbortTSProcessor
tspyAbortTSProcessor.restype = None
tspyAbortTSProcessor.argtypes = [c_void_p]

# void tspyDeleteReport(void* report);

tspyDeleteReport = _lib.tspyDeleteReport
tspyDeleteReport.restype = None
tspyDeleteReport.argtypes = [c_void_p]

# void tspyDeleteTSProcessor(void* tsp);

tspyDeleteTSProcessor = _lib.tspyDeleteTSProcessor
tspyDeleteTSProcessor.restype = None
tspyDeleteTSProcessor.argtypes = [c_void_p]

# void tspyLogReport(void* report, int severity, const uint8_t* buffer, size_t size);

tspyLogReport = _lib.tspyLogReport
tspyLogReport.restype = None
tspyLogReport.argtypes = [c_void_p, c_int, POINTER(c_uint8), c_size_t]

# void* tspyNewAsyncReport(int severity, bool sync_log, bool timed_log, size_t log_msg_count);

tspyNewAsyncReport = _lib.tspyNewAsyncReport
tspyNewAsyncReport.restype = c_void_p
tspyNewAsyncReport.argtypes = [c_int, c_bool, c_bool, c_size_t]

# void* tspyNewTSProcessor(void* report);

tspyNewTSProcessor = _lib.tspyNewTSProcessor
tspyNewTSProcessor.restype = c_void_p
tspyNewTSProcessor.argtypes = [c_void_p]

# void* tspyNullReport();

tspyNullReport = _lib.tspyNullReport
tspyNullReport.restype = c_void_p
tspyNullReport.argtypes = []

# void tspySetMaxSeverity(void* report, int severity);

tspySetMaxSeverity = _lib.tspySetMaxSeverity
tspySetMaxSeverity.restype = None
tspySetMaxSeverity.argtypes = [c_void_p, c_int]

# bool tspyStartTSProcessor(void* tsp, const tspyTSProcessorArgs* args, const uint8_t* plugins, size_t plugins_size);

tspyStartTSProcessor = _lib.tspyStartTSProcessor
tspyStartTSProcessor.restype = c_bool
tspyStartTSProcessor.argtypes = [c_void_p, POINTER(tspyTSProcessorArgs), POINTER(c_uint8), c_size_t]

# void* tspyStdErrReport();

tspyStdErrReport = _lib.tspyStdErrReport
tspyStdErrReport.restype = c_void_p
tspyStdErrReport.argtypes = []

# void tspyTerminateAsyncReport(void* report);

tspyTerminateAsyncReport = _lib.tspyTerminateAsyncReport
tspyTerminateAsyncReport.restype = None
tspyTerminateAsyncReport.argtypes = [c_void_p]

# uint32_t tspyVersionInteger();

tspyVersionInteger = _lib.tspyVersionInteger
tspyVersionInteger.restype = c_uint32
tspyVersionInteger.argtypes = []

# void tspyVersionString(uint8_t* buffer, size_t* size);

tspyVersionString = _lib.tspyVersionString
tspyVersionString.restype = None
tspyVersionString.argtypes = [POINTER(c_uint8), POINTER(c_size_t)]

# void tspyWaitTSProcessor(void* tsp);

tspyWaitTSProcessor = _lib.tspyWaitTSProcessor
tspyWaitTSProcessor.restype = None
tspyWaitTSProcessor.argtypes = [c_void_p]
