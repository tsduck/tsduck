//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Reproducible PRNG (pseudo-random numbers generator) based on the seed.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsRandomGenerator.h"
#include "tsSHA256.h"

namespace ts {
    //!
    //! Reproducible PRNG (pseudo-random numbers generator) based on the seed.
    //! There is no external source of entropy. The same sequences of seed() and read()
    //! always return the same pseudo-random data.
    //! @ingroup crypto
    //!
    //! State is 32 bytes, state = state1 || state2, with state1 = state[0..15] and state2 = state[15..31].
    //!
    //! Initial step:
    //! - Load initial seed, minimum 64 bytes, multiple steps if necessary.
    //! - state = SHA-256 (seed)
    //! Generation loop:
    //! - state1 = AES-128[key=state2] (state1)
    //! - read bytes from state1
    //! - when state1 fully read, state = SHA-256 (state)
    //! On re-seed:
    //! - state = SHA-256 (seed || state)
    //! - rewind byte generation
    //!
    class TSDUCKDLL ReproducibleRandomGenerator: public RandomGenerator
    {
        TS_NOCOPY(ReproducibleRandomGenerator);
    public:
        //!
        //! Minimal initial accumulated seed size.
        //!
        static constexpr size_t MIN_SEED_SIZE = SHA256::BLOCK_SIZE;

        //!
        //! Constructor.
        //!
        ReproducibleRandomGenerator() = default;

        //!
        //! Return to initial state, not seeded.
        //!
        void reset();

        // Implementation of RandomGenerator interface:
        virtual UString name() const override;
        virtual bool seed(const void*, size_t) override;
        virtual bool ready() const override;
        virtual bool read(void*, size_t) override;

    private:
        static constexpr size_t STATE_SIZE = SHA256::HASH_SIZE;
        static constexpr size_t STATE1_SIZE = STATE_SIZE / 2;
        static constexpr size_t STATE2_SIZE = STATE_SIZE / 2;

        bool    _success = true;
        bool    _ready = false;
        size_t  _init_seed_size = 0;
        SHA256  _sha {};
        uint8_t _state[STATE_SIZE] {};
        size_t  _next = 0;
    };
}
