//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2025, Thierry Lelegard
//  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

package io.tsduck;

/**
 * General transport stream characteristics.
 * @ingroup java
 */
public class TS {

    /**
     * MPEG TS packet size in bytes.
     */
    static public final int PKT_SIZE = 188;

    /**
     * MPEG TS packet size in bits.
     */
    static public final int PKT_SIZE_BITS = 8 * PKT_SIZE;

    /**
     * Size in bytes of a Reed-Solomon outer FEC.
     */
    static public final int RS_SIZE = 16;

    /**
     * Size in bytes of a TS packet with trailing Reed-Solomon outer FEC.
     */
    static public final int PKT_RS_SIZE = PKT_SIZE + RS_SIZE;

    /**
     * Size in bytes of a timestamp preceeding a TS packet in M2TS files (Blu-ray disc).
     */
    static public final int M2TS_HEADER_SIZE = 4;

    /**
     * Size in bytes of an TS packet in M2TS files (Blu-ray disc).
     */
    static public final int PKT_M2TS_SIZE = M2TS_HEADER_SIZE + PKT_SIZE;
}
