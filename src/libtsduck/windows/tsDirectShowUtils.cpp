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

#include "tsDirectShowUtils.h"
#include "tsDuckContext.h"
#include "tsHFBand.h"
#include "tsMediaTypeUtils.h"
#include "tsWinUtils.h"
#include "tsMemory.h"
#include "tsNullReport.h"
TSDUCK_SOURCE;

// Put the value of a property (named "type") into a COM object.
// Report errors through a variable named report (must be accessible).
// Return true on success, false on error.
#define PUT(obj,type,value) ts::ComSuccess((obj)->put_##type(value), u"error setting " #type, report)

// Same with a variable value. Silently return true if the variable is unset.
#define PUTVAR(obj,type,var) ((var).set() ? PUT(obj,type,(var).value()) : true)


//-----------------------------------------------------------------------------
// Enumerate all devices of the specified class.
// Fill a vector of monikers to these objects.
// Return true on success, false on error.
//-----------------------------------------------------------------------------

bool ts::EnumerateDevicesByClass(const ::CLSID& clsid, std::vector <ComPtr <::IMoniker>>& monikers, Report& report, ::DWORD flags)
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
    ::HRESULT hr = enum_devices->CreateClassEnumerator(clsid, enum_monikers.creator(), flags);
    if (!ComSuccess(hr, u"CreateClassEnumerator", report)) {
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
// Reset the content of locator objects.
//-----------------------------------------------------------------------------

bool ts::ResetLocator(::ILocator* loc, Report& report)
{
    return loc != nullptr &&
        PUT(loc, CarrierFrequency, long(-1)) &&
        PUT(loc, Modulation, ::BDA_MOD_NOT_SET) &&
        PUT(loc, InnerFEC, ::BDA_FEC_METHOD_NOT_SET) &&
        PUT(loc, InnerFECRate, ::BDA_BCC_RATE_NOT_SET) &&
        PUT(loc, OuterFEC, ::BDA_FEC_METHOD_NOT_SET) &&
        PUT(loc, OuterFECRate, ::BDA_BCC_RATE_NOT_SET) &&
        PUT(loc, SymbolRate, long(-1));
}

bool ts::ResetDVBTLocator(::IDVBTLocator* loc, Report& report)
{
    return ResetLocator(loc, report) &&
        PUT(loc, Bandwidth, long(-1)) &&
        PUT(loc, LPInnerFEC, ::BDA_FEC_METHOD_NOT_SET) &&
        PUT(loc, LPInnerFECRate, ::BDA_BCC_RATE_NOT_SET) &&
        PUT(loc, HAlpha, ::BDA_HALPHA_NOT_SET) &&
        PUT(loc, Guard, ::BDA_GUARD_NOT_SET) &&
        PUT(loc, Mode, ::BDA_XMIT_MODE_NOT_SET) &&
        PUT(loc, OtherFrequencyInUse, 0);
}

bool ts::ResetDVBSLocator(::IDVBSLocator* loc, Report& report)
{
    return ResetLocator(loc, report) &&
        PUT(loc, SignalPolarisation, ::BDA_POLARISATION_NOT_SET) &&
        PUT(loc, WestPosition, 0) &&
        PUT(loc, OrbitalPosition, long(-1)) &&
        PUT(loc, Azimuth, long(-1)) &&
        PUT(loc, Elevation, long(-1));
}

bool ts::ResetATSCLocator(::IATSCLocator* loc, Report& report)
{
    return ResetLocator(loc, report) &&
        PUT(loc, PhysicalChannel, long(-1)) &&
        PUT(loc, TSID, long(-1));
}

bool ts::ResetATSCLocator2(::IATSCLocator2* loc, Report& report)
{
    return ResetATSCLocator(loc, report) &&
        PUT(loc, ProgramNumber, long(-1));
}


//-----------------------------------------------------------------------------
// Reset the content of a TuningSpace object.
//-----------------------------------------------------------------------------

bool ts::ResetTuningSpace(::ITuningSpace* tspace, ::WCHAR* name, const ::GUID& ntype, ::ILocator* dlocator, Report& report)
{
    return tspace != nullptr &&
        PUT(tspace, UniqueName, name) &&
        PUT(tspace, FriendlyName, name) &&
        PUT(tspace, _NetworkType, ntype) &&
        PUT(tspace, DefaultLocator, dlocator);
}

bool ts::ResetDVBTuningSpace(::IDVBTuningSpace* tspace, ::WCHAR* name, const ::GUID& ntype, ::DVBSystemType stype, ::ILocator* dlocator, Report& report)
{
    return ResetTuningSpace(tspace, name, ntype, dlocator, report) &&
        PUT(tspace, SystemType, stype);
}

bool ts::ResetDVBTuningSpace2(::IDVBTuningSpace2* tspace, ::WCHAR* name, const ::GUID& ntype, ::DVBSystemType stype, ::ILocator* dlocator, Report& report)
{
    return ResetDVBTuningSpace(tspace, name, ntype, stype, dlocator, report) && 
        PUT(tspace, NetworkID, long(-1));
}

bool ts::ResetDVBSTuningSpace(::IDVBSTuningSpace* tspace, ::WCHAR* name, const ::GUID& ntype, ::DVBSystemType stype, ::ILocator* dlocator, Report& report)
{
    return ResetDVBTuningSpace2(tspace, name, ntype, stype, dlocator, report) && 
        PUT(tspace, LNBSwitch, 11700000) &&
        PUT(tspace, LowOscillator, 9750000) &&
        PUT(tspace, HighOscillator, 10600000) &&
        PUT(tspace, SpectralInversion, ::BDA_SPECTRAL_INVERSION_NOT_SET);
}

bool ts::ResetATSCTuningSpace(::IATSCTuningSpace* tspace, ::WCHAR* name, const ::GUID& ntype, ::TunerInputType ttype, ::ILocator* dlocator, Report& report)
{
    return ResetTuningSpace(tspace, name, ntype, dlocator, report) &&
        PUT(tspace, InputType, ttype) &&
        PUT(tspace, CountryCode, 0) &&
        PUT(tspace, MinChannel, long(ttype == ::TunerInputType::TunerInputAntenna ? 1 : -1)) &&
        PUT(tspace, MaxChannel, long(ttype == ::TunerInputType::TunerInputAntenna ? 99 : 9999)) &&
        PUT(tspace, MinMinorChannel, long(ttype == ::TunerInputType::TunerInputAntenna ? 0 : -1)) &&
        PUT(tspace, MaxMinorChannel, 999) &&
        PUT(tspace, MinPhysicalChannel, long(ttype == ::TunerInputType::TunerInputAntenna ? 1 : 2)) &&
        PUT(tspace, MaxPhysicalChannel, 158);
}

bool ts::ResetDigitalCableTuningSpace(::IDigitalCableTuningSpace* tspace, ::WCHAR* name, const ::GUID& ntype, ::TunerInputType ttype, ::ILocator* dlocator, Report& report)
{
    return ResetATSCTuningSpace(tspace, name, ntype, ttype, dlocator, report) &&
        PUT(tspace, MinMajorChannel, -1) &&
        PUT(tspace, MaxMajorChannel, 99) &&
        PUT(tspace, MinSourceID, 0) &&
        PUT(tspace, MaxSourceID, 0x7FFFFFFF);
}


//-----------------------------------------------------------------------------
// Get a DirectShow tuning space from a network type (Windows-specific).
//-----------------------------------------------------------------------------

bool ts::GetTuningSpaceFromNetworkType(const ::GUID& network_type, TunerType& tuner_type, ComPtr<::ITuningSpace>& tuning_space, Report& report)
{
    // Make sure that previous object is released.
    tuning_space.release();
    tuner_type = TT_UNDEFINED;

    // Now, we have to try all known network types, one by one.

    // DVB-C network.
    if (network_type == CLSID_DVBCNetworkProvider) {
        tuner_type = TT_DVB_C;
        ComPtr<::IDVBCLocator> loc(CLSID_DVBCLocator, ::IID_IDVBCLocator, report);
        if (!ResetLocator(loc.pointer(), report)) {
            return false;
        }
        ComPtr<::IDVBTuningSpace> tspace(CLSID_DVBTuningSpace, ::IID_IDVBTuningSpace, report);
        if (!ResetDVBTuningSpace(tspace.pointer(), L"TSDuck DVB-C Tuning Space", network_type, ::DVBSystemType::DVB_Cable, loc.pointer(), report)) {
            return false;
        }
        tuning_space.assign(tspace);
        return true;
    }

    // DVB-T network.
    if (network_type == CLSID_DVBTNetworkProvider) {
        tuner_type = TT_DVB_T;
        ComPtr<::IDVBTLocator> loc(CLSID_DVBTLocator, ::IID_IDVBTLocator, report);
        if (!ResetDVBTLocator(loc.pointer(), report)) {
            return false;
        }
        ComPtr<::IDVBTuningSpace> tspace(CLSID_DVBTuningSpace, ::IID_IDVBTuningSpace, report);
        if (!ResetDVBTuningSpace(tspace.pointer(), L"TSDuck DVB-T Tuning Space", network_type, ::DVBSystemType::DVB_Terrestrial, loc.pointer(), report)) {
            return false;
        }
        tuning_space.assign(tspace);
        return true;
    }

    // DVB-S network.
    if (network_type == CLSID_DVBSNetworkProvider) {
        tuner_type = TT_DVB_S;
        ComPtr<::IDVBSLocator> loc(CLSID_DVBSLocator, ::IID_IDVBSLocator, report);
        if (!ResetDVBSLocator(loc.pointer(), report)) {
            return false;
        }
        ComPtr<::IDVBSTuningSpace> tspace(CLSID_DVBSTuningSpace, ::IID_IDVBSTuningSpace, report);
        if (!ResetDVBSTuningSpace(tspace.pointer(), L"TSDuck DVB-S Tuning Space", network_type, ::DVBSystemType::DVB_Satellite, loc.pointer(), report)) {
            return false;
        }
        tuning_space.assign(tspace);
        return true;
    }

    // ATSC terrestrial network.
    if (network_type == CLSID_ATSCNetworkProvider) {
        tuner_type = TT_ATSC;
        ComPtr<::IATSCLocator> loc(CLSID_ATSCLocator, ::IID_IATSCLocator, report);
        if (!ResetATSCLocator(loc.pointer(), report)) {
            return false;
        }
        ComPtr<::IATSCTuningSpace> tspace(CLSID_ATSCTuningSpace, ::IID_IATSCTuningSpace, report);
        if (!ResetATSCTuningSpace(tspace.pointer(), L"TSDuck ATSC Tuning Space", network_type, ::TunerInputType::TunerInputAntenna, loc.pointer(), report)) {
            return false;
        }

        tuning_space.assign(tspace);
        return true;
    }

    // ATSC cable network.
    if (network_type == DIGITAL_CABLE_NETWORK_TYPE) {
        tuner_type = TT_ATSC;
        ComPtr<::IDigitalCableLocator> loc(CLSID_DigitalCableLocator, ::IID_IDigitalCableLocator, report);
        if (!ResetATSCLocator2(loc.pointer(), report)) {
            return false;
        }
        ComPtr<::IDigitalCableTuningSpace> tspace(CLSID_DigitalCableTuningSpace, ::IID_IDigitalCableTuningSpace, report);
        if (!ResetDigitalCableTuningSpace(tspace.pointer(), L"TSDuck DigitalCable Tuning Space", network_type, ::TunerInputType::TunerInputCable, loc.pointer(), report)) {
            return false;
        }
        tuning_space.assign(tspace);
        return true;
    }

    // ISDB-S network.
    // There are two GUID with similar names but distinct values.
    // The differences are unknown, so treat them equally.
    if (network_type == ISDB_SATELLITE_TV_NETWORK_TYPE || network_type == ISDB_S_NETWORK_TYPE) {
        tuner_type = TT_ISDB_S;
        ComPtr<::IISDBSLocator> loc(CLSID_ISDBSLocator, ::IID_IISDBSLocator, report);
        if (!ResetDVBSLocator(loc.pointer(), report)) {
            return false;
        }
        // Found no ISDB-S tuning space, using DVB-S one instead.
        ComPtr<::IDVBSTuningSpace> tspace(CLSID_DVBSTuningSpace, ::IID_IDVBSTuningSpace, report);
        if (!ResetDVBSTuningSpace(tspace.pointer(), L"TSDuck ISDB-S Tuning Space", network_type, ::DVBSystemType::DVB_Satellite, loc.pointer(), report)) {
            return false;
        }
        tuning_space.assign(tspace);
        return true;
    }

    // ISDB-T network.
    if (network_type == ISDB_TERRESTRIAL_TV_NETWORK_TYPE) {
        tuner_type = TT_ISDB_T;
        // Found no ISDB-T locator, using DVB-T one instead.
        ComPtr<::IDVBTLocator> loc(CLSID_DVBTLocator, ::IID_IDVBTLocator, report);
        if (!ResetDVBTLocator(loc.pointer(), report)) {
            return false;
        }
        // Found no ISDB-T tuning space, using DVB-T one instead.
        ComPtr<::IDVBTuningSpace> tspace(CLSID_DVBTuningSpace, ::IID_IDVBTuningSpace, report);
        if (!ResetDVBTuningSpace(tspace.pointer(), L"TSDuck ISDB-T Tuning Space", network_type, ::DVBSystemType::DVB_Terrestrial, loc.pointer(), report)) {
            return false;
        }
        tuning_space.assign(tspace);
        return true;
    }

    // ISDB-C network.
    if (network_type == ISDB_CABLE_TV_NETWORK_TYPE) {
        tuner_type = TT_ISDB_C;
        // Found no ISDB-C locator, using DVB-C one instead.
        ComPtr<::IDVBCLocator> loc(CLSID_DVBCLocator, ::IID_IDVBCLocator, report);
        if (!ResetLocator(loc.pointer(), report)) {
            return false;
        }
        // Found no ISDB-C tuning space, using DVB-C one instead.
        ComPtr<::IDVBTuningSpace> tspace(CLSID_DVBTuningSpace, ::IID_IDVBTuningSpace, report);
        if (!ResetDVBTuningSpace(tspace.pointer(), L"TSDuck ISDB-C Tuning Space", network_type, ::DVBSystemType::DVB_Cable, loc.pointer(), report)) {
            return false;
        }
        tuning_space.assign(tspace);
        return true;
    }

    return false;
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

bool ts::CreateTuneRequest(DuckContext& duck, ComPtr<::ITuneRequest>& request, ::ITuningSpace* tuning_space, const ModulationArgs& params, Report& report)
{
    if (tuning_space == 0) {
        return false;
    }

    // Create a DirectShow tune request
    ComPtr<::ITuneRequest> tune_request;
    ::HRESULT hr = tuning_space->CreateTuneRequest(tune_request.creator());
    if (!ComSuccess(hr, u"cannot create DirectShow tune request", report)) {
        return false;
    }
    assert(!tune_request.isNull());

    // Report to use when errors shall be reported in debug mode only
    Report& debug_report(report.debug() ? report : NULLREP);

    // If this is a DVB tuning space, get DVB interface of tune request and set ids to wildcards.
    ComPtr<::IDVBTuneRequest> dvb_request;
    dvb_request.queryInterface(tune_request.pointer(), ::IID_IDVBTuneRequest, debug_report);
    if (!dvb_request.isNull() &&
        (!PUT(dvb_request, ONID, -1) ||
         !PUT(dvb_request, TSID, -1) ||
         !PUT(dvb_request, SID, -1)))
    {
        return false;
    }

    // If this is an ATSC tuning space, get ATSC interface of tune request and set channel and
    // minor channel to wildcards.
    ComPtr<::IATSCChannelTuneRequest> atsc_request;
    atsc_request.queryInterface(tune_request.pointer(), ::IID_IATSCChannelTuneRequest, debug_report);
    if (!atsc_request.isNull() &&
        (!PUT(atsc_request, Channel, -1) ||
         !PUT(atsc_request, MinorChannel, -1)))
    {
        return false;
    }

    // Create a locator (where to find the physical TS, ie. tuning params).
    ComPtr<::IDigitalLocator> locator;
    if (!CreateLocator(duck, locator, params, report)) {
        return false;
    }
    assert(!locator.isNull());

    // Set the locator in the tune request
    hr = tune_request->put_Locator(locator.pointer());
    if (!ComSuccess(hr, u"ITuneRequest::put_Locator", report)) {
        return false;
    }

    // Tune request fully built.
    request.assign(tune_request);
    return true;
}


//-----------------------------------------------------------------------------
// Create a Locator object for tuning parameters.
//-----------------------------------------------------------------------------

bool ts::CreateLocator(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params, Report& report)
{
    const DeliverySystem delsys = params.delivery_system.value(DS_UNDEFINED);
    const TunerType ttype = TunerTypeOf(delsys);

    // Create the locator depending on the type.
    switch (ttype) {
        case TT_DVB_S:
            return CreateLocatorDVBS(duck, locator, params, report);
        case TT_DVB_T:
            return CreateLocatorDVBT(duck, locator, params, report);
        case TT_DVB_C:
            return CreateLocatorDVBC(duck, locator, params, report);
        case TT_ATSC:
            return CreateLocatorATSC(duck, locator, params, report);
        case TT_ISDB_S:
        case TT_ISDB_T:
        case TT_ISDB_C:
        case TT_UNDEFINED:
        default:
            report.error(u"cannot convert %s parameters to DirectShow tuning parameters", {DeliverySystemEnum.name(delsys)});
            return false;
    }
}


//-----------------------------------------------------------------------------
// Create an IDigitalLocator object for DVB-T parameters
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::CreateLocatorDVBT(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params, Report& report)
{
    ComPtr<::IDVBTLocator> loc(CLSID_DVBTLocator, ::IID_IDVBTLocator, report);

    if (loc.isNull() ||
        !CheckModVar(params.inversion, u"spectral inversion", SpectralInversionEnum, report) ||
        !CheckModVar(params.bandwidth, u"bandwidth", BandWidthEnum, report) ||
        !CheckModVar(params.fec_hp, u"FEC", InnerFECEnum, report) ||
        !CheckModVar(params.fec_lp, u"FEC", InnerFECEnum, report) ||
        !CheckModVar(params.modulation, u"constellation", ModulationEnum, report) ||
        !CheckModVar(params.transmission_mode, u"transmission mode", TransmissionModeEnum, report) ||
        !CheckModVar(params.guard_interval, u"guard interval", GuardIntervalEnum, report) ||
        !CheckModVar(params.hierarchy, u"hierarchy", HierarchyEnum, report) ||
        !PUT(loc, CarrierFrequency, long(params.frequency.value() / 1000)) ||  // frequency in kHz
        !PUT(loc, Modulation, ::ModulationType(params.modulation.value())) ||
        !PUT(loc, Bandwidth, long(params.bandwidth.value())) ||
        !PUT(loc, Guard, ::GuardInterval(params.guard_interval.value())) ||
        !PUT(loc, LPInnerFEC, ::BDA_FEC_VITERBI) ||
        !PUT(loc, LPInnerFECRate, ::BinaryConvolutionCodeRate(params.fec_lp.value())) ||
        !PUT(loc, Mode, ::TransmissionMode(params.transmission_mode.value())) ||
        !PUT(loc, HAlpha, ::HierarchyAlpha(params.hierarchy.value())))
    {
        return false;
    }

    if (params.plp.set() && params.plp != PLP_DISABLE) {
        report.warning(u"DVT-T2 PLP selection disabled on Windows");
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

bool ts::CreateLocatorDVBC(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params, Report& report)
{
    ComPtr<::IDVBCLocator> loc(CLSID_DVBCLocator, ::IID_IDVBCLocator, report);

    if (loc.isNull() ||
        !CheckModVar(params.inversion, u"spectral inversion", SpectralInversionEnum, report) ||
        !CheckModVar(params.inner_fec, u"FEC", InnerFECEnum, report) ||
        !CheckModVar(params.modulation, u"modulation", ModulationEnum, report) ||
        !PUT(loc, CarrierFrequency, long(params.frequency.value() / 1000)) ||  // frequency in kHz
        !PUT(loc, Modulation, ::ModulationType(params.modulation.value())) ||
        !PUT(loc, InnerFEC, ::BDA_FEC_VITERBI) ||
        !PUT(loc, InnerFECRate, ::BinaryConvolutionCodeRate(params.inner_fec.value())) ||
        !PUT(loc, SymbolRate, long(params.symbol_rate.value())))
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

bool ts::CreateLocatorDVBS(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params, Report& report)
{
    // Specify DiSEqC satellite number.
    // Note however that most drivers ignore it...
    ::LNB_Source source = ::BDA_LNB_SOURCE_NOT_SET;
    switch (params.satellite_number.value(0)) {
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
        !CheckModVar(params.modulation, u"modulation", ModulationEnum, report) ||
        !CheckModVar(params.inner_fec, u"FEC", InnerFECEnum, report) ||
        !CheckModVar(params.polarity, u"polarity", PolarizationEnum, report) ||
        !PUT(loc, CarrierFrequency, long(params.frequency.value() / 1000)) ||  // frequency in kHz
        !PUT(loc, Modulation, ::ModulationType(params.modulation.value())) ||
        !PUT(loc, SignalPolarisation, ::Polarisation(params.polarity.value())) ||
        !PUT(loc, InnerFEC, ::BDA_FEC_VITERBI) ||
        !PUT(loc, InnerFECRate, ::BinaryConvolutionCodeRate(params.inner_fec.value())) ||
        !PUT(loc, SymbolRate, long(params.symbol_rate.value())) ||
        !PUT(loc, LocalSpectralInversionOverride, ::SpectralInversion(params.inversion.value())) ||
        !PUT(loc, LocalOscillatorOverrideLow, long(params.lnb.value().lowFrequency() / 1000)) ||    // frequency in kHz
        !PUT(loc, LocalOscillatorOverrideHigh, long(params.lnb.value().highFrequency() / 1000)) ||  // frequency in kHz
        !PUT(loc, LocalLNBSwitchOverride, long(params.lnb.value().switchFrequency() / 1000)) ||     // frequency in kHz
        !PUT(loc, DiseqLNBSource, source))
    {
        return false;
    }

    // DVB-S2 specific parameters
    if (params.delivery_system == DS_DVB_S2 &&
        (!CheckModVar(params.pilots, u"pilot", PilotEnum, report) ||
         !CheckModVar(params.roll_off, u"roll-off factor", RollOffEnum, report) ||
         !PUT(loc, SignalPilot, ::Pilot(params.pilots.value())) ||
         !PUT(loc, SignalRollOff, ::RollOff(params.roll_off.value()))))
    {
        return false;
    }

    locator.assign(loc); // loc and loc2 are two interfaces of the same object
    return true;
}


//-----------------------------------------------------------------------------
// Create an IDigitalLocator object for ATSC parameters
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::CreateLocatorATSC(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params, Report& report)
{
    ComPtr<::IATSCLocator> loc(CLSID_ATSCLocator, ::IID_IATSCLocator, report);

    // Get UHF and VHF band descriptions in the default region.
    const HFBand* uhf = duck.uhfBand();
    const HFBand* vhf = duck.vhfBand();

    // It seems that with DirectShow, the CarrierFrequency must be set to -1
    // for ATSC tuning to work and the physicalChannel used instead. This means
    // we need to take the frequency and map it to the corresponding HF channel
    // using the global HF band region.

    const uint64_t freq = params.frequency.value(0);
    long physical_channel = 0;
    if (uhf->inBand(freq)) {
        physical_channel = uhf->channelNumber(freq);
    }
    else if (vhf->inBand(freq)) {
        physical_channel = vhf->channelNumber(freq);
    }
    else {
        report.error(u"frequency %'d Hz is in neither the UHF nor VHF band", {freq});
        return false;
    }

    report.debug(u"mapped frequency %'d to physical channel %d", {freq, physical_channel});

    if (loc.isNull() ||
        !CheckModVar(params.inversion, u"spectral inversion", SpectralInversionEnum, report) ||
        !CheckModVar(params.modulation, u"modulation", ModulationEnum, report) ||
        !PUT(loc, CarrierFrequency, -1) ||
        !PUT(loc, InnerFEC, ::BDA_FEC_METHOD_NOT_SET) ||
        !PUT(loc, InnerFECRate, ::BDA_BCC_RATE_NOT_SET) ||
        !PUT(loc, OuterFEC, ::BDA_FEC_METHOD_NOT_SET) ||
        !PUT(loc, OuterFECRate, ::BDA_BCC_RATE_NOT_SET) ||
        !PUT(loc, Modulation, ::ModulationType(params.modulation.value())) ||
        !PUT(loc, SymbolRate, -1) ||
        !PUT(loc, PhysicalChannel, physical_channel) ||
        !PUT(loc, TSID, -1))
    {
        return false;
    }

    locator.assign(loc);
    return true;
}
