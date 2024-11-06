//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup mpeg
//!  Some utilities for auxiliary ISDB data structures.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class UString;
    class DuckContext;

    //!
    //! This function displays the content of the "dummy byte part of a TS packet" in an ISDB broadcast TS.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [in,out] strm A standard stream in output mode (text mode).
    //! @param [in] data Address of the dummy byte area.
    //! @param [in] size Size in byte of the dummy byte area.
    //! @param [in] margin Left margin content.
    //! @see ARIB STD-B31, section 5.5
    //!
    TSDUCKDLL void ISDBDisplayTSPDummyByte(DuckContext& duck, std::ostream& strm, const uint8_t* data, size_t size, const UString& margin);
}
