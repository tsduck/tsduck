//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReproducibleRandomGenerator.h"
#include "tsAES.h"


//----------------------------------------------------------------------------
// Return to initial state, not seeded.
//----------------------------------------------------------------------------


void ts::ReproducibleRandomGenerator::reset()
{
    _ready = false;
    _init_seed_size = 0;
    _success = _sha.init();
}


//----------------------------------------------------------------------------
// Implementation of RandomGenerator interface:
//----------------------------------------------------------------------------

ts::UString ts::ReproducibleRandomGenerator::name() const
{
    return u"ReproducibleRandomGenerator";
}

bool ts::ReproducibleRandomGenerator::ready() const
{
    return _success && _ready;
}


//----------------------------------------------------------------------------
// Seed (add entropy). Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::ReproducibleRandomGenerator::seed(const void* data, size_t size)
{
    if (_success) {
        if (!_ready) {
            // Initial phase.
            _success = _sha.add(data, size);
            _init_seed_size += size;
            _ready = _init_seed_size >= MIN_SEED_SIZE;
            if (_success && _ready) {
                _success = _sha.getHash(_state, sizeof(_state));
                _next = STATE1_SIZE;  // next byte to read in state1 is here after end
            }
        }
        else {
            // Re-seed after initial phase.
            _success = _sha.init() && _sha.add(data, size) && _sha.add(_state, sizeof(_state)) && _sha.getHash(_state, sizeof(_state));
            _next = STATE1_SIZE;  // next byte to read in state1 is here after end
        }
    }
    return _success;
}


//----------------------------------------------------------------------------
// Get random data. Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::ReproducibleRandomGenerator::read(void* buffer, size_t size)
{
    if (!_success || !_ready || buffer == nullptr) {
        return false;
    }

    uint8_t* out = reinterpret_cast<uint8_t*>(buffer);

    while (_success && size > 0) {
        // state1 = AES-128[key=state2] (state1)
        if (_next >= STATE1_SIZE) {
            AES aes;
            _success = aes.setKey(_state + STATE1_SIZE, STATE2_SIZE) && aes.encryptInPlace(_state, STATE1_SIZE);
            _next = 0; // next byte to read in state1
        }

        // read bytes from state1
        const size_t chunk_size = std::min(size, STATE1_SIZE - _next);
        ::memcpy(out, _state + _next, chunk_size);
        out += chunk_size;
        size -= chunk_size;
        _next += chunk_size;

        // when state1 fully read, state = SHA-256 (state)
        if (_next >= STATE1_SIZE) {
            _success = _sha.init() && _sha.add(_state, sizeof(_state)) && _sha.getHash(_state, sizeof(_state));
        }
    }

    return _success;
}
