//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Xoshiro256** PRNG (pseudo-random numbers generator).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsRandomGenerator.h"

namespace ts {
    //!
    //! Xoshiro256** PRNG (pseudo-random numbers generator).
    //! @ingroup libtscore crypto
    //! @see https://en.wikipedia.org/wiki/Xorshift
    //!
    class TSCOREDLL Xoshiro256ss: public RandomGenerator
    {
        TS_NOCOPY(Xoshiro256ss);
    public:
        //!
        //! Minimal initial accumulated seed size.
        //!
        static constexpr size_t MIN_SEED_SIZE = 32;

        //!
        //! Constructor.
        //!
        Xoshiro256ss() = default;

        //!
        //! Return to initial state, not seeded.
        //!
        void reset();

        //!
        //! Fast non-virtual generation of 64-bit random (not part of RandomGenerator interface).
        //! @return 64-bit random value.
        //!
        uint64_t read64();

        // Implementation of RandomGenerator interface:
        virtual UString name() const override;
        virtual bool seed(const void*, size_t) override;
        virtual bool ready() const override;
        virtual bool read(void*, size_t) override;

    private:
        bool     _seeded = false;
        size_t   _next_seed = 0;
        uint64_t _state[4] {0, 0, 0, 0};
    };
}
