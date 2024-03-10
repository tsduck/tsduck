//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Triple-DES (EDE) block cipher
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"

namespace ts {
    //!
    //! Triple-DES block cipher.
    //! @ingroup crypto
    //!
    class TSDUCKDLL TDES: public BlockCipher
    {
        TS_NOCOPY(TDES);
    public:
        TDES();                                  //!< Constructor.
        static constexpr size_t BLOCK_SIZE = 8;  //!< TDES block size in bytes.
        static constexpr size_t KEY_SIZE = 24;   //!< TDES key size in bytes.

        // Implementation of BlockCipher interface:
        virtual UString name() const override;
        virtual size_t blockSize() const override;
        virtual size_t minKeySize() const override;
        virtual size_t maxKeySize() const override;
        virtual bool isValidKeySize (size_t size) const override;

    protected:
        //! @cond nodoxygen
#if defined(TS_WINDOWS)
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const override;
#else
        virtual bool setKeyImpl(const void* key, size_t key_length) override;
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;
#endif
        //! @endcond


#if !defined(TS_WINDOWS)
    private:
        uint32_t _ek[3][32] {};  // Encryption keys
        uint32_t _dk[3][32] {};  // Decryption keys
#endif
    };
}
