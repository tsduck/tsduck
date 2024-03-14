//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  SHA-512 hash.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsHash.h"

namespace ts {
    //!
    //! SHA-512 hash.
    //! @ingroup crypto
    //!
    class TSDUCKDLL SHA512: public Hash
    {
        TS_NOCOPY(SHA512);
    public:
        //!
        //! Constructor.
        //!
        SHA512() : Hash(u"SHA-512", HASH_SIZE) {}

        //!
        //! SHA-512 hash size in bytes (512 bits).
        //!
        static constexpr size_t HASH_SIZE = 512/8;

    protected:
#if defined(TS_WINDOWS)
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const override;
#else
        virtual const EVP_MD_CTX* referenceContext() const override;
#endif
    };
}
