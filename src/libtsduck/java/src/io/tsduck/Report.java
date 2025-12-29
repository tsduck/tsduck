//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2026, Thierry Lelegard
//  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
