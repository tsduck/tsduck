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
 * A wrapper class for C++ TSProcessor.
 * @ingroup java
 */
public final class TSProcessor extends PluginEventHandlerRegistry {

    /*
     * Set the address of the C++ object.
     */
    private native void initNativeObject(Report report);

    /*
     * List of parameters to set before start().
     */
    public boolean ignoreJointTermination = false;  //!< Option -\-ignore-joint-termination
    public boolean logPluginIndex = false;          //!< Option -\-log-plugin-index
    public int bufferSize = 16 * 1024 * 1024;       //!< Option -\-buffer-size-mb (in bytes here)
    public int maxFlushedPackets = 0;               //!< Option -\-max-flushed-packets (zero means default)
    public int maxInputPackets = 0;                 //!< Option -\-max-input-packets (zero means default)
    public int maxOutputPackets = 0;                //!< Option -\-max-output-packets (zero means unlimited)
    public int initialInputPackets = 0;             //!< Option -\-initial-input-packets (zero means default)
    public int addInputStuffingNull = 0;            //!< The nullpkt in option -\-add-input-stuffing nullpkt/inpkt
    public int addInputStuffingInput = 0;           //!< The inpkt in option -\-add-input-stuffing nullpkt/inpkt
    public int addStartStuffing = 0;                //!< Option -\-add-start-stuffing
    public int addStopStuffing = 0;                 //!< Option -\-add-stop-stuffing
    public int bitrate = 0;                         //!< Option -\-bitrate
    public int bitrateAdjustInterval = 5000;        //!< Option -\-bitrate-adjust-interval (in milliseconds)
    public int receiveTimeout = 0;                  //!< Option -\-receive-timeout
    public String appName = "";                     //!< Application name, for help messages.
    public String[] input = null;                   //!< Input plugin name and arguments (array of strings)
    public String[][] plugins =  null;              //!< Packet processor plugins names and arguments (array of arrays of strings)
    public String[] output = null;                  //!< Output plugin name and arguments (array of strings)

    /**
     * Constructor
     * @param report The report object to use. If null, the logs are dropped.
     * Be sure to specify a thread-safe report such has AsyncReport or a subclass of AbstractAsyncReport.
     */
    public TSProcessor(Report report) {
        initNativeObject(report);
    }

    /**
     * Start the TS processor.
     * All properties shall have been set before calling this method.
     * @return True on success, false on error.
     */
    public native boolean start();

    /**
     * Abort the processing.
     */
    public native void abort();

    /**
     * Suspend the calling thread until TS processing is completed.
     */
    public native void waitForTermination();

    /**
     * Delete the encapsulated C++ object.
     */
    @Override
    public native void delete();
}
