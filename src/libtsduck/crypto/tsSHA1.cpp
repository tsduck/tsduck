//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSHA1.h"
#include "tsInitCryptoLibrary.h"

#if defined(TS_WINDOWS)

void ts::SHA1::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_SHA1_ALGORITHM);
    fetch.getAlgorithm(algo, length);
}

#else

const EVP_MD_CTX* ts::SHA1::referenceContext() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchHashAlgorithm fetch("SHA1");
    return fetch.referenceContext();
}

#endif
