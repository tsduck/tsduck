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
        static constexpr size_t BLOCK_SIZE = 8;  //!< TDES block size in bytes (same as DES).
        static constexpr size_t KEY_SIZE = 24;   //!< TDES key size in bytes (3 DES keys).

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
        virtual const EVP_CIPHER* getAlgorithm() const override;
#endif
        //! @endcond
    };
}
