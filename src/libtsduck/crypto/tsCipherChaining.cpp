//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCipherChaining.h"


//----------------------------------------------------------------------------
// Constructor for subclasses
//----------------------------------------------------------------------------

ts::CipherChaining::CipherChaining(BlockCipher* cipher, size_t iv_min_blocks, size_t iv_max_blocks, size_t work_blocks) :
    algo(cipher),
    block_size(algo == nullptr ? 0 : algo->blockSize()),
    iv_min_size(iv_min_blocks * block_size),
    iv_max_size(iv_max_blocks * block_size),
    iv(iv_max_blocks * block_size),
    work(work_blocks * block_size)
{
}


//----------------------------------------------------------------------------
// Default implementation of virtual methods.
//----------------------------------------------------------------------------

size_t ts::CipherChaining::minIVSize() const
{
    return iv_min_size;
}

size_t ts::CipherChaining::maxIVSize() const
{
    return iv_max_size;
}


//----------------------------------------------------------------------------
// Implementation of BlockCipher interface:
//----------------------------------------------------------------------------

size_t ts::CipherChaining::blockSize() const
{
    return algo == nullptr ? 0 : algo->blockSize();
}

size_t ts::CipherChaining::minKeySize() const
{
    return algo == nullptr ? 0 : algo->minKeySize();
}

size_t ts::CipherChaining::maxKeySize() const
{
    return algo == nullptr ? 0 : algo->maxKeySize();
}

bool ts::CipherChaining::isValidKeySize(size_t size) const
{
    return algo == nullptr ? 0 : algo->isValidKeySize(size);
}

size_t ts::CipherChaining::minRounds() const
{
    return algo == nullptr ? 0 : algo->minRounds();
}

size_t ts::CipherChaining::maxRounds() const
{
    return algo == nullptr ? 0 : algo->maxRounds();
}

size_t ts::CipherChaining::defaultRounds() const
{
    return algo == nullptr ? 0 : algo->defaultRounds();
}

bool ts::CipherChaining::setKeyImpl(const void* key, size_t key_length, size_t rounds)
{
    return algo != nullptr && algo->setKey(key, key_length, rounds);
}


//----------------------------------------------------------------------------
// Set a new IV.
//----------------------------------------------------------------------------

bool ts::CipherChaining::setIV(const void* iv_data, size_t iv_length)
{
    if (iv_min_size == 0 && iv_length == 0) {
        iv.clear();
        return true;
    }
    else if (iv_data == nullptr || iv_length < iv_min_size || iv_length > iv_max_size) {
        iv.clear();
        return false;
    }
    else {
        iv.copy(iv_data, iv_length);
        return true;
    }
}
