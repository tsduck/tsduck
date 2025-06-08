//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtsduck hardware
//!  Check support for Dektec devices.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Check if this version of TSDuck was built with Dektec support.
    //! All Dektec-related code, when available, is in a separate shared library libtsdektec.
    //! As a side effect of calling this function, if Dektec is supported, the shared library
    //! libtsdektec is loaded and its features are registered.
    //! @ingroup hardware
    //! @return True is Dektec devices are supported. Always false on macOS or on Windows/Linux on non-Intel platforms.
    //!
    TSDUCKDLL bool HasDektecSupport();
}
