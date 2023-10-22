//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for objects which are defined by standards.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStandards.h"

namespace ts {
    //!
    //! Abstract base class for objects which are defined by standards.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL AbstractDefinedByStandards
    {
        TS_INTERFACE(AbstractDefinedByStandards);
    public:
        //!
        //! Get the list of standards which define this object.
        //! @return A bit mask of standards.
        //!
        virtual Standards definingStandards() const = 0;
    };
}
