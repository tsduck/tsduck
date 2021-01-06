//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2021, Thierry Lelegard
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
 */
public abstract class Report {

    // Severity levels, same values as C++ counterparts.
    static public final int FATAL   = -5;
    static public final int SEVERE  = -4;
    static public final int ERROR   = -3;
    static public final int WARNING = -2;
    static public final int INFO    = -1;
    static public final int VERBOSE =  0;
    static public final int DEBUG   =  1;

    // The address of the underlying C++ object.
    protected long nativeObject = 0;

    // Load native library on startup.
    static {
        NativeLibrary.loadLibrary();
    }
    
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
    	log(ERROR, message);
    }

    /**
     * Log a messages at warning level.
     * @param message Message to report.
     */
    public void warning(String message) {
    	log(WARNING, message);
    }

    /**
     * Log a messages at info level.
     * @param message Message to report.
     */
    public void info(String message) {
    	log(INFO, message);
    }

    /**
     * Log a messages at verbose level.
     * @param message Message to report.
     */
    public void verbose(String message) {
    	log(VERBOSE, message);
    }

    /**
     * Log a messages at debug level.
     * @param message Message to report.
     */
    public void debug(String message) {
    	log(DEBUG, message);
    }
}
