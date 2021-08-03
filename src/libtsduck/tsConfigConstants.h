//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  TSDuck configuration constants.
//!
//----------------------------------------------------------------------------

#pragma once

#if defined(DOXYGEN)
//!
//! If this symbol is defined, bitrate values are represented as fractions of 64-bit integers.
//! Without this symbol, bitrates are represented as fixed-point numbers of a given compile-time precision.
//! Using fraction instead of fixed-point numbers gives a better precision but seriously impacts the
//! overall performance os computations involving bitrates.
//! @see ts::Fraction
//! @see ts::BitRate
//!
#define TS_BITRATE_FRACTION 1
#endif

//!
//! Define the precision (number of decimal digits) of bitrate values.
//! This is used when bitrates are reprensented as fixed-point numbers instead of fractions.
//! @see ts::FixedPoint
//! @see ts::BitRate
//! @see TS_BITRATE_FRACTION
//!
#if !defined(TS_BITRATE_DECIMALS)
#define TS_BITRATE_DECIMALS 1
#endif
