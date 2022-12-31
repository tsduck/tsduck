//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
        //! Get a random integer value.
        //! @tparam INT An integer type for the result.
        //! @param [out] value The return integer value.
        //! @return True on success, false on error.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        bool readInt(INT& value)
        {
            return read(&value, sizeof(value));
        }

        //!
        //! Virtual destructor
        //!
        virtual ~RandomGenerator();
    };
}
