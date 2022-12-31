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

#include "tsBetterSystemRandomGenerator.h"
#include "tsSingletonManager.h"
#include "tsNullReport.h"
#include "tsFileUtils.h"
#include "tsGuardMutex.h"

// Define singleton instance
TS_DEFINE_SINGLETON(ts::BetterSystemRandomGenerator);

// A fixed AES-128 key for post-processing.
namespace {
    const uint8_t _fixedKey[] = {
        0x68, 0xA3, 0xA1, 0xE0, 0x68, 0x89, 0x7F, 0x9A, 0x05, 0xD5, 0x90, 0xDC, 0xD9, 0x0D, 0x70, 0x4F,
    };
}


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::BetterSystemRandomGenerator::BetterSystemRandomGenerator() :
    SystemRandomGenerator(),
    _report(nullptr),
    _mutex(),
    _ready(true),
    _state_file(UserHomeDirectory() + PathSeparator + u".tsseed"),
    _aes(),
    _sha(),
    _index(AES::BLOCK_SIZE), // at end of block, will need update
    _state(),
    _pool(AES::BLOCK_SIZE)
{
    // Read the previous content of the seed file.
    if (!_state.loadFromFile(_state_file, AES::BLOCK_SIZE) || _state.size() != AES::BLOCK_SIZE) {
        // Can't read seed file, maybe first access, read random data and writes it once.
        _state.resize(AES::BLOCK_SIZE);
        if (SystemRandomGenerator::read(_state.data(), _state.size())) {
            // Got new random data, write seed file. Do not check errors here (?)
            _state.saveToFile(_state_file);
        }
        else {
            // Could not get random data from system PRNG.
            _ready = false;
        }
    }

    // Initialize the AES engine with our fixed key.
    if (!_aes.setKey(_fixedKey, sizeof(_fixedKey))) {
        _ready = false;
    }
}


//----------------------------------------------------------------------------
// Implementation of RandomGenerator interface:
//----------------------------------------------------------------------------

ts::UString ts::BetterSystemRandomGenerator::name() const
{
    return u"BetterSystemRandomGenerator";
}


//----------------------------------------------------------------------------
// Check if PRNG is ready. If not ready, must be seeded again.
//----------------------------------------------------------------------------

bool ts::BetterSystemRandomGenerator::ready() const
{
    GuardMutex lock(_mutex);
    return _ready && SystemRandomGenerator::ready();
}


//----------------------------------------------------------------------------
// Update the content of the random pool with new data.
//----------------------------------------------------------------------------

bool ts::BetterSystemRandomGenerator::updatePool()
{
    uint8_t r1[AES::BLOCK_SIZE];
    uint8_t r2[AES::BLOCK_SIZE];

    // R1 = read SystemRandomGenerator
    // R2 = AES[K] R1
    if (!SystemRandomGenerator::read(r1, sizeof(r1)) || !_aes.encrypt(r1, sizeof(r1), r2, sizeof(r2))) {
        return false;
    }

    // R1 = R2 xor state
    for (size_t i = 0; i < AES::BLOCK_SIZE; ++i) {
        r1[i] = r2[i] ^ _state[i];
    }

    // pool = AES[K] R1 ==> output of BetterSystemRandomGenerator
    assert(_pool.size() == AES::BLOCK_SIZE);
    if (!_aes.encrypt(r1, sizeof(r1), _pool.data(), _pool.size())) {
        return false;
    }

    // R1 = read SystemRandomGenerator
    if (!SystemRandomGenerator::read(r1, sizeof(r1))) {
        return false;
    }

    // R2 = R1 xor pool xor state.
    for (size_t i = 0; i < AES::BLOCK_SIZE; ++i) {
        r2[i] = r1[i] ^ _pool[i] ^ _state[i];
    }

    // h = SHA-256 (R2)
    uint8_t h[SHA256::HASH_SIZE];
    if (!_sha.hash(r2, sizeof(r2), h, sizeof(h))) {
        return false;
    }

    // state = truncated h
    assert(_state.size() <= sizeof(h));
    _state.copy(h, AES::BLOCK_SIZE);

    // Save state file.
    if (!_state.saveToFile(_state_file, _report)) {
        return false;
    }

    // Now _pool is ready with new random data.
    _index = 0;
    return true;
}


//----------------------------------------------------------------------------
// Get random data. Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::BetterSystemRandomGenerator::read(void* buffer, size_t size)
{
    GuardMutex lock(_mutex);

    // Filter trivial cases.
    if (!_ready || buffer == nullptr) {
        return false;
    }

    // Cast buffer boundaries.
    uint8_t* data = reinterpret_cast<uint8_t*>(buffer);
    uint8_t* const end = data + size;

    // Fill the buffer with rndom data.
    while (data < end) {
        // Copy as much random data as we can.
        while (_index < _pool.size() && data < end) {
            *data++ = _pool[_index++];
        }
        // Reload pool buffer if more data is needed.
        if (data < end && !updatePool()) {
            return false;
        }
    }
    return true;
}
