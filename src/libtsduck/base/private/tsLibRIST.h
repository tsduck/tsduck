//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Safely include the librist definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

#if !defined(TS_NO_RIST)
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(documentation)
    #include <librist/librist.h>
    extern "C" { // to be removed after release of librist 0.2.7
        #include <librist/librist_srp.h>
    } // same as above
    TS_POP_WARNING()
#endif
