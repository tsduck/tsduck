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
        //! Properties of this algorithm.
        //! @return A constant reference to the properties.
        static const BlockCipherProperties& Properties();

        //! Constructor for subclasses which add some properties, such as chaining mode.
        //! @param [in] props Constant reference to a block of properties of this block cipher.
        DES(const BlockCipherProperties& props);

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
    class TSDUCKDLL ECB<DES>: public DES
    {
        TS_NOCOPY(ECB);
    public:
        ECB();
    protected:
        static const BlockCipherProperties& Properties();
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
    class TSDUCKDLL CBC<DES>: public DES
    {
        TS_NOCOPY(CBC);
    public:
        CBC();
    protected:
        static const BlockCipherProperties& Properties();
        CBC(const BlockCipherProperties& props);
#if defined(TS_WINDOWS)
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const override;
#else
        virtual const EVP_CIPHER* getAlgorithm() const override;
#endif
    };
    //! @endcond
}
