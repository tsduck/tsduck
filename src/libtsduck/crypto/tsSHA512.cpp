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


//----------------------------------------------------------------------------
// Implementation of Hash interface:
//----------------------------------------------------------------------------

ts::SHA512::SHA512()
{
    InitCryptographicLibrary();
}

ts::UString ts::SHA512::name() const
{
    return u"SHA-512";
}

size_t ts::SHA512::hashSize() const
{
    return HASH_SIZE;
}


//----------------------------------------------------------------------------
// System-specific implementation.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

::LPCWSTR ts::SHA512::algorithmId() const
{
    return BCRYPT_SHA512_ALGORITHM;
}

#else

TS_STATIC_INSTANCE(ts::PresetHashContext, ("SHA512"), Preset);

const EVP_MD_CTX* ts::SHA512::referenceContext() const
{
    return Preset::Instance().context();
}

#endif
