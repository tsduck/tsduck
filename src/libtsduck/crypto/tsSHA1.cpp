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

::LPCWSTR ts::SHA1::algorithmId() const
{
    return BCRYPT_SHA1_ALGORITHM;
}

#else

TS_STATIC_INSTANCE(ts::PresetHashContext, ("SHA1"), Preset);

const EVP_MD_CTX* ts::SHA1::referenceContext() const
{
    return Preset::Instance().context();
}

#endif
