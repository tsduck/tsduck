#-----------------------------------------------------------------------------
#
#  TSDuck Python bindings to InputSwitcher.
#
#-----------------------------------------------------------------------------

from . import lib
from .native import NativeObject
import ctypes

##
# A Python exception class which is thrown in case of start error in InputSwitcher.
# @ingroup python
#
class SwitchStartError(Exception):
    pass

##
# A wrapper class for C++ InputSwitcher.
# @ingroup python
#
class InputSwitcher(NativeObject):

    ##
    # Constructor.
    # @param report The ts.Report object to use.
    #
    def __init__(self, report):
        super().__init__()
        self._report = report
        self._native_object = lib.tspyNewInputSwitcher(self._report._native_object)

        # Publicly customizable input switcher options:
        ## Fast switch between input plugins.
        self.fast_switch = False
        ## Delayed switch between input plugins.
        self.delayed_switch = False
        ## Terminate when one input plugin completes.
        self.terminate = False
        ## Run a resource monitoring thread.
        self.monitor = False
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
        ## Application name, for help messages.
        self.app_name = ""
        ## Input plugins name and arguments (list of lists of strings).
        self.inputs = []
        ## Output plugin name and arguments (list of strings)
        self.output = []

    # Explicitly free the underlying C++ object (inherited).
    def delete(self):
        lib.tspyDeleteInputSwitcher(self._native_object)
        super().delete()

    ##
    # Start the input switcher.
    # All properties shall have been set before calling this method.
    # @return None.
    #
    def start(self):
        # Build global options. Copy expected fields in C++ struct from this object.
        # When an expected field name is "prefix_int", fetch field "prefix[int]".
        args = lib.tspyInputSwitcherArgs()
        for f in args._fields_:
            name = f[0]
            setattr(args, name, getattr(self, name))

        # Build UTF-16 buffer with application names and plugins.
        plugins = lib.InByteBuffer(self.app_name)
        for pl in self.inputs:
            plugins.extend('-I')
            plugins.extend(pl)
        if len(self.output) > 0:
            plugins.extend('-O')
            plugins.extend(self.output)

        # Start the processing.
        if not lib.tspyStartInputSwitcher(self._native_object, ctypes.byref(args), plugins.data_ptr(), plugins.size()):
            raise SwitchStartError("Error starting input switcher")

    ##
    # Switch to another input plugin.
    # @param plugin_index Index of the new input plugin.
    # @return None.
    #
    def setInput(self, plugin_index):
        lib.tspyInputSwitcherSetInput(self._native_object, plugin_index)

    ##
    # Switch to the next input plugin.
    # @return None.
    #
    def nextInput(self):
        lib.tspyInputSwitcherNextInput(self._native_object)

    ##
    # Switch to the previous input plugin.
    # @return None.
    #
    def previousInput(self):
        lib.tspyInputSwitcherPreviousInput(self._native_object)

    ##
    # Get the index of the current input plugin.
    # @return The index of the current input plugin.
    #
    def currentInput(self):
        return int(lib.tspyInputSwitcherCurrentInput(self._native_object))

    ##
    # Terminate the processing.
    # @return None.
    #
    def stop(self):
        lib.tspyStopInputSwitcher(self._native_object)

    ##
    # Suspend the calling thread until input switcher session is completed.
    # @return None.
    #
    def waitForTermination(self):
        lib.tspyWaitInputSwitcher(self._native_object)
