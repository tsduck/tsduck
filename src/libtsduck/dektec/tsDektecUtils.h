//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
