//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declare which crypto accelerations are implemented.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

// Some global constant private booleans which are defined when the accelerated
// modules are compiled with accelerated instructions.
extern const bool tsCRC32IsAccelerated;
extern const bool tsAESIsAccelerated;
extern const bool tsSHA1IsAccelerated;
extern const bool tsSHA256IsAccelerated;
extern const bool tsSHA512IsAccelerated;
