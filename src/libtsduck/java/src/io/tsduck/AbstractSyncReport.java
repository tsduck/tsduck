//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2026, Thierry Lelegard
//  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

package io.tsduck;

/**
 * An abstract Report class which can be derived by applications to get synchronous log messages.
 * @ingroup java
 *
 * Handling messages is performed synchronously, meaning that Java calls TSDuck which logs
 * messages which calls the Java message handler.
 *
 * This class is not thread-safe. It shall be entirely used in the same Java thread.
 * This report shall be passed to TSDuck features which synchronously execute in the
 * caller thread only. Specifically, this class is not suitable for use with
 * {@link io.tsduck.TSProcessor} (use {@link io.tsduck.AbstractAsyncReport} instead).
 */
public abstract class AbstractSyncReport extends Report {

    // Set the address of the C++ object.
    private native void initNativeObject(String logMethodName, int severity);

    /**
     * Constructor (for subclasses).
     * @param severity Initial severity.
     */
    protected AbstractSyncReport(int severity) {
        initNativeObject("logMessageHandler", severity);
    }

    /**
     * Delete the encapsulated C++ object.
     */
    @Override
    public native void delete();

    /**
     * This method is invoked each time a message is logged.
     * If a subclass wants to intercept log messages, it should override this method.
     * @param severity Severity of the message.
     * @param message Message line.
     */
    abstract public void logMessageHandler(int severity, String message);
}
