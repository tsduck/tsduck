//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//!  @ingroup windows
//!  DirectShow & BDA utilities (Windows-specific).
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsModulationArgs.h"
#include "tsComPtr.h"

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
    //! Get a DirectShow tuning space from a network type (Windows-specific).
    //! @param [in] network_type GUID of network type.
    //! @param [out] tuner_type Corresponding tuner type.
    //! @param [out] tuning_space Corresponding tuning space.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool GetTuningSpaceFromNetworkType(const ::GUID& network_type, TunerType& tuner_type, ComPtr<::ITuningSpace>& tuning_space, Report& report);

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
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateTuneRequest(DuckContext& duck, ComPtr<::ITuneRequest>& request, ::ITuningSpace* tuning_space, const ModulationArgs& params, Report& report);

    //!
    //! Reset the content of a TuningSpace object.
    //! @param [in,out] tspace Pointer to the ITuningSpace interface of the object.
    //! @param [in] name Name to use as unique name and friendly name.
    //! @param [in] ntype GUID of associated network type.
    //! @param [in] dlocator Default locator to set.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool ResetTuningSpace(::ITuningSpace* tspace, ::WCHAR* name, const ::GUID& ntype, ::ILocator* dlocator, Report& report);

    //!
    //! Reset the content of a DVBTuningSpace object.
    //! @param [in,out] tspace Pointer to the IDVBTuningSpace interface of the object.
    //! @param [in] name Name to use as unique name and friendly name.
    //! @param [in] ntype GUID of associated network type.
    //! @param [in] stype DVB system type.
    //! @param [in] dlocator Default locator to set.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool ResetDVBTuningSpace(::IDVBTuningSpace* tspace, ::WCHAR* name, const ::GUID& ntype, ::DVBSystemType stype, ::ILocator* dlocator, Report& report);

    //!
    //! Reset the content of a DVBTuningSpace2 object.
    //! @param [in,out] tspace Pointer to the IDVBTuningSpace2 interface of the object.
    //! @param [in] name Name to use as unique name and friendly name.
    //! @param [in] ntype GUID of associated network type.
    //! @param [in] stype DVB system type.
    //! @param [in] dlocator Default locator to set.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool ResetDVBTuningSpace2(::IDVBTuningSpace2* tspace, ::WCHAR* name, const ::GUID& ntype, ::DVBSystemType stype, ::ILocator* dlocator, Report& report);

    //!
    //! Reset the content of a DVBSTuningSpace object.
    //! @param [in,out] tspace Pointer to the IDVBSTuningSpace interface of the object.
    //! @param [in] name Name to use as unique name and friendly name.
    //! @param [in] ntype GUID of associated network type.
    //! @param [in] stype DVB system type.
    //! @param [in] dlocator Default locator to set.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool ResetDVBSTuningSpace(::IDVBSTuningSpace* tspace, ::WCHAR* name, const ::GUID& ntype, ::DVBSystemType stype, ::ILocator* dlocator, Report& report);

    //!
    //! Reset the content of an ATSCTuningSpace object.
    //! @param [in,out] tspace Pointer to the IATSCTuningSpace interface of the object.
    //! @param [in] name Name to use as unique name and friendly name.
    //! @param [in] ntype GUID of associated network type.
    //! @param [in] ttype Tuner input type.
    //! @param [in] dlocator Default locator to set.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool ResetATSCTuningSpace(::IATSCTuningSpace* tspace, ::WCHAR* name, const ::GUID& ntype, ::TunerInputType ttype, ::ILocator* dlocator, Report& report);

    //!
    //! Reset the content of a DigitalCableTuningSpace object.
    //! @param [in,out] tspace Pointer to the IDigitalCableTuningSpace interface of the object.
    //! @param [in] name Name to use as unique name and friendly name.
    //! @param [in] ntype GUID of associated network type.
    //! @param [in] ttype Tuner input type.
    //! @param [in] dlocator Default locator to set.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool ResetDigitalCableTuningSpace(::IDigitalCableTuningSpace* tspace, ::WCHAR* name, const ::GUID& ntype, ::TunerInputType ttype, ::ILocator* dlocator, Report& report);

    //!
    //! Reset the content of a Locator object.
    //! @param [in,out] locator Pointer to the ILocator interface of the object.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool ResetLocator(::ILocator* locator, Report& report);

    //!
    //! Reset the content of a DVBTLocator object.
    //! @param [in,out] locator Pointer to the IDVBTLocator interface of the object.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool ResetDVBTLocator(::IDVBTLocator* locator, Report& report);

    //!
    //! Reset the content of a DVBSLocator object.
    //! @param [in,out] locator Pointer to the IDVBSLocator interface of the object.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool ResetDVBSLocator(::IDVBSLocator* locator, Report& report);

    //!
    //! Reset the content of an ATSCLocator object.
    //! @param [in,out] locator Pointer to the IATSCLocator interface of the object.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool ResetATSCLocator(::IATSCLocator* locator, Report& report);

    //!
    //! Reset the content of an ATSCLocator2 object.
    //! @param [in,out] locator Pointer to the IATSCLocator2 interface of the object.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool ResetATSCLocator2(::IATSCLocator2* locator, Report& report);

    //!
    //! Create a Locator object for tuning parameters.
    //! A locator object indicates where to find the physical TS, ie. tuning params.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params Tuning parameters in portable format. All required parameters must be set.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocator(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params, Report& report);

    //!
    //! Create an IDigitalLocator object for DVB-S parameters.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params DVB-S parameters in portable format. All required parameters must be set.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocatorDVBS(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params, Report& report);

    //!
    //! Create an IDigitalLocator object for DVB-T parameters.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params DVB-T parameters in portable format. All required parameters must be set.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocatorDVBT(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params, Report& report);

    //!
    //! Create an IDigitalLocator object for DVB-C parameters.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params DVB-C parameters in portable format. All required parameters must be set.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocatorDVBC(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params, Report& report);

    //!
    //! Create an IDigitalLocator object for ATSC parameters.
    //! @param [in,out] duck TSDuck execution context.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params ATSC parameters in portable format. All required parameters must be set.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocatorATSC(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params, Report& report);
}
