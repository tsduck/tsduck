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

#pragma once
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

template<class CIPHER>
ts::CTR<CIPHER>::CTR(size_t counter_bits) :
    CipherChainingTemplate<CIPHER>(1, 1, 2),
    _counter_bits(0)
{
    setCounterBits(counter_bits);
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

template<class CIPHER>
size_t ts::CTR<CIPHER>::minMessageSize() const
{
    return 0;
}

template<class CIPHER>
bool ts::CTR<CIPHER>::residueAllowed() const
{
    return true;
}

template<class CIPHER>
ts::UString ts::CTR<CIPHER>::name() const
{
    return this->algo == nullptr ? UString() : this->algo->name() + u"-CTR";
}


//----------------------------------------------------------------------------
// Set counter size in bits.
//----------------------------------------------------------------------------

template<class CIPHER>
void ts::CTR<CIPHER>::setCounterBits(size_t counter_bits)
{
    if (counter_bits == 0) {
        // Default size is half the block size in bits.
        this->_counter_bits = this->block_size * 4;
    }
    else {
        // Counter cannot be larger than the block size.
        this->_counter_bits = std::min(counter_bits, this->block_size * 8);
    }
}


//----------------------------------------------------------------------------
// Increment the counter in the first work block.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CTR<CIPHER>::incrementCounter()
{
    // We must have two work blocks.
    if (this->work.size() < 2 * this->block_size) {
        return false;
    }

    size_t bits = _counter_bits;
    bool carry = true; // initial increment.

    // The first work block contains the "input block" or counter to increment.
    for (uint8_t* b = this->work.data() + this->block_size - 1; carry && bits > 0 && b > this->work.data(); --b) {
        const size_t bits_in_byte = std::min<size_t>(bits, 8);
        bits -= bits_in_byte;
        const uint8_t mask = uint8_t(0xFF >> (8 - bits_in_byte));
        *b = (*b & ~mask) | (((*b & mask) + 1) & mask);
        carry = (*b & mask) == 0x00;
    }
    return true;
}


//----------------------------------------------------------------------------
// Encryption in CTR mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CTR<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    if (this->algo == nullptr ||
        this->iv.size() != this->block_size ||
        this->work.size() < 2 * this->block_size ||
        cipher_maxsize < plain_length)
    {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    // work[0] = iv
    ::memcpy(this->work.data(), this->iv.data(), this->block_size);

    // Loop on all blocks, including last truncated one.

    const uint8_t* pt = reinterpret_cast<const uint8_t*>(plain);
    uint8_t* ct = reinterpret_cast<uint8_t*>(cipher);

    while (plain_length > 0) {
        // work[1] = encrypt(work[0])
        if (!this->algo->encrypt(this->work.data(), this->block_size, this->work.data() + this->block_size, this->block_size)) {
            return false;
        }
        // This block size:
        const size_t size = std::min(plain_length, this->block_size);
        // cipher-text = plain-text XOR work[1]
        for (size_t i = 0; i < size; ++i) {
            ct[i] = this->work[this->block_size + i] ^ pt[i];
        }
        // work[0] += 1
        if (!incrementCounter()) {
            return false;
        }
        // advance one block
        ct += size;
        pt += size;
        plain_length -= size;
    }
    return true;
}


//----------------------------------------------------------------------------
// Decryption in CTR mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CTR<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    // With CTR, the encryption and decryption are identical operations.
    return this->encryptImpl(cipher, cipher_length, plain, plain_maxsize, plain_length);
}
