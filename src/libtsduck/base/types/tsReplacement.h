//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Replacement policy in containers of data structures.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEnumUtils.h"

namespace ts {
    //!
    //! Enumeration type used to indicate the replacement policy in containers of data structures.
    //! Each data structure is supposed to have a unique "identifier" inside the container.
    //! The exact interpretation of each policy may depend on the service which uses it.
    //! Can be used as a bit mask.
    //!
    enum class Replacement : uint8_t {
        NONE    = 0x00,  //!< No known standard
        UPDATE  = 0x01,  //!< Update data structures with similar "identifier".
        REPLACE = 0x02,  //!< Replace data structures with similar "identifier".
        ADD     = 0x04,  //!< Add new data structures with "identifier" not present in the container.
        REMOVE  = 0x08,  //!< Remove from the container any data structure with unknown "identifier" for the service.
    };
}

TS_ENABLE_BITMASK_OPERATORS(ts::Replacement);
