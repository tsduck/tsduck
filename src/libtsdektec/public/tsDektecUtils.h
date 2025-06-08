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
#include "tsBitRate.h"
#include "tsModulation.h"

namespace ts {

    class ModulationArgs;

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
    //! Get the versions of Dektec API and drivers in one single string.
    //! @ingroup hardware
    //! @return A string describing the Dektec versions (or the lack of Dektec support).
    //!
    TSDEKTECDLL UString GetDektecVersions();

    //!
    //! Get the versions of Dektec API and drivers.
    //! @ingroup hardware
    //! @param [out] versions All versions. The map index is the driver or API name and the map value is its version.
    //!
    TSDEKTECDLL void GetDektecVersions(std::map<UString,UString>& versions);

    //!
    //! Enumeration (names/values) for Dektec modulation constants (DTAPI_MOD_DVBS_QPSK, etc).
    //! @ingroup hardware
    //! @return A constant reference to the enumeration description.
    //!
    TSDEKTECDLL const Names& DektecModulationTypes();

    //!
    //! Enumeration (names/values) for Dektec VSB constants (DTAPI_MOD_ATSC_VSB8, etc).
    //! @ingroup hardware
    //! @return A constant reference to the enumeration description.
    //!
    TSDEKTECDLL const Names& DektecVSB();

    //!
    //! Enumeration (names/values) for Dektec FEC constants (DTAPI_MOD_1_2, etc).
    //! @ingroup hardware
    //! @return A constant reference to the enumeration description.
    //!
    TSDEKTECDLL const Names& DektecFEC();

    //!
    //! Enumeration (names/values) for Dektec spectral inversion constants (DTAPI_MOD_SPECNONINV, etc).
    //! @ingroup hardware
    //! @return A constant reference to the enumeration description.
    //!
    TSDEKTECDLL const Names& DektecInversion();

    //!
    //! Enumeration (names/values) for Dektec DVB-T properties constants (DTAPI_MOD_DVBT_*, etc).
    //! @ingroup hardware
    //! @return A constant reference to the enumeration description.
    //!
    TSDEKTECDLL const Names& DektecDVBTProperty();

    //!
    //! Enumeration (names/values) for Dektec DTU-315 modulator power modes.
    //! @ingroup hardware
    //! @return A constant reference to the enumeration description.
    //!
    TSDEKTECDLL const Names& DektecPowerMode();

    //!
    //! Convert a InnerFEC value into a "FEC type" for Dektec modulator cards.
    //! @param [out] out Returned FEC type (DTAPI_MOD_* value). Unchanged in case of error.
    //! @param [out] in Input FEC type (enum type).
    //! @return True on success, false on error (includes unsupported operation).
    //!
    TSDEKTECDLL bool ToDektecCodeRate(int& out, InnerFEC in);

    //!
    //! Attempt to get a "FEC type" for Dektec modulator cards from a ModulationArgs.
    //! @param [out] fec FEC type (DTAPI_MOD_* value). Unchanged in case of error.
    //! @param [in] args Modulation arguments.
    //! @return True on success, false on error (includes unsupported operation).
    //!
    TSDEKTECDLL bool GetDektecCodeRate(int& fec, const ModulationArgs& args);

    //!
    //! Attempt to get a "modulation type" for Dektec modulator cards from a ModulationArgs.
    //! @param [out] type Modulation type (DTAPI_MOD_* value). Unchanged in case of error.
    //! @param [in] args Modulation arguments.
    //! @return True on success, false on error (includes unsupported operation).
    //!
    TSDEKTECDLL bool GetDektecModulationType(int& type, const ModulationArgs& args);

    //!
    //! Attempt to convert the tuning parameters in modulation parameters for Dektec modulator cards.
    //! @param [out] modulation_type Modulation type (DTAPI_MOD_* value).
    //! @param [out] param0 Modulation-specific parameter 0.
    //! @param [out] param1 Modulation-specific parameter 1.
    //! @param [out] param2 Modulation-specific parameter 2.
    //! @param [in] args Modulation arguments.
    //! @return True on success, false on error (includes unsupported operation).
    //!
    TSDEKTECDLL bool GetDektecModulation(int& modulation_type, int& param0, int& param1, int& param2, const ModulationArgs& args);

    //!
    //! Attempt to compute a bitrate from a ModulationArgs using the Dektec DTAPI library.
    //! @param [out] bitrate Computed bitrate.
    //! @param [in] args Modulation arguments.
    //! @return True on success, false on error (includes unsupported operation).
    //!
    TSDEKTECDLL bool GetDektecBitRate(BitRate& bitrate, const ModulationArgs& args);
}
