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
