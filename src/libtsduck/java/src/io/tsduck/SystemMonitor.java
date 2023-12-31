//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2024, Thierry Lelegard
//  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
