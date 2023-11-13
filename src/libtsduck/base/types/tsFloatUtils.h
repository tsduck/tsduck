//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup cpp
//!  Some utilities on floating point types.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMemory.h"

namespace ts {
    //!
    //! This template function compares two floating point values for approximated equality.
    //! @tparam T A floating point type.
    //! @param a First value to test.
    //! @param b Second value to test.
    //! @param epsilon The approximation tolerance.
    //! @return True if @a a and @a b are approximately equal, false otherwise.
    //!
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    inline bool equal_float(T a, T b, T epsilon = 2 * std::numeric_limits<T>::epsilon())
    {
        const T aa = std::fabs(a);
        const T ab = std::fabs(b);
        return std::fabs(a - b) <= ((aa < ab ? ab : aa) * epsilon);
    }

    //!
    //! This template function checks if a floating point type matches a given IEEE represenation.
    //! @tparam T An floating point type, any size or representation.
    //! @tparam storage_bits Expected size in bits of the type.
    //! @tparam exponent_bits Expected number of bits in the exponent.
    //! @tparam mantissa_bits Expected number of bits in the mantissa.
    //! @return True if @a T matches the expected sizes, false otherwise.
    //!
    template <typename T, int storage_bits, int exponent_bits, int mantissa_bits,
              typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    inline constexpr bool match_ieee_float()
    {
        return
            std::numeric_limits<T>::is_iec559 &&
            sizeof(T) * 8 == storage_bits &&
            (std::numeric_limits<T>::max_exponent - std::numeric_limits<T>::min_exponent >= (1 << (exponent_bits - 1))) &&
            (std::numeric_limits<T>::max_exponent - std::numeric_limits<T>::min_exponent < (1 << exponent_bits)) &&
            std::numeric_limits<T>::digits == mantissa_bits + 1;
    }

    // Fallback for non-floating types.
    //! @cond nodoxygen
    template <typename T, int storage_bits, int exponent_bits, int mantissa_bits,
              typename std::enable_if<!std::is_floating_point<T>::value, int>::type = 0>
    inline constexpr bool match_ieee_float() { return false; }
    //! @endcond

    //!
    //! Definition of an IEEE floating point type with a given represenation.
    //! @tparam storage_bits Expected size in bits of the type.
    //! @tparam exponent_bits Expected number of bits in the exponent.
    //! @tparam mantissa_bits Expected number of bits in the mantissa.
    //!
    template <int storage_bits, int exponent_bits, int mantissa_bits>
    struct ieee_float {
        //!
        //! The corresponding floating type.
        //! When no predefined floating point type matches the requirement,
        //! the generated type is @c void.
        //!
        typedef typename std::conditional<
            match_ieee_float<float, storage_bits, exponent_bits, mantissa_bits>(),
            float,
            typename std::conditional<
                match_ieee_float<double, storage_bits, exponent_bits, mantissa_bits>(),
                double,
                typename std::conditional<
                    match_ieee_float<long double, storage_bits, exponent_bits, mantissa_bits>(),
                    long double,
                    void
                >::type
            >::type
        >::type type;
    };

    //!
    //! 32-bit IEEE floating point type.
    //!
    typedef typename ieee_float<32, 8, 23>::type ieee_float32_t;

    //!
    //! 64-bit IEEE floating point type.
    //!
    typedef typename ieee_float<64, 11, 52>::type ieee_float64_t;

    //!
    //! 80-bit IEEE floating point type.
    //!
    //! Some systems cannot implement this. It is typically only available on Intel platforms.
    //! On other platforms, this type is @a void..
    //!
    typedef typename ieee_float<80, 15, 64>::type ieee_float80_t;

    //!
    //! 128-bit IEEE floating point type.
    //!
    //! Some systems cannot implement this.
    //! GCC and Clang on Intel use a 80-bit floating type for long double.
    //! In that case, this type is @a void.
    //!
    typedef typename ieee_float<128, 15, 112>::type ieee_float128_t;

    //!
    //! Function getting a 32-bit IEEE float from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit IEEE float in big endian representation.
    //! @return The 32-bit IEEE float in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline ieee_float32_t GetFloat32BE(const void* p)
    {
        const uint32_t i = GetUInt32BE(p);
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(undefined-reinterpret-cast)
        return *reinterpret_cast<const ieee_float32_t*>(&i);
        TS_POP_WARNING()
    }

    //!
    //! Function getting a 32-bit IEEE float from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit IEEE float in little endian representation.
    //! @return The 32-bit IEEE float in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline ieee_float32_t GetFloat32LE(const void* p)
    {
        const uint32_t i = GetUInt32LE(p);
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(undefined-reinterpret-cast)
        return *reinterpret_cast<const ieee_float32_t*>(&i);
        TS_POP_WARNING()
    }

    //!
    //! Function getting a 32-bit IEEE float from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 32-bit IEEE float in big endian representation.
    //! @return The 32-bit IEEE float in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline ieee_float32_t GetFloat32(const void* p) { return GetFloat32BE(p); }

    //!
    //! Function getting a 64-bit IEEE float from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit IEEE float in big endian representation.
    //! @return The 64-bit IEEE float in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline ieee_float64_t GetFloat64BE(const void* p)
    {
        const uint64_t i = GetUInt64BE(p);
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(undefined-reinterpret-cast)
        return *reinterpret_cast<const ieee_float64_t*>(&i);
        TS_POP_WARNING()
    }

    //!
    //! Function getting a 64-bit IEEE float from serialized data in little endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit IEEE float in little endian representation.
    //! @return The 64-bit IEEE float in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline ieee_float64_t GetFloat64LE(const void* p)
    {
        const uint64_t i = GetUInt64LE(p);
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(undefined-reinterpret-cast)
        return *reinterpret_cast<const ieee_float64_t*>(&i);
        TS_POP_WARNING()
    }

    //!
    //! Function getting a 64-bit IEEE float from serialized data in big endian representation.
    //!
    //! @param [in] p An address pointing to a 64-bit IEEE float in big endian representation.
    //! @return The 64-bit IEEE float in native byte order, deserialized from @a p.
    //!
    TSDUCKDLL inline ieee_float64_t GetFloat64(const void* p) { return GetFloat64BE(p); }

    //!
    //! Function serializing a 32-bit IEEE float data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit IEEE float.
    //! @param [in]  f The 32-bit IEEE float in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutFloat32BE(void* p, ieee_float32_t f)
    {
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(undefined-reinterpret-cast)
        PutUInt32BE(p, *reinterpret_cast<const uint32_t*>(&f));
        TS_POP_WARNING()
    }

    //!
    //! Function serializing a 32-bit IEEE float data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit IEEE float.
    //! @param [in]  f The 32-bit IEEE float in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutFloat32LE(void* p, ieee_float32_t f)
    {
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(undefined-reinterpret-cast)
        PutUInt32LE(p, *reinterpret_cast<const uint32_t*>(&f));
        TS_POP_WARNING()
    }

    //!
    //! Function serializing a 32-bit IEEE float data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 32-bit IEEE float.
    //! @param [in]  f The 32-bit IEEE float in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutFloat32(void* p, ieee_float32_t f) { PutFloat32BE(p, f); }

    //!
    //! Function serializing a 64-bit IEEE float data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit IEEE float.
    //! @param [in]  f The 64-bit IEEE float in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutFloat64BE(void* p, ieee_float64_t f)
    {
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(undefined-reinterpret-cast)
        PutUInt64BE(p, *reinterpret_cast<const uint64_t*>(&f));
        TS_POP_WARNING()
    }

    //!
    //! Function serializing a 64-bit IEEE float data in little endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit IEEE float.
    //! @param [in]  f The 64-bit IEEE float in native byte order to serialize in little endian representation.
    //!
    TSDUCKDLL inline void PutFloat64LE(void* p, ieee_float64_t f)
    {
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(undefined-reinterpret-cast)
        PutUInt64LE(p, *reinterpret_cast<const uint64_t*>(&f));
        TS_POP_WARNING()
    }

    //!
    //! Function serializing a 64-bit IEEE float data in big endian representation.
    //!
    //! @param [out] p An address where to serialize the 64-bit IEEE float.
    //! @param [in]  f The 64-bit IEEE float in native byte order to serialize in big endian representation.
    //!
    TSDUCKDLL inline void PutFloat64(void* p, ieee_float64_t f) { PutFloat64BE(p, f); }
}
