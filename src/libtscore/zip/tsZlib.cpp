//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsZlib.h"
#include "tsMemory.h"
#include "tsVersionInfo.h"

// We use "sdefl" on Windows and when TS_NO_ZLIB is defined.
#if defined(TS_WINDOWS) && !defined(TS_NO_ZLIB)
    #define TS_NO_ZLIB 1
#endif

#if defined(TS_NO_ZLIB)
    // Use "sdefl".

    // Disable SIMD instructions on Arm32, the compilation of infl.h fails.
    #if defined(TS_ARM32) && !defined(SINFL_NO_SIMD)
        #define SINFL_NO_SIMD 1
    #endif

    // Force the implementation of functions inside defl.h and infl.h.
    #define SINFL_IMPLEMENTATION
    #define SDEFL_IMPLEMENTATION

    // The header files defl.h and infl.h generates many compilation warnings.
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(missing-field-initializers)
    TS_LLVM_NOWARNING(old-style-cast)
    TS_LLVM_NOWARNING(shorten-64-to-32)
    TS_LLVM_NOWARNING(comma)
    TS_LLVM_NOWARNING(padded)
    TS_LLVM_NOWARNING(sign-conversion)
    TS_LLVM_NOWARNING(unsafe-buffer-usage)
    TS_LLVM_NOWARNING(switch-default)
    TS_LLVM_NOWARNING(zero-as-null-pointer-constant)
    TS_LLVM_NOWARNING(reserved-identifier)
    TS_LLVM_NOWARNING(unused-function)
    TS_GCC_NOWARNING(missing-field-initializers)
    TS_GCC_NOWARNING(old-style-cast)
    TS_GCC_NOWARNING(switch-default)
    TS_GCC_NOWARNING(zero-as-null-pointer-constant)
    TS_GCC_NOWARNING(unused-function)
    TS_GCC_NOWARNING(sign-compare)
    TS_MSC_NOWARNING(4018)
    TS_MSC_NOWARNING(4505)
    #include "sdefl.h"
    #include "sinfl.h"
    TS_POP_WARNING()

#else
    // Use zlib
    #if !defined(ZLIB_CONST)
        #define ZLIB_CONST 1
    #endif
    #include <zlib.h>
#endif


//----------------------------------------------------------------------------
// Get the Zlib version.
//----------------------------------------------------------------------------

// Register for options --version.
TS_REGISTER_FEATURE(u"zlib", u"Deflate library", ALWAYS, ts::Zlib::GetLibraryVersion);

ts::UString ts::Zlib::GetLibraryVersion()
{
#if defined(TS_NO_ZLIB)
    return u"Small Deflate (sdefl) 1.00";
#else
    return UString::Format(u"zlib version %s (compiled with %s)", zlibVersion(), ZLIB_VERSION);
#endif
}


//----------------------------------------------------------------------------
// Check a zlib status, return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::Zlib::checkZlibStatus(void* stream, int status, const UChar* func, Report& report)
{
#if !defined(TS_NO_ZLIB)
    if (status != Z_OK && status != Z_STREAM_END && status != Z_BUF_ERROR) {
        UString msg;
        msg.format(u"zlib error %d", status);
        if (func != nullptr && *func != 0) {
            msg.format(u" from %s", func);
        }
        if (stream != nullptr && reinterpret_cast<::z_stream*>(stream)->msg != nullptr) {
            msg.format(u", %s", reinterpret_cast<::z_stream*>(stream)->msg);
        }
        report.error(msg);
        return false;
    }
#endif
    return true;
}


//----------------------------------------------------------------------------
// Compress data according to the DEFLATE algorithm.
//----------------------------------------------------------------------------

bool ts::Zlib::Compress(ByteBlock& out, const void* in, size_t in_size, int level, Report& report)
{
    // Level shall be in range 0-9.
    level = std::max(0, std::min(9, level));

#if defined(TS_NO_ZLIB)

    // Maximum possible size of compressed data.
    const size_t max_out = size_t(::sdefl_bound(int(in_size)));

    // Resize the output buffer to this size. Since the API of sdefl does not get a maximum output buffer size,
    // we can only hope that no buffer overflow will occur. And since we are paranoid, we write a canary at the
    // end of buffer and we will check it later.
    out.resize(max_out + 4);
    constexpr uint32_t canary = 0xDEADBEEF;
    PutUInt32(out.data() + max_out, canary);

    // Compress in one call.
    ::sdefl data;
    TS_ZERO(data);
    const int len = ::zsdeflate(&data, out.data(), in, int(in_size), level);
    if (len < 0) {
        report.error(u"sdefl error %d from zsdeflate", len);
        return false;
    }
    else if (GetUInt32(out.data() + max_out) != canary) {
        report.fatal(u"buffer overflow in zsdeflate(), probable memory corruption, expect a crash or worse");
        return false;
    }
    else {
        // Final size of output.
        out.resize(size_t(len));
        return true;
    }

#else
    // We compress, the output cannot be much larger than input.
    // In any case, we will resize if not large enough.
    out.resize(256 + in_size);

    ::z_stream strm;
    TS_ZERO(strm);
    int status = ::deflateInit(&strm, level);
    if (!checkZlibStatus(&strm, status, u"deflateInit", report)) {
        return false;
    }

    strm.next_in = reinterpret_cast<decltype(strm.next_in)>(in);
    strm.avail_in = static_cast<decltype(strm.avail_in)>(in_size);
    strm.next_out = reinterpret_cast<decltype(strm.next_out)>(out.data());
    strm.avail_out = static_cast<decltype(strm.avail_out)>(out.size());

    do {
        status = ::deflate(&strm, Z_FINISH);
        if (!checkZlibStatus(&strm, status, u"deflate", report)) {
            return false;
        }
        if (status != Z_STREAM_END && strm.avail_out == 0) {
            // No enough space in output buffer, resize it.
            size_t previous = strm.total_out;
            out.resize(previous + 10'000);
            strm.next_out = reinterpret_cast<decltype(strm.next_out)>(out.data() + previous);
            strm.avail_out = static_cast<decltype(strm.avail_out)>(out.size() - previous);
        }
    } while (status != Z_STREAM_END);

    // Final size is now known.
    out.resize(size_t(strm.total_out));

    status = ::deflateEnd(&strm);
    return checkZlibStatus(&strm, status, u"deflateEnd", report);

#endif
}


//----------------------------------------------------------------------------
// Decompress data according to the DEFLATE algorithm.
//----------------------------------------------------------------------------

bool ts::Zlib::Decompress(ByteBlock& out, const void* in, size_t in_size, Report& report)
{
#if defined(TS_NO_ZLIB)

    // There is no way to know the decompressed size and there is also no way to continue
    // decompressing if the buffer is too small. We adopt the following strategy: start with
    // some probable max size, then retry up to 3 times, doubling the buffer size each time.
    out.resize(512 + in_size * 4);
    int len = 0;
    for (int count = 3; count > 0; --count) {
        len = ::zsinflate(out.data(), int(out.size()), in, int(in_size));
        if (len >= 0) {
            // Success, final size of output.
            out.resize(size_t(len));
            return true;
        }
        out.resize(2 * out.size());
    }
    report.error(u"sdefl error %d from zsinflate", len);
    return false;

#else
    // Resize to some arbitrary larger size than input.
    // In any case, we will resize if not large enough.
    out.resize(3 * in_size);

    ::z_stream strm;
    TS_ZERO(strm);
    strm.next_in = reinterpret_cast<decltype(strm.next_in)>(in);
    strm.avail_in = static_cast<decltype(strm.avail_in)>(in_size);

    int status = ::inflateInit(&strm);
    if (!checkZlibStatus(&strm, status, u"inflateInit", report)) {
        return false;
    }

    strm.next_out = reinterpret_cast<decltype(strm.next_out)>(out.data());
    strm.avail_out = static_cast<decltype(strm.avail_out)>(out.size());

    do {
        status = ::inflate(&strm, Z_FINISH);
        if (!checkZlibStatus(&strm, status, u"inflate", report)) {
            return false;
        }
        if (status != Z_STREAM_END && strm.avail_out == 0) {
            // No enough space in output buffer, resize it.
            size_t previous = strm.total_out;
            out.resize(previous + 2 * in_size);
            strm.next_out = reinterpret_cast<decltype(strm.next_out)>(out.data() + previous);
            strm.avail_out = static_cast<decltype(strm.avail_out)>(out.size() - previous);
        }
    } while (status != Z_STREAM_END);

    // Final size is now known.
    out.resize(size_t(strm.total_out));

    status = ::inflateEnd(&strm);
    return checkZlibStatus(&strm, status, u"deflateEnd", report);

#endif
}
