//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

package io.tsduck;

/**
 * A wrapper class for C++ InputSwitcher.
 * @ingroup java
 */
public final class InputSwitcher extends PluginEventHandlerRegistry {

    /*
     * Set the address of the C++ object.
     */
    private native void initNativeObject(Report report);

    /*
     * List of parameters to set before start().
     */
    public boolean fastSwitch = false;     //!< Fast switch between input plugins.
    public boolean delayedSwitch = false;  //!< Delayed switch between input plugins.
    public boolean terminate = false;      //!< Terminate when one input plugin completes.
    public boolean reusePort = false;      //!< Reuse-port socket option.
    public int firstInput = 0;             //!< Index of first input plugin.
    public int primaryInput = -1;          //!< Index of primary input plugin, negative if there is none.
    public int cycleCount = 1;             //!< Number of input cycles to execute (0 = infinite).
    public int bufferedPackets = 0;        //!< Input buffer size in packets (0=default).
    public int maxInputPackets = 0;        //!< Maximum input packets to read at a time (0=default).
    public int maxOutputPackets = 0;       //!< Maximum input packets to send at a time (0=default).
    public int sockBuffer = 0;             //!< Socket buffer size (0=default).
    public int remoteServerPort = 0;       //!< UDP server port for remote control (0=none).
    public int receiveTimeout = 0;         //!< Receive timeout before switch (0=none).
    public String eventCommand = "";       //!< External shell command to run on a switching event.
    public String eventUDPAddress = "";    //!< Remote IPv4 address or host name to receive switching event JSON description.
    public int eventUDPPort = 0;           //!< Remote UDP port to receive switching event JSON description.
    public String eventLocalAddress = "";  //!< Outgoing local interface for UDP event description.
    public int eventTTL = 0;               //!< Time-to-live socket option for UDP event description.
    public String appName = "";            //!< Application name, for help messages.
    public String[][] inputs = null;       //!< Input plugins name and arguments (array of arrays of strings)
    public String[] output = null;         //!< Output plugin name and arguments (array of strings)

    /**
     * Constructor
     * @param report The report object to use. If null, the logs are dropped.
     * Be sure to specify a thread-safe report such has AsyncReport or a subclass of AbstractAsyncReport.
     */
    public InputSwitcher(Report report) {
        initNativeObject(report);
    }

    /**
     * Start the input switcher session.
     * All properties shall have been set before calling this method.
     * @return True on success, false on error.
     */
    public native boolean start();

    /**
     * Switch to another input plugin.
     * @param pluginIndex Index of the new input plugin.
     */
    public native void setInput(int pluginIndex);

    /**
     * Switch to the next input plugin.
     */
     public native void nextInput();

    /**
     * Switch to the previous input plugin.
     */
     public native void previousInput();

    /**
     * Get the index of the current input plugin.
     * @return The index of the current input plugin.
     */
     public native int currentInput();

    /**
     * Terminate the processing.
     */
    public native void stop();

    /**
     * Suspend the calling thread until input switcher session is completed.
     */
    public native void waitForTermination();

    /**
     * Delete the encapsulated C++ object.
     */
    @Override
    public native void delete();
}
