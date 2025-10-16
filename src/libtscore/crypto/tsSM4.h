//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  SM4 block cipher
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"
#include "tsECB.h"
#include "tsCBC.h"

namespace ts {
    //!
    //! SM4 block cipher
    //! @ingroup libtscore crypto
    //!
    class TSCOREDLL SM4: public BlockCipher
    {
        TS_NOCOPY(SM4);
    public:
        SM4();                                 //!< Constructor.
        virtual ~SM4() override;               //!< Destructor.
        static constexpr size_t BLOCK_SIZE = 16;  //!< SM4 block size in bytes.
        static constexpr size_t KEY_SIZE = 16;    //!< SM4 key size in bytes.

    protected:
        //! Properties of this algorithm.
        //! @return A constant reference to the properties.
        static const BlockCipherProperties& Properties();

        //! Constructor for subclasses which add some properties, such as chaining mode.
        //! @param [in] props Constant reference to a block of properties of this block cipher.
        SM4(const BlockCipherProperties& props);

#if !defined(TS_NO_OPENSSL)
        virtual const EVP_CIPHER* getAlgorithm() const override;
#endif
    };

#if !defined(TS_NO_CRYPTO_LIBRARY) && !defined(DOXYGEN)
    //
    // Chaining blocks specializations, when implemented in the system cryptographic library.
    //
    template<>
    class TSCOREDLL ECB<SM4>: public SM4
    {
        TS_NOCOPY(ECB);
    public:
        ECB();
    protected:
        static const BlockCipherProperties& Properties();
        ECB(const BlockCipherProperties& props);
#if !defined(TS_NO_OPENSSL)
        virtual const EVP_CIPHER* getAlgorithm() const override;
#endif
    };

    template<>
    class TSCOREDLL CBC<SM4>: public SM4
    {
        TS_NOCOPY(CBC);
    public:
        CBC();
    protected:
        static const BlockCipherProperties& Properties();
        CBC(const BlockCipherProperties& props);
#if !defined(TS_NO_OPENSSL)
        virtual const EVP_CIPHER* getAlgorithm() const override;
#endif
    };

#endif // TS_NO_CRYPTO_LIBRARY
}
