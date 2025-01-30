//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Improved system-provided PRNG (pseudo-random numbers generator).
//!  Use SystemRandomGenerator as base and add AES-based post-processing.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSystemRandomGenerator.h"
#include "tsReport.h"
#include "tsByteBlock.h"
#include "tsUString.h"
#include "tsSHA256.h"
#include "tsAES128.h"

namespace ts {
    //!
    //! Improved system-provided PRNG (pseudo-random numbers generator).
    //! @ingroup crypto
    //!
    //! Use SystemRandomGenerator as base and add AES-based post-processing.
    //! Implemented as a thread-safe singleton.
    //!
    //! An entropy state is stored in $HOME/.tsseed. The value of a @e state
    //! is initially loaded from this file. The first time the generator is
    //! used (no file), the @e state is loaded from the system PRNG.
    //!
    //! A fixed AES-128 key @e K is used for the post-processing. The size
    //! of the @e state is 16 bytes, the AES block size.
    //!
    //! Description of post-processing, added to the sytem PRNG:
    //! - R1 = read SystemRandomGenerator
    //! - R2 = AES-128[K] (R1)
    //! - R3 = R2 xor state
    //! - R4 = AES-128[K] (R3)
    //! - R4 ==> output of BetterSystemRandomGenerator
    //! - R5 = read SystemRandomGenerator
    //! - state = SHA-256 (R5 xor R4 xor state)
    //!
    //! Known limitations:
    //! - The entropy file is rewritten after each block => poor performances.
    //! - Concurrent processes overwrite the same .tsseed file.
    //!
    //! Additional notes:
    //! - On UNIX systems, is TSDuck is built with option NOOPENSSL, the random
    //!   is directly extracted from SystemRandomGenerator.
    //!
    class TSCOREDLL BetterSystemRandomGenerator: public SystemRandomGenerator
    {
        TS_SINGLETON(BetterSystemRandomGenerator);
    public:
        //! Destructor.
        virtual ~BetterSystemRandomGenerator() override;

#if !defined(TS_NO_CRYPTO_LIBRARY)

        // Implementation of RandomGenerator interface:
        virtual UString name() const override;
        virtual bool ready() const override;
        virtual bool read(void*, size_t) override;

    private:
        mutable std::recursive_mutex _mutex {};
        Report*   _report = nullptr;  // Where to report errors.
        bool      _ready = true;      // Fully initialized.
        UString   _state_file;        // Name of the entropy state file.
        AES128    _aes {};            // AES engine.
        SHA256    _sha {};            // SHA-256 engine.
        size_t    _index;             // Next index in _pool.
        ByteBlock _state {};          // Entropy state.
        ByteBlock _pool;              // Random data pool.

        // Update the content of the random pool with new data.
        bool updatePool();

#endif
    };
}
