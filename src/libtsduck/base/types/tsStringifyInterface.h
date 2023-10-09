//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract interface converting an object to UString.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class UString;

    //!
    //! An interface to be implemented by classes supporting a conversion to UString.
    //! @ingroup cpp
    //!
    class TSDUCKDLL StringifyInterface
    {
        TS_INTERFACE(StringifyInterface);
    public:
        //!
        //! Convert to a string object.
        //! @return This object, converted as a string.
        //!
        virtual UString toString() const = 0;
    };
}
