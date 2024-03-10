//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
        SHA256();                                   //!< Constructor
        static constexpr size_t HASH_SIZE = 256/8;  //!< SHA-256 hash size in bytes (256 bits).

        // Implementation of Hash interface:
        virtual UString name() const override;
        virtual size_t hashSize() const override;

    protected:
        //! @cond nodoxygen
#if defined(TS_WINDOWS)
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const override;
#else
        virtual const EVP_MD_CTX* referenceContext() const override;
#endif
        //! @endcond
    };
}
