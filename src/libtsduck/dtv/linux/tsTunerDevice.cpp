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
//
// Linux implementation of the ts::TunerDevice class.
//
//-----------------------------------------------------------------------------

#include "tsTunerDevice.h"
#include "tsDuckContext.h"
#include "tsHFBand.h"
#include "tsTime.h"
#include "tsSysUtils.h"
#include "tsFileUtils.h"
#include "tsSignalAllocator.h"
#include "tsTS.h"
#include "tsMemory.h"
#include "tsTunerDeviceInfo.h"

// We used to report "bit error rate", "signal/noise ratio", "signal strength",
// "uncorrected blocks". But the corresponding ioctl commands (FE_READ_BER, FE_READ_SNR,
// FE_READ_SIGNAL_STRENGTH, FE_READ_UNCORRECTED_BLOCKS) are marked as deprecated with
// DVB API v5 and most drivers now return error 524 (ENOTSUPP). So, we simply drop the
// feature. Also note that there are several forms of "unsupported" in errno and 524
// is usually not defined...
#if !defined(DVB_ENOTSUPP)
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(unused-macros)
    #define DVB_ENOTSUPP 524
    TS_POP_WARNING()
#endif

#define MAX_OVERFLOW  8   // Maximum consecutive overflow

#define FE_ZERO (::fe_status_t(0))


//-----------------------------------------------------------------------------
// Constructor and destructor.
//-----------------------------------------------------------------------------

ts::TunerDevice::TunerDevice(DuckContext& duck) :
    TunerBase(duck),
    _is_open(false),
    _info_only(false),
    _device_name(),
    _device_info(),
    _device_path(),
    _signal_timeout(DEFAULT_SIGNAL_TIMEOUT),
    _signal_timeout_silent(false),
    _receive_timeout(0),
    _delivery_systems(),
    _reading_dvr(false),
    _aborted(false),
    _frontend_name(),
    _demux_name(),
    _dvr_name(),
    _frontend_fd(-1),
    _demux_fd(-1),
    _dvr_fd(-1),
    _demux_bufsize(DEFAULT_DEMUX_BUFFER_SIZE),
    _fe_info(),
    _signal_poll(DEFAULT_SIGNAL_POLL),
    _rt_signal(-1),
    _rt_timer(nullptr),
    _rt_timer_valid(false)
{
}

ts::TunerDevice::~TunerDevice()
{
    // Cleanup receive timer resources
    setReceiveTimeout(0);
}


//-----------------------------------------------------------------------------
// Ioctl hell
//-----------------------------------------------------------------------------
//
// The documentation of the LinuxTV API is/was a joke, unprecise, confusing,
// etc. There is ambiguity about the following ioctl's:
//
//   FE_SET_TONE, FE_SET_VOLTAGE, FE_DISEQC_SEND_BURST.
//
// These ioctl's take an enum value as input. In the old V3 API, the parameter
// is passed by value. In the V5 documentation, it is passed by reference.
// Most sample programs (a bit old) use the "pass by value" method.
//
// V3 documentation: https://www.linuxtv.org/docs/dvbapi/dvbapi.html
//   int ioctl(int fd, int request = FE_SET_TONE, fe_sec_tone_mode_t tone);
//   int ioctl(int fd, int request = FE_SET_VOLTAGE, fe_sec_voltage_t voltage);
//   int ioctl(int fd, int request = FE_DISEQC_SEND_BURST, fe_sec_mini_cmd_t burst);
//
// V5 documentation: https://www.linuxtv.org/downloads/v4l-dvb-apis-new/uapi/dvb/frontend_fcalls.html
//   int ioctl(int fd, FE_SET_TONE, enum fe_sec_tone_mode *tone)
//   int ioctl(int fd, FE_SET_VOLTAGE, enum fe_sec_voltage *voltage)
//   int ioctl(int fd, FE_DISEQC_SEND_BURST, enum fe_sec_mini_cmd *tone)
//
// Interestingly, the following ioctl's which take an int as argument use the
// "pass by value" method in V5:
//
//   FE_ENABLE_HIGH_LNB_VOLTAGE, FE_SET_FRONTEND_TUNE_MODE
//
// To isolate that mess from the rest of the code, the define the following wrappers.
//

namespace {
    inline int ioctl_fe_set_tone(int fd, fe_sec_tone_mode_t tone)
    {
        return ::ioctl(fd, ts::ioctl_request_t(FE_SET_TONE), tone);
    }
    inline int ioctl_fe_set_voltage(int fd, fe_sec_voltage_t voltage)
    {
        return ::ioctl(fd, ts::ioctl_request_t(FE_SET_VOLTAGE), voltage);
    }
    inline int ioctl_fe_diseqc_send_burst(int fd, fe_sec_mini_cmd_t burst)
    {
        return ::ioctl(fd, ts::ioctl_request_t(FE_DISEQC_SEND_BURST), burst);
    }
}


//-----------------------------------------------------------------------------
// Linux implementation of services from ts::TunerBase.
// Get the list of all existing DVB tuners.
//-----------------------------------------------------------------------------

bool ts::TunerBase::GetAllTuners(DuckContext& duck, TunerPtrVector& tuners)
{
    // Reset returned vector
    tuners.clear();

    // Get list of all DVB adapters
    UStringVector names;

    // Flat naming scheme (old kernels < 2.4 and still found on Android).
    ExpandWildcardAndAppend(names, u"/dev/dvb*.frontend*");

    // Modern Linux DVB folder naming scheme.
    ExpandWildcardAndAppend(names, u"/dev/dvb/adapter*/frontend*");

    // Open all tuners
    tuners.reserve(names.size());
    bool ok = true;
    for (const auto& it : names) {

        UString tuner_name(it);
        tuner_name.substitute(u".frontend", u":");
        tuner_name.substitute(u"/frontend", u":");

        const size_t index = tuners.size();
        tuners.resize(index + 1);
        tuners[index] = new TunerDevice(duck);
        if (!tuners[index]->open(tuner_name, true)) {
            ok = false;
            tuners[index].clear();
            tuners.resize(index);
        }
    }

    return ok;
}


//-----------------------------------------------------------------------------
// Get/set basic parameters.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::isOpen() const
{
    return _is_open;
}

bool ts::TunerDevice::infoOnly() const
{
    return _info_only;
}

const ts::DeliverySystemSet& ts::TunerDevice::deliverySystems() const
{
    return _delivery_systems;
}

ts::UString ts::TunerDevice::deviceName() const
{
    return _device_name;
}

ts::UString ts::TunerDevice::deviceInfo() const
{
    return _device_info;
}

ts::UString ts::TunerDevice::devicePath() const
{
    return _device_path;
}

ts::MilliSecond ts::TunerDevice::receiveTimeout() const
{
    return _receive_timeout;
}

void ts::TunerDevice::setSignalTimeout(MilliSecond t)
{
    _signal_timeout = t;
}

void ts::TunerDevice::setSignalTimeoutSilent(bool silent)
{
    _signal_timeout_silent = silent;
}

void ts::TunerDevice::setSignalPoll(MilliSecond t)
{
    _signal_poll = t;
}

void ts::TunerDevice::setDemuxBufferSize(size_t s)
{
    _demux_bufsize = s;
}


//-----------------------------------------------------------------------------
// Open the tuner.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::open(const UString& device_name, bool info_only)
{
    if (_is_open) {
        _duck.report().error(u"tuner already open");
        return false;
    }

    _info_only = info_only;

    // Check if this system uses flat or directory DVB naming.
    const bool dvb_directory = IsDirectory(u"/dev/dvb");

    // Analyze device name: /dev/dvb/adapterA[:F[:M[:V]]]
    // Alternate old flat format: /dev/dvbA[:F[:M[:V]]]
    int adapter_nb = 0;
    int frontend_nb = 0;
    int demux_nb = 0;
    int dvr_nb = 0;
    UStringVector fields;
    if (device_name.empty()) {
        // Default tuner is first one
        fields.push_back(dvb_directory ? u"/dev/dvb/adapter0" : u"/dev/dvb0");
    }
    else if (!device_name.startWith(u"/dev/dvb")) {
        // If the name does not start with /dev/dvb, check if this is a known device full description.
        TunerPtrVector all_tuners;
        GetAllTuners(_duck, all_tuners);
        for (const auto& it : all_tuners) {
            if (device_name.similar(it->deviceInfo())) {
                fields.push_back(it->deviceName());
                break;
            }
        }
        if (fields.empty()) {
            _duck.report().error(u"unknown tuner \"%s\"", {device_name});
            return false;
        }
    }
    else {
        device_name.split(fields, u':', false);
    }
    const size_t fcount = fields.size();
    bool ok = fcount >= 1 && fcount <= 4 &&
        (fcount < 2 || fields[1].toInteger(frontend_nb)) &&
        (fcount < 3 || fields[2].toInteger(demux_nb)) &&
        (fcount < 4 || fields[3].toInteger(dvr_nb));
    if (!ok) {
        _duck.report().error(u"invalid DVB tuner name %s", {device_name});
        return false;
    }

    // The adapter number is the integer value at end of first field.
    const size_t n = fields[0].find_last_not_of(u"0123456789");
    if (n < fields[0].size()) {
        fields[0].substr(n + 1).toInteger(adapter_nb);
    }

    // If not specified, use frontend index for demux
    if (fcount < 3) {
        demux_nb = frontend_nb;
    }

    // If not specified, use frontend index for dvr
    if (fcount < 4) {
        dvr_nb = frontend_nb;
    }

    // Rebuild full TSDuck device name.
    _device_name = fields[0];
    if (dvr_nb != 0) {
        _device_name += UString::Format(u":%d:%d:%d", {frontend_nb, demux_nb, dvr_nb});
    }
    else if (demux_nb != 0) {
        _device_name += UString::Format(u":%d:%d", {frontend_nb, demux_nb});
    }
    else if (frontend_nb != 0) {
        _device_name += UString::Format(u":%d", {frontend_nb});
    }

    // Rebuild device names for frontend, demux and dvr.
    const UChar sep = dvb_directory ? u'/' : u'.';
    _frontend_name.format(u"%s%cfrontend%d", {fields[0], sep, frontend_nb});
    _demux_name.format(u"%s%cdemux%d", {fields[0], sep, demux_nb});
    _dvr_name.format(u"%s%cdvr%d", {fields[0], sep, dvr_nb});

    // Use the frontend device as "device path" for the tuner.
    _device_path = _frontend_name;

    // Open DVB adapter frontend. The frontend device is opened in non-blocking mode.
    // All configuration and setup operations are non-blocking anyway.
    // Reading events, however, is a blocking operation.
    if ((_frontend_fd = ::open(_frontend_name.toUTF8().c_str(), (info_only ? O_RDONLY : O_RDWR) | O_NONBLOCK)) < 0) {
        _duck.report().error(u"error opening %s: %s", {_frontend_name, SysErrorCodeMessage()});
        return false;
    }

    // Get characteristics of the frontend
    if (::ioctl(_frontend_fd, ioctl_request_t(FE_GET_INFO), &_fe_info) < 0) {
        _duck.report().error(u"error getting info on %s: %s", {_frontend_name, SysErrorCodeMessage()});
        return close() || false;
    }
    _fe_info.name[sizeof(_fe_info.name) - 1] = 0;
    _device_info = UString::FromUTF8(_fe_info.name);

    // Get tuner device information (if available).
    const TunerDeviceInfo devinfo(adapter_nb, frontend_nb, _duck.report());
    const UString devname(devinfo.fullName());
    if (!devname.empty()) {
        if (!_device_info.empty()) {
            _device_info.append(u", ");
        }
        _device_info.append(devname);
    }

    // Get the set of delivery systems for this frontend. Use DTV_ENUM_DELSYS to list all delivery systems.
    // If this failed, probably due to an obsolete driver, use the tuner type from FE_GET_INFO. This gives
    // only one tuner type but this is better than nothing.
    _delivery_systems.clear();
    DTVProperties props;
#if defined(DTV_ENUM_DELSYS)
    props.add(DTV_ENUM_DELSYS);
    if (::ioctl(_frontend_fd, ioctl_request_t(FE_GET_PROPERTY), props.getIoctlParam()) >= 0) {
        // DTV_ENUM_DELSYS succeeded, get all delivery systems.
        props.getValuesByCommand(_delivery_systems, DTV_ENUM_DELSYS);
    }
#else
    if (false) {
    }
#endif
    else {
        // DTV_ENUM_DELSYS failed, convert tuner type from FE_GET_INFO.
        const SysErrorCode err = LastSysErrorCode();
        const bool can2g = (_fe_info.caps & FE_CAN_2G_MODULATION) != 0;
        switch (_fe_info.type) {
            case FE_QPSK:
                _delivery_systems.insert(DS_DVB_S);
                if (can2g) {
                    _delivery_systems.insert(DS_DVB_S2);
                }
                break;
            case FE_QAM:
                _delivery_systems.insert(DS_DVB_C);
                if (can2g) {
                    _delivery_systems.insert(DS_DVB_C2);
                }
                break;
            case FE_OFDM:
                _delivery_systems.insert(DS_DVB_T);
                if (can2g) {
                    _delivery_systems.insert(DS_DVB_T2);
                }
                break;
            case FE_ATSC:
                _delivery_systems.insert(DS_ATSC);
                break;
            default:
                _duck.report().error(u"invalid tuner type %d for %s", {_fe_info.type, _frontend_name});
                close();
                return false;
        }
        _duck.report().verbose(u"error getting delivery systems of %s (%s), using %s", {_frontend_name, SysErrorCodeMessage(err), _delivery_systems.toString()});
    }

    // Open DVB adapter DVR (tap for TS packets) and adapter demux
    if (_info_only) {
        _dvr_fd = _demux_fd = -1;
    }
    else {
        if ((_dvr_fd = ::open(_dvr_name.toUTF8().c_str(), O_RDONLY)) < 0) {
            _duck.report().error(u"error opening %s: %s", {_dvr_name, SysErrorCodeMessage()});
            close();
            return false;
        }
        if ((_demux_fd = ::open(_demux_name.toUTF8().c_str(), O_RDWR)) < 0) {
            _duck.report().error(u"error opening %s: %s", {_demux_name, SysErrorCodeMessage()});
            close();
            return false;
        }
    }

    return _is_open = true;
}


//-----------------------------------------------------------------------------
// Hard close of the tuner, report can be null.
//-----------------------------------------------------------------------------

void ts::TunerDevice::hardClose(Report* report)
{
    // Stop the demux
    if (_demux_fd >= 0 && ::ioctl(_demux_fd, ioctl_request_t(DMX_STOP)) < 0 && report != nullptr) {
        report->error(u"error stopping demux on %s: %s", {_demux_name, SysErrorCodeMessage()});
    }

    // Close DVB adapter devices
    if (_dvr_fd >= 0) {
        ::close(_dvr_fd);
        _dvr_fd = -1;
    }
    if (_demux_fd >= 0) {
        ::close(_demux_fd);
        _demux_fd = -1;
    }
    if (_frontend_fd >= 0) {
        ::close(_frontend_fd);
        _frontend_fd = -1;
    }
}


//-----------------------------------------------------------------------------
// Close tuner.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::close(bool silent)
{
    // Close all file descriptors.
    hardClose(silent ? nullptr : &_duck.report());

    // Cleanup state.
    _is_open = false;
    _device_name.clear();
    _device_info.clear();
    _device_path.clear();
    _delivery_systems.clear();
    _reading_dvr = false;
    _aborted = false;
    _frontend_name.clear();
    _demux_name.clear();
    _dvr_name.clear();

    return true;
}


//-----------------------------------------------------------------------------
// Abort any pending or blocked reception.
//-----------------------------------------------------------------------------

void ts::TunerDevice::abort(bool silent)
{
    // Hord close of all file descriptors, hoping that pending I/O's will be canceled.
    // In the case of a current read operation on the dvr, it has been noticed that
    // closing the file descriptor make the read operation hang forever. We try to
    // mitigate this risk with a volatile boolean which is set around read() but
    // there is still a small risk of race condition (in which case we hang).
    _aborted = true;
    if (!_reading_dvr) {
        hardClose(silent ? nullptr : &_duck.report());
    }
}


//-----------------------------------------------------------------------------
// Get frontend status, encapsulate weird error management.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::getFrontendStatus(::fe_status_t& status)
{
    status = FE_ZERO;

    // Filter previous abort.
    if (_aborted) {
        return false;
    }

    errno = 0;
    const bool ok = ::ioctl(_frontend_fd, ioctl_request_t(FE_READ_STATUS), &status) == 0;
    const SysErrorCode err = LastSysErrorCode();
    if (ok || (!ok && err == EBUSY && status != FE_ZERO)) {
        return true;
    }
    else {
        _duck.report().error(u"error reading status on %s: %s", {_frontend_name, SysErrorCodeMessage(err)});
        return false;
    }
}


//-----------------------------------------------------------------------------
// Extract DTV_STAT_* properties and store it into a SignalState.
//-----------------------------------------------------------------------------

void ts::TunerDevice::GetStat(SignalState& state, Variable<SignalState::Value> SignalState::* field, const DTVProperties& props, uint32_t cmd)
{
    int64_t value = 0;
    ::fecap_scale_params scale = ::FE_SCALE_NOT_AVAILABLE;
    if (props.getStatByCommand(value, scale, cmd, 0)) {
        switch (scale) {
            case ::FE_SCALE_DECIBEL:
                state.*field = SignalState::Value(value, SignalState::Unit::MDB);
                break;
            case ::FE_SCALE_RELATIVE:
                state.setPercent(field, value, 0, 65535);
                break;
            case ::FE_SCALE_COUNTER:
                state.*field = SignalState::Value(value, SignalState::Unit::COUNTER);
                break;
            case ::FE_SCALE_NOT_AVAILABLE:
            default:
                (state.*field).clear();
                break;
        }
    }
    else {
        (state.*field).clear();
    }
}

void ts::TunerDevice::GetStatRatio(SignalState& state, Variable<SignalState::Value> SignalState::* field, const DTVProperties& props, uint32_t cmd1, uint32_t cmd2)
{
    int64_t value1 = 0;
    int64_t value2 = 0;
    ::fecap_scale_params scale2 = ::FE_SCALE_NOT_AVAILABLE;
    ::fecap_scale_params scale1 = ::FE_SCALE_NOT_AVAILABLE;
    if (props.getStatByCommand(value1, scale1, cmd1, 0) &&
        props.getStatByCommand(value2, scale2, cmd2, 0) &&
        scale1 == ::FE_SCALE_COUNTER &&
        scale2 == ::FE_SCALE_COUNTER &&
        value2 != 0)
    {
        // Store the ratio in percentage.
        state.setPercent(field, (100 * value1) / value2, 0, 100);
    }
    else {
        (state.*field).clear();
    }
}



//-----------------------------------------------------------------------------
// Get the state of the signal.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::getSignalState(SignalState& state)
{
    state.clear();

    if (!_is_open) {
        _duck.report().error(u"tuner not open");
        return false;
    }

    // Filter previous abort.
    if (_aborted) {
        return -1;
    }

    // Get signal lock.
    ::fe_status_t status = FE_ZERO;
    getFrontendStatus(status);
    state.signal_locked = (status & ::FE_HAS_LOCK) != 0;

#if defined(DTV_STAT_SIGNAL_STRENGTH)

    // Get the statistics from the DVB API, if supported.

    DTVProperties props;
    props.addStat(DTV_STAT_SIGNAL_STRENGTH);
    props.addStat(DTV_STAT_CNR);
    props.addStat(DTV_STAT_POST_ERROR_BIT_COUNT);
    props.addStat(DTV_STAT_POST_TOTAL_BIT_COUNT);
    props.addStat(DTV_STAT_ERROR_BLOCK_COUNT);
    props.addStat(DTV_STAT_TOTAL_BLOCK_COUNT);

    if (::ioctl(_frontend_fd, ioctl_request_t(FE_GET_PROPERTY), props.getIoctlParam()) < 0) {
        const SysErrorCode err = LastSysErrorCode();
        _duck.report().error(u"error getting tuner statistics: %s", {SysErrorCodeMessage(err)});
        return false;
    }

    props.reportStat(_duck.report(), Severity::Debug);
    GetStat(state, &SignalState::signal_strength, props, DTV_STAT_SIGNAL_STRENGTH);
    GetStat(state, &SignalState::signal_noise_ratio, props, DTV_STAT_CNR);
    GetStatRatio(state, &SignalState::bit_error_rate, props, DTV_STAT_POST_ERROR_BIT_COUNT, DTV_STAT_POST_TOTAL_BIT_COUNT);
    GetStatRatio(state, &SignalState::packet_error_rate, props, DTV_STAT_ERROR_BLOCK_COUNT, DTV_STAT_TOTAL_BLOCK_COUNT);

#else

    // Try to get the signal strength from the legacy API.
    uint16_t strength = 0;
    if (::ioctl(_frontend_fd, ioctl_request_t(FE_READ_SIGNAL_STRENGTH), &strength) < 0) {
        const SysErrorCode err = LastSysErrorCode();
        // Silently ignore deprecated feature, see comment at beginning of file.
        if (err != DVB_ENOTSUPP) {
            _duck.report().error(u"error reading signal strength on %s: %s", {_frontend_name, SysErrorCodeMessage(err)});
        }
        return -1;
    }

    // Assume that the returned value is a uniform range.
    // Strength is an uint16_t: 0x0000 = 0%, 0xFFFF = 100%
    state.setPercent(&SignalState::signal_strength, strength, 0, 0xFFFF);

#endif

    return true;
}


//-----------------------------------------------------------------------------
// Get current tuning parameters.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::getCurrentTuning(ModulationArgs& params, bool reset_unknown)
{
    // Filter previous abort.
    if (_aborted) {
        return false;
    }

    // Closed but not aborted deserves an error message.
    if (!_is_open) {
        _duck.report().error(u"tuner not open");
        return false;
    }

    // Get the current delivery system
    DTVProperties props;
    props.add(DTV_DELIVERY_SYSTEM);
    if (::ioctl(_frontend_fd, ioctl_request_t(FE_GET_PROPERTY), props.getIoctlParam()) < 0) {
        const SysErrorCode err = LastSysErrorCode();
        _duck.report().error(u"error getting current delivery system from tuner: %s", {SysErrorCodeMessage(err)});
        return false;
    }

    const DeliverySystem delsys = DeliverySystem(props.getByCommand(DTV_DELIVERY_SYSTEM));
    params.delivery_system = delsys;

    // Get specific tuning parameters
    switch (delsys) {
        case DS_DVB_S:
        case DS_DVB_S2:
        case DS_DVB_S_TURBO: {
            // Note: it is useless to get the frequency of a DVB-S tuner since it
            // returns the intermediate frequency and there is no unique satellite
            // frequency for a given intermediate frequency.
            if (reset_unknown) {
                params.frequency.clear();
                params.satellite_number.clear();
                params.lnb.clear();
                params.polarity.clear();
            }

            props.clear();
            props.add(DTV_INVERSION);
            props.add(DTV_SYMBOL_RATE);
            props.add(DTV_INNER_FEC);
            props.add(DTV_MODULATION);
            props.add(DTV_PILOT);
            props.add(DTV_ROLLOFF);
#if defined(DTV_STREAM_ID)
            props.add(DTV_STREAM_ID);
#endif

            if (::ioctl(_frontend_fd, ioctl_request_t(FE_GET_PROPERTY), props.getIoctlParam()) < 0) {
                const SysErrorCode err = LastSysErrorCode();
                _duck.report().error(u"error getting tuning parameters: %s", {SysErrorCodeMessage(err)});
                return false;
            }

            params.inversion = SpectralInversion(props.getByCommand(DTV_INVERSION));
            params.symbol_rate = props.getByCommand(DTV_SYMBOL_RATE);
            params.inner_fec = InnerFEC(props.getByCommand(DTV_INNER_FEC));
            params.modulation = Modulation(props.getByCommand(DTV_MODULATION));
            params.pilots = Pilot(props.getByCommand(DTV_PILOT));
            params.roll_off = RollOff(props.getByCommand(DTV_ROLLOFF));

            // With the Linux DVB API, all multistream selection info are passed in the "stream id".
#if defined(DTV_STREAM_ID)
            const uint32_t id = props.getByCommand(DTV_STREAM_ID);
#else
            const uint32_t id = PLP_DISABLE;
#endif
            params.isi = id & 0x000000FF;
            params.pls_code = (id >> 8) & 0x0003FFFF;
            params.pls_mode = PLSMode(id >> 26);
            break;
        }
        case DS_DVB_T:
        case DS_DVB_T2: {
            props.clear();
            props.add(DTV_FREQUENCY);
            props.add(DTV_INVERSION);
            props.add(DTV_BANDWIDTH_HZ);
            props.add(DTV_CODE_RATE_HP);
            props.add(DTV_CODE_RATE_LP);
            props.add(DTV_MODULATION);
            props.add(DTV_TRANSMISSION_MODE);
            props.add(DTV_GUARD_INTERVAL);
            props.add(DTV_HIERARCHY);
#if defined(DTV_STREAM_ID)
            props.add(DTV_STREAM_ID);
#endif

            if (::ioctl(_frontend_fd, ioctl_request_t(FE_GET_PROPERTY), props.getIoctlParam()) < 0) {
                const SysErrorCode err = LastSysErrorCode();
                _duck.report().error(u"error getting tuning parameters: %s", {SysErrorCodeMessage(err)});
                return false;
            }

            params.frequency = props.getByCommand(DTV_FREQUENCY);
            params.inversion = SpectralInversion(props.getByCommand(DTV_INVERSION));
            params.bandwidth = props.getByCommand(DTV_BANDWIDTH_HZ);
            params.fec_hp = InnerFEC(props.getByCommand(DTV_CODE_RATE_HP));
            params.fec_lp = InnerFEC(props.getByCommand(DTV_CODE_RATE_LP));
            params.modulation = Modulation(props.getByCommand(DTV_MODULATION));
            params.transmission_mode = TransmissionMode(props.getByCommand(DTV_TRANSMISSION_MODE));
            params.guard_interval = GuardInterval(props.getByCommand(DTV_GUARD_INTERVAL));
            params.hierarchy = Hierarchy(props.getByCommand(DTV_HIERARCHY));
#if defined(DTV_STREAM_ID)
            params.plp = props.getByCommand(DTV_STREAM_ID);
#else
            params.plp = PLP_DISABLE;
#endif
            break;
        }
        case DS_DVB_C_ANNEX_A:
        case DS_DVB_C_ANNEX_B:
        case DS_DVB_C_ANNEX_C:
        case DS_DVB_C2: {
            props.clear();
            props.add(DTV_FREQUENCY);
            props.add(DTV_INVERSION);
            props.add(DTV_SYMBOL_RATE);
            props.add(DTV_INNER_FEC);
            props.add(DTV_MODULATION);

            if (::ioctl(_frontend_fd, ioctl_request_t(FE_GET_PROPERTY), props.getIoctlParam()) < 0) {
                const SysErrorCode err = LastSysErrorCode();
                _duck.report().error(u"error getting tuning parameters: %s", {SysErrorCodeMessage(err)});
                return false;
            }

            params.frequency = props.getByCommand(DTV_FREQUENCY);
            params.inversion = SpectralInversion(props.getByCommand(DTV_INVERSION));
            params.symbol_rate = props.getByCommand(DTV_SYMBOL_RATE);
            params.inner_fec = InnerFEC(props.getByCommand(DTV_INNER_FEC));
            params.modulation = Modulation(props.getByCommand(DTV_MODULATION));
            break;
        }
        case DS_ATSC: {
            props.clear();
            props.add(DTV_FREQUENCY);
            props.add(DTV_INVERSION);
            props.add(DTV_MODULATION);

            if (::ioctl(_frontend_fd, ioctl_request_t(FE_GET_PROPERTY), props.getIoctlParam()) < 0) {
                const SysErrorCode err = LastSysErrorCode();
                _duck.report().error(u"error getting tuning parameters: %s", {SysErrorCodeMessage(err)});
                return false;
            }

            params.frequency = props.getByCommand(DTV_FREQUENCY);
            params.inversion = SpectralInversion(props.getByCommand(DTV_INVERSION));
            params.modulation = Modulation(props.getByCommand(DTV_MODULATION));
            break;
        }
        case DS_ISDB_S: {
            // Note: same remark about the frequency as DVB-S tuner.
            if (reset_unknown) {
                params.frequency.clear();
                params.satellite_number.clear();
                params.lnb.clear();
                params.polarity.clear();
            }

            props.clear();
            props.add(DTV_INVERSION);
            props.add(DTV_SYMBOL_RATE);
            props.add(DTV_INNER_FEC);
#if defined(DTV_STREAM_ID)
            props.add(DTV_STREAM_ID);
#endif
            if (::ioctl(_frontend_fd, ioctl_request_t(FE_GET_PROPERTY), props.getIoctlParam()) < 0) {
                const SysErrorCode err = LastSysErrorCode();
                _duck.report().error(u"error getting tuning parameters: %s", {SysErrorCodeMessage(err)});
                return false;
            }

            params.inversion = SpectralInversion(props.getByCommand(DTV_INVERSION));
            params.symbol_rate = props.getByCommand(DTV_SYMBOL_RATE);
            params.inner_fec = InnerFEC(props.getByCommand(DTV_INNER_FEC));
            params.stream_id.clear();
#if defined(DTV_STREAM_ID)
            const uint32_t val = props.getByCommand(DTV_STREAM_ID);
            if (val != DTVProperties::UNKNOWN) {
                // Warning: stream id may be incorrect when returned from the driver.
                // We should update it when possible with the actual transport stream id from the inner stream.
                params.stream_id = val;
            }
#endif
            break;
        }
        case DS_ISDB_T: {
            props.clear();
            props.add(DTV_FREQUENCY);
            props.add(DTV_INVERSION);
            props.add(DTV_BANDWIDTH_HZ);
            props.add(DTV_TRANSMISSION_MODE);
            props.add(DTV_GUARD_INTERVAL);
            props.add(DTV_ISDBT_SOUND_BROADCASTING);
            props.add(DTV_ISDBT_SB_SUBCHANNEL_ID);
            props.add(DTV_ISDBT_SB_SEGMENT_COUNT);
            props.add(DTV_ISDBT_SB_SEGMENT_IDX);
            props.add(DTV_ISDBT_LAYER_ENABLED);
            props.add(DTV_ISDBT_PARTIAL_RECEPTION);
            props.add(DTV_ISDBT_LAYERA_FEC);
            props.add(DTV_ISDBT_LAYERA_MODULATION);
            props.add(DTV_ISDBT_LAYERA_SEGMENT_COUNT);
            props.add(DTV_ISDBT_LAYERA_TIME_INTERLEAVING);
            props.add(DTV_ISDBT_LAYERB_FEC);
            props.add(DTV_ISDBT_LAYERB_MODULATION);
            props.add(DTV_ISDBT_LAYERB_SEGMENT_COUNT);
            props.add(DTV_ISDBT_LAYERB_TIME_INTERLEAVING);
            props.add(DTV_ISDBT_LAYERC_FEC);
            props.add(DTV_ISDBT_LAYERC_MODULATION);
            props.add(DTV_ISDBT_LAYERC_SEGMENT_COUNT);
            props.add(DTV_ISDBT_LAYERC_TIME_INTERLEAVING);

            if (::ioctl(_frontend_fd, ioctl_request_t(FE_GET_PROPERTY), props.getIoctlParam()) < 0) {
                const SysErrorCode err = LastSysErrorCode();
                _duck.report().error(u"error getting tuning parameters: %s", {SysErrorCodeMessage(err)});
                return false;
            }

            uint32_t val = 0;
            params.frequency = props.getByCommand(DTV_FREQUENCY);
            params.inversion = SpectralInversion(props.getByCommand(DTV_INVERSION));
            params.bandwidth = props.getByCommand(DTV_BANDWIDTH_HZ);
            params.transmission_mode = TransmissionMode(props.getByCommand(DTV_TRANSMISSION_MODE));
            params.guard_interval = GuardInterval(props.getByCommand(DTV_GUARD_INTERVAL));
            params.sound_broadcasting.clear();
            if ((val = props.getByCommand(DTV_ISDBT_SOUND_BROADCASTING)) != DTVProperties::UNKNOWN) {
                params.sound_broadcasting = val != 0;
            }
            params.sb_subchannel_id.clear();
            if ((val = props.getByCommand(DTV_ISDBT_SB_SUBCHANNEL_ID)) != DTVProperties::UNKNOWN) {
                params.sb_subchannel_id = int(val);
            }
            params.sb_segment_count.clear();
            if ((val = props.getByCommand(DTV_ISDBT_SB_SEGMENT_COUNT)) != DTVProperties::UNKNOWN) {
                params.sb_segment_count = int(val);
            }
            params.sb_segment_index.clear();
            if ((val = props.getByCommand(DTV_ISDBT_SB_SEGMENT_IDX)) != DTVProperties::UNKNOWN) {
                params.sb_segment_index = int(val);
            }
            params.isdbt_partial_reception.clear();
            if ((val = props.getByCommand(DTV_ISDBT_PARTIAL_RECEPTION)) != DTVProperties::UNKNOWN) {
                params.isdbt_partial_reception = val != 0;
            }
            params.isdbt_layers.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYER_ENABLED)) != DTVProperties::UNKNOWN) {
                params.isdbt_layers = UString();
                if ((val & 0x01) != 0) {
                    params.isdbt_layers.value().append(1, u'A');
                }
                if ((val & 0x02) != 0) {
                    params.isdbt_layers.value().append(1, u'B');
                }
                if ((val & 0x04) != 0) {
                    params.isdbt_layers.value().append(1, u'C');
                }
            }
            params.layer_a_fec.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYERA_FEC)) != DTVProperties::UNKNOWN) {
                params.layer_a_fec = InnerFEC(val);
            }
            params.layer_a_modulation.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYERA_MODULATION)) != DTVProperties::UNKNOWN) {
                params.layer_a_modulation = Modulation(val);
            }
            params.layer_a_segment_count.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYERA_SEGMENT_COUNT)) != DTVProperties::UNKNOWN && val <= uint32_t(ModulationArgs::MAX_ISDBT_SEGMENT_COUNT)) {
                params.layer_a_segment_count = int(val);
            }
            params.layer_a_time_interleaving.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYERA_TIME_INTERLEAVING)) != DTVProperties::UNKNOWN && ModulationArgs::IsValidISDBTTimeInterleaving(int(val))) {
                params.layer_a_time_interleaving = int(val);
            }
            params.layer_b_fec.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYERB_FEC)) != DTVProperties::UNKNOWN) {
                params.layer_b_fec = InnerFEC(val);
            }
            params.layer_b_modulation.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYERB_MODULATION)) != DTVProperties::UNKNOWN) {
                params.layer_b_modulation = Modulation(val);
            }
            params.layer_b_segment_count.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYERB_SEGMENT_COUNT)) != DTVProperties::UNKNOWN && val <= uint32_t(ModulationArgs::MAX_ISDBT_SEGMENT_COUNT)) {
                params.layer_b_segment_count = int(val);
            }
            params.layer_b_time_interleaving.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYERB_TIME_INTERLEAVING)) != DTVProperties::UNKNOWN && ModulationArgs::IsValidISDBTTimeInterleaving(int(val))) {
                params.layer_b_time_interleaving = int(val);
            }
            params.layer_c_fec.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYERC_FEC)) != DTVProperties::UNKNOWN) {
                params.layer_c_fec = InnerFEC(val);
            }
            params.layer_c_modulation.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYERC_MODULATION)) != DTVProperties::UNKNOWN) {
                params.layer_c_modulation = Modulation(val);
            }
            params.layer_c_segment_count.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYERC_SEGMENT_COUNT)) != DTVProperties::UNKNOWN && val <= uint32_t(ModulationArgs::MAX_ISDBT_SEGMENT_COUNT)) {
                params.layer_c_segment_count = int(val);
            }
            params.layer_c_time_interleaving.clear();
            if ((val = props.getByCommand(DTV_ISDBT_LAYERC_TIME_INTERLEAVING)) != DTVProperties::UNKNOWN && ModulationArgs::IsValidISDBTTimeInterleaving(int(val))) {
                params.layer_c_time_interleaving = int(val);
            }
            break;
        }
        case DS_ISDB_C:
        case DS_DVB_H:
        case DS_ATSC_MH:
        case DS_DTMB:
        case DS_CMMB:
        case DS_DAB:
        case DS_DSS:
        case DS_UNDEFINED:
        default:  {
            _duck.report().error(u"cannot get current tuning for delivery system %s", {DeliverySystemEnum.name(delsys)});
            return false;
        }
    }

    // Some drivers sometimes return weird values for spectral inversion.
    // Reset it in case of invalid value.
    if (params.inversion.set() && params.inversion.value() != SPINV_AUTO && params.inversion.value() != SPINV_ON && params.inversion.value() != SPINV_OFF) {
        params.inversion.clear();
    }

    return true;
}


//-----------------------------------------------------------------------------
// Discard all pending frontend events
//-----------------------------------------------------------------------------

void ts::TunerDevice::discardFrontendEvents()
{
    if (!_aborted) {
        ::dvb_frontend_event event;
        _duck.report().debug(u"starting discarding frontend events");
        while (::ioctl(_frontend_fd, ioctl_request_t(FE_GET_EVENT), &event) >= 0) {
            _duck.report().debug(u"one frontend event discarded");
        }
        _duck.report().debug(u"finished discarding frontend events");
    }
}


//-----------------------------------------------------------------------------
// Tune operation, return true on success, false on error
//-----------------------------------------------------------------------------

bool ts::TunerDevice::dtvTune(DTVProperties& props)
{
    // Filter previous abort.
    if (_aborted) {
        return false;
    }

    _duck.report().debug(u"tuning on %s", {_frontend_name});
    props.report(_duck.report(), Severity::Debug);
    if (::ioctl(_frontend_fd, ioctl_request_t(FE_SET_PROPERTY), props.getIoctlParam()) < 0) {
        const SysErrorCode err = LastSysErrorCode();
        _duck.report().error(u"tuning error on %s: %s", {_frontend_name, SysErrorCodeMessage(err)});
        return false;
    }
    return true;
}


//-----------------------------------------------------------------------------
// Clear tuner, return true on success, false on error
//-----------------------------------------------------------------------------

bool ts::TunerDevice::dtvClear()
{
    DTVProperties props;
    props.add(DTV_CLEAR);
    return dtvTune(props);
}


//-----------------------------------------------------------------------------
// Setup the dish for satellite tuners.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::dishControl(const ModulationArgs& params, const LNB::Transposition& trans)
{
    // Extracted from DVB/doc/HOWTO-use-the-frontend-api:
    //
    // Before you set the frontend parameters you have to setup DiSEqC switches
    // and the LNB. Modern LNB's switch their polarisation depending of the DC
    // component of their input (13V for vertical polarisation, 18V for
    // horizontal). When they see a 22kHz signal at their input they switch
    // into the high band and use a somewhat higher intermediate frequency
    // to downconvert the signal.
    //
    // When your satellite equipment contains a DiSEqC switch device to switch
    // between different satellites you have to send the according DiSEqC
    // commands, usually command 0x38. Take a look into the DiSEqC spec
    // available at http://www.eutelsat.org/ for the complete list of commands.
    //
    // The burst signal is used in old equipments and by cheap satellite A/B
    // switches.
    //
    // Voltage, burst and 22kHz tone have to be consistent to the values
    // encoded in the DiSEqC commands.

    // Setup structure for precise 15ms
    ::timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 15000000; // 15 ms

    // Stop 22 kHz continuous tone (was on if previously tuned on high band)
    if (ioctl_fe_set_tone(_frontend_fd, SEC_TONE_OFF) < 0) {
        _duck.report().error(u"DVB frontend FE_SET_TONE error: %s", {SysErrorCodeMessage()});
        return false;
    }

    // Setup polarisation voltage: 13V for vertical polarisation, 18V for horizontal
    if (ioctl_fe_set_voltage(_frontend_fd, params.polarity == POL_VERTICAL ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18) < 0) {
        _duck.report().error(u"DVB frontend FE_SET_VOLTAGE error: %s", {SysErrorCodeMessage()});
        return false;
    }

    // Wait at least 15ms.
    ::nanosleep(&delay, nullptr);

    // Send tone burst: A for satellite 0, B for satellite 1.
    // Notes:
    //   1) DiSEqC switches may address up to 4 dishes (satellite number 0 to 3)
    //      while non-DiSEqC switches can address only 2 (satellite number 0 to 1).
    //      This is why the DiSEqC command has space for 2 bits (4 states) while
    //      the "send tone burst" command is binary (A or B).
    //   2) The Linux DVB API is not specific about FE_DISEQC_SEND_BURST. Reading
    //      szap or szap-s2 source code, the code would be (satellite_number & 0x04) ? SEC_MINI_B : SEC_MINI_A.
    //      However, this does not seem logical. Secondly, a report from 2007 in linux-dvb
    //      mailing list suggests that the szap code should be (satellite_number & 0x01).
    //      In reply to this report, the answer was "thanks, committed" but it does
    //      not appear to be committed. Here, we use the "probably correct" code.
    if (ioctl_fe_diseqc_send_burst(_frontend_fd, params.satellite_number == 0 ? SEC_MINI_A : SEC_MINI_B) < 0) {
        _duck.report().error(u"DVB frontend FE_DISEQC_SEND_BURST error: %s", {SysErrorCodeMessage()});
        return false;
    }

    // Wait 15ms
    ::nanosleep(&delay, nullptr);

    // Send DiSEqC commands. See DiSEqC spec ...
    const bool high_band = trans.band_index > 0;
    ::dvb_diseqc_master_cmd cmd;
    cmd.msg_len = 4;    // Message size (meaningful bytes in msg)
    cmd.msg[0] = 0xE0;  // Command from master, no reply expected, first transmission
    cmd.msg[1] = 0x10;  // Any LNB or switcher (master to all)
    cmd.msg[2] = 0x38;  // Write to port group 0
    cmd.msg[3] = 0xF0 | // Clear all 4 flags first, then set according to next 4 bits
        ((uint8_t(params.satellite_number.value()) << 2) & 0x0F) |
        (params.polarity == POL_VERTICAL ? 0x00 : 0x02) |
        (high_band ? 0x01 : 0x00);
    cmd.msg[4] = 0x00;  // Unused
    cmd.msg[5] = 0x00;  // Unused

    if (::ioctl(_frontend_fd, ioctl_request_t(FE_DISEQC_SEND_MASTER_CMD), &cmd) < 0) {
        _duck.report().error(u"DVB frontend FE_DISEQC_SEND_MASTER_CMD error: %s", {SysErrorCodeMessage()});
        return false;
    }

    // Wait 15ms
    ::nanosleep(&delay, nullptr);

    // Start the 22kHz continuous tone when tuning to a transponder in the high band
    if (::ioctl_fe_set_tone(_frontend_fd, high_band ? SEC_TONE_ON : SEC_TONE_OFF) < 0) {
        _duck.report().error(u"DVB frontend FE_SET_TONE error: %s", {SysErrorCodeMessage()});
        return false;
    }
    return true;
}


//-----------------------------------------------------------------------------
// Tune to the specified parameters and start receiving.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::tune(ModulationArgs& params)
{
    // Filter previous abort.
    if (_aborted) {
        return false;
    }

    // Initial parameter checks.
    if (!checkTuneParameters(params)) {
        return false;
    }

    // Clear tuner state.
    discardFrontendEvents();
    if (!dtvClear()) {
        return false;
    }

    // For all tuners except satellite, the frequency is in Hz, on 32 bits.
    uint32_t freq = uint32_t(params.frequency.value());

    // In case of satellite delivery, we need to control the dish.
    if (IsSatelliteDelivery(params.delivery_system.value())) {
        if (!params.lnb.set()) {
            _duck.report().warning(u"no LNB set for satellite delivery %s", {DeliverySystemEnum.name(params.delivery_system.value())});
        }
        else {
            _duck.report().debug(u"using LNB %s", {params.lnb.value()});
            // Compute transposition information from the LNB.
            LNB::Transposition trans;
            if (!params.lnb.value().transpose(trans, params.frequency.value(), params.polarity.value(POL_NONE), _duck.report())) {
                return false;
            }
            // For satellite, Linux DVB API uses an intermediate frequency in kHz
            freq = uint32_t(trans.intermediate_frequency / 1000);
            // We need to control the dish only if this is not a "stacked" transposition.
            if (trans.stacked) {
                _duck.report().debug(u"LNB uses stacked transposition, no dish control required");
            }
            else {
                // Setup the dish (polarity, band).
                if (!dishControl(params, trans)) {
                    return false;
                }
                // Clear tuner state again.
                discardFrontendEvents();
            }
        }
    }

    // The bandwidth, when set, is in Hz.
    const uint32_t bwhz = params.bandwidth.value(0);

    // Now build a list of tuning parameters.
    // The delivery system and frequency are required everywhere.
    DTVProperties props;
    props.add(DTV_DELIVERY_SYSTEM, uint32_t(params.delivery_system.value()));
    props.add(DTV_FREQUENCY, freq);

    // Other parameters depend on tuner type
    switch (params.delivery_system.value()) {
        case DS_DVB_S:
        case DS_DVB_S2:
        case DS_DVB_S_TURBO: {
            props.addVar(DTV_MODULATION, params.modulation);
            props.addVar(DTV_SYMBOL_RATE, params.symbol_rate);
            props.addVar(DTV_INNER_FEC, params.inner_fec);
            props.addVar(DTV_INVERSION, params.inversion);
            props.addVar(DTV_ROLLOFF, params.roll_off);
            props.addVar(DTV_PILOT, params.pilots);
#if defined(DTV_STREAM_ID)
            if (params.isi.set() && params.isi.value() != ISI_DISABLE) {
                // With the Linux DVB API, all multistream selection info are passed in the "stream id".
                const uint32_t id =
                    (uint32_t(params.pls_mode.value(ModulationArgs::DEFAULT_PLS_MODE)) << 26) |
                    ((params.pls_code.value(ModulationArgs::DEFAULT_PLS_CODE) & 0x0003FFFF) << 8) |
                    (params.isi.value() & 0x000000FF);
                _duck.report().debug(u"using DVB-S2 multi-stream id 0x%X (%<d)", {id});
                props.add(DTV_STREAM_ID, id);
            }
#endif
            break;
        }
        case DS_DVB_T:
        case DS_DVB_T2: {
            props.addVar(DTV_MODULATION, params.modulation);
            props.addVar(DTV_INVERSION, params.inversion);
            if (bwhz > 0) {
                props.add(DTV_BANDWIDTH_HZ, bwhz);
            }
            props.addVar(DTV_CODE_RATE_HP, params.fec_hp);
            props.addVar(DTV_CODE_RATE_LP, params.fec_lp);
            props.addVar(DTV_TRANSMISSION_MODE, params.transmission_mode);
            props.addVar(DTV_GUARD_INTERVAL, params.guard_interval);
            props.addVar(DTV_HIERARCHY, params.hierarchy);
#if defined(DTV_STREAM_ID)
            props.addVar(DTV_STREAM_ID, params.plp);
#endif
            break;
        }
        case DS_DVB_C_ANNEX_A:
        case DS_DVB_C_ANNEX_B:
        case DS_DVB_C_ANNEX_C:
        case DS_DVB_C2: {
            props.addVar(DTV_MODULATION, params.modulation);
            props.addVar(DTV_INVERSION, params.inversion);
            props.addVar(DTV_INNER_FEC, params.inner_fec);
            props.addVar(DTV_SYMBOL_RATE, params.symbol_rate);
            break;
        }
        case DS_ATSC: {
            props.addVar(DTV_MODULATION, params.modulation);
            props.addVar(DTV_INVERSION, params.inversion);
            break;
        }
        case DS_ISDB_S: {
            props.addVar(DTV_SYMBOL_RATE, params.symbol_rate);
            props.addVar(DTV_INNER_FEC, params.inner_fec);
            props.addVar(DTV_INVERSION, params.inversion);
#if defined(DTV_STREAM_ID)
            props.addVar(DTV_STREAM_ID, params.stream_id);
#endif
            break;
        }
        case DS_ISDB_T: {
            props.addVar(DTV_INVERSION, params.inversion);
            if (bwhz > 0) {
                props.add(DTV_BANDWIDTH_HZ, bwhz);
            }
            props.addVar(DTV_TRANSMISSION_MODE, params.transmission_mode);
            props.addVar(DTV_GUARD_INTERVAL, params.guard_interval);
            props.addVar(DTV_ISDBT_SOUND_BROADCASTING, params.sound_broadcasting);
            props.addVar(DTV_ISDBT_SB_SUBCHANNEL_ID, params.sb_subchannel_id);
            props.addVar(DTV_ISDBT_SB_SEGMENT_COUNT, params.sb_segment_count);
            props.addVar(DTV_ISDBT_SB_SEGMENT_IDX, params.sb_segment_index);
            if (params.isdbt_layers.set()) {
                const UString& layers(params.isdbt_layers.value());
                uint32_t val = 0;
                for (size_t i = 0; i < layers.size(); ++i) {
                    switch (layers[i]) {
                        case u'a': case u'A': val |= 0x01; break;
                        case u'b': case u'B': val |= 0x02; break;
                        case u'c': case u'C': val |= 0x04; break;
                        default: break;
                    }
                }
                props.add(DTV_ISDBT_LAYER_ENABLED, val);
            }
            props.add(DTV_ISDBT_PARTIAL_RECEPTION, params.isdbt_partial_reception.set() ? uint32_t(params.isdbt_partial_reception.value()) : uint32_t(-1));
            props.add(DTV_ISDBT_LAYERA_FEC, params.layer_a_fec.set() ? uint32_t(params.layer_a_fec.value()) : uint32_t(::FEC_AUTO));
            props.add(DTV_ISDBT_LAYERA_MODULATION, params.layer_a_modulation.set() ? uint32_t(params.layer_a_modulation.value()) : uint32_t(::QAM_AUTO));
            props.add(DTV_ISDBT_LAYERA_SEGMENT_COUNT, params.layer_a_segment_count.set() ? uint32_t(params.layer_a_segment_count.value()) : uint32_t(-1));
            props.add(DTV_ISDBT_LAYERA_TIME_INTERLEAVING, params.layer_a_time_interleaving.set() ? uint32_t(params.layer_a_time_interleaving.value()) : uint32_t(-1));
            props.add(DTV_ISDBT_LAYERB_FEC, params.layer_b_fec.set() ? uint32_t(params.layer_b_fec.value()) : uint32_t(::FEC_AUTO));
            props.add(DTV_ISDBT_LAYERB_MODULATION, params.layer_b_modulation.set() ? uint32_t(params.layer_b_modulation.value()) : uint32_t(::QAM_AUTO));
            props.add(DTV_ISDBT_LAYERB_SEGMENT_COUNT, params.layer_b_segment_count.set() ? uint32_t(params.layer_b_segment_count.value()) : uint32_t(-1));
            props.add(DTV_ISDBT_LAYERB_TIME_INTERLEAVING, params.layer_b_time_interleaving.set() ? uint32_t(params.layer_b_time_interleaving.value()) : uint32_t(-1));
            props.add(DTV_ISDBT_LAYERC_FEC, params.layer_c_fec.set() ? uint32_t(params.layer_c_fec.value()) : uint32_t(::FEC_AUTO));
            props.add(DTV_ISDBT_LAYERC_MODULATION, params.layer_c_modulation.set() ? uint32_t(params.layer_c_modulation.value()) : uint32_t(::QAM_AUTO));
            props.add(DTV_ISDBT_LAYERC_SEGMENT_COUNT, params.layer_c_segment_count.set() ? uint32_t(params.layer_c_segment_count.value()) : uint32_t(-1));
            props.add(DTV_ISDBT_LAYERC_TIME_INTERLEAVING, params.layer_c_time_interleaving.set() ? uint32_t(params.layer_c_time_interleaving.value()) : uint32_t(-1));
            break;
        }
        case DS_ISDB_C:
        case DS_DVB_H:
        case DS_ATSC_MH:
        case DS_DTMB:
        case DS_CMMB:
        case DS_DAB:
        case DS_DSS:
        case DS_UNDEFINED:
        default:  {
            _duck.report().error(u"cannot tune on delivery system %s", {DeliverySystemEnum.name(params.delivery_system.value())});
            return false;
        }
    }

    props.add(DTV_TUNE);
    return dtvTune(props);
}


//-----------------------------------------------------------------------------
// Start receiving packets.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::TunerDevice::start()
{
    if (!_is_open) {
        _duck.report().error(u"DVB tuner not open");
        return false;
    }

    // Filter previous abort.
    if (_aborted) {
        return false;
    }

    // Set demux buffer size (default value is 2 kB, fine for sections,
    // completely undersized for full TS capture.

    if (::ioctl(_demux_fd, ioctl_request_t(DMX_SET_BUFFER_SIZE), _demux_bufsize) < 0) {
        _duck.report().error(u"error setting buffer size on %s: %s", {_demux_name, SysErrorCodeMessage()});
        return false;
    }

    // Apply a filter to the demux.

    // The Linux DVB API defines two types of filters: sections and PES.
    // A section filter actually filter sections. On the other hand, a
    // so-called "PES" filter is based on PID's, not PES headers.
    // These PID's may contain anything, not limited to PES data.
    // The magic value 0x2000 is used in the Linux DVB API to say
    // "all PID's" (remember that the max value for a PID is 0x1FFF).
    // Specifying a "PES filter" with PID 0x2000, we get the full TS.

    ::dmx_pes_filter_params filter;
    TS_ZERO(filter);

    filter.pid = 0x2000;                // Means "all PID's"
    filter.input = DMX_IN_FRONTEND;     // Read from frontend device
    filter.output = DMX_OUT_TS_TAP;     // Redirect TS packets to DVR device
    filter.pes_type = DMX_PES_OTHER;    // Any type of PES
    filter.flags = DMX_IMMEDIATE_START; // Start capture immediately

    if (::ioctl(_demux_fd, ioctl_request_t(DMX_SET_PES_FILTER), &filter) < 0) {
        _duck.report().error(u"error setting filter on %s: %s", {_demux_name, SysErrorCodeMessage()});
        return false;
    }

    // Wait for input signal locking if a non-zero timeout is specified.

    bool signal_ok = true;
    for (MilliSecond remain_ms = _signal_timeout; remain_ms > 0; remain_ms -= _signal_poll) {

        // Read the frontend status
        ::fe_status_t status = FE_ZERO;
        getFrontendStatus(status);

        // If the input signal is locked, cool...
        signal_ok = (status & FE_HAS_LOCK) != 0;
        if (signal_ok || _aborted) {
            break;
        }

        // Wait the polling time
        SleepThread(_signal_poll < remain_ms ? _signal_poll : remain_ms);
    }

    // If the timeout has expired, error

    if (_aborted) {
        return false;
    }
    else if (!signal_ok) {
        _duck.report().log(_signal_timeout_silent ? Severity::Debug : Severity::Error, u"no input signal lock after %d milliseconds", {_signal_timeout});
        return false;
    }
    else {
        return true;
    }
}


//-----------------------------------------------------------------------------
// Stop receiving packets.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::TunerDevice::stop(bool silent)
{
    if (!_is_open) {
        if (!silent) {
            _duck.report().error(u"DVB tuner not open");
        }
        return false;
    }

    // Stop the demux
    if (!_aborted && ::ioctl(_demux_fd, ioctl_request_t(DMX_STOP)) < 0) {
        if (!silent) {
            _duck.report().error(u"error stopping demux on %s: %s", {_demux_name, SysErrorCodeMessage()});
        }
        return false;
    }

    return true;
}


//-----------------------------------------------------------------------------
// Empty signal handler, simply interrupt system calls and report EINTR.
//-----------------------------------------------------------------------------

namespace {
    void empty_signal_handler(int)
    {
    }
}


//-----------------------------------------------------------------------------
// Timeout for receive operation (none by default).
// If zero, no timeout is applied.
// Return true on success, false on errors.
//-----------------------------------------------------------------------------

bool ts::TunerDevice::setReceiveTimeout(MilliSecond timeout)
{
    if (timeout > 0) {
        // Set an actual receive timer.
        if (_rt_signal < 0) {
            // Allocate one real-time signal.
            if ((_rt_signal = SignalAllocator::Instance()->allocate()) < 0) {
                _duck.report().error(u"cannot set tuner receive timer, no more signal available");
                return false;
            }

            // Handle the allocated signal
            struct ::sigaction sac;
            TS_ZERO(sac);
            ::sigemptyset(&sac.sa_mask);
            TS_PUSH_WARNING()
            TS_LLVM_NOWARNING(disabled-macro-expansion)
            sac.sa_handler = empty_signal_handler;
            TS_POP_WARNING()
            if (::sigaction(_rt_signal, &sac, nullptr) < 0) {
                _duck.report().error(u"error setting tuner receive timer signal: %s", {SysErrorCodeMessage()});
                SignalAllocator::Instance()->release(_rt_signal);
                _rt_signal = -1;
                return false;
            }
        }

        // Create a timer which triggers the signal
        if (!_rt_timer_valid) {
            ::sigevent sev;
            TS_ZERO(sev);
            sev.sigev_notify = SIGEV_SIGNAL;
            sev.sigev_signo = _rt_signal;
            if (::timer_create(CLOCK_REALTIME, &sev, &_rt_timer) < 0) {
                _duck.report().error(u"error creating tuner receive timer: %s", {SysErrorCodeMessage()});
                return false;
            }
            _rt_timer_valid = true;
        }

        // Now ready to process receive timeout
        _receive_timeout = timeout;
        return true;
    }
    else {
        // Cancel receive timer
        _receive_timeout = 0;
        bool ok = true;

        // Disable and release signal
        if (_rt_signal >= 0) {
            // Ignore further signal delivery
            struct ::sigaction sac;
            TS_ZERO(sac);
            ::sigemptyset(&sac.sa_mask);
            TS_PUSH_WARNING()
            TS_LLVM_NOWARNING(disabled-macro-expansion)
            sac.sa_handler = SIG_IGN;
            TS_POP_WARNING()
            if (::sigaction(_rt_signal, &sac, nullptr) < 0) {
                _duck.report().error(u"error ignoring tuner receive timer signal: %s", {SysErrorCodeMessage()});
                ok = false;
            }
            // Release signal
            SignalAllocator::Instance()->release(_rt_signal);
            _rt_signal = -1;
        }

        // Disarm and delete timer
        if (_rt_timer_valid) {
            _rt_timer_valid = false;
            if (::timer_delete(_rt_timer) < 0) {
                _duck.report().error(u"error deleting tuner receive timer: %s", {SysErrorCodeMessage()});
                ok = false;
            }
        }

        return ok;
    }
}


//-----------------------------------------------------------------------------
// Read complete 188-byte TS packets in the buffer and return the
// number of actually received packets (in the range 1 to max_packets).
// Returning zero means error or end of input.
//-----------------------------------------------------------------------------

size_t ts::TunerDevice::receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort)
{
    if (!_is_open) {
        _duck.report().error(u"DVB tuner not open");
        return 0;
    }

    // Filter previous abort.
    if (_aborted) {
        return 0;
    }

    char* const data = reinterpret_cast<char*>(buffer);
    size_t req_size = max_packets * PKT_SIZE;
    size_t got_size = 0;
    int overflow_count = 0;

    // Set deadline if receive timeout in effect
    Time time_limit;
    if (_receive_timeout > 0) {
        assert(_rt_timer_valid);
        // Arm the receive timer.
        // Note that _receive_timeout is in milliseconds and ::itimerspec is in nanoseconds.
        ::itimerspec timeout;
        timeout.it_value.tv_sec = long(_receive_timeout / 1000);
        timeout.it_value.tv_nsec = long(1000000 * (_receive_timeout % 1000));
        timeout.it_interval.tv_sec = 0;
        timeout.it_interval.tv_nsec = 0;
        if (::timer_settime(_rt_timer, 0, &timeout, nullptr) < 0) {
            _duck.report().error(u"error arming tuner receive timer: %s", {SysErrorCodeMessage()});
            return 0;
        }
        // Deadline time
        time_limit = Time::CurrentLocalTime() + _receive_timeout;
    }

    // Loop on read until we get enough
    while (got_size < req_size && !_aborted) {

        // Read some data
        bool got_overflow = false;
        _reading_dvr = true;
        const ssize_t insize = ::read(_dvr_fd, data + got_size, req_size - got_size);
        _reading_dvr = false;

        if (insize > 0) {
            // Normal case: some data were read
            assert(got_size + size_t(insize) <= req_size);
            got_size += size_t(insize);
        }
        else if (insize == 0) {
            // End of file. Truncate potential partial packet at eof.
            got_size -= got_size % PKT_SIZE;
        }
        else if (errno == EINTR) {
            // Input was interrupted by a signal.
            // If the application should be interrupted, stop now.
            if (_aborted || (abort != nullptr && abort->aborting())) {
                break;
            }
        }
        else if (errno == EOVERFLOW) {
            got_overflow = true;
        }
        else {
            _duck.report().error(u"receive error on %s: %s", {_dvr_name, SysErrorCodeMessage()});
            break;
        }

        // Input overflow management: If an input overflow occurs more than
        // MAX_OVERFLOW consecutive times, an error is generated.
        if (!got_overflow) {
            // Reset overflow count
            overflow_count = 0;
        }
        else if (++overflow_count > MAX_OVERFLOW) {
            _duck.report().error(u"input overflow, possible packet loss");
            break;
        }

        // If the receive timeout is exceeded, stop now.
        // FIXME: There is a race condition here. If the receiver timer is
        // triggered between this test and the start of the next read, the
        // next read will not be interrupted and the receive timer will not
        // apply to this read.
        if (_receive_timeout > 0 && Time::CurrentLocalTime() >= time_limit) {
            if (got_size == 0) {
                _duck.report().error(u"receive timeout on %s", {_device_name});
            }
            break;
        }
    }

    // Disarm the receive timer.
    if (_receive_timeout > 0) {
        ::itimerspec timeout;
        timeout.it_value.tv_sec = 0;
        timeout.it_value.tv_nsec = 0;
        timeout.it_interval.tv_sec = 0;
        timeout.it_interval.tv_nsec = 0;
        if (::timer_settime(_rt_timer, 0, &timeout, nullptr) < 0) {
            _duck.report().error(u"error disarming tuner receive timer: %s", {SysErrorCodeMessage()});
        }
    }

    // Look for unsynchronized packets in reception buffer.
    // Similar code was initially introduced in the Windows version because
    // such loss of synchronization was actually observed. In response to
    // some weird reception errors with the Hauppauge Nova-TD-500, this
    // code was also added in the Linux version but the errors were
    // different. So, this code is apparently useless on Linux, although
    // it adds some robustness at the expense of some performance degradation.

    for (size_t offset = 0; offset + PKT_SIZE <= got_size; offset += PKT_SIZE) {
        if (data[offset] != SYNC_BYTE) {
            // Error, lost synchronization.
            // Look for at least 10 successive sync bytes.
            const size_t needed_packet_count = std::min<size_t>(10, (got_size - offset) / PKT_SIZE);
            const size_t last_possible_resync_offset = got_size - needed_packet_count * PKT_SIZE;
            size_t resync_offset = offset;
            bool found = false;
            while (resync_offset <= last_possible_resync_offset) {
                if (data[resync_offset] == SYNC_BYTE) {
                    // Possible packet here, look for needed packet count
                    found = true;
                    for (size_t n = 1; found && n < needed_packet_count; ++n) {
                        found = data[resync_offset + n * PKT_SIZE] == SYNC_BYTE;
                    }
                    if (found) {
                        break;
                    }
                }
                // No packet or not enough packets here, look further
                resync_offset++;
            }

            // If not enough packets found for reliable resynchronization, drop the rest.
            if (!found) {
                resync_offset = got_size;
            }

            // Report error
            _duck.report().error(u"tuner packet synchronization lost, dropping %'d bytes", {resync_offset - offset});

            // Pack rest of buffer
            ::memmove(data + offset, data + resync_offset, got_size - resync_offset);
            got_size -= resync_offset - offset;
        }
    }

    // Return the number of input packets.
    return got_size / PKT_SIZE;
}


//-----------------------------------------------------------------------------
//  This routine displays a list of flags
//-----------------------------------------------------------------------------

namespace {
    void DisplayFlags(std::ostream& strm,
                      const ts::UString& margin,
                      const ts::UString& name,
                      uint32_t value,
                      const ts::Enumeration& table)
    {
        const size_t max_width = 78;
        bool first = true;
        strm << margin << name << ": ";
        size_t width = margin.size() + name.size() + 2;

        for (uint32_t flag = 1; flag != 0; flag = flag << 1) {
            if ((value & flag) != 0) {
                const ts::UString flag_name(table.name(flag));
                if (width + 2 + flag_name.size() > max_width) {
                    strm << (first ? "" : ",") << std::endl
                         << margin << "  " << flag_name;
                    width = margin.size() + 2 + flag_name.size();
                }
                else if (first) {
                    strm << flag_name;
                    width += flag_name.size();
                }
                else {
                    strm << ", " << flag_name;
                    width += 2 + flag_name.size();
                }
                first = false;
            }
        }
        strm << std::endl;
    }
}


//-----------------------------------------------------------------------------
//  This routine displays a name/value pair
//-----------------------------------------------------------------------------

namespace {
    void Display(std::ostream& strm, const ts::UString& margin, const ts::UString& name, const ts::UString& value, const ts::UString& unit)
    {
        strm << margin << name.toJustified(value, 50, u'.', 1) << " " << unit << std::endl;
    }
}


//-----------------------------------------------------------------------------
// Display the characteristics and status of the tuner.
//-----------------------------------------------------------------------------

std::ostream& ts::TunerDevice::displayStatus(std::ostream& strm, const ts::UString& margin, bool extended)
{
    if (!_is_open) {
        _duck.report().error(u"DVB tuner not open");
        return strm;
    }

    // Strings for enum fe_status
    const Enumeration enum_fe_status({
        {u"has signal",  ::FE_HAS_SIGNAL},
        {u"has carrier", ::FE_HAS_CARRIER},
        {u"has viterbi", ::FE_HAS_VITERBI},
        {u"has sync",    ::FE_HAS_SYNC},
        {u"has lock",    ::FE_HAS_LOCK},
        {u"timedout",    ::FE_TIMEDOUT},
        {u"reinit",      ::FE_REINIT},
    });

    // Strings for enum fe_caps
    const Enumeration enum_fe_caps({
        {u"inversion auto",         ::FE_CAN_INVERSION_AUTO},
        {u"FEC 1/2",                ::FE_CAN_FEC_1_2},
        {u"FEC 2/3",                ::FE_CAN_FEC_2_3},
        {u"FEC 3/4",                ::FE_CAN_FEC_3_4},
        {u"FEC 4/5",                ::FE_CAN_FEC_4_5},
        {u"FEC 5/6",                ::FE_CAN_FEC_5_6},
        {u"FEC 6/7",                ::FE_CAN_FEC_6_7},
        {u"FEC 7/8",                ::FE_CAN_FEC_7_8},
        {u"FEC 8/9",                ::FE_CAN_FEC_8_9},
        {u"FEC auto",               ::FE_CAN_FEC_AUTO},
        {u"QPSK",                   ::FE_CAN_QPSK},
        {u"16-QAM",                 ::FE_CAN_QAM_16},
        {u"32-QAM",                 ::FE_CAN_QAM_32},
        {u"64-QAM",                 ::FE_CAN_QAM_64},
        {u"128-QAM",                ::FE_CAN_QAM_128},
        {u"256-QAM",                ::FE_CAN_QAM_256},
        {u"QAM auto",               ::FE_CAN_QAM_AUTO},
        {u"transmission mode auto", ::FE_CAN_TRANSMISSION_MODE_AUTO},
        {u"bandwidth auto",         ::FE_CAN_BANDWIDTH_AUTO},
        {u"guard interval auto",    ::FE_CAN_GUARD_INTERVAL_AUTO},
        {u"hierarchy auto",         ::FE_CAN_HIERARCHY_AUTO},
        {u"8-VSB",                  ::FE_CAN_8VSB},
        {u"16-VSB",                 ::FE_CAN_16VSB},
        {u"extended caps",          ::FE_HAS_EXTENDED_CAPS},
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
        {u"multistream",            int(0x4000000)},
#else
        {u"multistream",            ::FE_CAN_MULTISTREAM},
#endif
        {u"turbo FEC",              ::FE_CAN_TURBO_FEC},
        {u"2nd generation",         ::FE_CAN_2G_MODULATION},
        {u"needs bending",          ::FE_NEEDS_BENDING},
        {u"recover",                ::FE_CAN_RECOVER},
        {u"mute TS",                int(::FE_CAN_MUTE_TS)},
    });

    // Read current status, ignore errors.
    ::fe_status_t status = FE_ZERO;
    if (getFrontendStatus(status) && status != FE_ZERO) {
        DisplayFlags(strm, margin, u"Status", uint32_t(status), enum_fe_status);
        strm << std::endl;
    }

    // Read current signal status.
    SignalState state;
    if (getSignalState(state)) {
        strm << margin << "Signal: " << state.toString() << std::endl << std::endl;
    }

    // Read current tuning parameters. Ignore errors (some fields may be unset).
    ModulationArgs params;
    getCurrentTuning(params, false);

    // Display delivery system.
    DeliverySystem delsys = params.delivery_system.value(DS_UNDEFINED);
    if (delsys == DS_UNDEFINED) {
        delsys = _delivery_systems.preferred();
    }
    const TunerType ttype = TunerTypeOf(delsys);
    Display(strm, margin, u"Delivery system", DeliverySystemEnum.name(delsys), u"");

    // Display frequency characteristics
    const uint64_t freq = params.frequency.value(0);
    const uint64_t hz_factor = IsSatelliteDelivery(delsys) ? 1000 : 1;
    strm << margin << "Frequencies:" << std::endl;
    if (freq > 0) {
        Display(strm, margin, u"  Current", UString::Decimal(freq), u"Hz");
        if (IsTerrestrialDelivery(delsys)) {
            // Get UHF and VHF band descriptions in the default region.
            const HFBand* uhf = _duck.uhfBand();
            const HFBand* vhf = _duck.vhfBand();
            if (uhf->inBand(freq, true)) {
                Display(strm, margin, u"  UHF channel", UString::Decimal(uhf->channelNumber(freq)), u"");
            }
            else if (vhf->inBand(freq, true)) {
                Display(strm, margin, u"  VHF channel", UString::Decimal(vhf->channelNumber(freq)), u"");
            }
        }
    }
    Display(strm, margin, u"  Min", UString::Decimal(hz_factor * _fe_info.frequency_min), u"Hz");
    Display(strm, margin, u"  Max", UString::Decimal(hz_factor * _fe_info.frequency_max), u"Hz");
    Display(strm, margin, u"  Step", UString::Decimal(hz_factor * _fe_info.frequency_stepsize), u"Hz");
    Display(strm, margin, u"  Tolerance", UString::Decimal(hz_factor * _fe_info.frequency_tolerance), u"Hz");

    // Display symbol rate characteristics.
    if (ttype == TT_DVB_S || ttype == TT_DVB_C || ttype == TT_ISDB_S || ttype == TT_ISDB_C) {
        const uint32_t symrate = params.symbol_rate.value(0);
        strm << margin << "Symbol rates:" << std::endl;
        if (symrate > 0) {
            Display(strm, margin, u"  Current", UString::Decimal(symrate), u"sym/s");
        }
        Display(strm, margin, u"  Min", UString::Decimal(_fe_info.symbol_rate_min), u"sym/s");
        Display(strm, margin, u"  Max", UString::Decimal(_fe_info.symbol_rate_max), u"sym/s");
        Display(strm, margin, u"  Tolerance", UString::Decimal(_fe_info.symbol_rate_tolerance), u"sym/s");
    }

    // Frontend-specific information
    if (params.inversion.set()) {
        Display(strm, margin, u"Spectral inversion", SpectralInversionEnum.name(params.inversion.value()), u"");
    }
    if (params.inner_fec.set()) {
        Display(strm, margin, u"FEC(inner)", InnerFECEnum.name(params.inner_fec.value()), u"");
    }
    if (params.modulation.set()) {
        Display(strm, margin, u"Modulation", ModulationEnum.name(params.modulation.value()), u"");
    }
    if (params.bandwidth.set()) {
        Display(strm, margin, u"Bandwidth", UString::Decimal(params.bandwidth.value()), u"Hz");
    }
    if (params.fec_hp.set()) {
        Display(strm, margin, u"FEC(high priority)", InnerFECEnum.name(params.fec_hp.value()), u"");
    }
    if (params.fec_lp.set()) {
        Display(strm, margin, u"FEC(low priority)", InnerFECEnum.name(params.fec_lp.value()), u"");
    }
    if (params.transmission_mode.set()) {
        Display(strm, margin, u"Transmission mode", TransmissionModeEnum.name(params.transmission_mode.value()), u"");
    }
    if (params.guard_interval.set()) {
        Display(strm, margin, u"Guard interval", GuardIntervalEnum.name(params.guard_interval.value()), u"");
    }
    if (params.hierarchy.set()) {
        Display(strm, margin, u"Hierarchy", HierarchyEnum.name(params.hierarchy.value()), u"");
    }
    if (params.plp.set() && params.plp != PLP_DISABLE) {
        Display(strm, margin, u"PLP", UString::Decimal(params.plp.value()), u"");
    }

    // Display general capabilities
    strm << std::endl;
    DisplayFlags(strm, margin, u"Capabilities", uint32_t(_fe_info.caps), enum_fe_caps);

    return strm;
}
