//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Jerome Leveque
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
//  Monitor PID bitrate
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class BitrateMonitorPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        BitrateMonitorPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override { return true; }
        virtual BitRate getBitrate() override { return 0; }
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:

        // Default values
        static const BitRate DEFAULT_BITRATE_MIN = 10;
        static const BitRate DEFAULT_BITRATE_MAX = 0xFFFFFFFF;
        static const size_t  DEFAULT_TIME_WINDOW_SIZE = 5;

        // Type indicating status of current bitrate, regarding allowed range
        enum RangeStatus {LOWER, IN_RANGE, GREATER};

        PID         _pid;                  // Monitored PID
        BitRate     _min_bitrate;          // Minimum allowed bitrate
        BitRate     _max_bitrate;          // Maximum allowed bitrate
        RangeStatus _last_bitrate_status;  // Status of the last bitrate, regarding allowed range
        UString     _alarm_command;        // Alarm command name
        time_t      _last_second;          // Last second number
        size_t      _window_size;          // Size (in seconds) of the time window, used to compute bitrate.
        bool        _startup;              // Measurement in progress.
        size_t      _pkt_count_index;      // Index for packet number array.
        std::vector<PacketCounter> _pkt_count; // Array with the number of packets received during the last time window.
                                               // Numbers are stored second per second.

        // Run the alarm command.
        void runAlarmCommand(const ts::UString& parameter);

        // Compute bitrate. Report any alarm.
        void computeBitrate();

        // Inaccessible operations
        BitrateMonitorPlugin() = delete;
        BitrateMonitorPlugin(const BitrateMonitorPlugin&) = delete;
        BitrateMonitorPlugin& operator=(const BitrateMonitorPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::BitrateMonitorPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::BitrateMonitorPlugin::BitrateMonitorPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Monitor bitrate for a given pid.", u"[options] pid"),
    _pid(PID_NULL),
    _min_bitrate(0),
    _max_bitrate(0),
    _last_bitrate_status(LOWER),
    _alarm_command(),
    _last_second(0),
    _window_size(0),
    _startup(false),
    _pkt_count_index(0),
    _pkt_count()
{
    option(u""             ,  0,  PIDVAL, 1, 1);   // PID nb is a required parameter
    option(u"alarm_command", 'a', STRING);
    option(u"time_interval", 't', UINT16);
    option(u"min"          ,  0,  UINT32);
    option(u"max"          ,  0,  UINT32);

    setHelp(u"PID:\n"
            u"      Specifies the PID to monitor.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -a command\n"
            u"  --alarm_command command\n"
            u"      Command to be run when an alarm is detected\n"
            u"      (bitrate out of range).\n"
            u"\n"
            u"  --min value\n"
            u"      Set minimum allowed value for bitrate (bits/s).\n"
            u"      Default value = 10 bits/s.\n"
            u"\n"
            u"  --max value\n"
            u"      Set maximum allowed value for bitrate (bits/s).\n"
            u"      Default value = 2^32 bits/s.\n"
            u"\n"
            u"  -t value\n"
            u"  --time_interval value\n"
            u"      Time interval (in seconds) used to compute the bitrate.\n"
            u"      Default value = 5 seconds.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::BitrateMonitorPlugin::start()
{
    // Get command line arguments
    _alarm_command = value(u"alarm_command");
    _pid = intValue<PID>(u"", PID_NULL);
    _window_size = intValue(u"time_interval", DEFAULT_TIME_WINDOW_SIZE);
    _min_bitrate = intValue(u"min", DEFAULT_BITRATE_MIN);
    _max_bitrate = intValue(u"max", DEFAULT_BITRATE_MAX);

    if (_min_bitrate > _max_bitrate) {
        tsp->error(u"bad parameters, bitrate min (%'d) > max (%'d), exiting", {_min_bitrate, _max_bitrate});
        return false;
    }

    // Initialize array wick packets count.
    _pkt_count.resize(_window_size);
    _pkt_count_index = 0;

    for (uint16_t i = 0; i < _pkt_count.size(); i++) {
        _pkt_count[i] = 0;
    }

    _last_bitrate_status = IN_RANGE;
    _last_second = time(NULL);
    _startup = true;

    return true;
}


//----------------------------------------------------------------------------
// Run the alarm command, if one was specified as the plugin option.
// The given string parameter describes the alarm.
//----------------------------------------------------------------------------

void ts::BitrateMonitorPlugin::runAlarmCommand(const ts::UString& parameter)
{
    // Do nothing if alarm command was not specified
    if (_alarm_command != "") {
        const UString completeCommand(_alarm_command + u" \"" + parameter + u'"');
        // Flawfinder: ignore: system causes a new program to execute and is difficult to use safely.
        if (::system(completeCommand.toUTF8().c_str()) != 0) {
            tsp->error(u"unable to run alarm command %s", {completeCommand});
        }
    }
}


//----------------------------------------------------------------------------
// Compute bitrate for the monitored PID. Report an alarm if the bitrate
// is out of allowed range, or back in it.
//----------------------------------------------------------------------------

void ts::BitrateMonitorPlugin::computeBitrate()
{
    // Bitrate is computed with the following formula :
    // (Sum of packets received during the last time window) * (packet size) /
    // (time window)

    PacketCounter total_pkt_count = 0;
    for (uint16_t i = 0; i < _pkt_count.size(); i++) {
        total_pkt_count  += _pkt_count[i];
    }

    const BitRate bitrate = BitRate(total_pkt_count * PKT_SIZE * 8 / _pkt_count.size());

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
        ts::UString alarmMessage(UString::Format(u"pid %d (0x%X) - bitrate (%'d bits/s)", {_pid, _pid, bitrate}));
        switch (new_bitrate_status) {
            case LOWER:
                alarmMessage += UString::Format(u" is lower than allowed minimum (%'d bits/s)", {_min_bitrate});
                break;
            case IN_RANGE:
                alarmMessage += UString::Format(u" is back in allowed range (%'d-%'d bits/s)", {_min_bitrate, _max_bitrate});
                break;
            case GREATER:
                alarmMessage = UString::Format(u" is greater than allowed maximum (%'d bits/s)", {_max_bitrate});
                break;
            default:
                assert(false); // should not get there
        }

        tsp->warning(alarmMessage);

        // Call alarm script if defined, and pass the alarm message as parameter.
        runAlarmCommand(alarmMessage);

        // Update status
        _last_bitrate_status = new_bitrate_status;
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::BitrateMonitorPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    time_t now = time(NULL);

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
            _startup = !(_pkt_count_index == 0);
        }

        _last_second = now;
    }

    // If packet's PID matches, increment the number of packets received during the current second.
    if (pkt.getPID() == _pid) {
        _pkt_count[_pkt_count_index]++;
    }

    // Pass all packets
    return TSP_OK;
}
