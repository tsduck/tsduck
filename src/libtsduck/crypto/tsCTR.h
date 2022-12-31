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
//!  Counter (CTR) chaining mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCipherChaining.h"

namespace ts {
    //!
    //! Counter (CTR) chaining mode.
    //! @ingroup crypto
    //!
    //! CTR can process a residue. The plain text and cipher text can have any size.
    //!
    //! @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template <class CIPHER>
    class CTR: public CipherChainingTemplate<CIPHER>
    {
        TS_NOCOPY(CTR);
    public:
        //!
        //! Constructor.
        //! @param [in] counter_bits Number of bits of the counter part in the IV.
        //! See setCounterBits() for an explanation of this value.
        //!
        CTR(size_t counter_bits = 0);

        //!
        //! Set the size of the counter part in the IV.
        //! @param [in] counter_bits Number of bits of the counter part in the IV.
        //! In CTR mode, the IV is considered as an integer in big-endian representation.
        //! The counter part of the IV uses the least significant bits of the IV.
        //! The default value (when specified as zero) is half the size of the IV.
        //!
        void setCounterBits(size_t counter_bits);

        //!
        //! Get the size of the counter part in the IV.
        //! @return Number of bits of the counter part in the IV.
        //! See setCounterBits() for an explanation of this value.
        //!
        size_t counterBits() const {return _counter_bits;}

        // Implementation of CipherChaining interface.
        virtual size_t minMessageSize() const override;
        virtual bool residueAllowed() const override;

        // Implementation of BlockCipher interface.
        virtual UString name() const override;

    protected:
        // Implementation of BlockCipher interface.
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;

    private:
        size_t _counter_bits; // size in bits of the counter part.

        // We need two work blocks.
        // The first one contains the "input block" or counter.
        // The second one contains the "output block", the encrypted counter.
        // This private method increments the counter block.
        bool incrementCounter();
    };
}

#include "tsCTRTemplate.h"
