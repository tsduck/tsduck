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
#  TSDuck Python bindings to TSProcessor classes.
#
#-----------------------------------------------------------------------------

from . import lib
from .native import NativeObject
import ctypes
import re

##
# A Python exception class which is thrown in case of start error in TSProcessor.
# @ingroup python
#
class TSPStartError(Exception):
    pass

##
# A wrapper class for C++ TSProcessor.
# @ingroup python
#
class TSProcessor(NativeObject):

    ##
    # Constructor.
    # @param report The ts.Report object to use.
    #
    def __init__(self, report):
        super().__init__()
        self._report = report
        self._native_object = lib.tspyNewTSProcessor(self._report._native_object)

        # Publicly customizable tsp options:
        ## Option -\-monitor.
        self.monitor = False                   
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
        lib.tspyDeleteTSProcessor(self._native_object)
        super().delete()

    ##
    # Start the TS processor.
    # All properties shall have been set before calling this method.
    # @return None.
    #
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
        if not lib.tspyStartTSProcessor(self._native_object, ctypes.byref(args), plugins.data_ptr(), plugins.size()):
            raise TSPStartError("Error starting TS processor")

    ##
    # Abort the TSProcessor.
    # @return None.
    #
    def abort(self):
        lib.tspyAbortTSProcessor(self._native_object)

    ##
    # Suspend the calling thread until TS processing is completed.
    # @return None.
    #
    def waitForTermination(self):
        lib.tspyWaitTSProcessor(self._native_object)
