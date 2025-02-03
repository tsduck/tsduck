//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Cipher Block Chaining (CBC) mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"

namespace ts {
    //!
    //! Cipher Block Chaining (CBC) mode.
    //!
    //! No padding is performed. The plain text and cipher text sizes must be
    //! multiples of the block size of the underlying block cipher.
    //!
    //! @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //! @ingroup libtscore crypto
    //!
    template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
    class CBC: public CIPHER
    {
        TS_NOCOPY(CBC);
    public:
        //! Default constructor.
        CBC();

    protected:
        //! Properties of this algorithm.
        //! @return A constant reference to the properties.
        static const BlockCipherProperties& Properties();

        //! Constructor for subclasses which add some properties, such as fixed IV.
        //! @param [in] props Constant reference to a block of properties of this block cipher.
        CBC(const BlockCipherProperties& props);

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
const ts::BlockCipherProperties& ts::CBC<CIPHER>::Properties()
{
    // Need 3 work blocks. The last 2 are only used with "in place" decryption.
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(CIPHER::Properties(), u"CBC", false, CIPHER::BLOCK_SIZE, 3, CIPHER::BLOCK_SIZE);
    return props;
}

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
ts::CBC<CIPHER>::CBC() : CIPHER(CBC::Properties())
{
}

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
ts::CBC<CIPHER>::CBC(const BlockCipherProperties& props) : CIPHER(props)
{
    props.assertCompatibleChaining(CBC::Properties());
}


//----------------------------------------------------------------------------
// Encryption in CBC mode.
// The algorithm is safe with overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::CBC<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();

    if (plain_length % bsize != 0 || this->currentIV().size() != bsize || cipher_maxsize < plain_length) {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    const uint8_t* previous = this->currentIV().data();
    const uint8_t* pt = reinterpret_cast<const uint8_t*>(plain);
    uint8_t* ct = reinterpret_cast<uint8_t*>(cipher);

    while (plain_length > 0) {
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

    return true;
}


//----------------------------------------------------------------------------
// Decryption in CBC mode.
// The algorithm needs to specifically process overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::CBC<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();
    uint8_t* work2 = this->work.data() + bsize;
    uint8_t* work3 = this->work.data() + 2 * bsize;

    if (cipher_length % bsize != 0 || this->currentIV().size() != bsize || plain_maxsize < cipher_length) {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    const uint8_t* previous = this->currentIV().data();
    const uint8_t* ct = reinterpret_cast<const uint8_t*>(cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*>(plain);

    while (cipher_length > 0) {
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

    return true;
}

#endif
