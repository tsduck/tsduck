//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for PRNG's (pseudo-random numbers generators)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Abstract base class for PRNG's (pseudo-random numbers generators).
    //! @ingroup crypto
    //!
    class TSDUCKDLL RandomGenerator
    {
    public:
        //!
        //! Algorithm name (informational only).
        //! @return The algorithm name.
        //!
        virtual UString name() const = 0;

        //!
        //! Seed (add entropy) to the PRNG.
        //! @param [in] addr Address of entropy data.
        //! @param [in] size Size in bytes of entropy data.
        //! @return True on success, false on error.
        //!
        virtual bool seed(const void* addr, size_t size) = 0;

        //!
        //! Check if the PRNG is ready.
        //! If not ready, it must be seeded again.
        //! @return True if the PRNG is ready.
        //!
        virtual bool ready() const = 0;

        //!
        //! Get random data.
        //! @param [out] addr Address of returned random data.
        //! @param [in] size Requested size in bytes of random data.
        //! The method always fill the buffer.
        //! @return True on success, false on error.
        //!
        virtual bool read(void* addr, size_t size) = 0;

        //!
        //! Get random data in a byte block.
        //! The default implementation uses read().
        //! @param [out] data Returned random data.
        //! @param [in] size Requested size in bytes of random data.
        //! @return True on success, false on error.
        //!
        virtual bool readByteBlock(ByteBlock& data, size_t size);

        //!
        //! Get a random integer value inside a defined range.
        //! @tparam INT_T An integer type for the result.
        //! @param [out] value The return integer value.
        //! @param [in] min Minimum value to return.
        //! @param [in] max Maximum value to return.
        //! @return True on success, false on error.
        //!
        template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value>::type* = nullptr>
        bool readInt(INT_T& value, INT_T min = std::numeric_limits<INT_T>::min(), INT_T max = std::numeric_limits<INT_T>::max());

        //!
        //! Virtual destructor
        //!
        virtual ~RandomGenerator();
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

template <typename INT_T, typename std::enable_if<std::is_integral<INT_T>::value>::type*>
bool ts::RandomGenerator::readInt(INT_T& value, INT_T min, INT_T max)
{
    if (min > max) {
        return false;
    }
    const bool ok = read(&value, sizeof(value));
    if (ok && (value < min || value > max)) {
        typedef typename std::make_unsigned<INT_T>::type UINT_T;
        const UINT_T top = UINT_T(max - min + 1);
        value = INT_T(UINT_T(value) % top) + min;
    }
    return ok;
}
