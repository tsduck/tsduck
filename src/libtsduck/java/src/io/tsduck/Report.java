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
 * Base class for TSDuck report classes.
 * @ingroup java
 */
public abstract class Report extends NativeObject {

    /*
     * Severity levels, same values as C++ counterparts.
     */
    static public final int Fatal   = -5;  //!< Fatal error, typically aborts the application.
    static public final int Severe  = -4;  //!< Severe error.
    static public final int Error   = -3;  //!< Regular error.
    static public final int Warning = -2;  //!< Warning message.
    static public final int Info    = -1;  //!< Information message.
    static public final int Verbose = 0;   //!< Verbose information.
    static public final int Debug   = 1;   //!< First debug level.

    /**
     * Set the maximum severity of the report.
     * @param severity Severity level.
     */
    public native void setMaxSeverity(int severity);

    /**
     * Log a message to the report.
     * @param severity Severity level of the message.
     * @param message Message to report.
     */
    public native void log(int severity, String message);

    /**
     * Log a messages at error level.
     * @param message Message to report.
     */
    public void error(String message) {
        log(Error, message);
    }

    /**
     * Log a messages at warning level.
     * @param message Message to report.
     */
    public void warning(String message) {
        log(Warning, message);
    }

    /**
     * Log a messages at info level.
     * @param message Message to report.
     */
    public void info(String message) {
        log(Info, message);
    }

    /**
     * Log a messages at verbose level.
     * @param message Message to report.
     */
    public void verbose(String message) {
        log(Verbose, message);
    }

    /**
     * Log a messages at debug level.
     * @param message Message to report.
     */
    public void debug(String message) {
        log(Debug, message);
    }

    /**
     * Formatted line prefix header for a severity.
     * @param severity Severity value.
     * @return A string to prepend to messages. Empty for Info and Verbose levels.
     */
    public static native String header(int severity);
}
