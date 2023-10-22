//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  ATIS-0800006 AES-based TS packet encryption (ATIS-IDSA).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDVS042.h"
#include "tsAES.h"

namespace ts {
    //!
    //! ATIS-0800006 AES-based TS packet encryption (ATIS-IDSA).
    //! @ingroup crypto
    //!
    class TSDUCKDLL IDSA : public DVS042<AES>
    {
        TS_NOCOPY(IDSA);
    public:
        //!
        //! ATIS-IDSA control words size in bytes (AES-128 key size).
        //!
        static constexpr size_t KEY_SIZE = 16;

        //!
        //! Constructor.
        //!
        IDSA();

        // Implementation of BlockCipher interface.
        virtual UString name() const override;

    private:
        virtual bool setIV(const void* iv_, size_t iv_length) override;
        virtual bool setShortIV(const void* iv_, size_t iv_length) override;
    };
}
