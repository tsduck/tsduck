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
//  DVB tuner. Linux implementation
//
//-----------------------------------------------------------------------------

#include "tsTuner.h"
#include "tsTime.h"
#include "tsSysUtils.h"
#include "tsSignalAllocator.h"
#include "tsStringUtils.h"
#include "tsNullReport.h"
#include "tsToInteger.h"
#include "tsDecimal.h"
#include "tsFormat.h"
#include "tsMemoryUtils.h"
TSDUCK_SOURCE;

#define MAX_OVERFLOW  8   // Maximum consecutive overflow

#if defined (TS_NEED_STATIC_CONST_DEFINITIONS)
const ts::MilliSecond ts::Tuner::DEFAULT_SIGNAL_TIMEOUT;
const ts::MilliSecond ts::Tuner::DEFAULT_SIGNAL_POLL;
const size_t ts::Tuner::DEFAULT_DEMUX_BUFFER_SIZE;
#endif


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
        return ::ioctl(fd, FE_SET_TONE, tone);
    }
    inline int ioctl_fe_set_voltage(int fd, fe_sec_voltage_t voltage)
    {
        return ::ioctl(fd, FE_SET_VOLTAGE, voltage);
    }
    inline int ioctl_fe_diseqc_send_burst(int fd, fe_sec_mini_cmd_t burst)
    {
        return ::ioctl(fd, FE_DISEQC_SEND_BURST, burst);
    }
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------

ts::Tuner::~Tuner ()
{
    // Close tuner devices if open
    close (NULLREP);

    // Cleanup receive timer resources
    setReceiveTimeout (0, NULLREP);
}


//-----------------------------------------------------------------------------
// Default constructor,
//-----------------------------------------------------------------------------

ts::Tuner::Tuner(const std::string& device_name) :
    _is_open(false),
    _info_only(true),
    _tuner_type(DVB_T),
    _device_name(device_name),
    _device_info(),
    _signal_timeout(DEFAULT_SIGNAL_TIMEOUT),
    _signal_timeout_silent(false),
    _receive_timeout(0),
    _delivery_systems(),
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
    _rt_timer(0),
    _rt_timer_valid(false)
{
}


//-----------------------------------------------------------------------------
// Constructor from one device name.
//-----------------------------------------------------------------------------

ts::Tuner::Tuner(const std::string& device_name, bool info_only, Report& report) :
    Tuner(device_name)
{
    // Flawfinder: ignore: this is our open(), not ::open().
    this->open(device_name, info_only, report);
}


//-----------------------------------------------------------------------------
// Get the list of all existing DVB tuners.
//-----------------------------------------------------------------------------

bool ts::Tuner::GetAllTuners(TunerPtrVector& tuners, Report& report)
{
    // Reset returned vector
    tuners.clear();

    // Get list of all DVB adapters
    StringVector names;
    ExpandWildcard(names, "/dev/dvb/adapter*");

    // Open all tuners
    tuners.reserve(names.size());
    bool ok = true;
    for (StringVector::const_iterator it = names.begin(); it != names.end(); ++it) {
        const size_t index = tuners.size();
        tuners.resize (index + 1);
        tuners[index] = new Tuner (*it, true, report);
        if (!tuners[index]->isOpen()) {
            ok = false;
            tuners[index].clear();
            tuners.resize (index);
        }
    }

    return ok;
}


//-----------------------------------------------------------------------------
// Open the tuner.
//-----------------------------------------------------------------------------

// Flawfinder: ignore: this is our open(), not ::open().
bool ts::Tuner::open(const std::string& device_name, bool info_only, Report& report)
{
    if (_is_open) {
        report.error ("DVB tuner already open");
        return false;
    }

    _info_only = info_only;

    // Analyze device name: /dev/dvb/adapterA[:F[:M[:V]]]

    int frontend_nb = 0;
    int demux_nb = 0;
    int dvr_nb = 0;
    StringVector fields;
    if (device_name.empty()) {
        // Default tuner is first one
        fields.push_back ("/dev/dvb/adapter0");
    }
    else {
        SplitString (fields, device_name, ':', false);
    }
    const size_t fcount = fields.size();
    bool ok = fcount >= 1 && fcount <= 4 &&
        (fcount < 2 || ToInteger (frontend_nb, fields[1])) &&
        (fcount < 3 || ToInteger (demux_nb, fields[2])) &&
        (fcount < 4 || ToInteger (dvr_nb, fields[3]));
    if (!ok) {
        report.error ("invalid DVB tuner name " + device_name);
        return false;
    }

    _device_name = fields[0];
    if (dvr_nb != 0) {
        _device_name += Format (":%d:%d:%d", frontend_nb, demux_nb, dvr_nb);
    }
    else if (demux_nb != 0) {
        _device_name += Format (":%d:%d", frontend_nb, demux_nb);
    }
    else if (frontend_nb != 0) {
        _device_name += Format (":%d", frontend_nb);
    }
    _frontend_name = fields[0] + Format ("/frontend%d", frontend_nb);
    _demux_name = fields[0] + Format ("/demux%d", demux_nb);
    _dvr_name = fields[0] + Format ("/dvr%d", dvr_nb);

    // Open DVB adapter frontend. The frontend device is opened in non-blocking mode.
    // All configuration and setup operations are non-blocking anyway.
    // Reading events, however, is a blocking operation.

    // Flawfinder: ignore: trusted open()
    if ((_frontend_fd = ::open(_frontend_name.c_str(), (info_only ? O_RDONLY : O_RDWR) | O_NONBLOCK)) < 0) {
        report.error("error opening " + _frontend_name + ": " + ErrorCodeMessage());
        return false;
    }

    // Get characteristics of the frontend

    if (::ioctl(_frontend_fd, FE_GET_INFO, &_fe_info) < 0) {
        report.error("error getting info on " + _frontend_name + ": " + ErrorCodeMessage());
        return close(report) || false;
    }
    _fe_info.name[sizeof(_fe_info.name) - 1] = 0;
    _device_info = _fe_info.name;
    _delivery_systems.reset();

    switch (_fe_info.type) {
        case ::FE_QPSK:
            _tuner_type = DVB_S;
            _delivery_systems.set (DS_DVB_S);
#if TS_DVB_API_VERSION >= 501
            if ((_fe_info.caps & FE_CAN_2G_MODULATION) != 0) {
                _delivery_systems.set(DS_DVB_S2);
            }
#endif
            break;
        case ::FE_QAM:
            _tuner_type = DVB_C;
            _delivery_systems.set(DS_DVB_C);
#if TS_DVB_API_VERSION >= 501
            if ((_fe_info.caps & FE_CAN_2G_MODULATION) != 0) {
                _delivery_systems.set(DS_DVB_C2);
            }
#endif
            break;
        case ::FE_OFDM:
            _tuner_type = DVB_T;
            _delivery_systems.set (DS_DVB_T);
#if TS_DVB_API_VERSION >= 501
            if ((_fe_info.caps & FE_CAN_2G_MODULATION) != 0) {
                _delivery_systems.set(DS_DVB_T2);
            }
#endif
            break;
        case ::FE_ATSC:
            _tuner_type = ATSC;
            break;
        default:
            report.error("unsupported frontend type %d on %s (%s)", int (_fe_info.type), _frontend_name.c_str(), _fe_info.name);
            return close(report) || false;
    }

    // Open DVB adapter DVR (tap for TS packets) and adapter demux

    if (_info_only) {
        _dvr_fd = _demux_fd = -1;
    }
    else {
        // Flawfinder: ignore: trusted ::open().
        if ((_dvr_fd = ::open(_dvr_name.c_str(), O_RDONLY)) < 0) {
            report.error("error opening " + _dvr_name + ": " + ErrorCodeMessage());
            return close(report) || false;
        }
        // Flawfinder: ignore: trusted ::open().
        if ((_demux_fd = ::open(_demux_name.c_str(), O_RDWR)) < 0) {
            report.error("error opening " + _demux_name + ": " + ErrorCodeMessage());
            return close(report) || false;
        }
    }

    return _is_open = true;
}


//-----------------------------------------------------------------------------
// Close tuner.
//-----------------------------------------------------------------------------

bool ts::Tuner::close (Report& report)
{
    // Stop the demux
    if (_demux_fd >= 0 && ::ioctl (_demux_fd, DMX_STOP) < 0) {
        report.error ("error stopping demux on " + _demux_name + ": " + ErrorCodeMessage());
    }

    // Close DVB adapter devices
    if (_dvr_fd >= 0) {
        ::close (_dvr_fd);
        _dvr_fd = -1;
    }
    if (_demux_fd >= 0) {
        ::close (_demux_fd);
        _demux_fd = -1;
    }
    if (_frontend_fd >= 0) {
        ::close (_frontend_fd);
        _frontend_fd = -1;
    }

    _is_open = false;
    _device_name.clear();
    _device_info.clear();
    _frontend_name.clear();
    _demux_name.clear();
    _dvr_name.clear();

    return true;
}


//-----------------------------------------------------------------------------
// Check if a signal is present and locked
//-----------------------------------------------------------------------------

bool ts::Tuner::signalLocked (Report& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }

    ::fe_status_t status;
    if (::ioctl (_frontend_fd, FE_READ_STATUS, &status) < 0) {
        report.error ("error reading status on " + _frontend_name + ": " + ErrorCodeMessage());
        return false;
    }

    return (status & ::FE_HAS_LOCK) != 0;
}


//-----------------------------------------------------------------------------
// Return signal strength, in percent (0=bad, 100=good)
// Return a negative value on error.
//-----------------------------------------------------------------------------

int ts::Tuner::signalStrength (Report& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }

    uint16_t strength;
    if (::ioctl (_frontend_fd, FE_READ_SIGNAL_STRENGTH, &strength) < 0) {
        report.error ("error reading signal strength on " + _frontend_name + ": " + ErrorCodeMessage());
        return -1;
    }

    // Strength is an uint16_t: 0x0000 = 0%, 0xFFFF = 100%
    return (int (strength) * 100) / 0xFFFF;
}


//-----------------------------------------------------------------------------
// Return signal quality, in percent (0=bad, 100=good)
// Return a negative value on error.
//-----------------------------------------------------------------------------

int ts::Tuner::signalQuality (Report& report)
{
    // No known signal quality on Linux. BER (bit error rate) is supported
    // by the API but the unit is not clearly defined, the returned value
    // is often zero. So, BER is generally unreliable / unusable.
    return -1;
}


//-----------------------------------------------------------------------------
// Get current tuning parameters for DVB-S tuners, return system error code
//-----------------------------------------------------------------------------

ts::ErrorCode ts::Tuner::getCurrentTuningDVBS (TunerParametersDVBS& params)
{
    // Note: it is useless to get the frequency of a DVB-S tuner since it
    // returns the intermediate frequency and there is no unique satellite
    // frequency for a given intermediate frequency.

    DTVProperties props;
    props.add (DTV_INVERSION);
    props.add (DTV_SYMBOL_RATE);
    props.add (DTV_INNER_FEC);
    props.add (DTV_DELIVERY_SYSTEM);
    props.add (DTV_MODULATION);
    props.add (DTV_PILOT);
    props.add (DTV_ROLLOFF);

    if (::ioctl (_frontend_fd, FE_GET_PROPERTY, props.getIoctlParam()) < 0) {
        return LastErrorCode();
    }

    params.inversion = SpectralInversion (props.getByCommand (DTV_INVERSION));
    params.symbol_rate = props.getByCommand (DTV_SYMBOL_RATE);
    params.inner_fec = InnerFEC (props.getByCommand (DTV_INNER_FEC));
    params.delivery_system = fromLinuxDeliverySystem (::fe_delivery_system (props.getByCommand (DTV_DELIVERY_SYSTEM)));
    params.modulation = Modulation (props.getByCommand (DTV_MODULATION));
    params.pilots = Pilot (props.getByCommand (DTV_PILOT));
    params.roll_off = RollOff (props.getByCommand (DTV_ROLLOFF));

    return SYS_SUCCESS;
}


//-----------------------------------------------------------------------------
// Get current tuning parameters for DVB-C tuners, return system error code
//-----------------------------------------------------------------------------

ts::ErrorCode ts::Tuner::getCurrentTuningDVBC (TunerParametersDVBC& params)
{
    DTVProperties props;
    props.add (DTV_FREQUENCY);
    props.add (DTV_INVERSION);
    props.add (DTV_SYMBOL_RATE);
    props.add (DTV_INNER_FEC);
    props.add (DTV_MODULATION);

    if (::ioctl (_frontend_fd, FE_GET_PROPERTY, props.getIoctlParam()) < 0) {
        return LastErrorCode();
    }

    params.frequency = props.getByCommand (DTV_FREQUENCY);
    params.inversion = SpectralInversion (props.getByCommand (DTV_INVERSION));
    params.symbol_rate = props.getByCommand (DTV_SYMBOL_RATE);
    params.inner_fec = InnerFEC (props.getByCommand (DTV_INNER_FEC));
    params.modulation = Modulation (props.getByCommand (DTV_MODULATION));

    return SYS_SUCCESS;
}


//-----------------------------------------------------------------------------
// Get current tuning parameters for DVB-T tuners, return system error code
//-----------------------------------------------------------------------------

ts::ErrorCode ts::Tuner::getCurrentTuningDVBT (TunerParametersDVBT& params)
{
    DTVProperties props;
    props.add (DTV_FREQUENCY);
    props.add (DTV_INVERSION);
    props.add (DTV_BANDWIDTH_HZ);
    props.add (DTV_CODE_RATE_HP);
    props.add (DTV_CODE_RATE_LP);
    props.add (DTV_MODULATION);
    props.add (DTV_TRANSMISSION_MODE);
    props.add (DTV_GUARD_INTERVAL);
    props.add (DTV_HIERARCHY);

    if (::ioctl (_frontend_fd, FE_GET_PROPERTY, props.getIoctlParam()) < 0) {
        return LastErrorCode();
    }

    params.frequency = props.getByCommand (DTV_FREQUENCY);
    params.inversion = SpectralInversion (props.getByCommand (DTV_INVERSION));
    params.bandwidth = BandWidthCodeFromHz (props.getByCommand (DTV_BANDWIDTH_HZ));
    params.fec_hp = InnerFEC (props.getByCommand (DTV_CODE_RATE_HP));
    params.fec_lp = InnerFEC (props.getByCommand (DTV_CODE_RATE_LP));
    params.modulation = Modulation (props.getByCommand (DTV_MODULATION));
    params.transmission_mode = TransmissionMode (props.getByCommand (DTV_TRANSMISSION_MODE));
    params.guard_interval = GuardInterval (props.getByCommand (DTV_GUARD_INTERVAL));
    params.hierarchy = Hierarchy (props.getByCommand (DTV_HIERARCHY));

    return SYS_SUCCESS;
}


//-----------------------------------------------------------------------------
// Get current tuning parameters for ATSC tuners, return system error code
//-----------------------------------------------------------------------------

ts::ErrorCode ts::Tuner::getCurrentTuningATSC (TunerParametersATSC& params)
{
    DTVProperties props;
    props.add (DTV_FREQUENCY);
    props.add (DTV_INVERSION);
    props.add (DTV_MODULATION);

    if (::ioctl (_frontend_fd, FE_GET_PROPERTY, props.getIoctlParam()) < 0) {
        return LastErrorCode();
    }

    params.frequency = props.getByCommand (DTV_FREQUENCY);
    params.inversion = SpectralInversion (props.getByCommand (DTV_INVERSION));
    params.modulation = Modulation (props.getByCommand (DTV_MODULATION));

    return SYS_SUCCESS;
}


//-----------------------------------------------------------------------------
// Get the current tuning parameters
//-----------------------------------------------------------------------------

bool ts::Tuner::getCurrentTuning (TunerParameters& params, bool reset_unknown, Report& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }

    // Check subclass of TunerParameters

    if (params.tunerType() != _tuner_type) {
        report.error ("inconsistent tuner parameter type");
        return false;
    }

    // Get transponder tuning information

    ErrorCode error = SYS_SUCCESS;

    switch (_tuner_type) {
        case DVB_S: {
            TunerParametersDVBS* tpp = dynamic_cast<TunerParametersDVBS*> (&params);
            assert (tpp != 0);
            if (reset_unknown) {
                tpp->frequency = 0;
                tpp->polarity = TunerParametersDVBS::DEFAULT_POLARITY;
                tpp->satellite_number = TunerParametersDVBS::DEFAULT_SATELLITE_NUMBER;
                tpp->lnb.setUniversalLNB();
            }
            error = getCurrentTuningDVBS (*tpp);
            break;
        }
        case DVB_C: {
            TunerParametersDVBC* tpp = dynamic_cast<TunerParametersDVBC*> (&params);
            assert (tpp != 0);
            error = getCurrentTuningDVBC (*tpp);
            break;
        }
        case DVB_T: {
            TunerParametersDVBT* tpp = dynamic_cast<TunerParametersDVBT*> (&params);
            assert (tpp != 0);
            error = getCurrentTuningDVBT (*tpp);
            break;
        }
        case ATSC: {
            TunerParametersATSC* tpp = dynamic_cast<TunerParametersATSC*> (&params);
            assert (tpp != 0);
            error = getCurrentTuningATSC (*tpp);
            break;
        }
        default: {
            report.error ("cannot convert Linux DVB parameters to " + TunerTypeEnum.name (_tuner_type) + " parameters");
            return false;
        }
    }

    if (error != SYS_SUCCESS) {
        report.error ("error getting DVB frontend parameters : " + ErrorCodeMessage (error));
        return false;
    }

    return true;
}


//-----------------------------------------------------------------------------
// Discard all pending frontend events
//-----------------------------------------------------------------------------

void ts::Tuner::discardFrontendEvents (Report& report)
{
    ::dvb_frontend_event event;
    report.debug("starting discarding frontend events");
    while (::ioctl(_frontend_fd, FE_GET_EVENT, &event) >= 0) {
        report.debug("one frontend event discarded");
    }
    report.debug("finished discarding frontend events");
}


//-----------------------------------------------------------------------------
// Tune operation, return true on success, false on error
//-----------------------------------------------------------------------------

bool ts::Tuner::tune(DTVProperties& props, Report& report)
{
    report.debug ("tuning on " + _frontend_name);
    props.report (report, Severity::Debug);
    if (::ioctl (_frontend_fd, FE_SET_PROPERTY, props.getIoctlParam()) < 0) {
        report.error ("tuning error on " + _frontend_name + ": " + ErrorCodeMessage());
        return false;
    }
    return true;
}


//-----------------------------------------------------------------------------
// Clear tuner, return true on success, false on error
//-----------------------------------------------------------------------------

bool ts::Tuner::dtvClear (Report& report)
{
    DTVProperties props;
    props.add (DTV_CLEAR);
    return tune(props, report);
}


//-----------------------------------------------------------------------------
// Tune for DVB-S tuners, return true on success, false on error
//-----------------------------------------------------------------------------

bool ts::Tuner::tuneDVBS (const TunerParametersDVBS& params, Report& report)
{
    // Clear tuner state.
    if (!dtvClear(report)) {
        return false;
    }

    // For satellite, control the dish first
    //
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
        report.error ("DVB frontend FE_SET_TONE error: " + ErrorCodeMessage());
        return false;
    }

    // Setup polarisation voltage: 13V for vertical polarisation, 18V for horizontal
    if (ioctl_fe_set_voltage(_frontend_fd, params.polarity == POL_VERTICAL ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18) < 0) {
        report.error("DVB frontend FE_SET_VOLTAGE error: " + ErrorCodeMessage());
        return false;
    }

    // Wait at least 15ms.
    ::nanosleep(&delay, NULL);

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
        report.error("DVB frontend FE_DISEQC_SEND_BURST error: " + ErrorCodeMessage());
        return false;
    }

    // Wait 15ms
    ::nanosleep(&delay, NULL);

    // Send DiSEqC commands. See DiSEqC spec ...
    const bool high_band = params.lnb.useHighBand (params.frequency);
    ::dvb_diseqc_master_cmd cmd;
    cmd.msg_len = 4;    // Message size (meaningful bytes in msg)
    cmd.msg[0] = 0xE0;  // Command from master, no reply expected, first transmission
    cmd.msg[1] = 0x10;  // Any LNB or switcher (master to all)
    cmd.msg[2] = 0x38;  // Write to port group 0
    cmd.msg[3] = 0xF0 | // Clear all 4 flags first, then set according to next 4 bits
        ((uint8_t(params.satellite_number) << 2) & 0x0F) |
        (params.polarity == POL_VERTICAL ? 0x00 : 0x02) |
        (high_band ? 0x01 : 0x00);
    cmd.msg[4] = 0x00;  // Unused
    cmd.msg[5] = 0x00;  // Unused

    if (::ioctl(_frontend_fd, FE_DISEQC_SEND_MASTER_CMD, &cmd) < 0) {
        report.error("DVB frontend FE_DISEQC_SEND_MASTER_CMD error: " + ErrorCodeMessage());
        return false;
    }

    // Wait 15ms
    ::nanosleep (&delay, NULL);

    // Start the 22kHz continuous tone when tuning to a transponder in the high band
    if (::ioctl_fe_set_tone(_frontend_fd, high_band ? SEC_TONE_ON : SEC_TONE_OFF) < 0) {
        report.error("DVB frontend FE_SET_TONE error: " + ErrorCodeMessage());
        return false;
    }

    // End of dish setup, now configure the tuner
    if (!CheckModEnum(params.inversion, "spectral inversion", SpectralInversionEnum, report) ||
        !CheckModEnum(params.inner_fec, "FEC", InnerFECEnum, report) ||
        !CheckModEnum(params.modulation, "modulation", ModulationEnum, report) ||
        !CheckModEnum(params.pilots, "pilots", PilotEnum, report) ||
        !CheckModEnum(params.roll_off, "roll-off factor", RollOffEnum, report)) {
        return false;
    }

    // For DVB-S/S2, Linux DVB API uses an intermediate frequency in kHz
    const uint32_t intermediate_frequency = uint32_t(params.lnb.intermediateFrequency (params.frequency) / 1000);

    discardFrontendEvents (report);

    DTVProperties props;
    props.add(DTV_DELIVERY_SYSTEM, uint32_t(toLinuxDeliverySystem (params.delivery_system)));
    props.add(DTV_FREQUENCY, intermediate_frequency);
    props.add(DTV_MODULATION, uint32_t(params.modulation));
    props.add(DTV_SYMBOL_RATE, params.symbol_rate);
    props.add(DTV_INNER_FEC, uint32_t(params.inner_fec));
    props.add(DTV_INVERSION, uint32_t(params.inversion));
    props.add(DTV_ROLLOFF, uint32_t(params.roll_off));
    props.add(DTV_PILOT, uint32_t(params.pilots));
    props.add(DTV_TUNE);

    return tune(props, report);
}


//-----------------------------------------------------------------------------
// Tune for DVB-C tuners, return true on success, false on error
//-----------------------------------------------------------------------------

bool ts::Tuner::tuneDVBC (const TunerParametersDVBC& params, Report& report)
{
    if (!CheckModEnum (params.inversion, "spectral inversion", SpectralInversionEnum, report) ||
        !CheckModEnum (params.inner_fec, "FEC", InnerFECEnum, report) ||
        !CheckModEnum (params.modulation, "modulation", ModulationEnum, report)) {
        return false;
    }

    discardFrontendEvents (report);

    if (!dtvClear (report)) {
        return false;
    }
    DTVProperties props;
    props.add (DTV_FREQUENCY, uint32_t(params.frequency));
    props.add (DTV_MODULATION, uint32_t(params.modulation));
    props.add (DTV_INVERSION, uint32_t(params.inversion));
    props.add (DTV_INNER_FEC, uint32_t(params.inner_fec));
    props.add (DTV_SYMBOL_RATE, params.symbol_rate);
    props.add (DTV_TUNE);

    return tune(props, report);
}


//-----------------------------------------------------------------------------
// Tune for DVB-T tuners, return true on success, false on error
//-----------------------------------------------------------------------------

bool ts::Tuner::tuneDVBT (const TunerParametersDVBT& params, Report& report)
{
    if (!CheckModEnum (params.inversion, "spectral inversion", SpectralInversionEnum, report) ||
        !CheckModEnum (params.bandwidth, "bandwidth", BandWidthEnum, report) ||
        !CheckModEnum (params.fec_hp, "FEC", InnerFECEnum, report) ||
        !CheckModEnum (params.fec_lp, "FEC", InnerFECEnum, report) ||
        !CheckModEnum (params.modulation, "constellation", ModulationEnum, report) ||
        !CheckModEnum (params.transmission_mode, "transmission mode", TransmissionModeEnum, report) ||
        !CheckModEnum (params.guard_interval, "guard interval", GuardIntervalEnum, report) ||
        !CheckModEnum (params.hierarchy, "hierarchy", HierarchyEnum, report)) {
        return false;
    }

    discardFrontendEvents (report);

    if (!dtvClear (report)) {
        return false;
    }
    DTVProperties props;
    const uint32_t bwhz = BandWidthValueHz(params.bandwidth);
    props.add(DTV_FREQUENCY, uint32_t (params.frequency));
    props.add(DTV_MODULATION, uint32_t (params.modulation));
    props.add(DTV_INVERSION, uint32_t (params.inversion));
    if (bwhz > 0) {
        props.add(DTV_BANDWIDTH_HZ, bwhz);
    }
    props.add(DTV_CODE_RATE_HP, uint32_t (params.fec_hp));
    props.add(DTV_CODE_RATE_LP, uint32_t (params.fec_lp));
    props.add(DTV_TRANSMISSION_MODE, uint32_t (params.transmission_mode));
    props.add(DTV_GUARD_INTERVAL, uint32_t (params.guard_interval));
    props.add(DTV_HIERARCHY, uint32_t (params.hierarchy));
    props.add(DTV_TUNE);

    return tune(props, report);
}


//-----------------------------------------------------------------------------
// Tune for ATSC tuners, return true on success, false on error
//-----------------------------------------------------------------------------

bool ts::Tuner::tuneATSC (const TunerParametersATSC& params, Report& report)
{
    if (!CheckModEnum (params.inversion, "spectral inversion", SpectralInversionEnum, report) ||
        !CheckModEnum (params.modulation, "modulation", ModulationEnum, report)) {
        return false;
    }

    discardFrontendEvents (report);

    if (!dtvClear (report)) {
        return false;
    }

    DTVProperties props;
    props.add (DTV_FREQUENCY, uint32_t (params.frequency));
    props.add (DTV_MODULATION, uint32_t (params.modulation));
    props.add (DTV_INVERSION, uint32_t (params.inversion));
    props.add (DTV_TUNE);

    return tune(props, report);
}


//-----------------------------------------------------------------------------
// Tune to the specified parameters and start receiving.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::tune (const TunerParameters& params, Report& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }

    // Check subclass of TunerParameters

    if (params.tunerType() != _tuner_type) {
        report.error ("inconsistent tuner parameter type");
        return false;
    }

    // Dispatch depending on tuner type

    switch (_tuner_type) {
        case DVB_S: {
            const TunerParametersDVBS* tpp = dynamic_cast<const TunerParametersDVBS*> (&params);
            assert (tpp != 0);
            return tuneDVBS (*tpp, report);
        }
        case DVB_C: {
            const TunerParametersDVBC* tpp = dynamic_cast<const TunerParametersDVBC*> (&params);
            assert (tpp != 0);
            return tuneDVBC (*tpp, report);
        }
        case DVB_T: {
            const TunerParametersDVBT* tpp = dynamic_cast<const TunerParametersDVBT*> (&params);
            assert (tpp != 0);
            return tuneDVBT (*tpp, report);
        }
        case ATSC: {
            const TunerParametersATSC* tpp = dynamic_cast<const TunerParametersATSC*> (&params);
            assert (tpp != 0);
            return tuneATSC (*tpp, report);
        }
        default: {
            report.error ("cannot convert " + TunerTypeEnum.name (_tuner_type) + " parameters to Linux DVB parameters");
            return false;
        }
    }
}


//-----------------------------------------------------------------------------
// Start receiving packets.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::start (Report& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }

    // Set demux buffer size (default value is 2 kB, fine for sections,
    // completely undersized for full TS capture.

    if (::ioctl (_demux_fd, DMX_SET_BUFFER_SIZE, _demux_bufsize) < 0) {
        report.error ("error setting buffer size on " + _demux_name + ": " + ErrorCodeMessage());
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
    TS_ZERO (filter);

    filter.pid = 0x2000;                // Means "all PID's"
    filter.input = DMX_IN_FRONTEND;     // Read from frontend device
    filter.output = DMX_OUT_TS_TAP;     // Redirect TS packets to DVR device
    filter.pes_type = DMX_PES_OTHER;    // Any type of PES
    filter.flags = DMX_IMMEDIATE_START; // Start capture immediately

    if (::ioctl (_demux_fd, DMX_SET_PES_FILTER, &filter) < 0) {
        report.error ("error setting filter on " + _demux_name + ": " + ErrorCodeMessage());
        return false;
    }

    // Wait for input signal locking if a non-zero timeout is specified.

    bool signal_ok = true;
    for (MilliSecond remain_ms = _signal_timeout; remain_ms > 0; remain_ms -= _signal_poll) {

        // Read the frontend status
        ::fe_status_t status;
        if (::ioctl (_frontend_fd, FE_READ_STATUS, &status) < 0) {
            report.error ("error reading status of " + _frontend_name + ": " + ErrorCodeMessage());
            return false;
        }

        // If the input signal is locked, cool...
        signal_ok = (status & FE_HAS_LOCK) != 0;
        if (signal_ok) {
            break;
        }

        // Wait the polling time
        SleepThread (_signal_poll < remain_ms ? _signal_poll : remain_ms);
    }

    // If the timeout has expired, error

    if (!signal_ok) {
        if (!_signal_timeout_silent) {
            report.error ("no input DVB signal after %" FMT_INT64 "d milliseconds", _signal_timeout);
        }
        return false;
    }

    return true;
}


//-----------------------------------------------------------------------------
// Stop receiving packets.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::stop (Report& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return false;
    }

    // Stop the demux
    if (::ioctl (_demux_fd, DMX_STOP) < 0) {
        report.error ("error stopping demux on " + _demux_name + ": " + ErrorCodeMessage());
        return false;
    }

    return true;
}


//-----------------------------------------------------------------------------
// Empty signal handler, simply interrupt system calls and report EINTR.
//-----------------------------------------------------------------------------

namespace {
    void empty_signal_handler (int)
    {
    }
}


//-----------------------------------------------------------------------------
// Timeout for receive operation (none by default).
// If zero, no timeout is applied.
// Return true on success, false on errors.
//-----------------------------------------------------------------------------

bool ts::Tuner::setReceiveTimeout (MilliSecond timeout, Report& report)
{
    if (timeout > 0) {
        // Set an actual receive timer.
        if (_rt_signal < 0) {
            // Allocate one real-time signal.
            if ((_rt_signal = SignalAllocator::Instance()->allocate()) < 0) {
                report.error ("cannot set tuner receive timer, no more signal available");
                return false;
            }

            // Handle the allocated signal
            struct ::sigaction sac;
            TS_ZERO (sac);
            ::sigemptyset (&sac.sa_mask);
            sac.sa_handler = empty_signal_handler;
            if (::sigaction (_rt_signal, &sac, 0) < 0) {
                report.error ("error setting tuner receive timer signal: " + ErrorCodeMessage());
                SignalAllocator::Instance()->release (_rt_signal);
                _rt_signal = -1;
                return false;
            }
        }

        // Create a timer which triggers the signal
        if (!_rt_timer_valid) {
            ::sigevent sev;
            TS_ZERO (sev);
            sev.sigev_notify = SIGEV_SIGNAL;
            sev.sigev_signo = _rt_signal;
            if (::timer_create (CLOCK_REALTIME, &sev, &_rt_timer) < 0) {
                report.error ("error creating tuner receive timer: " + ErrorCodeMessage());
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
            TS_ZERO (sac);
            ::sigemptyset (&sac.sa_mask);
            sac.sa_handler = SIG_IGN;
            if (::sigaction (_rt_signal, &sac, 0) < 0) {
                report.error ("error ignoring tuner receive timer signal: " + ErrorCodeMessage());
                ok = false;
            }
            // Release signal
            SignalAllocator::Instance()->release (_rt_signal);
            _rt_signal = -1;
        }

        // Disarm and delete timer
        if (_rt_timer_valid) {
            _rt_timer_valid = false;
            if (::timer_delete (_rt_timer) < 0) {
                report.error ("error deleting tuner receive timer: " + ErrorCodeMessage());
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

size_t ts::Tuner::receive (TSPacket* buffer, size_t max_packets, const AbortInterface* abort, Report& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return 0;
    }

    char* const data = reinterpret_cast<char*> (buffer);
    size_t req_size = max_packets * PKT_SIZE;
    size_t got_size = 0;
    int overflow_count = 0;

    // Set deadline if receive timeout in effect
    Time time_limit;
    if (_receive_timeout > 0) {
        assert (_rt_timer_valid);
        // Arm the receive timer.
        // Note that _receive_timeout is in milliseconds and ::itimerspec is in nanoseconds.
        ::itimerspec timeout;
        timeout.it_value.tv_sec = long (_receive_timeout / 1000);
        timeout.it_value.tv_nsec = (unsigned long) (1000000 * (_receive_timeout % 1000));
        timeout.it_interval.tv_sec = 0;
        timeout.it_interval.tv_nsec = 0;
        if (::timer_settime (_rt_timer, 0, &timeout, 0) < 0) {
            report.error ("error arming tuner receive timer: " + ErrorCodeMessage());
            return 0;
        }
        // Deadline time
        time_limit = Time::CurrentLocalTime() + _receive_timeout;
    }

    // Loop on read until we get enough
    while (got_size < req_size) {

        // Read some data
        bool got_overflow = false;
        ssize_t insize = ::read (_dvr_fd, data + got_size, req_size - got_size);

        if (insize > 0) {
            // Normal case: some data were read
            assert (got_size + insize <= req_size);
            got_size += insize;
        }
        else if (insize == 0) {
            // End of file. Truncate potential partial packet at eof.
            got_size -= got_size % PKT_SIZE;
        }
        else if (errno == EINTR) {
            // Input was interrupted by a signal.
            // If the application should be interrupted, stop now.
            if (abort != 0 && abort->aborting()) {
                break;
            }
        }
        else if (errno == EOVERFLOW) {
            got_overflow = true;
        }
        else {
            report.error ("receive error on " + _dvr_name + ": " + ErrorCodeMessage());
            break;
        }

        // Input overflow management: If an input overflow occurs more than
        // MAX_OVERFLOW consecutive times, an error is generated.
        if (!got_overflow) {
            // Reset overflow count
            overflow_count = 0;
        }
        else if (++overflow_count > MAX_OVERFLOW) {
            report.error ("input overflow, possible packet loss");
            break;
        }

        // If the receive timeout is exceeded, stop now.
        // FIXME: There is a race condition here. If the receiver timer is
        // triggered between this test and the start of the next read, the
        // next read will not be interrupted and the receive timer will not
        // apply to this read.
        if (_receive_timeout > 0 && Time::CurrentLocalTime() >= time_limit) {
            if (got_size == 0) {
                report.error ("receive timeout on " + _device_name);
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
        if (::timer_settime (_rt_timer, 0, &timeout, 0) < 0) {
            report.error ("error disarming tuner receive timer: " + ErrorCodeMessage());
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
            const size_t needed_packet_count = std::min<size_t> (10, (got_size - offset) / PKT_SIZE);
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
            report.error ("tuner packet synchronization lost, dropping " + Decimal (resync_offset - offset) + " bytes");

            // Pack rest of buffer
            ::memmove (data + offset, data + resync_offset, got_size - resync_offset);
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
    void DisplayFlags (std::ostream& strm,
                       const std::string& margin,
                       const std::string& name,
                       uint32_t value,
                       const ts::Enumeration& table)
    {
        const size_t max_width = 78;
        bool first = true;
        strm << margin << name << ": ";
        size_t width = margin.size() + name.size() + 2;

        for (uint32_t flag = 1; flag != 0; flag = flag << 1) {
            if ((value & flag) != 0) {
                const std::string flag_name (table.name (flag));
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
    void Display (std::ostream& strm,
                  const std::string& margin,
                  const std::string& name,
                  const std::string& value,
                  const std::string& unit)
    {
        strm << margin
             << ts::JustifyLeft  (name + " ", 25, '.')
             << ts::JustifyRight (" " + value, 25, '.')
             << " " << unit << std::endl;
    }
}


//-----------------------------------------------------------------------------
//  This routine formats the percentage of an unsigned int
//-----------------------------------------------------------------------------

namespace {
    template <typename INT>
    std::string Percent (INT value)
    {
        return "(" + ts::Decimal((uint64_t(value) * 100) / uint64_t(std::numeric_limits<INT>::max())) + "%)";
    }
}


//-----------------------------------------------------------------------------
// Display the characteristics and status of the tuner.
//-----------------------------------------------------------------------------

std::ostream& ts::Tuner::displayStatus (std::ostream& strm, const std::string& margin, Report& report)
{
    if (!_is_open) {
        report.error ("DVB tuner not open");
        return strm;
    }

    // Strings for enum fe_status
    const Enumeration enum_fe_status
        ("has signal",  ::FE_HAS_SIGNAL,
         "has carrier", ::FE_HAS_CARRIER,
         "has viterbi", ::FE_HAS_VITERBI,
         "has sync",    ::FE_HAS_SYNC,
         "has lock",    ::FE_HAS_LOCK,
         "timedout",    ::FE_TIMEDOUT,
         "reinit",      ::FE_REINIT,
         TS_NULL);

    // Strings for enum fe_caps
    const Enumeration enum_fe_caps
        ("inversion auto",         ::FE_CAN_INVERSION_AUTO,
         "FEC 1/2",                ::FE_CAN_FEC_1_2,
         "FEC 2/3",                ::FE_CAN_FEC_2_3,
         "FEC 3/4",                ::FE_CAN_FEC_3_4,
         "FEC 4/5",                ::FE_CAN_FEC_4_5,
         "FEC 5/6",                ::FE_CAN_FEC_5_6,
         "FEC 6/7",                ::FE_CAN_FEC_6_7,
         "FEC 7/8",                ::FE_CAN_FEC_7_8,
         "FEC 8/9",                ::FE_CAN_FEC_8_9,
         "FEC auto",               ::FE_CAN_FEC_AUTO,
         "QPSK",                   ::FE_CAN_QPSK,
         "16-QAM",                 ::FE_CAN_QAM_16,
         "32-QAM",                 ::FE_CAN_QAM_32,
         "64-QAM",                 ::FE_CAN_QAM_64,
         "128-QAM",                ::FE_CAN_QAM_128,
         "256-QAM",                ::FE_CAN_QAM_256,
         "QAM auto",               ::FE_CAN_QAM_AUTO,
         "transmission mode auto", ::FE_CAN_TRANSMISSION_MODE_AUTO,
         "bandwidth auto",         ::FE_CAN_BANDWIDTH_AUTO,
         "guard interval auto",    ::FE_CAN_GUARD_INTERVAL_AUTO,
         "hierarchy auto",         ::FE_CAN_HIERARCHY_AUTO,
         "8-VSB",                  ::FE_CAN_8VSB,
         "16-VSB",                 ::FE_CAN_16VSB,
         "needs bending",          ::FE_NEEDS_BENDING,
         "recover",                ::FE_CAN_RECOVER,
         "mute TS",                ::FE_CAN_MUTE_TS,
#if TS_DVB_API_VERSION >= 502
         "turbo FEC",              ::FE_CAN_TURBO_FEC,
#endif
#if TS_DVB_API_VERSION >= 501
         "2nd generation",         ::FE_CAN_2G_MODULATION,
#endif
         TS_NULL);

    // Read current status
    ::fe_status_t status;
    if (::ioctl (_frontend_fd, FE_READ_STATUS, &status) < 0) {
        report.error ("ioctl FE_READ_STATUS on " + _frontend_name + ": " + ErrorCodeMessage());
        status = ::fe_status_t (0);
    }

    // Read current tuning parameters
    TunerParameters* params = TunerParameters::Factory (_tuner_type);
    if (params != 0 && !getCurrentTuning (*params, false, report)) {
        params = 0;
    }
    const TunerParametersDVBS* params_dvbs = dynamic_cast <const TunerParametersDVBS*> (params);
    const TunerParametersDVBC* params_dvbc = dynamic_cast <const TunerParametersDVBC*> (params);
    const TunerParametersDVBT* params_dvbt = dynamic_cast <const TunerParametersDVBT*> (params);
    const TunerParametersATSC* params_atsc = dynamic_cast <const TunerParametersATSC*> (params);

    // Read Bit Error Rate
    uint32_t ber;
    if (::ioctl (_frontend_fd, FE_READ_BER, &ber) < 0) {
        report.error ("ioctl FE_READ_BER on " + _frontend_name + ": " + ErrorCodeMessage());
        ber = 0;
    }

    // Read Signal/Noise Ratio
    uint16_t snr;
    if (::ioctl (_frontend_fd, FE_READ_SNR, &snr) < 0) {
        report.error ("ioctl FE_READ_SNR on " + _frontend_name + ": " + ErrorCodeMessage());
        snr = 0;
    }

    // Read signal strength
    uint16_t strength;
    if (::ioctl (_frontend_fd, FE_READ_SIGNAL_STRENGTH, &strength) < 0) {
        report.error ("ioctl FE_READ_SIGNAL_STRENGTH on " + _frontend_name + ": " + ErrorCodeMessage());
        strength = 0;
    }

    // Read uncorrected blocks
    uint32_t ublocks;
    if (::ioctl (_frontend_fd, FE_READ_UNCORRECTED_BLOCKS, &ublocks) < 0) {
        report.error ("ioctl FE_READ_UNCORRECTED_BLOCKS on " + _frontend_name + ": " + ErrorCodeMessage());
        ublocks = 0;
    }

    // Display current information
    DisplayFlags(strm, margin, "Status", uint32_t(status), enum_fe_status);
    strm << std::endl;
    Display(strm, margin, "Bit error rate",     Decimal(ber),      Percent(ber));
    Display(strm, margin, "Signal/noise ratio", Decimal(snr),      Percent(snr));
    Display(strm, margin, "Signal strength",    Decimal(strength), Percent(strength));
    Display(strm, margin, "Uncorrected blocks", Decimal(ublocks),  "");

    // Display frequency characteristics
    const uint64_t hz_factor = _fe_info.type == ::FE_QPSK ? 1000 : 1;
    strm << margin << "Frequencies:" << std::endl;
    if (params_dvbs != 0) {
        Display(strm, margin, "  Current", Decimal (params_dvbs->frequency), "Hz");
    }
    if (params_dvbc != 0) {
        Display (strm, margin, "  Current", Decimal (params_dvbc->frequency), "Hz");
    }
    if (params_dvbt != 0) {
        Display(strm, margin, "  Current", Decimal (params_dvbt->frequency), "Hz");
        if (UHF::InBand (params_dvbt->frequency)) {
            Display (strm, margin, "  UHF channel", Decimal (UHF::Channel (params_dvbt->frequency)), "");
        }
        else if (VHF::InBand (params_dvbt->frequency)) {
            Display (strm, margin, "  VHF channel", Decimal (VHF::Channel (params_dvbt->frequency)), "");
        }
    }
    if (params_atsc != 0) {
        Display (strm, margin, "  Current", Decimal (params_atsc->frequency), "Hz");
    }
    Display (strm, margin, "  Min", Decimal (hz_factor * _fe_info.frequency_min), "Hz");
    Display (strm, margin, "  Max", Decimal (hz_factor * _fe_info.frequency_max), "Hz");
    Display (strm, margin, "  Step", Decimal (hz_factor * _fe_info.frequency_stepsize), "Hz");
    Display (strm, margin, "  Tolerance", Decimal (hz_factor * _fe_info.frequency_tolerance), "Hz");

    // Display symbol rate characteristics.

    if (params_dvbs != 0 || params_dvbc != 0) {
        strm << margin << "Symbol rates:" << std::endl;
        Display (strm, margin, "  Current",
                 Decimal (params_dvbs != 0 ? params_dvbs->symbol_rate : params_dvbc->symbol_rate),
                 "sym/s");
        Display (strm, margin, "  Min", Decimal (_fe_info.symbol_rate_min), "sym/s");
        Display (strm, margin, "  Max", Decimal (_fe_info.symbol_rate_max), "sym/s");
        Display (strm, margin, "  Tolerance", Decimal (_fe_info.symbol_rate_tolerance), "sym/s");
    }

    // Frontend-specific information
    if (params_dvbs != 0) {
        Display (strm, margin, "Spectral inversion", SpectralInversionEnum.name (params_dvbs->inversion), "");
        Display (strm, margin, "FEC (inner)", InnerFECEnum.name (params_dvbs->inner_fec) , "");
    }
    if (params_dvbc != 0) {
        Display (strm, margin, "Spectral inversion", SpectralInversionEnum.name (params_dvbc->inversion), "");
        Display (strm, margin, "FEC (inner)", InnerFECEnum.name (params_dvbc->inner_fec) , "");
        Display (strm, margin, "Modulation", ModulationEnum.name (params_dvbc->modulation) , "");
    }
    if (params_dvbt != 0) {
        Display (strm, margin, "Spectral inversion", SpectralInversionEnum.name (params_dvbt->inversion), "");
        Display (strm, margin, "Bandwidth", BandWidthEnum.name (params_dvbt->bandwidth) , "");
        Display (strm, margin, "FEC (high priority)", InnerFECEnum.name (params_dvbt->fec_hp) , "");
        Display (strm, margin, "FEC (low priority)", InnerFECEnum.name (params_dvbt->fec_lp) , "");
        Display (strm, margin, "Constellation", ModulationEnum.name (params_dvbt->modulation) , "");
        Display (strm, margin, "Transmission mode", TransmissionModeEnum.name (params_dvbt->transmission_mode) , "");
        Display (strm, margin, "Guard interval", GuardIntervalEnum.name (params_dvbt->guard_interval) , "");
        Display (strm, margin, "Hierarchy", HierarchyEnum.name (params_dvbt->hierarchy) , "");
    }
    if (params_atsc != 0) {
        Display (strm, margin, "Spectral inversion", SpectralInversionEnum.name (params_atsc->inversion), "");
        Display (strm, margin, "Modulation", ModulationEnum.name (params_atsc->modulation) , "");
    }

    // Display general capabilities
    strm << std::endl;
    DisplayFlags (strm, margin, "Capabilities", uint32_t (_fe_info.caps), enum_fe_caps);

    return strm;
}


//-----------------------------------------------------------------------------
// Convert between TSDuck and Linux delivery systems.
//-----------------------------------------------------------------------------

ts::DeliverySystem ts::Tuner::fromLinuxDeliverySystem(::fe_delivery_system ds)
{
    switch (ds) {
        case ::SYS_DVBC_ANNEX_AC: return DS_DVB_C_ANNEX_AC;
        case ::SYS_DVBC_ANNEX_B:  return DS_DVB_C_ANNEX_B;
        case ::SYS_DVBT:   return DS_DVB_T;
        case ::SYS_DSS:    return DS_DSS;
        case ::SYS_DVBS:   return DS_DVB_S;
        case ::SYS_DVBS2:  return DS_DVB_S2;
        case ::SYS_DVBH:   return DS_DVB_H;
        case ::SYS_ISDBT:  return DS_ISDB_T;
        case ::SYS_ISDBS:  return DS_ISDB_S;
        case ::SYS_ISDBC:  return DS_ISDB_C;
        case ::SYS_ATSC:   return DS_ATSC;
        case ::SYS_ATSCMH: return DS_ATSC_MH;
        case ::SYS_DMBTH:  return DS_DMB_TH;
        case ::SYS_CMMB:   return DS_CMMB;
        case ::SYS_DAB:    return DS_DAB;
        default:           return DS_UNDEFINED;
    }
}

::fe_delivery_system ts::Tuner::toLinuxDeliverySystem (DeliverySystem ds)
{
    switch (ds) {
        case DS_DVB_S:   return ::SYS_DVBS;
        case DS_DVB_S2:  return ::SYS_DVBS2;
        case DS_DVB_T:   return ::SYS_DVBT;
        case DS_DVB_T2:  return ::SYS_DVBT;  // or ::SYS_UNDEFINED, which one is best ?
        case DS_DVB_C:   return ::SYS_DVBC_ANNEX_AC;
        case DS_DVB_C_ANNEX_AC: return ::SYS_DVBC_ANNEX_AC;
        case DS_DVB_C_ANNEX_B:  return ::SYS_DVBC_ANNEX_B;
        case DS_DVB_C2:  return ::SYS_DVBC_ANNEX_AC;  // or ::SYS_UNDEFINED, which one is best ?
        case DS_DVB_H:   return ::SYS_DVBH;
        case DS_ISDB_S:  return ::SYS_ISDBS;
        case DS_ISDB_T:  return ::SYS_ISDBT;
        case DS_ISDB_C:  return ::SYS_ISDBC;
        case DS_ATSC:    return ::SYS_ATSC;
        case DS_ATSC_MH: return ::SYS_ATSCMH;
        case DS_DMB_TH:  return ::SYS_DMBTH;
        case DS_CMMB:    return ::SYS_CMMB;
        case DS_DAB:     return ::SYS_DAB;
        case DS_DSS:     return ::SYS_DSS;
        default:         return ::SYS_UNDEFINED;
    }
}
