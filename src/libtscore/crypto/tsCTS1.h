//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Cipher text Stealing (CTS) mode, alternative 1.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"

namespace ts {
    //!
    //!  Cipher text Stealing (CTS) mode, alternative 1.
    //!  @ingroup libtscore crypto
    //!
    //!  Several incompatible designs of CTS exist. This one implements the
    //!  description in:
    //!  - Bruce Schneier, Applied Cryptography (2nd, Ed.), pp 191, 195
    //!  - RFC 2040, The RC5, RC5-CBC, RC5-CBC-Pad, and RC5-CTS Algorithms
    //!  - "CBC ciphertext stealing" in
    //!    http://en.wikipedia.org/wiki/Ciphertext_stealing
    //!
    //!  CTS can process a residue. The plain text and cipher text sizes must be
    //!  greater than the block size of the underlying block cipher.
    //!
    //! @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
    class CTS1: public CIPHER
    {
        TS_NOCOPY(CTS1);
    public:
        //! Default constructor.
        CTS1();

    protected:
        //! Properties of this algorithm.
        //! @return A constant reference to the properties.
        static const BlockCipherProperties& Properties();

        // Implementation of BlockCipher interface.
        //! @cond nodoxygen
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;
        //! @endcond
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
const ts::BlockCipherProperties& ts::CTS1<CIPHER>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(CIPHER::Properties(), u"CTS1", true, CIPHER::BLOCK_SIZE + 1, 3, CIPHER::BLOCK_SIZE);
    return props;
}

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
ts::CTS1<CIPHER>::CTS1() : CIPHER(CTS1::Properties())
{
}


//----------------------------------------------------------------------------
// Encryption in CTS mode.
// The algorithm is safe with overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::CTS1<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();
    uint8_t* work2 = this->work.data() + bsize;

    // Data shorter than block size cannot be processed, keep it clear
    // Note that CTS mode requires at least TWO blocks (the last one may
    // be incomplete). As a consequence, exactly one block is not enough.

    if (this->currentIV().size() != bsize || plain_length <= bsize || cipher_maxsize < plain_length) {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    // Encrypt all blocks in CBC mode, except the last one

    const uint8_t* previous = this->currentIV().data();
    const uint8_t* pt = reinterpret_cast<const uint8_t*>(plain);
    uint8_t* ct = reinterpret_cast<uint8_t*>(cipher);

    while (plain_length > bsize) {
        // work = previous-cipher XOR plain-text
        MemXor(work1, previous, pt, bsize);
        // cipher-text = encrypt (work)
        if (!CIPHER::encryptImpl(work1, bsize, ct, bsize, nullptr)) {
            return false;
        }
        // previous-cipher = cipher-text
        previous = ct;
        // advance one block
        ct += bsize;
        pt += bsize;
        plain_length -= bsize;
    }

    // Process final block (possibly incomplete)
    uint8_t* const previous_ct = ct - bsize;
    // work2 = last partial block, zero-padded
    MemZero(work2, bsize);
    MemCopy(work2, pt, plain_length);
    // work1 = previous-cipher XOR work2
    MemXor(work1, previous_ct, work2, bsize);
    // work2 = encrypt (work1)
    if (!CIPHER::encryptImpl(work1, bsize, work2, bsize, nullptr)) {
        return false;
    }
    // Swap last two blocks and truncate last one
    MemCopy(ct, previous_ct, plain_length);
    MemCopy(previous_ct, work2, bsize);
    return true;
}


//----------------------------------------------------------------------------
// Decryption in CTS mode.
// The algorithm needs to specifically process overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::CTS1<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();
    uint8_t* work2 = this->work.data() + bsize;
    uint8_t* work3 = this->work.data() + 2 * bsize;

    // Data shorter than block size cannot be encrypted.
    // Note that CTS mode requires at least TWO blocks (the last one may
    // be incomplete). As a consequence, exactly one block is not enough.

    if (this->currentIV().size() != bsize || cipher_length <= bsize || plain_maxsize < cipher_length) {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    // Decrypt blocks in CBC mode.
    // Stop before the last two blocks (complete one + possibly-partial one).

    const uint8_t* previous = this->currentIV().data();
    const uint8_t* ct = reinterpret_cast<const uint8_t*>(cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*>(plain);

    while (cipher_length > 2 * bsize) {
        // work = decrypt (cipher-text)
        if (!CIPHER::decryptImpl(ct, bsize, work1, bsize, nullptr)) {
            return false;
        }
        // plain-text = previous-cipher XOR work.
        // previous-cipher = cipher-text
        if (pt == ct) {
            // With overlapping buffer, need to save current cipher.
            MemCopy(work2, ct, bsize);
            MemXor(pt, previous, work1, bsize);
            previous = work2;
            std::swap(work2, work3);
        }
        else {
            MemXor(pt, previous, work1, bsize);
            previous = ct;
        }
        // advance one block
        ct += bsize;
        pt += bsize;
        cipher_length -= bsize;
    }

    // Process final two blocks.
    // The remaining size is exactly one complete block plus a partial one.
    // Say that the last plaintext is Pn and the previous one is Pn-1.
    // Say that the last ciphertext is Cn and the previous one is Cn-1.
    // The content of the last two blocks is: Cn, Cn-1(truncated)

    // Size of last partial block
    const size_t last_size = cipher_length - bsize;
    // work2 = Cn-1(truncated) unpadded
    MemCopy(work2, ct + bsize, last_size);
    // Decrypt Cn -> work1 = Cn-1 xor Pn(zero-padded)
    if (!CIPHER::decryptImpl(ct, bsize, work1, bsize, nullptr)) {
        return false;
    }
    // Obtain Pn = work(truncated) xor Cn-1(truncated),
    // move it to final place, in last partial block.
    MemXor(pt + bsize, work1, work2, last_size);
    // Build complete Cn-1 in padded:
    // First part already in padded, last part comes from work
    if (last_size < bsize) {
        MemCopy(work2 + last_size, work1 + last_size, bsize - last_size);
    }
    // Decrypt Cn-1 -> get curblock = Cn-2 xor Pn-1
    if (!CIPHER::decryptImpl(work2, bsize, pt, bsize, nullptr)) {
        return false;
    }
    // Get Pn-1
    MemXor(pt, pt, previous, bsize);
    return true;
}

#endif
