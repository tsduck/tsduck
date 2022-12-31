//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//-----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup hardware
//!  Some basic utilities for Dektec API, without direct reference to DTAPI.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsEnumeration.h"

namespace ts {
    //!
    //! Safe size in bytes of the FIFO of DTA devices.
    //! This is a legacy value, recent devices can report dynamically.
    //!
    const size_t DTA_FIFO_SIZE = 8 * 1024 * 1024;

    //!
    //! Maximum I/O size in bytes of DTA devices.
    //! This value is merely an advice, the absolute maximum is the FIFO size.
    //!
    const size_t DTA_MAX_IO_SIZE = 6 * 1024 * 1024;

    //!
    //! Maximum number of "hardware functions" per Dektec device.
    //! A hardware function is one input or output channel for instance.
    //! @see DTAPI documentation
    //!
    const size_t DTA_MAX_HW_FUNC = 75;

    //!
    //! Enumeration (names/values) for Dektec modulation constants (DTAPI_MOD_DVBS_QPSK, etc).
    //!
    TSDUCKDLL extern const Enumeration DektecModulationTypes;

    //!
    //! Enumeration (names/values) for Dektec VSB constants (DTAPI_MOD_ATSC_VSB8, etc).
    //!
    TSDUCKDLL extern const Enumeration DektecVSB;

    //!
    //! Enumeration (names/values) for Dektec FEC constants (DTAPI_MOD_1_2, etc).
    //!
    TSDUCKDLL extern const Enumeration DektecFEC;

    //!
    //! Enumeration (names/values) for Dektec spectral inversion constants (DTAPI_MOD_SPECNONINV, etc).
    //!
    TSDUCKDLL extern const Enumeration DektecInversion;

    //!
    //! Enumeration (names/values) for Dektec DVB-T properties constants (DTAPI_MOD_DVBT_*, etc).
    //!
    TSDUCKDLL extern const Enumeration DektecDVBTProperty;

    //!
    //! Enumeration (names/values) for Dektec DTU-315 modulator power modes.
    //!
    TSDUCKDLL extern const Enumeration DektecPowerMode;

    //!
    //! Check if this version of TSDuck was built with Dektec support.
    //! @return True is Dektec devices are supported. Always false on macOS or on Windows/Linux on non-Intel platforms.
    //!
    TSDUCKDLL bool HasDektecSupport();

    //!
    //! Get the versions of Dektec API and drivers in one single string.
    //! @return A string describing the Dektec versions (or the lack of Dektec support).
    //!
    TSDUCKDLL UString GetDektecVersions();

    //!
    //! Get the versions of Dektec API and drivers.
    //! @param [out] versions All versions. The map index is the driver or API name and the map value is its version.
    //!
    TSDUCKDLL void GetDektecVersions(std::map<UString,UString>& versions);
}
