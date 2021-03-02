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

## @cond doxygen
# Internal module, not documented.

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
    return ctypes.util.find_library('tsduck')

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
# Bindings to C++ functions from tspyDuckContext.cpp
#-----------------------------------------------------------------------------

# void* tspyNewDuckContext(void* report)

tspyNewDuckContext = _lib.tspyNewDuckContext
tspyNewDuckContext.restype = c_void_p
tspyNewDuckContext.argtypes = [c_void_p]

# void tspyDeleteDuckContext(void* duck)

tspyDeleteDuckContext = _lib.tspyDeleteDuckContext
tspyDeleteDuckContext.restype = None
tspyDeleteDuckContext.argtypes = [c_void_p]

# bool tspyDuckContextSetDefaultCharset(void* duck_ptr, const uint8_t* name, size_t name_size)

tspyDuckContextSetDefaultCharset = _lib.tspyDuckContextSetDefaultCharset
tspyDuckContextSetDefaultCharset.restype = c_bool
tspyDuckContextSetDefaultCharset.argtypes = [c_void_p, POINTER(c_uint8), c_size_t]

# void tspyDuckContextSetDefaultCASId(void* duck_ptr, uint16_t cas)

tspyDuckContextSetDefaultCASId = _lib.tspyDuckContextSetDefaultCASId
tspyDuckContextSetDefaultCASId.restype = None
tspyDuckContextSetDefaultCASId.argtypes = [c_void_p, c_uint16]

# void tspyDuckContextSetDefaultPDS(void* duck_ptr, uint32_t pds)

tspyDuckContextSetDefaultPDS = _lib.tspyDuckContextSetDefaultPDS
tspyDuckContextSetDefaultPDS.restype = None
tspyDuckContextSetDefaultPDS.argtypes = [c_void_p, c_uint32]

# void tspyDuckContextAddStandards(void* duck_ptr, uint32_t mask)

tspyDuckContextAddStandards = _lib.tspyDuckContextAddStandards
tspyDuckContextAddStandards.restype = None
tspyDuckContextAddStandards.argtypes = [c_void_p, c_uint32]

# void tspyDuckContextResetStandards(void* duck_ptr, uint32_t mask)

tspyDuckContextResetStandards = _lib.tspyDuckContextResetStandards
tspyDuckContextResetStandards.restype = None
tspyDuckContextResetStandards.argtypes = [c_void_p, c_uint32]

# uint32_t tspyDuckContextStandards(void* duck_ptr)

tspyDuckContextStandards = _lib.tspyDuckContextStandards
tspyDuckContextStandards.restype = c_uint32
tspyDuckContextStandards.argtypes = [c_void_p]

# void tspyDuckContextSetTimeReferenceOffset(void* duck_ptr, int64_t offset)

tspyDuckContextSetTimeReferenceOffset = _lib.tspyDuckContextSetTimeReferenceOffset
tspyDuckContextSetTimeReferenceOffset.restype = None
tspyDuckContextSetTimeReferenceOffset.argtypes = [c_void_p, c_int64]

# bool tspyDuckContextSetTimeReference(void* duck_ptr, const uint8_t* name, size_t name_size)

tspyDuckContextSetTimeReference = _lib.tspyDuckContextSetTimeReference
tspyDuckContextSetTimeReference.restype = c_bool
tspyDuckContextSetTimeReference.argtypes = [c_void_p, POINTER(c_uint8), c_size_t]

#-----------------------------------------------------------------------------
# Bindings to C++ functions from tspyPluginEventHandler.cpp
#-----------------------------------------------------------------------------

# void* tspyNewPyPluginEventHandler(ts::py::PluginEventHandler::PyCallback callback)

tspyNewPyPluginEventHandler = _lib.tspyNewPyPluginEventHandler
tspyNewPyPluginEventHandler.restype = c_void_p
tspyNewDuckContext.argtypes = [c_void_p]
# Don't know which type to use for ctypes.CFUNCTYPE() as first parameter.
# tspyNewPyPluginEventHandler.argtypes = [???]

# void tspyDeletePyPluginEventHandler(void* obj)

tspyDeletePyPluginEventHandler = _lib.tspyDeletePyPluginEventHandler
tspyDeletePyPluginEventHandler.restype = None
tspyDeletePyPluginEventHandler.argtypes = [c_void_p]

# void tspyPyPluginEventHandlerUpdateData(void* obj, void* data, size_t size)

tspyPyPluginEventHandlerUpdateData = _lib.tspyPyPluginEventHandlerUpdateData
tspyPyPluginEventHandlerUpdateData.restype = None
tspyPyPluginEventHandlerUpdateData.argtypes = [c_void_p, c_void_p, c_size_t]

#-----------------------------------------------------------------------------
# Bindings to C++ functions from tspyInfo.cpp
#-----------------------------------------------------------------------------

# uint32_t tspyVersionInteger()

tspyVersionInteger = _lib.tspyVersionInteger
tspyVersionInteger.restype = c_uint32
tspyVersionInteger.argtypes = []

# void tspyVersionString(uint8_t* buffer, size_t* size)

tspyVersionString = _lib.tspyVersionString
tspyVersionString.restype = None
tspyVersionString.argtypes = [POINTER(c_uint8), POINTER(c_size_t)]

#-----------------------------------------------------------------------------
# Bindings to C++ functions from tspyInputSwitcher.cpp
#-----------------------------------------------------------------------------

# struct tspyInputSwitcherArgs {...}
class tspyInputSwitcherArgs(ctypes.Structure):
    _fields_ = [
        ("fast_switch", c_long),         # Fast switch between input plugins (bool).
        ("delayed_switch", c_long),      # Delayed switch between input plugins (bool).
        ("terminate", c_long),           # Terminate when one input plugin completes (bool).
        ("monitor", c_long),             # Run a resource monitoring thread (bool).
        ("reuse_port", c_long),          # Reuse-port socket option (bool).
        ("first_input", c_long),         # Index of first input plugin.
        ("primary_input", c_long),       # Index of primary input plugin, negative if there is none.
        ("cycle_count", c_long),         # Number of input cycles to execute (0;
        ("buffered_packets", c_long),    # Input buffer size in packets (0=default).
        ("max_input_packets", c_long),   # Maximum input packets to read at a time (0=default).
        ("max_output_packets", c_long),  # Maximum input packets to send at a time (0=default).
        ("sock_buffer", c_long),         # Socket buffer size (0=default).
        ("remote_server_port", c_long),  # UDP server port for remote control (0=none).
        ("receive_timeout", c_long),     # Receive timeout before switch (0=none).
    ]

# void* tspyNewInputSwitcher(void* report)

tspyNewInputSwitcher = _lib.tspyNewInputSwitcher
tspyNewInputSwitcher.restype = c_void_p
tspyNewInputSwitcher.argtypes = [c_void_p]

# void tspyDeleteInputSwitcher(void* pyobj)

tspyDeleteInputSwitcher = _lib.tspyDeleteInputSwitcher
tspyDeleteInputSwitcher.restype = None
tspyDeleteInputSwitcher.argtypes = [c_void_p]

# void tspyStopInputSwitcher(void* pyobj)

tspyStopInputSwitcher = _lib.tspyStopInputSwitcher
tspyStopInputSwitcher.restype = None
tspyStopInputSwitcher.argtypes = [c_void_p]

# void tspyWaitInputSwitcher(void* tsp)

tspyWaitInputSwitcher = _lib.tspyWaitInputSwitcher
tspyWaitInputSwitcher.restype = None
tspyWaitInputSwitcher.argtypes = [c_void_p]

# void tspyInputSwitcherSetInput(void* pyobj, size_t index)

tspyInputSwitcherSetInput = _lib.tspyInputSwitcherSetInput
tspyInputSwitcherSetInput.restype = None
tspyInputSwitcherSetInput.argtypes = [c_void_p, c_size_t]

# void tspyInputSwitcherNextInput(void* tsp)

tspyInputSwitcherNextInput = _lib.tspyInputSwitcherNextInput
tspyInputSwitcherNextInput.restype = None
tspyInputSwitcherNextInput.argtypes = [c_void_p]

# void tspyInputSwitcherPreviousInput(void* tsp)

tspyInputSwitcherPreviousInput = _lib.tspyInputSwitcherPreviousInput
tspyInputSwitcherPreviousInput.restype = None
tspyInputSwitcherPreviousInput.argtypes = [c_void_p]

# size_t tspyInputSwitcherCurrentInput(void* pyobj)

tspyInputSwitcherCurrentInput = _lib.tspyInputSwitcherCurrentInput
tspyInputSwitcherCurrentInput.restype = c_size_t
tspyInputSwitcherCurrentInput.argtypes = [c_void_p]

# bool tspyStartInputSwitcher(void* pyobj, const tspyInputSwitcherArgs* pyargs, const uint8_t* plugins, size_t plugins_size)

tspyStartInputSwitcher = _lib.tspyStartInputSwitcher
tspyStartInputSwitcher.restype = c_bool
tspyStartInputSwitcher.argtypes = [c_void_p, POINTER(tspyInputSwitcherArgs), POINTER(c_uint8), c_size_t]

#-----------------------------------------------------------------------------
# Bindings to C++ functions from tspyReport.cpp 
#-----------------------------------------------------------------------------

# void tspyReportHeader(int severity, uint8_t* buffer, size_t* buffer_size)

tspyReportHeader = _lib.tspyReportHeader
tspyReportHeader.restype = None
tspyReportHeader.argtypes = [c_int, POINTER(c_uint8), POINTER(c_size_t)]

# void* tspyStdErrReport()

tspyStdErrReport = _lib.tspyStdErrReport
tspyStdErrReport.restype = c_void_p
tspyStdErrReport.argtypes = []

# void* tspyNullReport()

tspyNullReport = _lib.tspyNullReport
tspyNullReport.restype = c_void_p
tspyNullReport.argtypes = []

# void* tspyNewAsyncReport(int severity, bool sync_log, bool timed_log, size_t log_msg_count)

tspyNewAsyncReport = _lib.tspyNewAsyncReport
tspyNewAsyncReport.restype = c_void_p
tspyNewAsyncReport.argtypes = [c_int, c_bool, c_bool, c_size_t]

# void* tspyNewPyAsyncReport(ts::py::AsyncReport::LogCallback log, int severity, bool sync_log, size_t log_msg_count)

tspyNewPyAsyncReport = _lib.tspyNewPyAsyncReport
tspyNewPyAsyncReport.restype = c_void_p
# Don't know which type to use for ctypes.CFUNCTYPE() as first parameter.
# tspyNewPyAsyncReport.argtypes = [???, c_int, c_bool, c_size_t]

# void* tspyNewPySyncReport(ts::py::SyncReport::LogCallback log, int severity)

tspyNewPySyncReport = _lib.tspyNewPySyncReport
tspyNewPySyncReport.restype = c_void_p
# Don't know which type to use for ctypes.CFUNCTYPE() as first parameter.
# tspyNewPySyncReport.argtypes = [???, c_int, c_bool, c_size_t]

# void tspyTerminateAsyncReport(void* report)

tspyTerminateAsyncReport = _lib.tspyTerminateAsyncReport
tspyTerminateAsyncReport.restype = None
tspyTerminateAsyncReport.argtypes = [c_void_p]

# void tspyDeleteReport(void* report)

tspyDeleteReport = _lib.tspyDeleteReport
tspyDeleteReport.restype = None
tspyDeleteReport.argtypes = [c_void_p]

# void tspySetMaxSeverity(void* report, int severity)

tspySetMaxSeverity = _lib.tspySetMaxSeverity
tspySetMaxSeverity.restype = None
tspySetMaxSeverity.argtypes = [c_void_p, c_int]

# void tspyLogReport(void* report, int severity, const uint8_t* buffer, size_t size)

tspyLogReport = _lib.tspyLogReport
tspyLogReport.restype = None
tspyLogReport.argtypes = [c_void_p, c_int, POINTER(c_uint8), c_size_t]

#-----------------------------------------------------------------------------
# Bindings to C++ functions from tspySectionFile.cpp
#-----------------------------------------------------------------------------

# void* tspyNewSectionFile(void* report)

tspyNewSectionFile = _lib.tspyNewSectionFile
tspyNewSectionFile.restype = c_void_p
tspyNewSectionFile.argtypes = [c_void_p]

# void tspyDeleteSectionFile(void* sf)

tspyDeleteSectionFile = _lib.tspyDeleteSectionFile
tspyDeleteSectionFile.restype = None
tspyDeleteSectionFile.argtypes = [c_void_p]

# void tspySectionFileClear(void* sf)

tspySectionFileClear = _lib.tspySectionFileClear
tspySectionFileClear.restype = None
tspySectionFileClear.argtypes = [c_void_p]

# size_t tspySectionFileBinarySize(void* sf)

tspySectionFileBinarySize = _lib.tspySectionFileBinarySize
tspySectionFileBinarySize.restype = c_size_t
tspySectionFileBinarySize.argtypes = [c_void_p]

# size_t tspySectionFileSectionsCount(void* sf)

tspySectionFileSectionsCount = _lib.tspySectionFileSectionsCount
tspySectionFileSectionsCount.restype = c_size_t
tspySectionFileSectionsCount.argtypes = [c_void_p]

# size_t tspySectionFileTablesCount(void* sf)

tspySectionFileTablesCount = _lib.tspySectionFileTablesCount
tspySectionFileTablesCount.restype = c_size_t
tspySectionFileTablesCount.argtypes = [c_void_p]

# bool tspySectionFileLoadBinary(void* sf, const uint8_t* name, size_t name_size)

tspySectionFileLoadBinary = _lib.tspySectionFileLoadBinary
tspySectionFileLoadBinary.restype = c_bool
tspySectionFileLoadBinary.argtypes = [c_void_p, POINTER(c_uint8), c_size_t]

# bool tspySectionFileSaveBinary(void* sf, const uint8_t* name, size_t name_size)

tspySectionFileSaveBinary = _lib.tspySectionFileSaveBinary
tspySectionFileSaveBinary.restype = c_bool
tspySectionFileSaveBinary.argtypes = [c_void_p, POINTER(c_uint8), c_size_t]

# bool tspySectionFileLoadXML(void* sf, const uint8_t* name, size_t name_size)

tspySectionFileLoadXML = _lib.tspySectionFileLoadXML
tspySectionFileLoadXML.restype = c_bool
tspySectionFileLoadXML.argtypes = [c_void_p, POINTER(c_uint8), c_size_t]

# bool tspySectionFileSaveXML(void* sf, const uint8_t* name, size_t name_size)

tspySectionFileSaveXML = _lib.tspySectionFileSaveXML
tspySectionFileSaveXML.restype = c_bool
tspySectionFileSaveXML.argtypes = [c_void_p, POINTER(c_uint8), c_size_t]

# bool tspySectionFileSaveJSON(void* sf, const uint8_t* name, size_t name_size)

tspySectionFileSaveJSON = _lib.tspySectionFileSaveJSON
tspySectionFileSaveJSON.restype = c_bool
tspySectionFileSaveJSON.argtypes = [c_void_p, POINTER(c_uint8), c_size_t]

# size_t tspySectionFileToXML(void* sf, uint8_t* buffer, size_t* size)

tspySectionFileToXML = _lib.tspySectionFileToXML
tspySectionFileToXML.restype = c_size_t
tspySectionFileToXML.argtypes = [c_void_p, POINTER(c_uint8), POINTER(c_size_t)]

# size_t tspySectionFileToJSON(void* sf, uint8_t* buffer, size_t* size)

tspySectionFileToJSON = _lib.tspySectionFileToJSON
tspySectionFileToJSON.restype = c_size_t
tspySectionFileToJSON.argtypes = [c_void_p, POINTER(c_uint8), POINTER(c_size_t)]

# bool tspySectionLoadBuffer(void* sf, const uint8_t* buffer, size_t size)

tspySectionLoadBuffer = _lib.tspySectionLoadBuffer
tspySectionLoadBuffer.restype = c_bool
tspySectionLoadBuffer.argtypes = [c_void_p, POINTER(c_uint8), c_size_t]

# void tspySectionSaveBuffer(void* sf, uint8_t* buffer, size_t* size)

tspySectionSaveBuffer = _lib.tspySectionSaveBuffer
tspySectionSaveBuffer.restype = None
tspySectionSaveBuffer.argtypes = [c_void_p, POINTER(c_uint8), POINTER(c_size_t)]

# void tspySectionFileSetCRCValidation(void* sf, int mode)

tspySectionFileSetCRCValidation = _lib.tspySectionFileSetCRCValidation
tspySectionFileSetCRCValidation.restype = None
tspySectionFileSetCRCValidation.argtypes = [c_void_p, c_int]

# void tspySectionFileReorganizeEITs(void* sf, int year, int month, int day)

tspySectionFileReorganizeEITs = _lib.tspySectionFileReorganizeEITs
tspySectionFileReorganizeEITs.restype = None
tspySectionFileReorganizeEITs.argtypes = [c_void_p, c_int, c_int, c_int]

#-----------------------------------------------------------------------------
# Bindings to C++ functions from tspyTSProcessor.cpp
#-----------------------------------------------------------------------------

# struct tspyTSProcessorArgs {...}
class tspyTSProcessorArgs(ctypes.Structure):
    _fields_ = [
        ("monitor", c_long),                   # Run a resource monitoring thread (bool).
        ("ignore_joint_termination", c_long),  # Ignore "joint termination" options in plugins (bool).
        ("buffer_size", c_long),               # Size in bytes of the global TS packet buffer.
        ("max_flushed_packets", c_long),       # Max processed packets before flush.
        ("max_input_packets", c_long),         # Max packets per input operation.
        ("initial_input_packets", c_long),     # Initial number of input packets to read before starting the processing.
        ("add_input_stuffing_0", c_long),      # Add input stuffing: add instuff_nullpkt null packets ...
        ("add_input_stuffing_1", c_long),      # ...  every @a instuff_inpkt input packets.
        ("add_start_stuffing", c_long),        # Add null packets before actual input.
        ("add_stop_stuffing", c_long),         # Add null packets after end of actual input.
        ("bitrate", c_long),                   # Fixed input bitrate (user-specified).
        ("bitrate_adjust_interval", c_long),   # Bitrate adjust interval (in milliseconds).
        ("receive_timeout", c_long),           # Timeout on input operations (in milliseconds).
        ("log_plugin_index", c_long),          # Log plugin index with plugin name (bool).
    ]

# void* tspyNewTSProcessor(void* report)

tspyNewTSProcessor = _lib.tspyNewTSProcessor
tspyNewTSProcessor.restype = c_void_p
tspyNewTSProcessor.argtypes = [c_void_p]

# void tspyDeleteTSProcessor(void* tsp)

tspyDeleteTSProcessor = _lib.tspyDeleteTSProcessor
tspyDeleteTSProcessor.restype = None
tspyDeleteTSProcessor.argtypes = [c_void_p]

# void tspyAbortTSProcessor(void* tsp)

tspyAbortTSProcessor = _lib.tspyAbortTSProcessor
tspyAbortTSProcessor.restype = None
tspyAbortTSProcessor.argtypes = [c_void_p]

# void tspyWaitTSProcessor(void* tsp)

tspyWaitTSProcessor = _lib.tspyWaitTSProcessor
tspyWaitTSProcessor.restype = None
tspyWaitTSProcessor.argtypes = [c_void_p]

# void tspyTSProcessorRegisterEventHandler(void* tsp, ts::PluginEventHandlerInterface* handler, uint32_t event_code)

tspyTSProcessorRegisterEventHandler = _lib.tspyTSProcessorRegisterEventHandler
tspyTSProcessorRegisterEventHandler.restype = None
tspyTSProcessorRegisterEventHandler.argtypes = [c_void_p, c_void_p, c_uint32]

# void tspyTSProcessorRegisterInputEventHandler(void* tsp, ts::PluginEventHandlerInterface* handler)

tspyTSProcessorRegisterInputEventHandler = _lib.tspyTSProcessorRegisterInputEventHandler
tspyTSProcessorRegisterInputEventHandler.restype = None
tspyTSProcessorRegisterInputEventHandler.argtypes = [c_void_p, c_void_p]

# void tspyTSProcessorRegisterOutputEventHandler(void* tsp, ts::PluginEventHandlerInterface* handler)

tspyTSProcessorRegisterOutputEventHandler = _lib.tspyTSProcessorRegisterOutputEventHandler
tspyTSProcessorRegisterOutputEventHandler.restype = None
tspyTSProcessorRegisterOutputEventHandler.argtypes = [c_void_p, c_void_p]

# bool tspyStartTSProcessor(void* tsp, const tspyTSProcessorArgs* args, const uint8_t* plugins, size_t plugins_size)

tspyStartTSProcessor = _lib.tspyStartTSProcessor
tspyStartTSProcessor.restype = c_bool
tspyStartTSProcessor.argtypes = [c_void_p, POINTER(tspyTSProcessorArgs), POINTER(c_uint8), c_size_t]

## @endcond
