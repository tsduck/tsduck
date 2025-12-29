//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtsduck hardware
//!  Some basic utilities for VATek devices.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Check if this version of TSDuck was built with VATek support.
    //! @ingroup hardware
    //! @return True is VATek devices are supported.
    //!
    TSDUCKDLL bool HasVatekSupport();

    //!
    //! Get the version of VATek library.
    //! @ingroup hardware
    //! @return A string describing the VATek version (or the lack of VATek support).
    //!
    TSDUCKDLL UString GetVatekVersion();
}
