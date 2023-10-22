//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVB-CISSA AES-based TS packet encryption.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCBC.h"
#include "tsAES.h"

namespace ts {
    //!
    //! DVB-CISSA AES-based TS packet encryption.
    //! (CISSA = Common IPTV Software-oriented Scrambling Algorithm).
    //! @ingroup crypto
    //! @see ETSI TS 103 127, chapter 6
    //!
    class TSDUCKDLL DVBCISSA : public CBC<AES>
    {
        TS_NOCOPY(DVBCISSA);
    public:
        //!
        //! DVB-CISSA control words size in bytes (AES-128 key size).
        //!
        static constexpr size_t KEY_SIZE = 16;

        //!
        //! Constructor.
        //!
        DVBCISSA();

        // Implementation of BlockCipher interface.
        virtual UString name() const override;

    private:
        virtual bool setIV(const void* iv_, size_t iv_length) override;
    };
}
