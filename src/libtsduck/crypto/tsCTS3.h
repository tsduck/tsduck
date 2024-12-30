//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Cipher text Stealing (CTS) mode, alternative 3.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"

namespace ts {
    //!
    //!  Cipher text Stealing (CTS) mode, alternative 3.
    //!  @ingroup crypto
    //!
    //!  Several incompatible designs of CTS exist. This one implements the
    //!  description of "ECB ciphertext stealing" in
    //!  http://en.wikipedia.org/wiki/Ciphertext_stealing
    //!
    //!  CTS can process a residue. The plain text and cipher text sizes must be
    //!  greater than the block size of the underlying block cipher.
    //!
    //!  @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
    class CTS3: public CIPHER
    {
        TS_NOCOPY(CTS3);
    public:
        //! Default constructor.
        CTS3();

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
const ts::BlockCipherProperties& ts::CTS3<CIPHER>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(CIPHER::Properties(), u"CTS3", true, CIPHER::BLOCK_SIZE + 1, 2, 0);
    return props;
}

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
ts::CTS3<CIPHER>::CTS3() : CIPHER(CTS3::Properties())
{
}


//----------------------------------------------------------------------------
// Encryption in CTS3 mode.
// The algorithm needs to specifically process overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::CTS3<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();
    uint8_t* work2 = this->work.data() + bsize;

    if (plain_length <= bsize || cipher_maxsize < plain_length) {
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

    // work1 = encrypt (Pn-1)
    if (!CIPHER::encryptImpl(pt, bsize, work1, bsize, nullptr)) {
        return false;
    }
    // Cn = work1 (truncated)
    if (pt == ct) {
        // Overlaping buffers => copy into work2
        MemCopy(work2, work1, residue_size);
    }
    else {
        // Non overlaping buffers => directly copy into Cn
        MemCopy(ct + bsize, work1, residue_size);
    }
    // work1 = Pn (truncated) || work1 (residue)
    MemCopy(work1, pt + bsize, residue_size);
    // Cn-1 = encrypt (work1)
    if (!CIPHER::encryptImpl(work1, bsize, ct, bsize, nullptr)) {
        return false;
    }
    if (pt == ct) {
        // Overlaping buffers => copy Cn into final place.
        MemCopy(ct + bsize, work2, residue_size);
    }

    return true;
}


//----------------------------------------------------------------------------
// Decryption in CTS3 mode.
// The algorithm needs to specifically process overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::CTS3<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();
    uint8_t* work2 = this->work.data() + bsize;

    if (cipher_length <= bsize || plain_maxsize < cipher_length) {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    const uint8_t* ct = reinterpret_cast<const uint8_t*>(cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*>(plain);

    // Process in ECB mode, except the last 2 blocks
    while (cipher_length > 2 * bsize) {
        if (!CIPHER::decryptImpl(ct, bsize, pt, bsize, nullptr)) {
            return false;
        }
        ct += bsize;
        pt += bsize;
        cipher_length -= bsize;
    }

    // Process final two blocks.
    assert(cipher_length > bsize);
    const size_t residue_size = cipher_length - bsize;

    // work1 = decrypt (Cn-1)
    if (!CIPHER::decryptImpl(ct, bsize, work1, bsize, nullptr)) {
        return false;
    }
    // Pn = work1 (truncated)
    if (pt == ct) {
        // Overlaping buffers => copy into work2
        MemCopy(work2, work1, residue_size);
    }
    else {
        // Non overlaping buffers => directly copy into Pn
        MemCopy(pt + bsize, work1, residue_size);
    }
    // work1 (truncated) = Cn (truncated)
    MemCopy(work1, ct + bsize, residue_size);
    // Pn-1 = decrypt (work1)
    if (!CIPHER::decryptImpl(work1, bsize, pt, bsize, nullptr)) {
        return false;
    }
    if (pt == ct) {
        // Overlaping buffers => copy Pn into final place.
        MemCopy(pt + bsize, work2, residue_size);
    }

    return true;
}

#endif
