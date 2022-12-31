//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsSystemRandomGenerator.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::SystemRandomGenerator::SystemRandomGenerator() :
#if defined(TS_WINDOWS)
    _prov(0)
#else
    _fd(-1)
#endif
{
#if defined(TS_WINDOWS)
    if (!::CryptAcquireContext(&_prov, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET) &&
        !::CryptAcquireContext(&_prov, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET)) {
        _prov = 0;
    }
#else
    if ((_fd = ::open("/dev/urandom", O_RDONLY)) < 0) {  // Flawfinder: ignore: open()
        _fd = ::open("/dev/random", O_RDONLY);           // Flawfinder: ignore: open()
    }
#endif
}


ts::SystemRandomGenerator::~SystemRandomGenerator()
{
#if defined(TS_WINDOWS)
    if (_prov != 0) {
        ::CryptReleaseContext(_prov, 0);
        _prov = 0;
    }
#else
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }
#endif
}


//----------------------------------------------------------------------------
// Implementation of RandomGenerator interface:
//----------------------------------------------------------------------------

ts::UString ts::SystemRandomGenerator::name() const
{
    return u"SystemRandomGenerator";
}


//----------------------------------------------------------------------------
// Seed (add entropy). Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::SystemRandomGenerator::seed(const void* data, size_t size)
{
    // System random generators do not need to be seeded
    return true;
}


//----------------------------------------------------------------------------
// Check if PRNG is ready. If not ready, must be seeded again.
//----------------------------------------------------------------------------

bool ts::SystemRandomGenerator::ready() const
{
#if defined(TS_WINDOWS)
    return _prov != 0;
#else
    return _fd >= 0;
#endif
}


//----------------------------------------------------------------------------
// Get random data. Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::SystemRandomGenerator::read(void* buffer, size_t size)
{
    // Always succeed when size is zero. Some PRNG return an error when zero
    // is requested. For instance, with a zero size, the system PRNG of
    // Windows 7 succeeds while Windows 10 fails.
    if (size == 0) {
        return true;
    }

#if defined(TS_WINDOWS)
    return _prov != 0 && ::CryptGenRandom(_prov, ::DWORD(size), reinterpret_cast<::BYTE*>(buffer));
#else
    bool ok = _fd >= 0;
    char* data = reinterpret_cast<char*>(buffer);
    while (ok && size > 0) {
        ssize_t insize = ::read(_fd, data, size);
        if ((ok = insize > 0)) {
            assert(size_t(insize) <= size);
            size -= insize;
            data += insize;
        }
    }
    return ok;
#endif
}
