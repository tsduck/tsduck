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
#include "tsECB.h"
#include "tsCBC.h"

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
        static constexpr size_t BLOCK_SIZE = 8;  //!< TDES block size in bytes (same as DES).
        static constexpr size_t KEY_SIZE = 24;   //!< TDES key size in bytes (3 DES keys).

    protected:
        TS_BLOCK_CIPHER_DECLARE_PROPERTIES(TDES);

        //! Constructor for subclasses which add some properties, such as chaining mode.
        //! @param [in] props Constant reference to a block of properties of this block cipher.
        TDES(const BlockCipherProperties& props);

#if defined(TS_WINDOWS)
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const override;
#else
        virtual const EVP_CIPHER* getAlgorithm() const override;
#endif
    };

    //
    // Chaining blocks specializations, when implemented in the system cryptographic library.
    //
    //! @cond nodoxygen
    template<>
    class TSDUCKDLL ECB<TDES>: public TDES
    {
        TS_NOCOPY(ECB);
    public:
        ECB();
    protected:
        TS_BLOCK_CIPHER_DECLARE_PROPERTIES(ECB);
        ECB(const BlockCipherProperties& props);
#if defined(TS_WINDOWS)
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const override;
#else
        virtual const EVP_CIPHER* getAlgorithm() const override;
#endif
    };
    //! @endcond

    //! @cond nodoxygen
    template<>
    class TSDUCKDLL CBC<TDES>: public TDES
    {
        TS_NOCOPY(CBC);
    public:
        CBC();
    protected:
        TS_BLOCK_CIPHER_DECLARE_PROPERTIES(CBC);
        CBC(const BlockCipherProperties& props);
#if defined(TS_WINDOWS)
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const override;
#else
        virtual const EVP_CIPHER* getAlgorithm() const override;
#endif
    };
    //! @endcond
}
