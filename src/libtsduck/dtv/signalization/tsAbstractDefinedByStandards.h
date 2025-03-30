//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL AbstractDefinedByStandards
    {
        TS_INTERFACE(AbstractDefinedByStandards);
    public:
        //!
        //! Get the list of standards which define this object.
        //! @param [in] current_standards Current standards in the stream so far.
        //! This is a hint which may help the object to determine to which standard it belongs.
        //! This can be used by objects with slightly different semantics depending on the standard.
        //! @return A bit mask of standards.
        //!
        virtual Standards definingStandards(Standards current_standards = Standards::NONE) const = 0;
    };
}
