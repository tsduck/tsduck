//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Counter (CTR) chaining mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCipherChaining.h"
#include "tsMemory.h"

namespace ts {
    //!
    //! Counter (CTR) chaining mode.
    //! @ingroup crypto
    //!
    //! CTR can process a residue. The plain text and cipher text can have any size.
    //!
    //! @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template <class CIPHER>
    class CTR: public CipherChainingTemplate<CIPHER>
    {
        TS_NOCOPY(CTR);
    public:
        //!
        //! Constructor.
        //! @param [in] counter_bits Number of bits of the counter part in the IV.
        //! See setCounterBits() for an explanation of this value.
        //!
        CTR(size_t counter_bits = 0);

        //!
        //! Set the size of the counter part in the IV.
        //! @param [in] counter_bits Number of bits of the counter part in the IV.
        //! In CTR mode, the IV is considered as an integer in big-endian representation.
        //! The counter part of the IV uses the least significant bits of the IV.
        //! The default value (when specified as zero) is half the size of the IV.
        //!
        void setCounterBits(size_t counter_bits);

        //!
        //! Get the size of the counter part in the IV.
        //! @return Number of bits of the counter part in the IV.
        //! See setCounterBits() for an explanation of this value.
        //!
        size_t counterBits() const {return _counter_bits;}

        // Implementation of CipherChaining interface.
        virtual size_t minMessageSize() const override;
        virtual bool residueAllowed() const override;

        // Implementation of BlockCipher interface.
        virtual UString name() const override;

    protected:
        // Implementation of BlockCipher interface.
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;

    private:
        size_t _counter_bits; // size in bits of the counter part.

        // We need two work blocks.
        // The first one contains the "input block" or counter.
        // The second one contains the "output block", the encrypted counter.
        // This private method increments the counter block.
        bool incrementCounter();
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

template<class CIPHER>
ts::CTR<CIPHER>::CTR(size_t counter_bits) :
    CipherChainingTemplate<CIPHER>(1, 1, 2),
    _counter_bits(0)
{
    setCounterBits(counter_bits);
}

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
    std::memcpy(this->work.data(), this->iv.data(), this->block_size);

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
