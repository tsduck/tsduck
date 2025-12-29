//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2026, Thierry Lelegard
//  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

package io.tsduck;

/**
 * An abstract Report class which can be derived by applications to get asynchronous log messages.
 * @ingroup java
 *
 * This class is functionally similar to {@link ts::AsyncReport} except that the message handling can
 * be implemented in Java. This class is suitable for use with {@link io.tsduck.TSProcessor}.
 */
public abstract class AbstractAsyncReport extends Report {

    /*
     * Set the address of the C++ object.
     */
    private native void initNativeObject(String logMethodName, int severity, boolean syncLog, int logMsgCount);

    /**
     * Constructor (for subclasses).
     * @param severity Initial severity.
     * @param syncLog Synchronous log.
     * @param logMsgCount Maximum buffered log messages.
     */
    protected AbstractAsyncReport(int severity, boolean syncLog, int logMsgCount) {
        initNativeObject("logMessageHandler", severity, syncLog, logMsgCount);
    }

    /**
     * Synchronously terminates the asynchronous log thread.
     */
    public native void terminate();

    /**
     * Delete the encapsulated C++ object.
     */
    @Override
    public native void delete();

    /**
     * This method is invoked each time a message is logged.
     * If a subclass wants to intercept log messages, it should override this method.
     * Take care that this method is invoked in the context of a native thread.
     * @param severity Severity of the message.
     * @param message Message line.
     */
    abstract public void logMessageHandler(int severity, String message);
}
