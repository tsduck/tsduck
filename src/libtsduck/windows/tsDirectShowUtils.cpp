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
//
//  DirectShow & BDA utilities, Windows-specific.
//
//-----------------------------------------------------------------------------

#include "tsDirectShowUtils.h"
#include "tsMediaTypeUtils.h"
#include "tsComUtils.h"
#include "tsMemoryUtils.h"
#include "tsNullReport.h"
TSDUCK_SOURCE;

// Put the value of a property (named "type") into a COM object.
// Report errors through a variable named report (must be accessible).
// Return true on success, false on error.
#define PUT(obj,type,value) ComSuccess((obj)->put_##type(value), "error setting " #type, report)


//-----------------------------------------------------------------------------
// Enumerate all devices of the specified class.
// Fill a vector of monikers to these objects.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::EnumerateDevicesByClass(const ::CLSID& clsid, std::vector <ComPtr <::IMoniker>>& monikers, Report& report)
{
    // Reset content of vector
    monikers.clear();

    // Create a DirectShow System Device Enumerator
    ComPtr <::ICreateDevEnum> enum_devices(::CLSID_SystemDeviceEnum, ::IID_ICreateDevEnum, report);
    if (enum_devices.isNull()) {
        return false;
    }

    // Enumerate devices
    ComPtr <::IEnumMoniker> enum_monikers;
    ::HRESULT hr = enum_devices->CreateClassEnumerator(clsid, enum_monikers.creator(), 0);
    if (!ComSuccess(hr, "CreateClassEnumerator", report)) {
        return false;
    }
    if (hr != S_OK) {
        return true; // empty category
    }

    // Loop on all enumerated providers.
    ComPtr <::IMoniker> moniker;
    while (enum_monikers->Next(1, moniker.creator(), NULL) == S_OK) {
        monikers.push_back(moniker);
    }

    return true;
}


//-----------------------------------------------------------------------------
// Get names of a tuning space. Return empty string on error.
//-----------------------------------------------------------------------------

namespace {
    ts::UString ToStringAndFree(::HRESULT hr, ::BSTR& name, const ts::UChar* message, ts::Report& report)
    {
        ts::UString cname;
        if (ts::ComSuccess(hr, message, report)) {
            cname = ts::ToString(name);
        }
        if (name != NULL) {
            ::SysFreeString(name);
            name = NULL;
        }
        return cname;
    }
}

ts::UString ts::GetTuningSpaceFriendlyName(::ITuningSpace* tspace, Report& report)
{
    if (tspace == 0) {
        return UString();
    }
    else {
        ::BSTR name = NULL;
        return ToStringAndFree(tspace->get_FriendlyName(&name), name, u"ITuningSpace::get_FriendlyName", report);
    }
}

ts::UString ts::GetTuningSpaceUniqueName(::ITuningSpace* tspace, Report& report)
{
    if (tspace == 0) {
        return UString();
    }
    else {
        ::BSTR name = NULL;
        return ToStringAndFree(tspace->get_UniqueName(&name), name, u"ITuningSpace::get_UniqueName", report);
    }
}

ts::UString ts::GetTuningSpaceDescription(::ITuningSpace* tspace, Report& report)
{
    if (tspace == 0) {
        return UString();
    }

    // Get tuning space names.
    const UString fname(GetTuningSpaceFriendlyName(tspace, report));
    const UString uname(GetTuningSpaceUniqueName(tspace, report));
    UString tname;

    // Build description.
    if (!fname.empty()) {
        tname = u"\"" + fname + u"\"";
    }
    if (!uname.empty()) {
        if (!fname.empty()) {
            tname += u" (";
        }
        tname += uname;
        if (!fname.empty()) {
            tname += u")";
        }
    }

    // Check if this tuning space supports IDVBTuningSpace interface.
    ComPtr<::IDVBTuningSpace> dvb_tspace;
    dvb_tspace.queryInterface(tspace, ::IID_IDVBTuningSpace, NULLREP);
    if (!dvb_tspace.isNull()) {
        // This is a DVB tuning space. Get DVB system type.
        ::DVBSystemType sys_type = ::DVB_Cable;
        ::HRESULT hr = dvb_tspace->get_SystemType(&sys_type);
        if (ComSuccess(hr, u"cannot get DVB system type from tuning space \"" + fname + u"\"", report)) {
            if (!tname.empty()) {
                tname += u", DVB type: ";
            }
            tname += DVBSystemTypeName(sys_type);
        }
    }

    return tname;
}

ts::UString ts::GetTuningSpaceNetworkType(::ITuningSpace* tspace, Report& report)
{
    if (tspace == 0) {
        return UString();
    }

    // Get network type as a string.
    ::BSTR name = NULL;
    const UString type(ToStringAndFree(tspace->get_NetworkType(&name), name, u"ITuningSpace::get_NetworkType", report));
    
    // If the string looks like a GUID, try to find another way.
    if (type.empty() || type.front() == u'{') {
        // Get the network type as a GUID.
        ::GUID guid;
        if (SUCCEEDED(tspace->get__NetworkType(&guid))) {
            return NameGUID(guid);
        }
    }

    return type;
}


//-----------------------------------------------------------------------------
// Get the name for various enum values.
//-----------------------------------------------------------------------------

ts::UString ts::PinDirectionName(::PIN_DIRECTION dir)
{
    switch (dir) {
        case ::PINDIR_INPUT:  return u"input";
        case ::PINDIR_OUTPUT: return u"output";
        default:              return UString::Decimal(int(dir));
    }
}

ts::UString ts::DVBSystemTypeName(::DVBSystemType type)
{
    switch (type) {
        case ::DVB_Cable:        return u"DVB_Cable";
        case ::DVB_Terrestrial:  return u"DVB_Terrestrial";
        case ::DVB_Satellite:    return u"DVB_Satellite";
        case ::ISDB_Terrestrial: return u"ISDB_Terrestrial";
        case ::ISDB_Satellite:   return u"ISDB_Satellite";
        default:                 return UString::Decimal(int(type));
    }
}


//-----------------------------------------------------------------------------
// Create a DirectShow tune request object from tuning parameters.
//-----------------------------------------------------------------------------

bool ts::CreateTuneRequest(ComPtr<::ITuneRequest>& request, ::ITuningSpace* tuning_space, const TunerParameters& params, Report& report)
{
    if (tuning_space == 0) {
        return false;
    }

    // Create a DirectShow tune request
    ComPtr<::ITuneRequest> tune_request;
    ::HRESULT hr = tuning_space->CreateTuneRequest(tune_request.creator());
    if (!ComSuccess(hr, "cannot create DirectShow tune request", report)) {
        return false;
    }
    assert(!tune_request.isNull());

    // If this is a DVB tuning space, get DVB interface of tune request and set ids to wildcards.
    ComPtr<::IDVBTuneRequest> dvb_request;
    dvb_request.queryInterface(tune_request.pointer(), ::IID_IDVBTuneRequest, report);
    if (!dvb_request.isNull() &&
        (!PUT(dvb_request, ONID, -1) ||
         !PUT(dvb_request, TSID, -1) ||
         !PUT(dvb_request, SID, -1))) {
        return false;
    }

    // Create a locator (where to find the physical TS, ie. tuning params).
    ComPtr<::IDigitalLocator> locator;
    if (!CreateLocator(locator, params, report)) {
        return false;
    }
    assert(!locator.isNull());

    // Set the locator in the tune request
    hr = tune_request->put_Locator(locator.pointer());
    if (!ComSuccess(hr, "ITuneRequest::put_Locator", report)) {
        return false;
    }

    // Tune request fully built.
    request.assign(tune_request);
    return true;
}


//-----------------------------------------------------------------------------
// Create a Locator object for tuning parameters.
//-----------------------------------------------------------------------------

bool ts::CreateLocator(ComPtr<::IDigitalLocator>& locator, const TunerParameters& params, Report& report)
{
    // Try to convert the parameters to various known types.
    const TunerParametersDVBS* dvb_s = dynamic_cast<const TunerParametersDVBS*>(&params);
    const TunerParametersDVBT* dvb_t = dynamic_cast<const TunerParametersDVBT*>(&params);
    const TunerParametersDVBC* dvb_c = dynamic_cast<const TunerParametersDVBC*>(&params);

    // Create the locator depending on the type.
    if (dvb_s != 0) {
        return CreateLocatorDVBS(locator, *dvb_s, report);
    }
    else if (dvb_t != 0) {
        return CreateLocatorDVBT(locator, *dvb_t, report);
    }
    else if (dvb_c != 0) {
        return CreateLocatorDVBC(locator, *dvb_c, report);
    }
    else {
        report.error(u"cannot convert " + TunerTypeEnum.name(params.tunerType()) + " parameters to DirectShow tuning parameters");
        return false;
    }
}


//-----------------------------------------------------------------------------
// Create an IDigitalLocator object for DVB-T parameters
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::CreateLocatorDVBT(ComPtr<::IDigitalLocator>& locator, const TunerParametersDVBT& params, Report& report)
{
    ComPtr<::IDVBTLocator> loc(CLSID_DVBTLocator, ::IID_IDVBTLocator, report);

    if (loc.isNull() ||
        !CheckModEnum(params.inversion, "spectral inversion", SpectralInversionEnum, report) ||
        !CheckModEnum(params.bandwidth, "bandwidth", BandWidthEnum, report) ||
        !CheckModEnum(params.fec_hp, "FEC", InnerFECEnum, report) ||
        !CheckModEnum(params.fec_lp, "FEC", InnerFECEnum, report) ||
        !CheckModEnum(params.modulation, "constellation", ModulationEnum, report) ||
        !CheckModEnum(params.transmission_mode, "transmission mode", TransmissionModeEnum, report) ||
        !CheckModEnum(params.guard_interval, "guard interval", GuardIntervalEnum, report) ||
        !CheckModEnum(params.hierarchy, "hierarchy", HierarchyEnum, report) ||
        !PUT(loc, CarrierFrequency, long(params.frequency / 1000)) ||  // frequency in kHz
        !PUT(loc, Modulation, ::ModulationType(params.modulation)) ||
        !PUT(loc, Bandwidth, long(params.bandwidth)) ||
        !PUT(loc, Guard, ::GuardInterval(params.guard_interval)) ||
        !PUT(loc, LPInnerFEC, ::BDA_FEC_VITERBI) ||
        !PUT(loc, LPInnerFECRate, ::BinaryConvolutionCodeRate(params.fec_lp)) ||
        !PUT(loc, Mode, ::TransmissionMode(params.transmission_mode)) ||
        !PUT(loc, HAlpha, ::HierarchyAlpha(params.hierarchy)))
    {
        return false;
    }

    // Pending questions:
    // - Shall we call loc->put_OtherFrequencyInUse ? Documented as
    //   "specifies whether the frequency is being used by another
    //   DVB-T broadcaster". No idea what this means...
    // - No way to set params.inversion and params.fec_hp in IDVBTLocator

    locator.assign(loc);
    return true;
}


//-----------------------------------------------------------------------------
// Create an IDigitalLocator object for DVB-C parameters
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::CreateLocatorDVBC(ComPtr<::IDigitalLocator>& locator, const TunerParametersDVBC& params, Report& report)
{
    ComPtr<::IDVBCLocator> loc(CLSID_DVBCLocator, ::IID_IDVBCLocator, report);

    if (loc.isNull() ||
        !CheckModEnum(params.inversion, "spectral inversion", SpectralInversionEnum, report) ||
        !CheckModEnum(params.inner_fec, "FEC", InnerFECEnum, report) ||
        !CheckModEnum(params.modulation, "modulation", ModulationEnum, report) ||
        !PUT(loc, CarrierFrequency, long(params.frequency / 1000)) ||  // frequency in kHz
        !PUT(loc, Modulation, ::ModulationType(params.modulation)) ||
        !PUT(loc, InnerFEC, ::BDA_FEC_VITERBI) ||
        !PUT(loc, InnerFECRate, ::BinaryConvolutionCodeRate(params.inner_fec)) ||
        !PUT(loc, SymbolRate, long(params.symbol_rate)))
    {
        return false;
    }

    // Pending questions:
    // - No way to set params.inversion in IDVBCLocator

    locator.assign(loc);
    return true;
}


//-----------------------------------------------------------------------------
// Create an IDigitalLocator object for DVB-S/S2 parameters
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::CreateLocatorDVBS(ComPtr<::IDigitalLocator>& locator, const TunerParametersDVBS& params, Report& report)
{
    // Specify DiSEqC satellite number.
    // Note however that most drivers ignore it...
    ::LNB_Source source = ::BDA_LNB_SOURCE_NOT_SET;
    switch (params.satellite_number) {
        case 0:  source = ::BDA_LNB_SOURCE_A; break;
        case 1:  source = ::BDA_LNB_SOURCE_B; break;
        case 2:  source = ::BDA_LNB_SOURCE_C; break;
        case 3:  source = ::BDA_LNB_SOURCE_D; break;
        default: source = ::BDA_LNB_SOURCE_NOT_DEFINED; break;
    }

    // Microsoft oddity, part 1...
    //
    // The locator interface for DVB-S is IDVBSLocator. However, this interface did
    // not implement LNB control and DVB-S2. Starting with Windows 7, a new interface
    // IDVBSLocator2 is introduced to support LNB control and DVB-S2. However, unlike
    // all other locator interfaces, CLSID_DVBSLocator2 is not defined anywhere, not
    // in tuner.h and not even in the Windows 7 registry. So, since IDVBSLocator2 is
    // a subinterface of IDVBSLocator, we create an object of class CLSID_DVBSLocator
    // and we hope that on Windows 7 this object will also implement IDVBSLocator2.
    //
    // Microsoft oddity, part 2...
    //
    // Unlike other modulations, with pre-Windows 7 systems, some of the DVB-S
    // parameters must be set in the tuning space (IDVBSTuningSpace interface)
    // and not in the locator (IDVBSLocator interface). However, Microsoft seemed
    // to understand the mistake in Windows 7 and finally added these parameters
    // in IDVBSLocator2.
    //
    // Starting with TSDuck 3.x, we decided to completely drop support for versions
    // of Windows before Windows 7. We now require IDVBSLocator2.

    ComPtr<::IDVBSLocator2> loc(CLSID_DVBSLocator, ::IID_IDVBSLocator2, report);

    if (loc.isNull() ||
        !CheckModEnum(params.modulation, "modulation", ModulationEnum, report) ||
        !CheckModEnum(params.inner_fec, "FEC", InnerFECEnum, report) ||
        !CheckModEnum(params.polarity, "polarity", PolarizationEnum, report) ||
        !PUT(loc, CarrierFrequency, long(params.frequency / 1000)) ||  // frequency in kHz
        !PUT(loc, Modulation, ::ModulationType(params.modulation)) ||
        !PUT(loc, SignalPolarisation, ::Polarisation(params.polarity)) ||
        !PUT(loc, InnerFEC, ::BDA_FEC_VITERBI) ||
        !PUT(loc, InnerFECRate, ::BinaryConvolutionCodeRate(params.inner_fec)) ||
        !PUT(loc, SymbolRate, long(params.symbol_rate)) ||
        !PUT(loc, LocalSpectralInversionOverride, ::SpectralInversion(params.inversion)) ||
        !PUT(loc, LocalOscillatorOverrideLow, long(params.lnb.lowFrequency() / 1000)) ||    // frequency in kHz
        !PUT(loc, LocalOscillatorOverrideHigh, long(params.lnb.highFrequency() / 1000)) ||  // frequency in kHz
        !PUT(loc, LocalLNBSwitchOverride, long(params.lnb.switchFrequency() / 1000)) ||     // frequency in kHz
        !PUT(loc, DiseqLNBSource, source))
    {
        return false;
    }

    // DVB-S2 specific parameters
    if (params.delivery_system == DS_DVB_S2 &&
        (!CheckModEnum(params.pilots, "pilot", PilotEnum, report) ||
         !CheckModEnum(params.roll_off, "roll-off factor", RollOffEnum, report) ||
         !PUT(loc, SignalPilot, ::Pilot(params.pilots)) ||
         !PUT(loc, SignalRollOff, ::RollOff(params.roll_off))))
    {
        return false;
    }

    locator.assign(loc); // loc and loc2 are two interfaces of the same object
    return true;
}
