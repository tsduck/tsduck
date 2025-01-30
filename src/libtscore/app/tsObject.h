//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  General-purpose base class for polymophic objects.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class Object;

    //!
    //! Safe pointer for Object (thread-safe).
    //!
    using ObjectPtr = std::shared_ptr<Object>;

    //!
    //! General-purpose base class for polymophic objects.
    //! @ingroup cpp
    //!
    //! This type of object is typically derived by application-defined classes and
    //! used to communicate these user-data between independent modules or plugins.
    //! @see ObjectRepository
    //!
    class TSCOREDLL Object
    {
        TS_INTERFACE(Object);
    };
}
