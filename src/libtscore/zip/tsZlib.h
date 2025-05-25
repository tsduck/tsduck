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
    //! @ingroup libtscore system
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
        //! Check if "Small Deflate" or "sdefl" is the default compression library.
        //! @return True if "Small Deflate" is the default, false if "zlib" is available and the default.
        //!
        static bool DefaultSdefl();

        //!
        //! Check if "Small Deflate" or "sdefl" is supported.
        //! @return True if "Small Deflate" is supported, false if "zlib" is the only available option.
        //!
        static bool SdeflSupported();

        //!
        //! Compress data according to the DEFLATE algorithm.
        //! @param [out] out Output compressed data.
        //! @param [in] in Address of input data.
        //! @param [in] in_size Size in bytes of input data.
        //! @param [in] level Requested compression level, from 0 to 9.
        //! @param [in,out] report Where to report errors.
        //! @param [in] use_sdefl If true, force the usage of "sdefl" library. By default, use "zlib" on UNIX systems and "sdefl" on Windows.
        //! @return True on success, false on error.
        //!
        static bool Compress(ByteBlock& out, const void* in, size_t in_size, int level, Report& report = NULLREP, bool use_sdefl = false)
        {
            out.clear();
            return CompressAppend(out, in, in_size, level, report, use_sdefl);
        }

        //!
        //! Compress data according to the DEFLATE algorithm.
        //! @param [out] out Output compressed data.
        //! @param [in] in Input data.
        //! @param [in] level Requested compression level, from 0 to 9.
        //! @param [in,out] report Where to report errors.
        //! @param [in] use_sdefl If true, force the usage of "sdefl" library. By default, use "zlib" on UNIX systems and "sdefl" on Windows.
        //! @return True on success, false on error.
        //!
        static bool Compress(ByteBlock& out, const ByteBlock& in, int level, Report& report = NULLREP, bool use_sdefl = false)
        {
            out.clear();
            return CompressAppend(out, in.data(), in.size(), level, report, use_sdefl);
        }

        //!
        //! Compress data according to the DEFLATE algorithm.
        //! @param [in,out] out Output compressed data are appended at the end of the existing content.
        //! @param [in] in Address of input data.
        //! @param [in] in_size Size in bytes of input data.
        //! @param [in] level Requested compression level, from 0 to 9.
        //! @param [in,out] report Where to report errors.
        //! @param [in] use_sdefl If true, force the usage of "sdefl" library. By default, use "zlib" on UNIX systems and "sdefl" on Windows.
        //! @return True on success, false on error.
        //!
        static bool CompressAppend(ByteBlock& out, const void* in, size_t in_size, int level, Report& report = NULLREP, bool use_sdefl = false);

        //!
        //! Compress data according to the DEFLATE algorithm.
        //! @param [in,out] out Output compressed data are appended at the end of the existing content.
        //! @param [in] in Input data.
        //! @param [in] level Requested compression level, from 0 to 9.
        //! @param [in,out] report Where to report errors.
        //! @param [in] use_sdefl If true, force the usage of "sdefl" library. By default, use "zlib" on UNIX systems and "sdefl" on Windows.
        //! @return True on success, false on error.
        //!
        static bool CompressAppend(ByteBlock& out, const ByteBlock& in, int level, Report& report = NULLREP, bool use_sdefl = false)
        {
            return CompressAppend(out, in.data(), in.size(), level, report, use_sdefl);
        }

        //!
        //! Decompress data according to the DEFLATE algorithm.
        //! @param [out] out Output decompressed data.
        //! @param [in] in Address of input data.
        //! @param [in] in_size Size in bytes of input data.
        //! @param [in,out] report Where to report errors.
        //! @param [in] use_sdefl If true, force the usage of "sdefl" library. By default, use "zlib" on UNIX systems and "sdefl" on Windows.
        //! @return True on success, false on error.
        //!
        static bool Decompress(ByteBlock& out, const void* in, size_t in_size, Report& report = NULLREP, bool use_sdefl = false)
        {
            out.clear();
            return DecompressAppend(out, in, in_size, report, use_sdefl);
        }

        //!
        //! Decompress data according to the DEFLATE algorithm.
        //! @param [out] out Output decompressed data.
        //! @param [in] in Input data.
        //! @param [in,out] report Where to report errors.
        //! @param [in] use_sdefl If true, force the usage of "sdefl" library. By default, use "zlib" on UNIX systems and "sdefl" on Windows.
        //! @return True on success, false on error.
        //!
        static bool Decompress(ByteBlock& out, const ByteBlock& in, Report& report = NULLREP, bool use_sdefl = false)
        {
            out.clear();
            return DecompressAppend(out, in.data(), in.size(), report, use_sdefl);
        }

        //!
        //! Decompress data according to the DEFLATE algorithm.
        //! @param [out] out Output decompressed data are appended at the end of the existing content.
        //! @param [in] in Address of input data.
        //! @param [in] in_size Size in bytes of input data.
        //! @param [in,out] report Where to report errors.
        //! @param [in] use_sdefl If true, force the usage of "sdefl" library. By default, use "zlib" on UNIX systems and "sdefl" on Windows.
        //! @return True on success, false on error.
        //!
        static bool DecompressAppend(ByteBlock& out, const void* in, size_t in_size, Report& report = NULLREP, bool use_sdefl = false);

        //!
        //! Decompress data according to the DEFLATE algorithm.
        //! @param [out] out Output decompressed data are appended at the end of the existing content.
        //! @param [in] in Input data.
        //! @param [in,out] report Where to report errors.
        //! @param [in] use_sdefl If true, force the usage of "sdefl" library. By default, use "zlib" on UNIX systems and "sdefl" on Windows.
        //! @return True on success, false on error.
        //!
        static bool DecompressAppend(ByteBlock& out, const ByteBlock& in, Report& report = NULLREP, bool use_sdefl = false)
        {
            return DecompressAppend(out, in.data(), in.size(), report, use_sdefl);
        }

    private:
        // Check a zlib status, return true on success, false on error.
        static bool checkZlibStatus(void* stream, int status, const UChar* func, Report& report);
    };
}
