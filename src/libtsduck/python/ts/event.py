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
#  TSDuck Python bindings to plugin event handler.
#
#-----------------------------------------------------------------------------

from . import lib
from .native import NativeObject
import ctypes

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
        ##
        # A plugin-defined 32-bit code describing the event type.
        # There is no predefined list of event codes. The application typically
        # specifies the event code using a @c --event-code option in the plugin command.
        #
        self.event_code = 0
        ##
        # Plugin name as found in the plugin registry.
        #
        self.plugin_name = ""
        ##
        # Plugin index in the chain. For tsp, plugins are numbered from 0 (the input plugin)
        # to N-1 (the output plugin). For tsswitch, the input plugins are numbered from 0
        # to N-2 and the output plugin is N-1.
        #
        self.plugin_index = 0
        ##
        # Total number N of plugins in the chain.
        #
        self.plugin_count = 0
        ##
        # Known bitrate in the context of the plugin at the time of the event.
        #
        self.bitrate = 0
        ##
        # Number of packets which passed through the plugin at the time of the event.
        #
        self.plugin_packets = 0
        ##
        # Total number of packets which passed through the plugin thread at the time of the event.
        # It can be more than @a plugin_packets if some packets were not submitted to the plugin
        # (deleted or excluded packets).
        #
        self.total_packets = 0

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

        # An internal callback, called from the C++ class.
        def event_callback(evcode, name_data, name_size, plindex, plcount, bitrate, plpackets, total_packets, data, size):
            context = PluginEventContext()
            context.event_code = evcode
            context.plugin_name = ctypes.string_at(name_data, name_size).decode('utf-16')
            context.plugin_index = plindex
            context.plugin_count = plcount
            context.bitrate = bitrate
            context.plugin_packets = plpackets
            context.total_packets = total_packets
            evdata = bytes(ctypes.string_at(data, size))
            self.handlePluginEvent(context, evdata)

        # Keep a reference on the callback in the object instance.
        callback = ctypes.CFUNCTYPE(None, ctypes.c_uint32, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t, ctypes.c_size_t,
                                    ctypes.c_size_t, ctypes.c_size_t, ctypes.c_size_t,
                                    ctypes.POINTER(ctypes.c_ubyte), ctypes.c_size_t)
        self._cb = callback(event_callback)

        # Finally create the native object.
        self._native_object = lib.tspyNewPyPluginEventHandler(self._cb)

    # Explicitly free the underlying C++ object (inherited).
    def delete(self):
        lib.tspyDeletePyPluginEventHandler(self._native_object)
        super().delete()


    ##
    # This handler is invoked when a plugin signals an event for which this object is registered.
    # The application should override it to collect the event.
    # @param context An instance of PluginEventContext containing the details of the event.
    # @param data A bytes object containing the data of the event. This is a read-only
    # sequence of bytes. There is no way to return data from Python to the plugin.
    # @return None.
    # 
    def handlePluginEvent(self, context, data):
        pass
