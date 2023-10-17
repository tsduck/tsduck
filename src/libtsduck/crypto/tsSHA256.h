//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  SHA-256 hash.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsHash.h"

namespace ts {
    //!
    //! SHA-256 hash.
    //! @ingroup crypto
    //!
    class TSDUCKDLL SHA256: public Hash
    {
        TS_NOCOPY(SHA256);
    public:
        // Sizes in bytes
        static constexpr size_t HASH_SIZE  = 32;  //!< SHA-256 hash size in bytes.
        static constexpr size_t BLOCK_SIZE = 64;  //!< SHA-256 block size in bytes.

        // Implementation of Hash interface:
        virtual UString name() const override;
        virtual size_t hashSize() const override;
        virtual size_t blockSize() const override;
        virtual bool init() override;
        virtual bool add(const void* data, size_t size) override;
        virtual bool getHash(void* hash, size_t bufsize, size_t* retsize = nullptr) override;

        //! Constructor
        SHA256();

    private:
        uint64_t _length = 0;               // Total message size in bits (already hashed, ie. excluding _buf)
        size_t   _curlen = 0;               // Used bytes in _buf
        uint32_t _state[HASH_SIZE / 4] {};  // Current hash value (256 bits, 32 bytes, 8 uint32)
        uint8_t  _buf[BLOCK_SIZE] {};       // Current block to hash (512 bits)

        // The K array
        static const uint32_t K[64];

        // Compress one 512-bit block, accumulate hash in _state.
        void compress(const uint8_t* buf);

        // Runtime check once if accelerated SHA-256 instructions are supported on this CPU.
        static volatile bool _accel_checked;
        static volatile bool _accel_supported;

        // Accelerated versions, compiled in a separated module.
        void compressAccel(const uint8_t* buf);
    };
}
