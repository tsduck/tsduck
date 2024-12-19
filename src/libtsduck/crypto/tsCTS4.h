//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Cipher text Stealing (CTS) mode, alternative 4.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"

namespace ts {
    //!
    //!  Cipher text Stealing (CTS) mode, alternative 4.
    //!  @ingroup crypto
    //!
    //!  Several incompatible designs of CTS exist. This one implements the
    //!  weird STMicroelectronics STi71xx ECB-CTS implementation.
    //!
    //!  CTS can process a residue. The plain text and cipher text sizes must be
    //!  greater than the block size of the underlying block cipher.
    //!
    //!  @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
    class CTS4: public CIPHER
    {
        TS_NOCOPY(CTS4);
    public:
        //! Default constructor.
        CTS4();

    protected:
        TS_BLOCK_CIPHER_DECLARE_PROPERTIES(CTS4);

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

TS_BLOCK_CIPHER_DEFINE_PROPERTIES_TEMPLATE(ts::CTS4, CTS4, (CIPHER::PROPERTIES(), u"CTS4", true, CIPHER::BLOCK_SIZE + 1, 1, 0));

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
ts::CTS4<CIPHER>::CTS4() : CIPHER(CTS4::PROPERTIES())
{
}


//----------------------------------------------------------------------------
// Encryption in CTS4 mode.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::CTS4<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();

    if (plain_length < bsize || cipher_maxsize < plain_length) {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    const uint8_t* pt = reinterpret_cast<const uint8_t*>(plain);
    uint8_t* ct = reinterpret_cast<uint8_t*>(cipher);

    // Process in ECB mode, except the last 2 blocks
    while (plain_length > 2 * bsize) {
        if (!CIPHER::encryptImpl(pt, bsize, ct, bsize, nullptr)) {
            return false;
        }
        ct += bsize;
        pt += bsize;
        plain_length -= bsize;
    }

    // Process final two blocks.
    assert(plain_length > bsize);
    const size_t residue_size = plain_length - bsize;
    MemCopy(work1, pt + residue_size, bsize - residue_size);
    MemCopy(work1 + bsize - residue_size, pt + bsize, residue_size);

    if (!CIPHER::encryptImpl(work1, bsize, ct + residue_size, bsize, nullptr)) {
        return false;
    }

    MemCopy(work1, pt, residue_size);
    MemCopy(work1 + residue_size, ct + residue_size, bsize - residue_size);

    if (!CIPHER::encryptImpl(work1, bsize, ct, bsize, nullptr)) {
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Decryption in CTS4 mode.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::CTS4<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();

    if (cipher_length < bsize || plain_maxsize < cipher_length) {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    const uint8_t* ct = reinterpret_cast<const uint8_t*>(cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*>(plain);

    // Process in ECB mode, except the last block
    while (cipher_length > bsize) {
        if (!CIPHER::decryptImpl(ct, bsize, pt, bsize, nullptr)) {
            return false;
        }
        ct += bsize;
        pt += bsize;
        cipher_length -= bsize;
    }

    // Process final block
    assert(cipher_length <= bsize);
    MemCopy(work1, pt - bsize + cipher_length, bsize - cipher_length);
    MemCopy(work1 + bsize - cipher_length, ct, cipher_length);

    if (!CIPHER::decryptImpl(work1, bsize, pt - bsize + cipher_length, bsize, nullptr)) {
        return false;
    }

    return true;
}

#endif
