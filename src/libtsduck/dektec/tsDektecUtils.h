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
//!  Some basic utilities for Dektec API, without direct reference to DTAPI.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsNames.h"

namespace ts {
    //!
    //! Safe size in bytes of the FIFO of DTA devices.
    //! This is a legacy value, recent devices can report dynamically.
    //! @ingroup hardware
    //!
    const size_t DTA_FIFO_SIZE = 8 * 1024 * 1024;

    //!
    //! Maximum I/O size in bytes of DTA devices.
    //! This value is merely an advice, the absolute maximum is the FIFO size.
    //! @ingroup hardware
    //!
    const size_t DTA_MAX_IO_SIZE = 6 * 1024 * 1024;

    //!
    //! Maximum number of "hardware functions" per Dektec device.
    //! A hardware function is one input or output channel for instance.
    //! @ingroup hardware
   //! @see DTAPI documentation
    //!
    const size_t DTA_MAX_HW_FUNC = 75;

    //!
    //! Enumeration (names/values) for Dektec modulation constants (DTAPI_MOD_DVBS_QPSK, etc).
    //! @ingroup hardware
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& DektecModulationTypes();

    //!
    //! Enumeration (names/values) for Dektec VSB constants (DTAPI_MOD_ATSC_VSB8, etc).
    //! @ingroup hardware
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& DektecVSB();

    //!
    //! Enumeration (names/values) for Dektec FEC constants (DTAPI_MOD_1_2, etc).
    //! @ingroup hardware
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& DektecFEC();

    //!
    //! Enumeration (names/values) for Dektec spectral inversion constants (DTAPI_MOD_SPECNONINV, etc).
    //! @ingroup hardware
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& DektecInversion();

    //!
    //! Enumeration (names/values) for Dektec DVB-T properties constants (DTAPI_MOD_DVBT_*, etc).
    //! @ingroup hardware
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& DektecDVBTProperty();

    //!
    //! Enumeration (names/values) for Dektec DTU-315 modulator power modes.
    //! @ingroup hardware
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& DektecPowerMode();

    //!
    //! Check if this version of TSDuck was built with Dektec support.
    //! @ingroup hardware
    //! @return True is Dektec devices are supported. Always false on macOS or on Windows/Linux on non-Intel platforms.
    //!
    TSDUCKDLL bool HasDektecSupport();

    //!
    //! Get the versions of Dektec API and drivers in one single string.
    //! @ingroup hardware
    //! @return A string describing the Dektec versions (or the lack of Dektec support).
    //!
    TSDUCKDLL UString GetDektecVersions();

    //!
    //! Get the versions of Dektec API and drivers.
    //! @ingroup hardware
    //! @param [out] versions All versions. The map index is the driver or API name and the map value is its version.
    //!
    TSDUCKDLL void GetDektecVersions(std::map<UString,UString>& versions);
}
