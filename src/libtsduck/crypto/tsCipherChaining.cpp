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
