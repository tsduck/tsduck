//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsXoshiro256ss.h"


//----------------------------------------------------------------------------
// Custom portable ROL64. On Arm64, there are some cases of incorrect code
// generation with the one from tsRotate.h.
//----------------------------------------------------------------------------

namespace {
    inline uint64_t c_rol64(uint64_t x, int k)
    {
        return (x << k) | (x >> (64 - k));
    }
}


//----------------------------------------------------------------------------
// Return to initial state, not seeded.
//----------------------------------------------------------------------------

void ts::Xoshiro256ss::reset()
{
    _seeded = false;
    _next_seed = 0;
    TS_ZERO(_state);
}


//----------------------------------------------------------------------------
// Implementation of RandomGenerator interface:
//----------------------------------------------------------------------------

ts::UString ts::Xoshiro256ss::name() const
{
    return u"Xoshiro256**";
}

bool ts::Xoshiro256ss::ready() const
{
    return _seeded && (_state[0] != 0 || _state[1] != 0 || _state[2] != 0 || _state[3] != 0);
}


//----------------------------------------------------------------------------
// Fast non-virtual generation of 64-bit random.
//----------------------------------------------------------------------------

uint64_t ts::Xoshiro256ss::read64()
{
    const uint64_t result = c_rol64(_state[1] * 5, 7) * 9;
    const uint64_t t = _state[1] << 17;

    _state[2] ^= _state[0];
    _state[3] ^= _state[1];
    _state[1] ^= _state[2];
    _state[0] ^= _state[3];

    _state[2] ^= t;
    _state[3] = c_rol64(_state[3], 45);

    return result;
}


//----------------------------------------------------------------------------
// Seed (add entropy). Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::Xoshiro256ss::seed(const void* data, size_t size)
{
    if (data != nullptr && size > 0) {

        const uint8_t* in = reinterpret_cast<const uint8_t*>(data);
        uint8_t* out = reinterpret_cast<uint8_t*>(_state);
        constexpr size_t max = sizeof(_state);
        assert(_next_seed < max);

        while (size > 0) {
            const size_t chunk = std::min(size, max - _next_seed);
            std::memcpy(out + _next_seed, in, chunk);
            size -= chunk;
            in += chunk;
            _next_seed += chunk;
            if (_next_seed >= max) {
                _seeded = true;
                _next_seed = 0;
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Get random data. Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::Xoshiro256ss::read(void* buffer, size_t size)
{
    if (!_seeded || buffer == nullptr) {
        return false;
    }

    // Read full 64-bit chunks.
    uint64_t* out64 = reinterpret_cast<uint64_t*>(buffer);
    while (size >= sizeof(uint64_t)) {
        *out64++ = read64();
        size -= sizeof(uint64_t);
    }

    // Remaining bytes.
    if (size > 0) {
        const uint64_t last = read64();
        std::memcpy(out64, &last, size);
    }
    return true;
}
