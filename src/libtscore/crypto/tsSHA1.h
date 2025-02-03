//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
    //! @ingroup libtscore crypto
    //!
    class TSCOREDLL SHA1: public Hash
    {
        TS_NOCOPY(SHA1);
    public:
        SHA1() : Hash(u"SHA-1", HASH_SIZE) {}  //!< Constructor.
        virtual ~SHA1() override;              //!< Destructor.

        //!
        //! SHA-1 hash size in bytes (160 bits).
        //!
        static constexpr size_t HASH_SIZE = 160/8;

    protected:
#if defined(TS_WINDOWS)
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const override;
#elif !defined(TS_NO_OPENSSL)
        virtual const EVP_MD_CTX* referenceContext() const override;
#endif
    };
}
