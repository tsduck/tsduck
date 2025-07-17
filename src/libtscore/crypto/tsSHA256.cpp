//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSHA256.h"
#include "tsFetchAlgorithm.h"

ts::SHA256::~SHA256()
{
}

#if defined(TS_WINDOWS)

void ts::SHA256::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_SHA256_ALGORITHM);
    fetch.getAlgorithm(algo, length);
}

#elif !defined(TS_NO_OPENSSL)

const EVP_MD_CTX* ts::SHA256::referenceContext() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchHashAlgorithm fetch("SHA256");
    return fetch.referenceContext();
}

#endif
