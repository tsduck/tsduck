//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Digital TV tuner physical device.
//!  One version of this class exists for each operating system.
//   ==> macOS version => tuners are not implemented on macOS
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTunerBase.h"

namespace ts {
    //!
    //! Digital TV tuner physical device.
    //! One version of this class exists for each operating system.
    //!
    using TunerDevice = TunerBase;
}
