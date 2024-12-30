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
        TS_NOCOPY(UID);
    public:
        //!
        //! Get the instance of the CerrReport singleton.
        //! @return A reference to the CerrReport singleton.
        //!
        static UID& Instance();

    public:
        //!
        //! Generate a new UID.
        //! @return A new unique 64-bit value.
        //!
        uint64_t newUID();

    private:
        UID();
        std::atomic<uint64_t> _next_uid {};
    };
}
