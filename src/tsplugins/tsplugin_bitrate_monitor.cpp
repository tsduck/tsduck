//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Jerome Leveque, Thierry Lelegard
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
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Monitor PID or TS bitrate
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsForkPipe.h"
#include "tsTime.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class BitrateMonitorPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(BitrateMonitorPlugin);
    public:
        // Implementation of plugin API
        BitrateMonitorPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;
        virtual bool handlePacketTimeout() override;

    private:

        // Default values
        static constexpr BitRate DEFAULT_BITRATE_MIN = 10;
        static constexpr BitRate DEFAULT_BITRATE_MAX = 0xFFFFFFFF;
        static constexpr size_t  DEFAULT_TIME_WINDOW_SIZE = 5;

        // Type indicating status of current bitrate, regarding allowed range.
        enum RangeStatus {LOWER, IN_RANGE, GREATER};

        bool        _full_ts;              // Monitor full TS.
        PID         _pid;                  // Monitored PID (when _full_ts is false).
        UString     _tag;                  // Message tag.
        BitRate     _min_bitrate;          // Minimum allowed bitrate.
        BitRate     _max_bitrate;          // Maximum allowed bitrate.
        Second      _periodic_bitrate;     // Report bitrate at regular intervals, even if in range.
        Second      _periodic_countdown;   // Countdown to report bitrate.
        RangeStatus _last_bitrate_status;  // Status of the last bitrate, regarding allowed range.
        UString     _alarm_command;        // Alarm command name.
        UString     _alarm_prefix;         // Prefix for alarm messages.
        time_t      _last_second;          // Last second number.
        size_t      _window_size;          // Size (in seconds) of the time window, used to compute bitrate.
        bool        _startup;              // Measurement in progress.
        size_t      _pkt_count_index;      // Index for packet number array.
        std::vector<PacketCounter> _pkt_count;        // Number of packets received during last time window, second per second.
        TSPacketMetadata::LabelSet _labels_below;     // Set these labels on all packets when bitrate is below normal.
        TSPacketMetadata::LabelSet _labels_normal;    // Set these labels on all packets when bitrate is normal.
        TSPacketMetadata::LabelSet _labels_above;     // Set these labels on all packets when bitrate is above normal.
        TSPacketMetadata::LabelSet _labels_go_below;  // Set these labels on one packet when bitrate goes below normal.
        TSPacketMetadata::LabelSet _labels_go_normal; // Set these labels on one packet when bitrate goes back to normal.
        TSPacketMetadata::LabelSet _labels_go_above;  // Set these labels on one packet when bitrate goes above normal.
        TSPacketMetadata::LabelSet _labels_next;      // Set these labels on next packet.

        // Compute bitrate. Report any alarm.
        void computeBitrate();

        // Check time and compute bitrate when necessary.
        void checkTime();
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"bitrate_monitor", ts::BitrateMonitorPlugin);

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr ts::BitRate ts::BitrateMonitorPlugin::DEFAULT_BITRATE_MIN;
constexpr ts::BitRate ts::BitrateMonitorPlugin::DEFAULT_BITRATE_MAX;
constexpr size_t ts::BitrateMonitorPlugin::DEFAULT_TIME_WINDOW_SIZE;
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::BitrateMonitorPlugin::BitrateMonitorPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Monitor bitrate for TS or a given PID", u"[options]"),
    _full_ts(false),
    _pid(PID_NULL),
    _tag(),
    _min_bitrate(0),
    _max_bitrate(0),
    _periodic_bitrate(0),
    _periodic_countdown(0),
    _last_bitrate_status(LOWER),
    _alarm_command(),
    _alarm_prefix(),
    _last_second(0),
    _window_size(0),
    _startup(false),
    _pkt_count_index(0),
    _pkt_count(),
    _labels_below(),
    _labels_normal(),
    _labels_above(),
    _labels_go_below(),
    _labels_go_normal(),
    _labels_go_above(),
    _labels_next()
{
    // The PID was previously passed as argument. We now use option --pid.
    // We still accept the argument for legacy, but not both.
    option(u"", 0, PIDVAL, 0, 1);
    option(u"pid", 0, PIDVAL);
    help(u"pid",
         u"Specifies the PID to monitor. "
         u"By default, when no --pid is specified, monitor the bitrate of the full TS.");

    option(u"alarm-command", 'a', STRING);
    help(u"alarm-command", u"'command'",
         u"Command to run when the bitrate goes either out of range or back to normal. "
         u"The command receives an additional string parameter containing an informational message.");

    option(u"time-interval", 't', UINT16);
    help(u"time-interval",
         u"Time interval (in seconds) used to compute the bitrate. "
         u"Default: " + UString::Decimal(DEFAULT_TIME_WINDOW_SIZE) + u" s.");

    option(u"min", 0, UINT32);
    help(u"min",
         u"Set minimum allowed value for bitrate (bits/s). "
         u"Default: " + UString::Decimal(DEFAULT_BITRATE_MIN) + u" b/s.");

    option(u"max", 0, UINT32);
    help(u"max",
         u"Set maximum allowed value for bitrate (bits/s). "
         u"Default: " + UString::Decimal(DEFAULT_BITRATE_MAX) + u" b/s.");

    option(u"periodic-bitrate", 'p', POSITIVE);
    help(u"periodic-bitrate",
         u"Always report bitrate at the specific interval in seconds, even if the "
         u"bitrate is in range.");

    option(u"set-label-below", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"set-label-below", u"label1[-label2]",
         u"Set the specified labels on all packets while the bitrate is below normal. "
         u"Several --set-label-below options may be specified.");

    option(u"set-label-go-below", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"set-label-go-below", u"label1[-label2]",
         u"Set the specified labels on one packet when the bitrate goes below normal. "
         u"Several --set-label-go-below options may be specified.");

    option(u"set-label-above", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"set-label-above", u"label1[-label2]",
         u"Set the specified labels on all packets while the bitrate is above normal. "
         u"Several --set-label-above options may be specified.");

    option(u"set-label-go-above", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"set-label-go-above", u"label1[-label2]",
         u"Set the specified labels on one packet when the bitrate goes above normal. "
         u"Several --set-label-go-above options may be specified.");

    option(u"set-label-normal", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"set-label-normal", u"label1[-label2]",
         u"Set the specified labels on all packets while the bitrate is normal (within range). "
         u"Several --set-label-normal options may be specified.");

    option(u"set-label-go-normal", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketMetadata::LABEL_MAX);
    help(u"set-label-go-normal", u"label1[-label2]",
         u"Set the specified labels on one packet when the bitrate goes back to normal (within range). "
         u"Several --set-label-go-normal options may be specified.");

    option(u"tag", 0, STRING);
    help(u"tag", u"'string'",
         u"Message tag to be displayed in alarms. "
         u"Useful when the plugin is used several times in the same process.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::BitrateMonitorPlugin::getOptions()
{
    bool ok = true;

    // Get the PID. Accept either --pid or legacy argument, but not both.
    const bool got_legacy_arg = present(u"");
    const bool got_pid_option = present(u"pid");
    _full_ts = !got_legacy_arg && !got_pid_option;

    if (got_legacy_arg && got_pid_option) {
        tsp->error(u"specify either --pid or legacy argument, but not both");
        ok = false;
    }
    else if (got_legacy_arg) {
        _pid = intValue<PID>(u"");
    }
    else if (got_pid_option) {
        _pid = intValue<PID>(u"pid");
    }

    // Get options
    _tag = value(u"tag");
    _alarm_command = value(u"alarm-command");
    _window_size = intValue(u"time-interval", DEFAULT_TIME_WINDOW_SIZE);
    _min_bitrate = intValue(u"min", DEFAULT_BITRATE_MIN);
    _max_bitrate = intValue(u"max", DEFAULT_BITRATE_MAX);
    _periodic_bitrate = intValue(u"periodic-bitrate", 0);
    getIntValues(_labels_below, u"set-label-below");
    getIntValues(_labels_normal, u"set-label-normal");
    getIntValues(_labels_above, u"set-label-above");
    getIntValues(_labels_go_below, u"set-label-go-below");
    getIntValues(_labels_go_normal, u"set-label-go-normal");
    getIntValues(_labels_go_above, u"set-label-go-above");

    if (_min_bitrate > _max_bitrate) {
        tsp->error(u"bad parameters, bitrate min (%'d) > max (%'d), exiting", {_min_bitrate, _max_bitrate});
        ok = false;
    }

    // Prefix for alarm messages.
    _alarm_prefix = _tag;
    if (!_alarm_prefix.empty()) {
        _alarm_prefix += u": ";
    }
    if (_full_ts) {
        _alarm_prefix += u"TS";
    }
    else {
        _alarm_prefix += UString::Format(u"PID 0x%X (%d)", {_pid, _pid});
    }

    return ok;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::BitrateMonitorPlugin::start()
{
    // Initialize array with packets count.
    _pkt_count.resize(_window_size);
    _pkt_count_index = 0;

    for (uint16_t i = 0; i < _pkt_count.size(); i++) {
        _pkt_count[i] = 0;
    }

    _labels_next.reset();
    _periodic_countdown = _periodic_bitrate;
    _last_bitrate_status = IN_RANGE;
    _last_second = ::time(nullptr);
    _startup = true;

    // We must never wait for packets more than one second.
    tsp->setPacketTimeout(MilliSecPerSec);

    return true;
}


//----------------------------------------------------------------------------
// Compute bitrate, report alarms.
//----------------------------------------------------------------------------

void ts::BitrateMonitorPlugin::computeBitrate()
{
    // Bitrate is computed with the following formula :
    // (Sum of packets received during the last time window) * (packet size) / (time window)

    PacketCounter total_pkt_count = 0;
    for (uint16_t i = 0; i < _pkt_count.size(); i++) {
        total_pkt_count  += _pkt_count[i];
    }

    const BitRate bitrate = BitRate(total_pkt_count * PKT_SIZE * 8 / _pkt_count.size());

    // Periodic bitrate display.
    if (_periodic_bitrate > 0 && --_periodic_countdown <= 0) {
        _periodic_countdown = _periodic_bitrate;
        tsp->info(u"%s, %s bitrate: %'d bits/s", {Time::CurrentLocalTime().format(Time::DATE | Time::TIME), _alarm_prefix, bitrate});
    }

    // Check the bitrate value, regarding the allowed range.
    RangeStatus new_bitrate_status;
    if (bitrate < _min_bitrate) {
        new_bitrate_status = LOWER;
    }
    else if (bitrate > _max_bitrate) {
        new_bitrate_status = GREATER;
    }
    else {
        new_bitrate_status = IN_RANGE;
    }

    // Report an error, if the bitrate status has changed.
    if (new_bitrate_status != _last_bitrate_status) {
        ts::UString alarmMessage(UString::Format(u"%s bitrate (%'d bits/s) ", {_alarm_prefix, bitrate}));
        switch (new_bitrate_status) {
            case LOWER:
                alarmMessage += UString::Format(u"is lower than allowed minimum (%'d bits/s)", {_min_bitrate});
                _labels_next |= _labels_go_below;
                break;
            case IN_RANGE:
                alarmMessage += UString::Format(u"is back in allowed range (%'d-%'d bits/s)", {_min_bitrate, _max_bitrate});
                _labels_next |= _labels_go_normal;
                break;
            case GREATER:
                alarmMessage += UString::Format(u"is greater than allowed maximum (%'d bits/s)", {_max_bitrate});
                _labels_next |= _labels_go_above;
                break;
            default:
                assert(false); // should not get there
        }

        // Report alarm message as a tsp warning.
        tsp->warning(alarmMessage);

        // Call alarm script if defined, and pass the alarm message as parameter.
        // The command is run asynchronously, do not wait for completion.
        if (!_alarm_command.empty()) {
            ForkPipe::Launch(_alarm_command + u" \"" + alarmMessage + u'"', *tsp, ForkPipe::STDERR_ONLY, ForkPipe::STDIN_NONE);
        }

        // Update status
        _last_bitrate_status = new_bitrate_status;
    }
}


//----------------------------------------------------------------------------
// Check time and compute bitrate when necessary.
//----------------------------------------------------------------------------

void ts::BitrateMonitorPlugin::checkTime()
{
    const time_t now = ::time(nullptr);

    // NOTE : the computation method used here is meaningful only if at least
    // one packet is received per second (whatever its PID).

    // New second : compute the bitrate for the last time window
    if (now > _last_second) {

        // Bitrate computation is done only when the packet counter
        // array if fully filled (to avoid bad values at startup).
        if (!_startup) {
            computeBitrate();
        }

        // update index, and reset packet count.
        _pkt_count_index = (_pkt_count_index + 1) % _pkt_count.size();
        _pkt_count[_pkt_count_index] = 0;

        // We are no more at startup if the index cycles.
        if (_startup) {
            _startup = _pkt_count_index != 0;
        }

        _last_second = now;
    }
}


//----------------------------------------------------------------------------
// Packet timeout processing method.
//----------------------------------------------------------------------------

bool ts::BitrateMonitorPlugin::handlePacketTimeout()
{
    // Check time and bitrates.
    checkTime();

    // Always continue waiting, never abort.
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method.
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::BitrateMonitorPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Check time and bitrates.
    checkTime();

    // If packet's PID matches, increment the number of packets received during the current second.
    if (_full_ts || pkt.getPID() == _pid) {
        _pkt_count[_pkt_count_index]++;
    }

    // Set labels according to trigger.
    pkt_data.setLabels(_labels_next);
    _labels_next.reset();

    // Set labels according to state.
    switch (_last_bitrate_status) {
        case LOWER:
            pkt_data.setLabels(_labels_below);
            break;
        case IN_RANGE:
            pkt_data.setLabels(_labels_normal);
            break;
        case GREATER:
            pkt_data.setLabels(_labels_above);
            break;
        default:
            assert(false); // should not get there
    }

    // Pass all packets
    return TSP_OK;
}
