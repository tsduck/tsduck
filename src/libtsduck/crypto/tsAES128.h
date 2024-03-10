//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  AES-128 block cipher
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"
#include "tsAES.h"

namespace ts {
    //!
    //! AES-128 block cipher
    //! @ingroup crypto
    //!
    class TSDUCKDLL AES128: public BlockCipher
    {
        TS_NOCOPY(AES128);
    public:
        AES128() = default;                           //!< Constructor.
        virtual ~AES128() override = default;         //!< Destructor.
        static constexpr size_t BLOCK_SIZE = 16;      //!< AES-128 block size in bytes.
        static constexpr size_t KEY_SIZE = 16;        //!< AES-128 key size in bytes.

        // Implementation of BlockCipher interface:
        virtual UString name() const override;
        virtual size_t blockSize() const override;
        virtual size_t minKeySize() const override;
        virtual size_t maxKeySize() const override;
        virtual bool isValidKeySize(size_t size) const override;

    protected:
        // Implementation of BlockCipher interface:
        virtual bool setKeyImpl(const void* key, size_t key_length) override;
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;

    private:
        AES _aes {};
    };
}
