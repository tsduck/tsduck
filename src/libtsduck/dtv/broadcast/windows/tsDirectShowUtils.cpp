//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsDirectShowUtils.h"
#include "tsDuckContext.h"
#include "tsHFBand.h"
#include "tsMediaTypeUtils.h"
#include "tsWinUtils.h"
#include "tsMemory.h"
#include "tsNullReport.h"

// Put the value of a property (named "type") into a COM object.
// Report errors through a CucjContext variable named duck (must be accessible).
// Return true on success, false on error.
#define PUT(obj,type,value) ts::ComSuccess((obj)->put_##type(value), u"error setting " #type, duck.report())

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
    while (enum_monikers->Next(1, moniker.creator(), nullptr) == S_OK) {
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
        if (name != nullptr) {
            ::SysFreeString(name);
            name = nullptr;
        }
        return cname;
    }
}

ts::UString ts::GetTuningSpaceFriendlyName(::ITuningSpace* tspace, Report& report)
{
    if (tspace == nullptr) {
        return UString();
    }
    else {
        ::BSTR name = nullptr;
        return ToStringAndFree(tspace->get_FriendlyName(&name), name, u"ITuningSpace::get_FriendlyName", report);
    }
}

ts::UString ts::GetTuningSpaceUniqueName(::ITuningSpace* tspace, Report& report)
{
    if (tspace == nullptr) {
        return UString();
    }
    else {
        ::BSTR name = nullptr;
        return ToStringAndFree(tspace->get_UniqueName(&name), name, u"ITuningSpace::get_UniqueName", report);
    }
}

ts::UString ts::GetTuningSpaceClass(::ITuningSpace* tspace, Report& report)
{
    if (tspace == nullptr) {
        return UString();
    }
    else {
        ::BSTR name = nullptr;
        return ToStringAndFree(tspace->get_CLSID(&name), name, u"ITuningSpace::get_CLSID", report);
    }
}

ts::UString ts::GetTuningSpaceNetworkType(::ITuningSpace* tspace, Report& report)
{
    if (tspace == nullptr) {
        return UString();
    }

    // Get network type as a string.
    ::BSTR name = nullptr;
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

ts::UString ts::GetTuningSpaceDescription(::ITuningSpace* tspace, Report& report)
{
    if (tspace == nullptr) {
        return UString();
    }

    // Get tuning space names.
    const UString fname(GetTuningSpaceFriendlyName(tspace, report));
    const UString uname(GetTuningSpaceUniqueName(tspace, report));
    const UString ntype(GetTuningSpaceNetworkType(tspace, report));
    const UString clsid(GetTuningSpaceClass(tspace, report));
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
    if (!ntype.empty()) {
        if (!tname.empty()) {
            tname += u", network type: ";
        }
        tname += ntype;
    }
    if (!clsid.empty()) {
        if (!tname.empty()) {
            tname += u", class: ";
        }
        tname += u"\"" + clsid + u"\"";
    }

    // Check if this tuning space supports IDVBTuningSpace interface.
    ComPtr<::IDVBTuningSpace> dvb_tspace;
    dvb_tspace.queryInterface(tspace, ::IID_IDVBTuningSpace, NULLREP);
    if (!dvb_tspace.isNull()) {
        // This is a DVB tuning space. Get DVB system type.
        ::DVBSystemType sys_type = ::DVBSystemType::DVB_Cable;
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

bool ts::CreateTuneRequest(DuckContext& duck, ComPtr<::ITuneRequest>& request, ::ITuningSpace* tuning_space, const ModulationArgs& params)
{
    if (tuning_space == nullptr) {
        return false;
    }

    // Create a DirectShow tune request
    ComPtr<::ITuneRequest> tune_request;
    ::HRESULT hr = tuning_space->CreateTuneRequest(tune_request.creator());
    if (!ComSuccess(hr, u"cannot create DirectShow tune request", duck.report())) {
        return false;
    }
    assert(!tune_request.isNull());

    // Report to use when errors shall be reported in debug mode only
    Report& debug_report(duck.report().debug() ? duck.report() : NULLREP);

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
    if (!CreateLocator(duck, locator, params)) {
        return false;
    }
    assert(!locator.isNull());

    // Set the locator in the tune request
    hr = tune_request->put_Locator(locator.pointer());
    if (!ComSuccess(hr, u"ITuneRequest::put_Locator", duck.report())) {
        return false;
    }

    // Tune request fully built.
    request.assign(tune_request);
    return true;
}


//-----------------------------------------------------------------------------
// Create a Locator object for tuning parameters.
//-----------------------------------------------------------------------------

bool ts::CreateLocator(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params)
{
    const DeliverySystem delsys = params.delivery_system.value_or(DS_UNDEFINED);
    const TunerType ttype = TunerTypeOf(delsys);

    // Create the locator depending on the type.
    switch (ttype) {
        case TT_DVB_S:
            return CreateLocatorDVBS(duck, locator, params);
        case TT_DVB_T:
            return CreateLocatorDVBT(duck, locator, params);
        case TT_DVB_C:
            return CreateLocatorDVBC(duck, locator, params);
        case TT_ATSC:
            return CreateLocatorATSC(duck, locator, params);
        case TT_ISDB_S:
            return CreateLocatorISDBS(duck, locator, params);
        case TT_ISDB_T:
        case TT_ISDB_C:
        case TT_UNDEFINED:
        default:
            duck.report().error(u"cannot convert %s parameters to DirectShow tuning parameters", DeliverySystemEnum().name(delsys));
            return false;
    }
}


//-----------------------------------------------------------------------------
// Create an IDigitalLocator object for DVB-T parameters
//-----------------------------------------------------------------------------

bool ts::CreateLocatorDVBT(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params)
{
    ComPtr<::IDVBTLocator2> loc(CLSID_DVBTLocator2, ::IID_IDVBTLocator2, duck.report());

    if (loc.isNull() ||
        !CheckModVar(params.inversion, u"spectral inversion", SpectralInversionEnum(), duck.report()) ||
        !CheckModVar(params.fec_hp, u"FEC", InnerFECEnum(), duck.report()) ||
        !CheckModVar(params.fec_lp, u"FEC", InnerFECEnum(), duck.report()) ||
        !CheckModVar(params.modulation, u"constellation", ModulationEnum(), duck.report()) ||
        !CheckModVar(params.transmission_mode, u"transmission mode", TransmissionModeEnum(), duck.report()) ||
        !CheckModVar(params.guard_interval, u"guard interval", GuardIntervalEnum(), duck.report()) ||
        !CheckModVar(params.hierarchy, u"hierarchy", HierarchyEnum(), duck.report()) ||
        !PUT(loc, CarrierFrequency, long(params.frequency.value() / 1000)) ||  // frequency in kHz
        !PUT(loc, Modulation, ::ModulationType(params.modulation.value())) ||
        !PUT(loc, Bandwidth, long(params.bandwidth.value() / 1000000)) || // bandwidth in MHz
        !PUT(loc, Guard, ::GuardInterval(params.guard_interval.value())) ||
        !PUT(loc, LPInnerFEC, ::BDA_FEC_VITERBI) ||
        !PUT(loc, LPInnerFECRate, ::BinaryConvolutionCodeRate(params.fec_lp.value())) ||
        !PUT(loc, Mode, ::TransmissionMode(params.transmission_mode.value())) ||
        !PUT(loc, HAlpha, ::HierarchyAlpha(params.hierarchy.value())) ||
        (params.plp.has_value() && params.plp != PLP_DISABLE && !PUT(loc, PhysicalLayerPipeId, long(params.plp.value()))))
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
//-----------------------------------------------------------------------------

bool ts::CreateLocatorDVBC(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params)
{
    ComPtr<::IDVBCLocator> loc(CLSID_DVBCLocator, ::IID_IDVBCLocator, duck.report());

    if (loc.isNull() ||
        !CheckModVar(params.inversion, u"spectral inversion", SpectralInversionEnum(), duck.report()) ||
        !CheckModVar(params.inner_fec, u"FEC", InnerFECEnum(), duck.report()) ||
        !CheckModVar(params.modulation, u"modulation", ModulationEnum(), duck.report()) ||
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
//-----------------------------------------------------------------------------

bool ts::CreateLocatorDVBS(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params)
{
    // Specify DiSEqC satellite number.
    // Note however that most drivers ignore it...
    ::LNB_Source source = ::BDA_LNB_SOURCE_NOT_SET;
    switch (params.satellite_number.value_or(0)) {
        case 0:  source = ::BDA_LNB_SOURCE_A; break;
        case 1:  source = ::BDA_LNB_SOURCE_B; break;
        case 2:  source = ::BDA_LNB_SOURCE_C; break;
        case 3:  source = ::BDA_LNB_SOURCE_D; break;
        default: source = ::BDA_LNB_SOURCE_NOT_DEFINED; break;
    }

    //
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

    ComPtr<::IDVBSLocator2> loc(CLSID_DVBSLocator, ::IID_IDVBSLocator2, duck.report());

    //
    // Microsoft oddity, part 3...
    //
    // The DirectShow classes have not evolve and are still stuck with the legacy
    // model of low/high/switch frequencies. We try to emulate this with new LNB's.

    uint64_t low_freq = params.lnb.value().legacyLowOscillatorFrequency();
    uint64_t high_freq = params.lnb.value().legacyHighOscillatorFrequency();
    uint64_t switch_freq = params.lnb.value().legacySwitchFrequency();

    if (low_freq == 0) {
        // Cannot even find a low oscillator frequency. Get the local oscillator
        // frequency for this particular tune and pretend it is the low oscillator.
        LNB::Transposition tr;
        if (params.lnb.value().transpose(tr, params.frequency.value(), params.polarity.value(), NULLREP)) {
            low_freq = tr.oscillator_frequency;
        }
    }

    if (loc.isNull() ||
        !CheckModVar(params.modulation, u"modulation", ModulationEnum(), duck.report()) ||
        !CheckModVar(params.inner_fec, u"FEC", InnerFECEnum(), duck.report()) ||
        !CheckModVar(params.polarity, u"polarity", PolarizationEnum(), duck.report()) ||
        !PUT(loc, CarrierFrequency, long(params.frequency.value() / 1000)) ||  // frequency in kHz
        !PUT(loc, Modulation, ::ModulationType(params.modulation.value())) ||
        !PUT(loc, SignalPolarisation, ::Polarisation(params.polarity.value())) ||
        !PUT(loc, InnerFEC, ::BDA_FEC_VITERBI) ||
        !PUT(loc, InnerFECRate, ::BinaryConvolutionCodeRate(params.inner_fec.value())) ||
        !PUT(loc, SymbolRate, long(params.symbol_rate.value())) ||
        !PUT(loc, LocalSpectralInversionOverride, ::SpectralInversion(params.inversion.value())) ||
        !PUT(loc, LocalOscillatorOverrideLow, low_freq == 0 ? -1 : long(low_freq / 1000)) ||     // frequency in kHz, -1 means not set
        !PUT(loc, LocalOscillatorOverrideHigh, high_freq == 0 ? -1 : long(high_freq / 1000)) ||  // frequency in kHz, -1 means not set
        !PUT(loc, LocalLNBSwitchOverride, switch_freq == 0 ? -1 : long(switch_freq / 1000)) ||   // frequency in kHz, -1 means not set
        !PUT(loc, DiseqLNBSource, source))
    {
        return false;
    }

    // DVB-S2 specific parameters
    if (params.delivery_system == DS_DVB_S2 &&
        (!CheckModVar(params.pilots, u"pilot", PilotEnum(), duck.report()) ||
         !CheckModVar(params.roll_off, u"roll-off factor", RollOffEnum(), duck.report()) ||
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
//-----------------------------------------------------------------------------

bool ts::CreateLocatorATSC(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params)
{
    ComPtr<::IATSCLocator> loc(CLSID_ATSCLocator, ::IID_IATSCLocator, duck.report());

    // Get UHF and VHF band descriptions in the default region.
    const HFBand* uhf = duck.uhfBand();
    const HFBand* vhf = duck.vhfBand();

    // It seems that with DirectShow, the CarrierFrequency must be set to -1
    // for ATSC tuning to work and the physicalChannel used instead. This means
    // we need to take the frequency and map it to the corresponding HF channel
    // using the global HF band region.

    const uint64_t freq = params.frequency.value_or(0);
    long physical_channel = 0;
    if (uhf->inBand(freq)) {
        physical_channel = uhf->channelNumber(freq);
    }
    else if (vhf->inBand(freq)) {
        physical_channel = vhf->channelNumber(freq);
    }
    else {
        duck.report().error(u"frequency %'d Hz is in neither the UHF nor VHF band", freq);
        return false;
    }

    duck.report().debug(u"mapped frequency %'d to physical channel %d", freq, physical_channel);

    if (loc.isNull() ||
        !CheckModVar(params.inversion, u"spectral inversion", SpectralInversionEnum(), duck.report()) ||
        !CheckModVar(params.modulation, u"modulation", ModulationEnum(), duck.report()) ||
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


//-----------------------------------------------------------------------------
// Create an IDigitalLocator object for ISDB-S parameters
//-----------------------------------------------------------------------------

bool ts::CreateLocatorISDBS(DuckContext& duck, ComPtr<::IDigitalLocator>& locator, const ModulationArgs& params)
{
    ComPtr<::IISDBSLocator> loc(CLSID_ISDBSLocator, ::IID_IISDBSLocator, duck.report());

    if (loc.isNull() ||
        !CheckModVar(params.inner_fec, u"FEC", InnerFECEnum(), duck.report()) ||
        !CheckModVar(params.polarity, u"polarity", PolarizationEnum(), duck.report()) ||
        !PUT(loc, CarrierFrequency, long(params.frequency.value() / 1000)) ||  // frequency in kHz
        !PUT(loc, SignalPolarisation, ::Polarisation(params.polarity.value())) ||
        !PUT(loc, InnerFEC, ::BDA_FEC_VITERBI) ||
        !PUT(loc, InnerFECRate, ::BinaryConvolutionCodeRate(params.inner_fec.value())) ||
        !PUT(loc, SymbolRate, long(params.symbol_rate.value())))
    {
        return false;
    }

    // Pending questions:
    // - No way to set params.inversion in IISDBSLocator

    locator.assign(loc);
    return true;
}
