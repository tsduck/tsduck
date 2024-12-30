//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSHA512.h"
#include "tsSingleton.h"
#include "tsInitCryptoLibrary.h"

#if defined(TS_WINDOWS)

void ts::SHA512::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_SHA512_ALGORITHM);
    fetch.getAlgorithm(algo, length);
}

#else

// The singleton needs to be destroyed no later that OpenSSL cleanup.
TS_STATIC_INSTANCE_ATEXIT(const, ts::FetchHashAlgorithm, Preset, ("SHA512"), OPENSSL_atexit);

const EVP_MD_CTX* ts::SHA512::referenceContext() const
{
    return Preset->referenceContext();
}

#endif
