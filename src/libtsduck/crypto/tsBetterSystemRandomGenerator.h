//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
#include "tsSingleton.h"
#include "tsReport.h"
#include "tsByteBlock.h"
#include "tsUString.h"
#include "tsSHA256.h"
#include "tsAES.h"

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
    //! - R2 = AES[K] R1
    //! - R3 = R2 xor state
    //! - R4 = AES[K] R3
    //! - R4 ==> output of BetterSystemRandomGenerator
    //! - R5 = read SystemRandomGenerator
    //! - state = SHA-256 (R5 xor R4 xor state)
    //!
    //! Known limitations:
    //! - The entropy file is rewritten after each block => poor performances.
    //! - Concurrent processes overwrite the same .tsseed file.
    //!
    class TSDUCKDLL BetterSystemRandomGenerator: public SystemRandomGenerator
    {
        TS_DECLARE_SINGLETON(BetterSystemRandomGenerator);

    public:
        // Implementation of RandomGenerator interface:
        virtual UString name() const override;
        virtual bool ready() const override;
        virtual bool read(void*, size_t) override;

    private:
        mutable std::recursive_mutex _mutex {};
        Report*   _report = nullptr;  // Where to report errors.
        bool      _ready = true;      // Fully initialized.
        UString   _state_file;        // Name of the entropy state file.
        AES       _aes {};            // AES engine.
        SHA256    _sha {};            // SHA-256 engine.
        size_t    _index;             // Next index in _pool.
        ByteBlock _state {};          // Entropy state.
        ByteBlock _pool;              // Random data pool.

        // Update the content of the random pool with new data.
        bool updatePool();
    };
}
