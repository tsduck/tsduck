//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup cpp
//!  Pre-configuration header, defining configuration macros.
//!
//!  This header file shall remain empty in the source tree and in a full
//!  standard installation. During installation in a special configuration,
//!  with options excluding some dependencies (e.g. "make NOSRT=1 NOPCSC=1"),
//!  the appropriate symbols are added in the copy of this file in the target
//!  installation tree.
//!
//----------------------------------------------------------------------------

#pragma once
// VATek support is currently disabled.
// See https://github.com/VisionAdvanceTechnologyInc/vatek_sdk_2/issues/9
#define TS_NO_VATEK 1
