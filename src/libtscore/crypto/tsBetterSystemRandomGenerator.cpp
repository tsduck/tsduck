//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBetterSystemRandomGenerator.h"
#include "tsFileUtils.h"
#include "tsCerrReport.h"

// Define singleton instance
TS_DEFINE_SINGLETON(ts::BetterSystemRandomGenerator);

// Destructor.
ts::BetterSystemRandomGenerator::~BetterSystemRandomGenerator() {}

#if defined(TS_NO_CRYPTO_LIBRARY)
// Constructor.
ts::BetterSystemRandomGenerator::BetterSystemRandomGenerator() {}

// The rest is implemented only when a cryptographic library is used.
#else

// A fixed AES-128 key for post-processing.
namespace {
    const uint8_t _fixed_key[] = {
        0x68, 0xA3, 0xA1, 0xE0, 0x68, 0x89, 0x7F, 0x9A, 0x05, 0xD5, 0x90, 0xDC, 0xD9, 0x0D, 0x70, 0x4F,
    };
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::BetterSystemRandomGenerator::BetterSystemRandomGenerator() :
    _state_directory(UserHomeDirectory()),
    _state_file(_state_directory + fs::path::preferred_separator + u".tsseed")
{
    // Load initial entropy state. The initial state is an XOR between the .tsseed
    // file and the system PRNG. Without file, we only use the system PRNG.

    // Read the previous content of the seed file. Don't try to read if home directory is unknown.
    bool ok = !_state_directory.empty() && _state.loadFromFile(_state_file, STATE_SIZE) && _state.size() == STATE_SIZE;

    // If no .tsseed was loaded, initialize the state with zero (will be xored with system PRNG).
    if (!ok) {
        _state.set(0, STATE_SIZE);
    }

    // Read and xor some data from the system PRNG.
    ByteBlock sysdata;
    if (!readSuperRandom(sysdata, STATE_SIZE)) {
        // Could not get random data from system PRNG, we are dead...
        _ready = false;
        return;
    }
    _state.binXor(_state, sysdata);

    // If the file was not read, try to save its first version.
    if (!ok) {
        saveState(true);
    }

    // Initialize the AES engine with our fixed key.
    if (!_aes.setKey(_fixed_key, sizeof(_fixed_key))) {
        _ready = false;
    }

    CERR.debug(u"BetterSystemRandomGenerator initialized, ready: %s, seed loaded: %s", _ready, ok);
}


//----------------------------------------------------------------------------
// Implementation of RandomGenerator interface:
//----------------------------------------------------------------------------

ts::UString ts::BetterSystemRandomGenerator::name() const
{
    return u"BetterSystemRandomGenerator";
}


//----------------------------------------------------------------------------
// Read random from the superclass (SystemRandomGenerator).
//----------------------------------------------------------------------------

bool ts::BetterSystemRandomGenerator::readSuperRandom(ByteBlock& data, size_t size)
{
    // Important: use SystemRandomGenerator::read, not SystemRandomGenerator::readByteBlock
    // because readByteBlock() calls read() and we would loop back on ourselves.
    data.resize(size);
    return SystemRandomGenerator::read(data.data(), size);
}


//----------------------------------------------------------------------------
// Save entropy state when necessary.
//----------------------------------------------------------------------------

void ts::BetterSystemRandomGenerator::saveState(bool force_perms)
{
    // Don't try to save state if the directory does not exist.
    // Save state only once per cycle.
    if (!_state_directory.empty() && ++_state_cycle >= STATE_SAVE_CYCLE) {
        _state_cycle = 0;
        if (!_state.saveToFile(_state_file)) {
            // Ignore write errors.
            CERR.debug(u"BetterSystemRandomGenerator failed to save seed in %s", _state_file);
        }
        else if (force_perms) {
            // When requested, force file permissions to -rw-------. Ignore permission errors.
            std::error_code e;
            fs::permissions(_state_file, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace , e);
        }
    }
}


//----------------------------------------------------------------------------
// Check if PRNG is ready. If not ready, must be seeded again.
//----------------------------------------------------------------------------

bool ts::BetterSystemRandomGenerator::ready() const
{
    // If this BetterSystemRandomGenerator is not ready, fallback to SystemRandomGenerator.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _ready || SystemRandomGenerator::ready();
}


//----------------------------------------------------------------------------
// Update the content of the random pool with new data.
//----------------------------------------------------------------------------

bool ts::BetterSystemRandomGenerator::updatePool()
{
    ByteBlock r1(STATE_SIZE);
    ByteBlock r2(STATE_SIZE);

    // R1 = read SystemRandomGenerator
    // R2 = AES[K] R1
    if (!readSuperRandom(r1, r1.size()) || !_aes.encrypt(r1.data(), r1.size(), r2.data(), r2.size())) {
        return false;
    }

    // R1 = R2 xor state
    r1.binXor(r2, _state);

    // pool = AES[K] R1 ==> output of BetterSystemRandomGenerator
    _pool.resize(STATE_SIZE);
    if (!_aes.encrypt(r1.data(), r1.size(), _pool.data(), _pool.size())) {
        return false;
    }

    // R1 = read SystemRandomGenerator
    if (!readSuperRandom(r1, r1.size())) {
        return false;
    }

    // R2 = R1 xor pool xor state.
    r2.binXor(r1, _pool);
    r2.binXor(r2, _state);

    // h = SHA-256 (R2)
    ByteBlock h(SHA256::HASH_SIZE);
    if (!_sha.hash(r2.data(), r2.size(), h.data(), h.size())) {
        return false;
    }

    // state = truncated h
    assert(_state.size() <= h.size());
    _state.copy(h.data(), STATE_SIZE);

    // Save state file.
    saveState(false);

    // Now _pool is ready with new random data.
    _index = 0;
    return true;
}


//----------------------------------------------------------------------------
// Get random data. Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::BetterSystemRandomGenerator::read(void* buffer, size_t size)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    // Filter trivial cases.
    if (buffer == nullptr) {
        return false;
    }

    // When BetterSystemRandomGenerator could not be used, fall back to SystemRandomGenerator.
    if (!_ready) {
        return SystemRandomGenerator::read(buffer, size);
    }

    // Cast buffer boundaries.
    uint8_t* data = reinterpret_cast<uint8_t*>(buffer);
    uint8_t* const end = data + size;

    // Fill the buffer with random data.
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

#endif // TS_NO_CRYPTO_LIBRARY
