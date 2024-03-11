//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  AES-256 block cipher
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"

namespace ts {
    //!
    //! AES-256 block cipher
    //! @ingroup crypto
    //!
    class TSDUCKDLL AES256: public BlockCipher
    {
        TS_NOCOPY(AES256);
    public:
        AES256();                                 //!< Constructor.
        static constexpr size_t BLOCK_SIZE = 16;  //!< AES-256 block size in bytes.
        static constexpr size_t KEY_SIZE = 32;    //!< AES-256 key size in bytes.

        // Implementation of BlockCipher interface:
        virtual UString name() const override;
        virtual size_t blockSize() const override;
        virtual size_t minKeySize() const override;
        virtual size_t maxKeySize() const override;
        virtual bool isValidKeySize(size_t size) const override;

    protected:
        //! @cond nodoxygen
#if defined(TS_WINDOWS)
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const override;
#else
        virtual const EVP_CIPHER* getAlgorithm() const override;
#endif
        //! @endcond
    };
}
