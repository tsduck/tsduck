//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSHA1.h"
#include "tsSingleton.h"
#include "tsInitCryptoLibrary.h"

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(, ts::FetchBCryptAlgorithm, Fetch, (BCRYPT_SHA1_ALGORITHM));

void ts::SHA1::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    Fetch->getAlgorithm(algo, length);
}

#else

// The singleton needs to be destroyed no later that OpenSSL cleanup.
TS_STATIC_INSTANCE_ATEXIT(const, ts::FetchHashAlgorithm, Preset, ("SHA1"), OPENSSL_atexit);

const EVP_MD_CTX* ts::SHA1::referenceContext() const
{
    return Preset->referenceContext();
}

#endif
