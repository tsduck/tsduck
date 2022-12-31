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
 * A wrapper class for C++ SystemMonitor.
 * @ingroup java
 */
public final class SystemMonitor extends NativeObject {

    /*
     * Set the address of the C++ object.
     */
    private native void initNativeObject(Report report, String config);

    /**
     * Constructor
     * @param report The report object to use. If null, the logs are sent to the standard error.
     * Be sure to specify a thread-safe report such has AsyncReport or a subclass of AbstractAsyncReport.
     */
    public SystemMonitor(Report report) {
        initNativeObject(report, null);
    }

    /**
     * Constructor
     * @param report The report object to use. If null, the logs are sent to the standard error.
     * Be sure to specify a thread-safe report such has AsyncReport or a subclass of AbstractAsyncReport.
     * @param config Name of the monitoring configuration file.
     * If null or empty, the default configuration file is used.
     */
    public SystemMonitor(Report report, String config) {
        initNativeObject(report, config);
    }

    /**
     * Start the monitoring thread.
     */
    public native void start();

    /**
     * Stop the monitoring thread.
     */
    public native void stop();

    /**
     * Suspend the calling thread until the monitoring thread is terminated.
     * The monitoring thread is requested to stop. This method returns immediately,
     * use waitForTermination() to synchronously wait for its termination.
     */
    public native void waitForTermination();

    /**
     * Delete the encapsulated C++ object.
     */
    @Override
    public native void delete();
}
