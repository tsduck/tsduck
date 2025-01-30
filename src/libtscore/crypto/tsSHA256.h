//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
    class TSCOREDLL SHA256: public Hash
    {
        TS_NOCOPY(SHA256);
    public:
        SHA256() : Hash(u"SHA-256", HASH_SIZE) {}  //!< Constructor.
        virtual ~SHA256() override;                //!< Destructor.

        //!
        //! SHA-256 hash size in bytes (256 bits).
        //!
        static constexpr size_t HASH_SIZE = 256/8;

    protected:
#if defined(TS_WINDOWS)
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const override;
#elif !defined(TS_NO_OPENSSL)
        virtual const EVP_MD_CTX* referenceContext() const override;
#endif
    };
}
