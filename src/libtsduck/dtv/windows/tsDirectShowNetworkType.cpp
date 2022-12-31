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

#include "tsDirectShowNetworkType.h"
#include "tsDirectShowUtils.h"
#include "tsNullReport.h"
#include "tsLNB.h"

// Put the value of a property (named "type") into a COM object.
// Report errors through a variable named report (must be accessible).
// Return true on success, false on error.
#define PUT(obj,type,value) (ts::ComSuccess((obj)->put_##type(value), u"error setting " #type, report))


//-----------------------------------------------------------------------------
// Constructors and destructors.
//-----------------------------------------------------------------------------

ts::DirectShowNetworkType::DirectShowNetworkType() :
    _network_type(GUID_NULL),
    _network_type_name(),
    _system_type(::DVBSystemType::DVB_Cable),         // no "null" value
    _input_type(::TunerInputType::TunerInputAntenna), // no "null" value
    _tuner_type(TunerType::TT_UNDEFINED),
    _delivery_systems(),
    _tuning_space(),
    _tuning_space_name()
{
}

ts::DirectShowNetworkType::~DirectShowNetworkType()
{
    // Enforce cleanup in the right order
    clear();
}

void ts::DirectShowNetworkType::clear()
{
    _network_type = GUID_NULL;
    _network_type_name.clear();
    _system_type = ::DVBSystemType::DVB_Cable;          // no "null" value
    _input_type = ::TunerInputType::TunerInputAntenna;  // no "null" value
    _tuner_type = TunerType::TT_UNDEFINED;
    _delivery_systems.clear();
    _tuning_space.release();
    _tuning_space_name.clear();
}


//-----------------------------------------------------------------------------
// Initialize this object from a network type.
//-----------------------------------------------------------------------------

bool ts::DirectShowNetworkType::initialize(const::GUID& network_type, Report& report)
{
    // Reinitialize object state.
    clear();
    _network_type = network_type;
    _network_type_name = NameGUID(_network_type);

    // Try all known network types, one by one.

    // DVB-T network.
    if (_network_type == CLSID_DVBTNetworkProvider) {

        // No way to check if DS_DVB_T2 is supported, assume it.
        _system_type = ::DVBSystemType::DVB_Terrestrial;
        _input_type = ::TunerInputType::TunerInputAntenna;  // unused here
        _tuner_type = TT_DVB_T;
        _delivery_systems.insert(DS_DVB_T);
        _delivery_systems.insert(DS_DVB_T2);

        ComPtr<::IDVBTLocator2> loc(CLSID_DVBTLocator2, ::IID_IDVBTLocator2, report);
        if (!initDVBTLocator2(loc.pointer(), report)) {
            return false;
        }

        ComPtr<::IDVBTuningSpace2> tspace(CLSID_DVBTuningSpace, ::IID_IDVBTuningSpace2, report);
        if (!initDVBTuningSpace2(tspace.pointer(), L"TSDuck DVB-T Tuning Space", loc.pointer(), report)) {
            return false;
        }

        _tuning_space.assign(tspace);
        return true;
    }

    // DVB-S network.
    if (_network_type == CLSID_DVBSNetworkProvider) {

        // No way to check if DS_DVB_S2 is supported, assume it.
        _system_type = ::DVBSystemType::DVB_Satellite;
        _input_type = ::TunerInputType::TunerInputAntenna;  // unused here
        _tuner_type = TT_DVB_S;
        _delivery_systems.insert(DS_DVB_S);
        _delivery_systems.insert(DS_DVB_S2);

        ComPtr<::IDVBSLocator2> loc(CLSID_DVBSLocator, ::IID_IDVBSLocator2, report);
        if (!initDVBSLocator2(loc.pointer(), report)) {
            return false;
        }

        ComPtr<::IDVBSTuningSpace> tspace(CLSID_DVBSTuningSpace, ::IID_IDVBSTuningSpace, report);
        if (!initDVBSTuningSpace(tspace.pointer(), L"TSDuck DVB-S Tuning Space", loc.pointer(), report)) {
            return false;
        }

        _tuning_space.assign(tspace);
        return true;
    }

    // DVB-C network.
    if (_network_type == CLSID_DVBCNetworkProvider) {

        // No way to check which annex is supported. Skip annex B (too special).
        _system_type = ::DVBSystemType::DVB_Cable;
        _input_type = ::TunerInputType::TunerInputCable;  // unused here
        _tuner_type = TT_DVB_C;
        _delivery_systems.insert(DS_DVB_C_ANNEX_A);
        _delivery_systems.insert(DS_DVB_C_ANNEX_C);

        ComPtr<::IDVBCLocator> loc(CLSID_DVBCLocator, ::IID_IDVBCLocator, report);
        if (!initDVBCLocator(loc.pointer(), report)) {
            return false;
        }

        ComPtr<::IDVBTuningSpace2> tspace(CLSID_DVBTuningSpace, ::IID_IDVBTuningSpace2, report);
        if (!initDVBTuningSpace2(tspace.pointer(), L"TSDuck DVB-C Tuning Space", loc.pointer(), report)) {
            return false;
        }

        _tuning_space.assign(tspace);
        return true;
    }

    // ATSC terrestrial network.
    if (_network_type == CLSID_ATSCNetworkProvider) {

        _system_type = ::DVBSystemType::DVB_Terrestrial; // not really DVB
        _input_type = ::TunerInputType::TunerInputAntenna;
        _tuner_type = TT_ATSC;
        _delivery_systems.insert(DS_ATSC);

        ComPtr<::IATSCLocator2> loc(CLSID_ATSCLocator, ::IID_IATSCLocator2, report);
        if (!initATSCLocator2(loc.pointer(), report)) {
            return false;
        }

        ComPtr<::IATSCTuningSpace> tspace(CLSID_ATSCTuningSpace, ::IID_IATSCTuningSpace, report);
        if (!initATSCTuningSpace(tspace.pointer(), L"TSDuck ATSC Tuning Space", loc.pointer(), report)) {
            return false;
        }

        _tuning_space.assign(tspace);
        return true;
    }

    // ISDB-S network.
    // There are two GUID with similar names but distinct values.
    // The differences are unknown, so treat them equally.
    if (_network_type == ISDB_SATELLITE_TV_NETWORK_TYPE || _network_type == ISDB_S_NETWORK_TYPE) {

        _system_type = ::DVBSystemType::ISDB_Satellite;
        _input_type = ::TunerInputType::TunerInputAntenna;  // unused here
        _tuner_type = TT_ISDB_S;
        _delivery_systems.insert(DS_ISDB_S);

        ComPtr<::IISDBSLocator> loc(CLSID_ISDBSLocator, ::IID_IISDBSLocator, report);
        if (!initISDBSLocator(loc.pointer(), report)) {
            return false;
        }

        // Found no ISDB-S tuning space, using DVB-S one instead.
        ComPtr<::IDVBSTuningSpace> tspace(CLSID_DVBSTuningSpace, ::IID_IDVBSTuningSpace, report);
        if (!initDVBSTuningSpace(tspace.pointer(), L"TSDuck ISDB-S Tuning Space", loc.pointer(), report)) {
            return false;
        }

        _tuning_space.assign(tspace);
        return true;
    }

    // ISDB-T network.
    if (_network_type == ISDB_TERRESTRIAL_TV_NETWORK_TYPE) {

        _system_type = ::DVBSystemType::ISDB_Terrestrial;
        _input_type = ::TunerInputType::TunerInputAntenna;  // unused here
        _tuner_type = TT_ISDB_T;
        _delivery_systems.insert(DS_ISDB_T);

        // Found no ISDB-T locator, using DVB-T one instead.
        ComPtr<::IDVBTLocator> loc(CLSID_DVBTLocator, ::IID_IDVBTLocator, report);
        if (!initDVBTLocator(loc.pointer(), report)) {
            return false;
        }

        // Found no ISDB-T tuning space, using DVB-T one instead.
        ComPtr<::IDVBTuningSpace> tspace(CLSID_DVBTuningSpace, ::IID_IDVBTuningSpace, report);
        if (!initDVBTuningSpace(tspace.pointer(), L"TSDuck ISDB-T Tuning Space", loc.pointer(), report)) {
            return false;
        }

        _tuning_space.assign(tspace);
        return true;
    }

    // ISDB-C network.
    if (_network_type == ISDB_CABLE_TV_NETWORK_TYPE) {

        _system_type = ::DVBSystemType::DVB_Cable; // not really DVB
        _input_type = ::TunerInputType::TunerInputCable;  // unused here
        _tuner_type = TT_ISDB_C;
        _delivery_systems.insert(DS_ISDB_C);

        // Found no ISDB-C locator, using DVB-C one instead.
        ComPtr<::IDVBCLocator> loc(CLSID_DVBCLocator, ::IID_IDVBCLocator, report);
        if (!initDVBCLocator(loc.pointer(), report)) {
            return false;
        }

        // Found no ISDB-C tuning space, using DVB-C one instead.
        ComPtr<::IDVBTuningSpace> tspace(CLSID_DVBTuningSpace, ::IID_IDVBTuningSpace, report);
        if (!initDVBTuningSpace(tspace.pointer(), L"TSDuck ISDB-C Tuning Space", loc.pointer(), report)) {
            return false;
        }

        _tuning_space.assign(tspace);
        return true;
    }

    return false;
}


//-----------------------------------------------------------------------------
// Initialize the content of a TuningSpace object.
//-----------------------------------------------------------------------------

bool ts::DirectShowNetworkType::initDefaultLocator(::ITuningSpace* tspace, ::ILocator* dlocator, Report& report)
{
    return dlocator == nullptr || PUT(tspace, DefaultLocator, dlocator);
}

bool ts::DirectShowNetworkType::initTuningSpace(::ITuningSpace* tspace, const ::WCHAR* name, ::ILocator* dlocator, Report& report)
{
    // Keep the tuning space name.
    _tuning_space_name = UString(name);
    report.debug(u"initializing tuning space \"%s\"", {_tuning_space_name});

    // Setting the unique and friendly name is not critical.
    // Ignore error (display them in debug mode only).
    if (tspace != nullptr) {
        if (report.debug()) {
            ComSuccess(tspace->put_UniqueName(const_cast<::WCHAR*>(name)), u"error setting UniqueName on " + _tuning_space_name, report);
            ComSuccess(tspace->put_FriendlyName(const_cast<::WCHAR*>(name)), u"error setting FriendlyName on " + _tuning_space_name, report);
        }
        else {
            tspace->put_UniqueName(const_cast<::WCHAR*>(name));
            tspace->put_FriendlyName(const_cast<::WCHAR*>(name));
        }
    }

    return tspace != nullptr &&
        PUT(tspace, _NetworkType, _network_type) &&
        initDefaultLocator(tspace, dlocator, report);
}

bool ts::DirectShowNetworkType::initDVBTuningSpace(::IDVBTuningSpace* tspace, const ::WCHAR* name, ::ILocator* dlocator, Report& report)
{
    return initTuningSpace(tspace, name, nullptr, report) &&
        PUT(tspace, SystemType, _system_type) &&
        initDefaultLocator(tspace, dlocator, report);
}

bool ts::DirectShowNetworkType::initDVBTuningSpace2(::IDVBTuningSpace2* tspace, const ::WCHAR* name, ::ILocator* dlocator, Report& report)
{
    return initDVBTuningSpace(tspace, name, nullptr, report) &&
        PUT(tspace, NetworkID, -1) &&   // -1 = "not set"
        initDefaultLocator(tspace, dlocator, report);
}

bool ts::DirectShowNetworkType::initDVBSTuningSpace(::IDVBSTuningSpace* tspace, const ::WCHAR* name, ::ILocator* dlocator, Report& report)
{
    return initDVBTuningSpace2(tspace, name, nullptr, report) &&
        PUT(tspace, LNBSwitch, -1) &&   // -1 = "not set"
        PUT(tspace, LowOscillator, -1) &&   // -1 = "not set"
        PUT(tspace, HighOscillator, -1) &&   // -1 = "not set"
        PUT(tspace, SpectralInversion, ::BDA_SPECTRAL_INVERSION_NOT_SET) &&
        initDefaultLocator(tspace, dlocator, report);
}

bool ts::DirectShowNetworkType::initATSCTuningSpace(::IATSCTuningSpace* tspace, const ::WCHAR* name, ::ILocator* dlocator, Report& report)
{
    const bool terrestrial = _input_type == ::TunerInputType::TunerInputAntenna;
    return initTuningSpace(tspace, name, nullptr, report) &&
        PUT(tspace, InputType, _input_type) &&
        PUT(tspace, CountryCode, 0) &&
        PUT(tspace, MaxMinorChannel, 999) &&
        PUT(tspace, MaxPhysicalChannel, terrestrial ? 69 : 158) &&
        PUT(tspace, MaxChannel, terrestrial ? 99 : 9999) &&
        PUT(tspace, MinMinorChannel, -1) &&      // -1 = "not set"
        PUT(tspace, MinPhysicalChannel, -1) &&   // -1 = "not set"
        PUT(tspace, MinChannel, -1) &&           // -1 = "not set"
        initDefaultLocator(tspace, dlocator, report);
}

bool ts::DirectShowNetworkType::initDigitalCableTuningSpace(::IDigitalCableTuningSpace* tspace, const ::WCHAR* name, ::ILocator* dlocator, Report& report)
{
    return initATSCTuningSpace(tspace, name, nullptr, report) &&
        PUT(tspace, MaxMajorChannel, 99) &&
        PUT(tspace, MaxSourceID, 0x7FFFFFFF) &&
        PUT(tspace, MinMajorChannel, -1) &&    // -1 = "not set"
        PUT(tspace, MinSourceID, 0) &&
        initDefaultLocator(tspace, dlocator, report);
}


//-----------------------------------------------------------------------------
// Initialize the content of locator objects.
//-----------------------------------------------------------------------------

bool ts::DirectShowNetworkType::initDigitalLocator(::IDigitalLocator* loc, Report& report)
{
    return loc != nullptr &&
        PUT(loc, CarrierFrequency, -1) &&
        PUT(loc, Modulation, ::BDA_MOD_NOT_SET) &&
        PUT(loc, InnerFEC, ::BDA_FEC_METHOD_NOT_SET) &&
        PUT(loc, InnerFECRate, ::BDA_BCC_RATE_NOT_SET) &&
        PUT(loc, OuterFEC, ::BDA_FEC_METHOD_NOT_SET) &&
        PUT(loc, OuterFECRate, ::BDA_BCC_RATE_NOT_SET) &&
        PUT(loc, SymbolRate, -1);  // -1 = "not set"
}

bool ts::DirectShowNetworkType::initDVBCLocator(::IDVBCLocator* loc, Report& report)
{
    return initDigitalLocator(loc, report);
}

bool ts::DirectShowNetworkType::initDVBTLocator(::IDVBTLocator* loc, Report& report)
{
    return initDigitalLocator(loc, report) &&
        PUT(loc, Bandwidth, -1) &&
        PUT(loc, LPInnerFEC, ::BDA_FEC_METHOD_NOT_SET) &&
        PUT(loc, LPInnerFECRate, ::BDA_BCC_RATE_NOT_SET) &&
        PUT(loc, HAlpha, ::BDA_HALPHA_NOT_SET) &&
        PUT(loc, Guard, ::BDA_GUARD_NOT_SET) &&
        PUT(loc, Mode, ::BDA_XMIT_MODE_NOT_SET) &&
        PUT(loc, OtherFrequencyInUse, 0);
}

bool ts::DirectShowNetworkType::initDVBTLocator2(::IDVBTLocator2* loc, Report& report)
{
    return initDVBTLocator(loc, report) &&
        PUT(loc, PhysicalLayerPipeId, -1);  // -1 = "not set"
}

bool ts::DirectShowNetworkType::initDVBSLocator(::IDVBSLocator* loc, Report& report)
{
    return initDigitalLocator(loc, report) &&
        PUT(loc, SignalPolarisation, ::BDA_POLARISATION_NOT_SET) &&
        PUT(loc, WestPosition, 0) &&
        PUT(loc, OrbitalPosition, -1) &&  // -1 = "not set"
        PUT(loc, Azimuth, -1) &&          // -1 = "not set"
        PUT(loc, Elevation, -1);          // -1 = "not set"
}

bool ts::DirectShowNetworkType::initDVBSLocator2(::IDVBSLocator2* loc, Report& report)
{
    return initDVBSLocator(loc, report) &&
        PUT(loc, DiseqLNBSource, ::BDA_LNB_SOURCE_NOT_SET) &&
        PUT(loc, LocalLNBSwitchOverride, -1) &&   // -1 = "not set"
        PUT(loc, LocalOscillatorOverrideLow, -1) &&   // -1 = "not set"
        PUT(loc, LocalOscillatorOverrideHigh, -1) &&   // -1 = "not set"
        PUT(loc, LocalSpectralInversionOverride, ::BDA_SPECTRAL_INVERSION_NOT_SET) &&
        PUT(loc, SignalRollOff, ::BDA_ROLL_OFF_NOT_SET) &&
        PUT(loc, SignalPilot, ::BDA_PILOT_NOT_SET);
}

bool ts::DirectShowNetworkType::initISDBSLocator(::IISDBSLocator* loc, Report& report)
{
    return initDVBSLocator(loc, report);
}

bool ts::DirectShowNetworkType::initATSCLocator(::IATSCLocator* loc, Report& report)
{
    return initDigitalLocator(loc, report) &&
        PUT(loc, PhysicalChannel, -1) &&  // -1 = "not set"
        PUT(loc, TSID, -1);               // -1 = "not set"
}

bool ts::DirectShowNetworkType::initATSCLocator2(::IATSCLocator2* loc, Report& report)
{
    return initATSCLocator(loc, report) &&
        PUT(loc, ProgramNumber, -1); // -1 = "not set"
}

bool ts::DirectShowNetworkType::initDigitalCableLocator(::IDigitalCableLocator* loc, Report& report)
{
    return initATSCLocator2(loc, report);
}
