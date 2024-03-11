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


//----------------------------------------------------------------------------
// Implementation of Hash interface:
//----------------------------------------------------------------------------

ts::SHA1::SHA1()
{
    InitCryptographicLibrary();
}

ts::UString ts::SHA1::name() const
{
    return u"SHA-1";
}

size_t ts::SHA1::hashSize() const
{
    return HASH_SIZE;
}


//----------------------------------------------------------------------------
// System-specific implementation.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(ts::FetchBCryptAlgorithm, (BCRYPT_SHA1_ALGORITHM), Fetch);

void ts::SHA1::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    Fetch::Instance().getAlgorithm(algo, length);
}

#else

TS_STATIC_INSTANCE(ts::FetchHashAlgorithm, ("SHA1"), Preset);

const EVP_MD_CTX* ts::SHA1::referenceContext() const
{
    return Preset::Instance().referenceContext();
}

#endif
