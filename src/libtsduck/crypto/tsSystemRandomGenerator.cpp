//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSystemRandomGenerator.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::SystemRandomGenerator::SystemRandomGenerator()
{
#if defined(TS_WINDOWS)
    if (!::CryptAcquireContext(&_prov, nullptr, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET) &&
        !::CryptAcquireContext(&_prov, nullptr, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET))
    {
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
