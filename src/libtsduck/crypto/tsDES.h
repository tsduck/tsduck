//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DES block cipher
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"
#include "tsECB.h"
#include "tsCBC.h"

namespace ts {
    //!
    //! DES block cipher.
    //! @ingroup crypto
    //!
    class TSDUCKDLL DES: public BlockCipher
    {
        TS_NOCOPY(DES);
    public:
        DES();                                   //!< Constructor.
        static constexpr size_t BLOCK_SIZE = 8;  //!< DES block size in bytes.
        static constexpr size_t KEY_SIZE = 8;    //!< DES key size in bytes.

    protected:
        TS_BLOCK_CIPHER_DECLARE_PROPERTIES(DES);

        //! Constructor for subclasses which add some properties, such as chaining mode.
        //! @param [in] props Constant reference to a block of properties of this block cipher.
        DES(const BlockCipherProperties& props);

#if defined(TS_WINDOWS)
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const override;
#else
        virtual const EVP_CIPHER* getAlgorithm() const override;
#endif
    };

    //
    // Chaining blocks specializations, when implemented in the system cryptographic library.
    //
    //! @cond nodoxygen
#if !defined(TS_WINDOWS)

    template<>
    class ECB<DES>: public DES
    {
        TS_NOCOPY(ECB);
    public:
        ECB();
    protected:
        TS_BLOCK_CIPHER_DECLARE_PROPERTIES(ECB);
        ECB(const BlockCipherProperties& props);
        virtual const EVP_CIPHER* getAlgorithm() const override;
    };
#endif

    // Specialization for CBC is currently disabled, see:
    // https://stackoverflow.com/questions/78172656/openssl-how-to-encrypt-new-message-with-same-key-without-evp-encryptinit-ex-a
#if 0
    template<>
    class CBC<DES>: public DES
    {
        TS_NOCOPY(CBC);
    public:
        CBC();
    protected:
        TS_BLOCK_CIPHER_DECLARE_PROPERTIES(CBC);
        CBC(const BlockCipherProperties& props);
        virtual const EVP_CIPHER* getAlgorithm() const override;
    };

#endif
    //! @endcond
}
