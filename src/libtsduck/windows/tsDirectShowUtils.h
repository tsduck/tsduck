//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  DirectShow & BDA utilities, Windows-specific.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsTunerParametersDVBS.h"
#include "tsTunerParametersDVBC.h"
#include "tsTunerParametersDVBT.h"
#include "tsTunerParametersATSC.h"
#include "tsModulation.h"
#include "tsComPtr.h"

namespace ts {
    //!
    //! Translate a DirectShow network provider class id into a TSDuck tuner type (Windows-specific).
    //! @param [in] provider_clsid DirectShow network provider class.
    //! @param [out] tuner_type Returned TSDuck tuner type.
    //! @return True on success, false if no match is found.
    //!
    TSDUCKDLL bool NetworkProviderToTunerType(const ::GUID provider_clsid, TunerType& tuner_type);

    //!
    //! Enumerate all devices of the specified class.
    //! Fill a vector of monikers to these objects.
    //! @param [in] clsid Device class to enumerate.
    //! @param [out] monikers Returned vector of monikers to all devices of class @a clsid.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool EnumerateDevicesByClass(const ::CLSID& clsid,
                                           std::vector<ComPtr<::IMoniker>>& monikers,
                                           ReportInterface& report);

    //!
    //! Get the user-friendly name of a DirectShow tuning space (Windows-specific).
    //! @param [in] tuning Tuning space.
    //! @param [in,out] report Where to report errors.
    //! @return Tuning space name or an empty string on error.
    //!
    TSDUCKDLL std::string GetTuningSpaceFriendlyName(::ITuningSpace* tuning, ReportInterface& report);

    //!
    //! Get the unique name of a DirectShow tuning space (Windows-specific).
    //! @param [in] tuning Tuning space.
    //! @param [in,out] report Where to report errors.
    //! @return Tuning space name or an empty string on error.
    //!
    TSDUCKDLL std::string GetTuningSpaceUniqueName(::ITuningSpace* tuning, ReportInterface& report);

    //!
    //! Get full description of a DirectShow tuning space (Windows-specific).
    //! @param [in] tuning Tuning space.
    //! @param [in,out] report Where to report errors.
    //! @return Tuning description or an empty string on error.
    //!
    TSDUCKDLL std::string GetTuningSpaceDescription(::ITuningSpace* tuning, ReportInterface& report);

    //!
    //! Get the network type of a DirectShow tuning space (Windows-specific).
    //! @param [in] tuning Tuning space.
    //! @param [in,out] report Where to report errors.
    //! @return Network type or an empty string on error.
    //!
    TSDUCKDLL std::string GetTuningSpaceNetworkType(::ITuningSpace* tuning, ReportInterface& report);

    //!
    //! Get the name for a DirectShow pin direction value (Windows-specific).
    //! @param [in] dir Pin direction.
    //! @return Corresponding name.
    //!
    TSDUCKDLL std::string PinDirectionName(::PIN_DIRECTION dir);

    //!
    //! Get the name for a DirectShow @c DVBSystemType value (Windows-specific).
    //! @param [in] type DVB system type value.
    //! @return Corresponding name.
    //!
    TSDUCKDLL std::string DVBSystemTypeName(::DVBSystemType type);

    //!
    //! Create a DirectShow tune request object from tuning parameters.
    //! @param [out] request COM pointer to the ITuneRequest interface of the created object.
    //! @param [in] tuning_space Associated tuning space.
    //! @param [in] params Tuning parameters in portable format.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateTuneRequest(ComPtr<::ITuneRequest>& request, ::ITuningSpace* tuning_space, const TunerParameters& params, ReportInterface& report);

    //!
    //! Create a Locator object for tuning parameters.
    //! A locator object indicates where to find the physical TS, ie. tuning params.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params Tuning parameters in portable format.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocator(ComPtr<::IDigitalLocator>& locator, const TunerParameters& params, ReportInterface& report);

    //!
    //! Create an IDigitalLocator object for DVB-S parameters.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params DVB-S parameters in portable format.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocatorDVBS(ComPtr<::IDigitalLocator>& locator, const TunerParametersDVBS& params, ReportInterface& report);

    //!
    //! Create an IDigitalLocator object for DVB-T parameters.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params DVB-T parameters in portable format.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocatorDVBT(ComPtr<::IDigitalLocator>& locator, const TunerParametersDVBT& params, ReportInterface& report);

    //!
    //! Create an IDigitalLocator object for DVB-C parameters.
    //! @param [out] locator COM pointer to the IDigitalLocator interface of the created object.
    //! @param [in] params DVB-C parameters in portable format.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CreateLocatorDVBC(ComPtr<::IDigitalLocator>& locator, const TunerParametersDVBC& params, ReportInterface& report);
}
