//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of RFC 1951 data compression, a.k.a. zlib format.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! Implementation of RFC 1951 data compression, a.k.a. zlib format, a.k.a. DEFLATE.
    //! @ingroup system
    //!
    //! This interface is a proxy to the zlib library on UNIX systems (Linux, macOS, BSD).
    //! On Windows systems, the source code of TSDuck embeds the header-only implementation
    //! called "Small Deflate" or "sdefl". The sdefl implementation is also used when the
    //! macro TS_NO_ZLIB is defined on any system.
    //!
    //! @see RFC 1950 ZLIB Compressed Data Format Specification version 3.3
    //! @see RFC 1951 DEFLATE Compressed Data Format Specification version 1.3
    //! @see https://www.zlib.net
    //! @see https://github.com/vurtun/lib
    //!
    class TSCOREDLL Zlib
    {
    public:
        //!
        //! Get the Zlib library version.
        //! @return The Zlib library version.
        //!
        static UString GetLibraryVersion();

        //!
        //! Compress data according to the DEFLATE algorithm.
        //! @param [out] out Output compressed data.
        //! @param [in] in Address of input data.
        //! @param [in] in_size Size in bytes of input data.
        //! @param [in] level Requested compression level, from 0 to 9.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        static bool Compress(ByteBlock& out, const void* in, size_t in_size, int level, Report& report = NULLREP);

        //!
        //! Compress data according to the DEFLATE algorithm.
        //! @param [out] out Output compressed data.
        //! @param [in] in Input data.
        //! @param [in] level Requested compression level, from 0 to 9.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        static bool Compress(ByteBlock& out, const ByteBlock& in, int level, Report& report = NULLREP)
        {
            return Compress(out, in.data(), in.size(), level, report);
        }

        //!
        //! Decompress data according to the DEFLATE algorithm.
        //! @param [out] out Output decompressed data.
        //! @param [in] in Address of input data.
        //! @param [in] in_size Size in bytes of input data.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        static bool Decompress(ByteBlock& out, const void* in, size_t in_size, Report& report = NULLREP);

        //!
        //! Decompress data according to the DEFLATE algorithm.
        //! @param [out] out Output decompressed data.
        //! @param [in] in Input data.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        static bool Decompress(ByteBlock& out, const ByteBlock& in, Report& report = NULLREP)
        {
            return Decompress(out, in.data(), in.size(), report);
        }

    private:
        // Check a zlib status, return true on success, false on error.
        static bool checkZlibStatus(void* stream, int status, const UChar* func, Report& report);
    };
}
