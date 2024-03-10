//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  SHA-1 hash.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsHash.h"

namespace ts {
    //!
    //! SHA-1 hash.
    //! @ingroup crypto
    //!
    class TSDUCKDLL SHA1: public Hash
    {
        TS_NOCOPY(SHA1);
    public:
        //! SHA-1 hash size in bytes (160 bits).
        static constexpr size_t HASH_SIZE = 160/8;

        // Implementation of Hash interface:
        virtual UString name() const override;
        virtual size_t hashSize() const override;

#if !defined(TS_WINDOWS)
        virtual bool init() override;
        virtual bool add(const void* data, size_t size) override;
        virtual bool getHash(void* hash, size_t bufsize, size_t* retsize = nullptr) override;
#endif

        //! Constructor
        SHA1();

    protected:
#if defined(TS_WINDOWS) && !defined(DOXYGEN)
        virtual ::LPCWSTR algorithmId() const override;
#endif

    private:
#if !defined(TS_WINDOWS)
        static constexpr size_t BLOCK_SIZE = 64;  //!< SHA-1 block size in bytes.

        uint64_t _length = 0;                // Total message size in bits (already hashed, ie. excluding _buf)
        size_t   _curlen = 0;                // Used bytes in _buf
        uint32_t _state[HASH_SIZE / 4] {};   // Current hash value (160 bits)
        uint8_t  _buf[BLOCK_SIZE] {};        // Current block to hash (512 bits)

        // Compress one 512-bit block, accumulate hash in _state.
        void compress(const uint8_t* buf);

        // Runtime check once if accelerated SHA-1 instructions are supported on this CPU.
        static volatile bool _accel_checked;
        static volatile bool _accel_supported;

        // Accelerated versions, compiled in a separated module.
        static void initAccel();
        void compressAccel(const uint8_t* buf);
#endif
    };
}
