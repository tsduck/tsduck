//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Counter (CTR) chaining mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"
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
    template <class CIPHER, typename std::enable_if<std::is_base_of<BlockCipher, CIPHER>::value>::type* = nullptr>
    class CTR: public CIPHER
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

    protected:
        TS_BLOCK_CIPHER_DECLARE_PROPERTIES(CTR);

        // Implementation of BlockCipher interface.
        //! @cond nodoxygen
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;
        //! @endcond

    private:
        size_t _counter_bits = 0; // size in bits of the counter part.

        // We need two work blocks.
        // The first one contains the "input block" or counter.
        // The second one contains the "output block", the encrypted counter.
        // This private method increments the counter block.
        void incrementCounter();
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

TS_BLOCK_CIPHER_DEFINE_PROPERTIES_TEMPLATE(ts::CTR, CTR, (CIPHER::PROPERTIES(), u"CTR", true, 0, 2, CIPHER::BLOCK_SIZE));

template<class CIPHER, typename std::enable_if<std::is_base_of<ts::BlockCipher, CIPHER>::value>::type* N>
ts::CTR<CIPHER,N>::CTR(size_t counter_bits) : CIPHER(CTR::PROPERTIES())
{
    setCounterBits(counter_bits);
}


//----------------------------------------------------------------------------
// Set counter size in bits.
//----------------------------------------------------------------------------

template<class CIPHER, typename std::enable_if<std::is_base_of<ts::BlockCipher, CIPHER>::value>::type* N>
void ts::CTR<CIPHER,N>::setCounterBits(size_t counter_bits)
{
    if (counter_bits == 0) {
        // Default size is half the block size in bits.
        _counter_bits = this->properties.block_size * 4;
    }
    else {
        // Counter cannot be larger than the block size.
        _counter_bits = std::min(counter_bits, this->properties.block_size * 8);
    }
}


//----------------------------------------------------------------------------
// Increment the counter in the first work block.
//----------------------------------------------------------------------------

template<class CIPHER, typename std::enable_if<std::is_base_of<ts::BlockCipher, CIPHER>::value>::type* N>
void ts::CTR<CIPHER,N>::incrementCounter()
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();
    size_t bits = _counter_bits;
    bool carry = true; // initial increment.

    // The first work block contains the "input block" or counter to increment.
    for (uint8_t* b = work1 + bsize - 1; carry && bits > 0 && b > work1; --b) {
        const size_t bits_in_byte = std::min<size_t>(bits, 8);
        bits -= bits_in_byte;
        const uint8_t mask = uint8_t(0xFF >> (8 - bits_in_byte));
        *b = (*b & ~mask) | (((*b & mask) + 1) & mask);
        carry = (*b & mask) == 0x00;
    }
}


//----------------------------------------------------------------------------
// Encryption in CTR mode.
// The algorithm is safe with overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER, typename std::enable_if<std::is_base_of<ts::BlockCipher, CIPHER>::value>::type* N>
bool ts::CTR<CIPHER,N>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();
    uint8_t* work2 = this->work.data() + bsize;

    if (plain_length % bsize != 0 || this->currentIV().size() != bsize || cipher_maxsize < plain_length) {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    // work1 = iv
    MemCopy(work1, this->currentIV().data(), bsize);

    // Loop on all blocks, including last truncated one.
    const uint8_t* pt = reinterpret_cast<const uint8_t*>(plain);
    uint8_t* ct = reinterpret_cast<uint8_t*>(cipher);

    while (plain_length > 0) {
        // work2 = encrypt(work1)
        if (!CIPHER::encryptImpl(work1, bsize, work2, bsize, nullptr)) {
            return false;
        }
        // This block size:
        const size_t size = std::min(plain_length, bsize);
        // cipher-text = plain-text XOR work2
        MemXor(ct, work2, pt, size);
        // work1 += 1
        incrementCounter();
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

template<class CIPHER, typename std::enable_if<std::is_base_of<ts::BlockCipher, CIPHER>::value>::type* N>
bool ts::CTR<CIPHER,N>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    // With CTR, the encryption and decryption are identical operations.
    return encryptImpl(cipher, cipher_length, plain, plain_maxsize, plain_length);
}

#endif
