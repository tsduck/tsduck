//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsAES128.h"

namespace ts {
    //!
    //! ATIS-0800006 AES-based TS packet encryption (ATIS-IDSA).
    //! @ingroup libtsduck crypto
    //!
    class TSDUCKDLL IDSA: public DVS042<AES128>
    {
        TS_NOCOPY(IDSA);
    public:
        //! Default constructor.
        IDSA();
        //! Destructor.
        virtual ~IDSA() override;

    protected:
        //! Properties of this algorithm.
        //! @return A constant reference to the properties.
        static const BlockCipherProperties& Properties();
    };
}
