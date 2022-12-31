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
//!  Improved system-provided PRNG (pseudo-random numbers generator).
//!  Use SystemRandomGenerator as base and add AES-based post-processing.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSystemRandomGenerator.h"
#include "tsSingletonManager.h"
#include "tsReport.h"
#include "tsByteBlock.h"
#include "tsUString.h"
#include "tsMutex.h"
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
        Report*       _report;      // Where to report errors.
        mutable Mutex _mutex;       // Exclusive access to singleton.
        bool          _ready;       // Fully initialized.
        UString       _state_file;  // Name of the entropy state file.
        AES           _aes;         // AES engine.
        SHA256        _sha;         // SHA-256 engine.
        size_t        _index;       // Next index in _pool.
        ByteBlock     _state;       // Entropy state.
        ByteBlock     _pool;        // Random data pool.

        // Update the content of the random pool with new data.
        bool updatePool();
    };
}
