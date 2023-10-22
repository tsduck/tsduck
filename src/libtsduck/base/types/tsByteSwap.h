//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Low-level platform-dependent byte swapping functions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

#if defined(TS_LINUX)
    #include "tsBeforeStandardHeaders.h"
    #include <byteswap.h>
    #include "tsAfterStandardHeaders.h"
#endif

namespace ts {
    //!
    //! Perform a sign extension on 24 bit integers.
    //!
    //! @param [in] x A 32-bit integer containing a signed 24-bit value to extend.
    //! @return A 32-bit signed integer containing the signed 24-bit value with proper sign extension on 32-bits.
    //!
    TSDUCKDLL inline int32_t SignExtend24(int32_t x)
    {
    #if defined(TS_ARM64)
        asm("sbfm %w0, %w0, #0, #23" : "+r" (x)); return x;
    #else
        return (x & 0x00800000) == 0 ? (x & 0x00FFFFFF) : int32_t(uint32_t(x) | 0xFF000000);
    #endif
    }

    //!
    //! Perform a sign extension on 40 bit integers.
    //!
    //! @param [in] x A 64-bit integer containing a signed 40-bit value to extend.
    //! @return A 64-bit signed integer containing the signed 40-bit value with proper sign extension on 64-bits.
    //!
    TSDUCKDLL inline int64_t SignExtend40(int64_t x)
    {
    #if defined(TS_ARM64)
        asm("sbfm %0, %0, #0, #39" : "+r" (x)); return x;
    #else
        return (x & 0x0000008000000000) == 0 ? (x & 0x000000FFFFFFFFFF) : int64_t(uint64_t(x) | 0xFFFFFF0000000000);
    #endif
    }

    //!
    //! Perform a sign extension on 48 bit integers.
    //!
    //! @param [in] x A 64-bit integer containing a signed 48-bit value to extend.
    //! @return A 64-bit signed integer containing the signed 48-bit value with proper sign extension on 64-bits.
    //!
    TSDUCKDLL inline int64_t SignExtend48(int64_t x)
    {
    #if defined(TS_ARM64)
        asm("sbfm %0, %0, #0, #47" : "+r" (x)); return x;
    #else
        return (x & 0x0000800000000000) == 0 ? (x & 0x0000FFFFFFFFFFFF) : int64_t(uint64_t(x) | 0xFFFF000000000000);
    #endif
    }

    //!
    //! Inlined function performing byte swap on 16-bit integer data.
    //!
    //! This function unconditionally swaps bytes within an unsigned integer,
    //! regardless of the native endianness.
    //!
    //! @param [in] x A 16-bit unsigned integer to swap.
    //! @return The value of @a x where bytes were swapped.
    //!
    TSDUCKDLL inline uint16_t ByteSwap16(uint16_t x)
    {
    #if defined(TS_ARM64)
        asm("rev16 %w0, %w0" : "+r" (x)); return x;
    #elif defined(TS_LINUX)
        return bswap_16(x);
    #elif defined(TS_MSC)
        return _byteswap_ushort(x);
    #else
        return uint16_t((x << 8) | (x >> 8));
    #endif
    }

    //!
    //! Inlined function performing byte swap on 24-bit integer data.
    //!
    //! This function unconditionally swaps bytes within an unsigned integer,
    //! regardless of the native endianness.
    //!
    //! @param [in] x A 32-bit unsigned integer containing a 24-bit value to swap.
    //! @return The value of @a x where the three least significant bytes were swapped.
    //!
    TSDUCKDLL inline uint32_t ByteSwap24(uint32_t x)
    {
    #if defined(TS_ARM64)
        asm("rev %w0, %w0 \n lsr %w0, %w0, #8" : "+r" (x)); return x;
    #else
        return ((x << 16) & 0x00FF0000) | (x & 0x0000FF00) | ((x >> 16) & 0x000000FF);
    #endif
    }

    //!
    //! Inlined function performing byte swap on 32-bit integer data.
    //!
    //! This function unconditionally swaps bytes within an unsigned integer,
    //! regardless of the native endianness.
    //!
    //! @param [in] x A 32-bit unsigned integer to swap.
    //! @return The value of @a x where bytes were swapped.
    //!
    TSDUCKDLL inline uint32_t ByteSwap32(uint32_t x)
    {
    #if defined(TS_ARM64)
        asm("rev %w0, %w0" : "+r" (x)); return x;
    #elif defined(TS_LINUX)
        return bswap_32(x);
    #elif defined(TS_MSC)
        return _byteswap_ulong(x);
    #else
        return (x << 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x >> 24);
    #endif
    }

    //!
    //! Inlined function performing byte swap on 64-bit integer data.
    //!
    //! This function unconditionally swaps bytes within an unsigned integer,
    //! regardless of the native endianness.
    //!
    //! @param [in] x A 64-bit unsigned integer to swap.
    //! @return The value of @a x where bytes were swapped.
    //!
    TSDUCKDLL inline uint64_t ByteSwap64(uint64_t x)
    {
    #if defined(TS_ARM64)
        asm("rev %0, %0" : "+r" (x)); return x;
    #elif defined(TS_LINUX)
        return bswap_64(x);
    #elif defined(TS_MSC)
        return _byteswap_uint64(x);
    #else
        return
            ((x << 56)) |
            ((x << 40) & 0x00FF000000000000) |
            ((x << 24) & 0x0000FF0000000000) |
            ((x <<  8) & 0x000000FF00000000) |
            ((x >>  8) & 0x00000000FF000000) |
            ((x >> 24) & 0x0000000000FF0000) |
            ((x >> 40) & 0x000000000000FF00) |
            ((x >> 56));
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 16-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 16-bit unsigned integer to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint16_t CondByteSwap16BE(uint16_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return ByteSwap16(x);
    #else
        return x;
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 16-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 16-bit unsigned integer to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint16_t CondByteSwap16(uint16_t x)
    {
        return CondByteSwap16BE(x);
    }

    //!
    //! Inlined function performing conditional byte swap on 24-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 32-bit unsigned integer containing a 24-bit value to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where the three least
    //! significant bytes were swapped. On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint32_t CondByteSwap24BE(uint32_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return ByteSwap24(x);
    #else
        return x & 0x00FFFFFF;
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 24-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 32-bit unsigned integer containing a 24-bit value to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where the three least
    //! significant bytes were swapped. On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint32_t CondByteSwap24(uint32_t x)
    {
        return CondByteSwap24BE(x);
    }

    //!
    //! Inlined function performing conditional byte swap on 32-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 32-bit unsigned integer to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint32_t CondByteSwap32BE(uint32_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return ByteSwap32(x);
    #else
        return x;
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 32-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 32-bit unsigned integer to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint32_t CondByteSwap32(uint32_t x)
    {
        return CondByteSwap32BE(x);
    }

    //!
    //! Inlined function performing conditional byte swap on 64-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 64-bit unsigned integer to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint64_t CondByteSwap64BE(uint64_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return ByteSwap64(x);
    #else
        return x;
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 64-bit integer data
    //! to obtain the data in big endian representation.
    //!
    //! @param [in] x A 64-bit unsigned integer to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint64_t CondByteSwap64(uint64_t x)
    {
        return CondByteSwap64BE(x);
    }

    //!
    //! Inlined function performing conditional byte swap on 16-bit integer data
    //! to obtain the data in little endian representation.
    //!
    //! @param [in] x A 16-bit unsigned integer to conditionally swap.
    //! @return On big-endian platforms, return the value of @a x where bytes were swapped.
    //! On little-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint16_t CondByteSwap16LE(uint16_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return x;
    #else
        return ByteSwap16(x);
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 24-bit integer data
    //! to obtain the data in little endian representation.
    //!
    //! @param [in] x A 32-bit unsigned integer containing a 24-bit value to conditionally swap.
    //! @return On big-endian platforms, return the value of @a x where the three least
    //! significant bytes were swapped. On little-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint32_t CondByteSwap24LE(uint32_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return x & 0x00FFFFFF;
    #else
        return ByteSwap24(x);
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 32-bit integer data
    //! to obtain the data in little endian representation.
    //!
    //! @param [in] x A 32-bit unsigned integer to conditionally swap.
    //! @return On big-endian platforms, return the value of @a x where bytes were swapped.
    //! On little-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint32_t CondByteSwap32LE(uint32_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return x;
    #else
        return ByteSwap32(x);
    #endif
    }

    //!
    //! Inlined function performing conditional byte swap on 64-bit integer data
    //! to obtain the data in little endian representation.
    //!
    //! @param [in] x A 64-bit unsigned integer to conditionally swap.
    //! @return On big-endian platforms, return the value of @a x where bytes were swapped.
    //! On little-endian platforms, return the value of @a x unmodified.
    //!
    TSDUCKDLL inline uint64_t CondByteSwap64LE(uint64_t x)
    {
    #if defined(TS_LITTLE_ENDIAN)
        return x;
    #else
        return ByteSwap64(x);
    #endif
    }

    //!
    //! Template function performing conditional byte swap on integer data
    //! to obtain the data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] x An INT to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline INT CondByteSwapBE(INT x)
    {
#if defined(TS_BIG_ENDIAN)
        return x;
#else
        switch (sizeof(INT)) {
            case 1: return x;
            case 2: return static_cast<INT>(CondByteSwap16BE(static_cast<uint16_t>(x)));
            case 4: return static_cast<INT>(CondByteSwap32BE(static_cast<uint32_t>(x)));
            case 8: return static_cast<INT>(CondByteSwap64BE(static_cast<uint64_t>(x)));
            default: return 0;
        }
#endif
    }

    //!
    //! Template function performing conditional byte swap on integer data
    //! to obtain the data in little endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] x An INT to conditionally swap.
    //! @return On big-endian platforms, return the value of @a x where bytes were swapped.
    //! On little-endian platforms, return the value of @a x unmodified.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline INT CondByteSwapLE(INT x)
    {
#if defined(TS_BIG_ENDIAN)
        switch (sizeof(INT)) {
            case 1: return x;
            case 2: return static_cast<INT>(CondByteSwap16BE(static_cast<uint16_t>(x)));
            case 4: return static_cast<INT>(CondByteSwap32BE(static_cast<uint32_t>(x)));
            case 8: return static_cast<INT>(CondByteSwap64BE(static_cast<uint64_t>(x)));
            default: return 0;
        }
#else
        return x;
#endif
    }

    //!
    //! Template function performing conditional byte swap on integer data
    //! to obtain the data in big endian representation.
    //!
    //! @tparam INT Some integer type.
    //! @param [in] x An INT to conditionally swap.
    //! @return On little-endian platforms, return the value of @a x where bytes were swapped.
    //! On big-endian platforms, return the value of @a x unmodified.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    TSDUCKDLL inline INT CondByteSwap(INT x)
    {
        return CondByteSwapBE<INT>(x);
    }

    // Some specializations, for performance

#if !defined(DOXYGEN)
    template<> TSDUCKDLL inline uint8_t CondByteSwap   (uint8_t x) {return x;}
    template<> TSDUCKDLL inline int8_t  CondByteSwap   (int8_t  x) {return x;}
    template<> TSDUCKDLL inline uint8_t CondByteSwapBE (uint8_t x) {return x;}
    template<> TSDUCKDLL inline int8_t  CondByteSwapBE (int8_t  x) {return x;}
    template<> TSDUCKDLL inline uint8_t CondByteSwapLE (uint8_t x) {return x;}
    template<> TSDUCKDLL inline int8_t  CondByteSwapLE (int8_t  x) {return x;}
#endif
}
