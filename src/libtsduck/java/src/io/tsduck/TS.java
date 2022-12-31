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
