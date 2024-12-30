//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A singleton which generates 64-bit UID, unique integer
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! A singleton which generates 64-bit UID, unique integer.
    //! @ingroup cpp
    //!
    class TSDUCKDLL UID
    {
        TS_SINGLETON(UID);
    public:
        //!
        //! Generate a new UID.
        //! @return A new unique 64-bit value.
        //!
        uint64_t newUID();

    private:
        std::atomic<uint64_t> _next_uid {};
    };
}
