//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSHA512.h"
#include "tsFetchAlgorithm.h"

ts::SHA512::~SHA512()
{
}

#if defined(TS_WINDOWS)

void ts::SHA512::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_SHA512_ALGORITHM);
    fetch.getAlgorithm(algo, length);
}

#elif !defined(TS_NO_OPENSSL)

const EVP_MD_CTX* ts::SHA512::referenceContext() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchHashAlgorithm fetch("SHA512");
    return fetch.referenceContext();
}

#endif
