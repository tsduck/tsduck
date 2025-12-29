//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtsduck windows
//!  DirectShow & BDA utilities (Windows-specific).
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsModulationArgs.h"
#include "tsComPtr.h"
#include "tsDirectShow.h"

namespace ts {
    //!
    //! Enumerate all devices of the specified class.
    //! Fill a vector of monikers to these objects.
    //! @param [in] clsid Device class to enumerate.
    //! @param [out] monikers Returned vector of monikers to all devices of class @a clsid.
    //! @param [in,out] report Where to report errors.
    //! @param [in] flags Flags for CreateClassEnumerator().
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool EnumerateDevicesByClass(const ::CLSID& clsid,
                                           std::vector<ComPtr<::IMoniker>>& monikers,
                                           Report& report,
                                           ::DWORD flags = 0);

    //!
    //! Get the user-friendly name of a DirectShow tuning space (Windows-specific).
    //! @param [in] tuning Tuning space.
    //! @param [in,out] report Where to report errors.
    //! @return Tuning space name or an empty string on error.
    //!
    TSDUCKDLL UString GetTuningSpaceFriendlyName(::ITuningSpace* tuning, Report& report);

    //!
    //! Get the unique name of a DirectShow tuning space (Windows-specific).
    //! @param [in] tuning Tuning space.
    //! @param [in,out] report Where to report errors.
    //! @return Tuning space name or an empty string on error.
    //!
    TSDUCKDLL UString GetTuningSpaceUniqueName(::ITuningSpace* tuning, Report& report);

    //!
    //! Get the class name of a DirectShow tuning space (Windows-specific).
    //! @param [in] tuning Tuning space.
    //! @param [in,out] report Where to report errors.
    //! @return Tuning space class name or an empty string on error.
    //!
    TSDUCKDLL UString GetTuningSpaceClass(::ITuningSpace* tuning, Report& report);

    //!
    //! Get full description of a DirectShow tuning space (Windows-specific).
    //! @param [in] tuning Tuning space.
    //! @param [in,out] report Where to report errors.
    //! @return Tuning description or an empty string on error.
    //!
    TSDUCKDLL UString GetTuningSpaceDescription(::ITuningSpace* tuning, Report& report);

    //!
    //! Get the network type of a DirectShow tuning space (Windows-specific).
    //! @param [in] tuning Tuning space.
    //! @param [in,out] report Where to report errors.
    //! @return Network type or an empty string on error.
    //!
    TSDUCKDLL UString GetTuningSpaceNetworkType(::ITuningSpace* tuning, Report& report);

    //!
    //! Get the name for a DirectShow pin direction value (Windows-specific).
    //! @param [in] dir Pin direction.
    //! @return Corresponding name.
    //!
    TSDUCKDLL UString PinDirectionName(::PIN_DIRECTION dir);

    //!
    //! Get the name for a DirectShow @c DVBSystemType value (Windows-specific).
    //! @param [in] type DVB system type value.
    //! @return Corresponding name.
    //!
    TSDUCKDLL UString DVBSystemTypeName(::DVBSystemType type);

    //!
    //! Create a DirectShow tune request object from tuning parameters.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [out] request COM pointer to the ITuneRequest interface of the created object.
    //! @param [in] tuning_space Associated tuning space.
    //! @param [in] params Tuning parameters in portable format.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateTuneRequest(DuckContext& duck, ComPtr<::ITuneRequest>& request, ::ITuningSpace* tuning_space, const ModulationArgs& params);

    //!
    //! Create a Locator object for tuning parameters.
    //! A locator object indicates where to find the physical TS, ie. tuning params.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params Tuning parameters in portable format. All required parameters must be set.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocator(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params);

    //!
    //! Create an IDigitalLocator object for DVB-S parameters.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params DVB-S parameters in portable format. All required parameters must be set.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocatorDVBS(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params);

    //!
    //! Create an IDigitalLocator object for DVB-T parameters.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params DVB-T parameters in portable format. All required parameters must be set.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocatorDVBT(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params);

    //!
    //! Create an IDigitalLocator object for DVB-C parameters.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params DVB-C parameters in portable format. All required parameters must be set.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocatorDVBC(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params);

    //!
    //! Create an IDigitalLocator object for ATSC parameters.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params ATSC parameters in portable format. All required parameters must be set.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocatorATSC(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params);

    //!
    //! Create an IDigitalLocator object for ISDB-S parameters.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params DVB-S parameters in portable format. All required parameters must be set.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocatorISDBS(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params);
}
