//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

package io.tsduck;

/**
 * A wrapper class for C++ AsyncReport.
 * @ingroup java
 */
public final class AsyncReport extends Report {

    /*
     * Set the address of the C++ object.
     */
    private native void initNativeObject(int severity, boolean syncLog, boolean timedLog, int logMsgCount);

    /**
     * Constructor
     */
    public AsyncReport() {
        initNativeObject(Info, false, false, 512);
    }

    /**
     * Constructor
     * @param severity Initial severity.
     * @param syncLog Synchronous log.
     * @param timedLog Add time stamps in log messages.
     * @param logMsgCount Maximum buffered log messages.
     */
    public AsyncReport(int severity, boolean syncLog, boolean timedLog, int logMsgCount) {
        initNativeObject(severity, syncLog, timedLog, logMsgCount);
    }

    /**
     * Synchronously terminates the async log thread.
     */
    public native void terminate();

    /**
     * Delete the encapsulated C++ object.
     */
    @Override
    public native void delete();
}
