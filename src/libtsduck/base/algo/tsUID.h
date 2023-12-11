//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A singleton which generates 64-bit UID, unique integer
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSingleton.h"

namespace ts {
    //!
    //! A singleton which generates 64-bit UID, unique integer.
    //! @ingroup cpp
    //!
    class TSDUCKDLL UID
    {
        // This class is a singleton. Use static Instance() method.
        TS_DECLARE_SINGLETON(UID);

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
