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

        # The internal callback with profile above, called from the C++ class.
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
                carray_type = ctypes.c_uint8 * len(outdata)
                carray = ctypes.cast(carray_type.from_buffer(outdata), ctypes.POINTER(ctypes.c_uint8))
                lib.tspyPyPluginEventHandlerUpdateData(event_data_obj, carray, ctypes. c_size_t(len(outdata)))
            return success

        # Keep a reference on the callback in the object instance.
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
