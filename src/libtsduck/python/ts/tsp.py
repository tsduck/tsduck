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
#  TSDuck Python bindings to TSProcessor classes.
#
#-----------------------------------------------------------------------------

from . import lib
import ctypes
import re

class TSPStartError(Exception):
    pass

class TSProcessor:
    # Constructor with the ts.Report object.
    def __init__(self, report):
        self._report = report
        self._tsp_addr = ctypes.c_void_p(lib.tspyNewTSProcessor(self._report._report_addr))

        # Publicly customizable tsp options:
        self.monitor = False                   # --monitor
        self.ignore_joint_termination = False  # --ignore-joint-termination
        self.buffer_size = 16 * 1024 * 1024    # --buffer-size-mb (in bytes here)
        self.max_flushed_packets = 0           # --max-flushed-packets (zero means default)
        self.max_input_packets = 0             # --max-input-packets (zero means default)
        self.initial_input_packets = 0         # --initial-input-packets (zero means default)
        self.add_input_stuffing = [0, 0]       # --add-input-stuffing nullpkt/inpkt
        self.add_start_stuffing = 0            # --add-start-stuffing
        self.add_stop_stuffing = 0             # --add-stop-stuffing
        self.bitrate = 0                       # --bitrate
        self.bitrate_adjust_interval = 5000    # --bitrate-adjust-interval (in milliseconds)
        self.receive_timeout = 0               # --receive-timeout
        self.app_name = ""                     # application name, for help messages.
        self.input = []                        # input plugin name and arguments (list of strings)
        self.plugins = []                      # packet processor plugins names and arguments (list of lists of strings)
        self.output = []                       # output plugin name and arguments (list of strings)

    # Finalizer.
    def __del__(self):
        if self._tsp_addr.value != 0:
            lib.tspyDeleteReport(self._tsp_addr)
            self._tsp_addr = ctypes.c_void_p(0)
            self._report = None

    # Starts the TSProcessor.
    def start(self):
        # Build global options. Copy expected fields in C++ struct from this object.
        # When an expected field name is "prefix_int", fetch field "prefix[int]".
        args = lib.tspyTSProcessorArgs()
        reg = re.compile('^(.*)_(\d+)$') # match "prefix_int" as "(1)_(2)"
        for f in args._fields_:
            name = f[0]
            search = reg.match(name)
            if search:
                # Fetch field "prefix[int]"
                setattr(args, name, getattr(self, search.group(1))[int(search.group(2))])
            else:
                # Fetch field "name"
                setattr(args, name, getattr(self, name))

        # Build UTF-16 buffer with application names and plugins.
        plugins = lib.InByteBuffer(self.app_name)
        if len(self.input) > 0:
            plugins.extend('-I')
            plugins.extend(self.input)
        for pl in self.plugins:
            plugins.extend('-P')
            plugins.extend(pl)
        if len(self.output) > 0:
            plugins.extend('-O')
            plugins.extend(self.output)

        # Start the processing.
        if not lib.tspyStartTSProcessor(self._tsp_addr, ctypes.byref(args), plugins.data_ptr(), plugins.size()):
            raise TSPStartError("Error starting TS processor")

    # Aborts the TSProcessor.
    def abort(self):
        lib.tspyAbortTSProcessor(self._tsp_addr)

    # Suspend the calling thread until TS processing is completed.
    def waitForTermination(self):
        lib.tspyWaitTSProcessor(self._tsp_addr)
