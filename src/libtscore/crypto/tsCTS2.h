//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Cipher text Stealing (CTS) mode, alternative 2.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"

namespace ts {
    //!
    //!  Cipher text Stealing (CTS) mode, alternative 2.
    //!  @ingroup crypto
    //!
    //!  Several incompatible designs of CTS exist. This one implements the description in
    //!  http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/ciphertext%20stealing%20proposal.pdf
    //!
    //!  CTS can process a residue. The plain text and cipher text sizes must be
    //!  equal to or greater than the block size of the underlying block cipher.
    //!
    //!  @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
    class CTS2: public CIPHER
    {
        TS_NOCOPY(CTS2);
    public:
        //! Default constructor.
        CTS2();

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
const ts::BlockCipherProperties& ts::CTS2<CIPHER>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(CIPHER::Properties(), u"CTS2", true, CIPHER::BLOCK_SIZE, 3, CIPHER::BLOCK_SIZE);
    return props;
}

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
ts::CTS2<CIPHER>::CTS2() : CIPHER(CTS2::Properties())
{
}


//----------------------------------------------------------------------------
// Encryption in CTS mode.
// The algorithm is safe with overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::CTS2<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();

    if (this->currentIV().size() != bsize || plain_length < bsize || cipher_maxsize < plain_length) {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    // Encrypt all blocks in CBC mode, except the last one if partial
    const uint8_t* previous = this->currentIV().data();
    const uint8_t* pt = reinterpret_cast<const uint8_t*>(plain);
    uint8_t* ct = reinterpret_cast<uint8_t*>(cipher);

    while (plain_length >= bsize) {
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

    // Process final block if incomplete
    uint8_t* const previous_ct = ct - bsize;
    if (plain_length > 0) {
        // work = Cn-1 XOR Pn, truncated
        MemXor(work1, previous_ct, pt, plain_length);
        // pad with tail of Cn-1
        MemCopy(work1 + plain_length, previous_ct + plain_length, bsize - plain_length);
        // Cn = encrypt (work), truncating Cn-1
        if (!CIPHER::encryptImpl(work1, bsize, previous_ct + plain_length, bsize, nullptr)) {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Decryption in CTS mode.
// The algorithm needs to specifically process overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::CTS2<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();
    uint8_t* work2 = this->work.data() + bsize;
    uint8_t* work3 = this->work.data() + 2 * bsize;

    if (this->currentIV().size() != bsize || cipher_length < bsize || plain_maxsize < cipher_length) {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    // Decrypt blocks in CBC mode. If the last block is partial,
    // stop before the last two blocks (complete one + partial one).

    const uint8_t* previous = this->currentIV().data();
    const uint8_t* ct = reinterpret_cast<const uint8_t*>(cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*>(plain);

    const size_t residue_size = cipher_length % bsize;
    const size_t trick_size = residue_size == 0 ? 0 : bsize + residue_size;

    while (cipher_length > trick_size) {
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

    if (cipher_length > 0) {
        assert (cipher_length == trick_size);
        // The remaining size is exactly one complete block plus a partial one.
        // Say that the last plaintext is Pn and the previous one is Pn-1.
        // Say that the last ciphertext is Cn and the previous one is Cn-1.

        // work = decrypt (Cn)
        if (!CIPHER::decryptImpl(ct + residue_size, bsize, work1, bsize, nullptr)) {
            return false;
        }
        // Pn(truncated) = work(truncated) xor Cn-1(truncated)
        MemXor(pt + bsize, ct, work1, residue_size);
        // Rebuild Cn-1 in work
        MemCopy(work1, ct, residue_size);
        // Pn-1 = decrypt (Cn-1)
        if (!CIPHER::decryptImpl(work1, bsize, pt, bsize, nullptr)) {
            return false;
        }
        // Pn-1 = Pn-1 xor previous cipher
        MemXor(pt, pt, previous, bsize);
    }
    return true;
}

#endif
