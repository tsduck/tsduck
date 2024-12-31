//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2025, Thierry Lelegard
//  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

package io.tsduck;

/**
 * TSDuck library general information.
 * @ingroup java
 */
public class Info {

    /*
     * Load native library on startup.
     */
    static {
        NativeLibrary.loadLibrary();
    }

    /**
     * TSDuck version as an integer.
     * @return TSDuck version as an integer, suitable for comparison between versions.
     */
    public static native int intVersion();

    /**
     * TSDuck version as a string.
     * @return TSDuck version as a string.
     */
    public static native String version();
}
