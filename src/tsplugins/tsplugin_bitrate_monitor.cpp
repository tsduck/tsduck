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
#include "tsFormat.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class BitrateMonitorPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        BitrateMonitorPlugin (TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:

        // Default values
        static const uint32_t DEFAULT_BITRATE_MIN = 10;
        static const uint32_t DEFAULT_BITRATE_MAX = 0xFFFFFFFF;
        static const uint16_t DEFAULT_TIME_WINDOW_SIZE = 5;

        // Type indicating status of current bitrate, regarding allowed range
        enum RangeStatus {LOWER, IN_RANGE, GREATER};

        PID         _pid;                  // Monitored PID
        uint32_t    _min_bitrate;          // Minimum allowed bitrate
        uint32_t    _max_bitrate;          // Maximum allowed bitrate
        RangeStatus _last_bitrate_status;  // Status of the last bitrate, regarding allowed range
        std::string _alarm_command;        // Alarm command name
        time_t      _last_second;          // Last second number
        uint16_t    _window_size;          // Size (in seconds) of the time window
                                           // used to compute bitrate.
        uint16_t*   _pkt_count;            // Array with the number of packets received during the last time window.
                                           // Numbers are stored second per second.
        uint16_t    _pkt_count_index;      // Index for packet number array.
        bool        _startup;

        // To uncomment if using method 2 for bitrate computation
        //uint16_t  _pkt_count_new;        // Number of packets received during the last time window.
        //time_t    _last_bitrate_date;    // Date of the last bitrate computation.

        // Run the alarm command.
        void runAlarmCommand(const std::string& parameter);

        // Compute bitrate. Report any alarm.
        void computeBitrate();

        // To uncomment if using method 2 for bitrate computation
        //void computeBitrate(time_t time_interval);

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

ts::BitrateMonitorPlugin::BitrateMonitorPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Monitor bitrate for a given pid.", u"[options] pid"),
    _pid(PID_NULL),
    _min_bitrate(0),
    _max_bitrate(0),
    _last_bitrate_status(LOWER),
    _alarm_command(),
    _last_second(0),
    _window_size(0),
    _pkt_count(0),
    _pkt_count_index(0),
    _startup(false)
{
    option(u""             ,   0, PIDVAL, 1, 1);   // PID nb is a required parameter
    option(u"alarm_command", 'a', STRING);
    option(u"time_interval", 't', UINT16);
    option(u"min"          ,   0, UINT32);
    option(u"max"          ,   0, UINT32);

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

    _pid = intValue<PID> ("", PID_NULL);

    _window_size = intValue<uint16_t> ("time_interval", (uint16_t) DEFAULT_TIME_WINDOW_SIZE);

    // To uncomment if using method 2 for bitrate computation
    //_pkt_count_new = 0;
    //_last_bitrate_date = time(NULL);

    // Initialize array wick packets count
    _pkt_count = new uint16_t[_window_size];
    _pkt_count_index = 0;

    for (uint16_t i = 0; i<_window_size; i++) {
        _pkt_count[i] = 0;
    }

    _min_bitrate = intValue<uint32_t> ("min", (uint32_t) DEFAULT_BITRATE_MIN);
    _max_bitrate = intValue<uint32_t> ("max", (uint32_t) DEFAULT_BITRATE_MAX);

    if (_min_bitrate > _max_bitrate) {
        tsp->error("bad parameters, bitrate min (%u) > max (%u), exiting",
            _min_bitrate, _max_bitrate);
        return false;
    }

    _last_bitrate_status = IN_RANGE;
    _last_second = time (NULL);
    _startup = true;

    return true;
}



//----------------------------------------------------------------------------
// Run the alarm command, if one was specified as the plugin option.
// The given string parameter describes the alarm.
//----------------------------------------------------------------------------

void ts::BitrateMonitorPlugin::runAlarmCommand(const std::string& parameter)
{
    // Do nothing if alarm command was not specified
    if (_alarm_command != "") {
        const std::string completeCommand(_alarm_command + " " + '"' + parameter + '"');
        // Flawfinder: ignore: system causes a new program to execute and is difficult to use safely.
        if (::system(completeCommand.c_str()) != 0) {
            tsp->error("unable to run alarm command %s", completeCommand.c_str());
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

    uint16_t total_pkt_count = 0;

    for (uint16_t i = 0; i<_window_size; i++) {
        total_pkt_count  += _pkt_count[i];
    }

    uint32_t bitrate = total_pkt_count * PKT_SIZE * 8 / _window_size;

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
        std::string alarmMessage;
        switch (new_bitrate_status) {
            case LOWER:
                alarmMessage = Format("pid %u (0x%04X) - bitrate (%u bits/s) is lower than allowed minimum (%u bits/s)",
                                      _pid, _pid, bitrate, _min_bitrate);
                break;
            case IN_RANGE:
                alarmMessage = Format("pid %u (0x%04X) - bitrate (%u bits/s) is back in allowed range (%u-%u bits/s)",
                                      _pid, _pid, bitrate, _min_bitrate, _max_bitrate);
                break;
            case GREATER:
                alarmMessage = Format("pid %u (0x%04X) - bitrate (%u bits/s) is greater than allowed maximum (%u bits/s)",
                                      _pid, _pid, bitrate, _max_bitrate);
                break;
            default:
                assert(false); // should not get there
        }

        tsp->warning (alarmMessage);

        // Call alarm script if defined, and pass the alarm message as parameter.
        runAlarmCommand (alarmMessage);

        // Update status
        _last_bitrate_status = new_bitrate_status;
    }
}



//----------------------------------------------------------------------------
// Method 2 - Compute bitrate for the monitored PID. Report an alarm if the
// bitrate is out of allowed range, or back in it.
//----------------------------------------------------------------------------

//void ts::BitrateMonitorPlugin::computeBitrate(time_t time_interval)
//{
//  // Bitrate is computed with the following formula :
//  // (Sum of packets received during the last time interval) * (packet size) /
//  // (time interval)
//
//  uint32_t bitrate = (_pkt_count_new * 188 * 8) / (uint32_t) (time_interval);
//
//  // Check the bitrate value, regarding the allowed range.
//  RangeStatus new_bitrate_status;
//  if (bitrate < _min_bitrate) new_bitrate_status = LOWER;
//  else if (bitrate > _max_bitrate) new_bitrate_status = GREATER;
//  else new_bitrate_status = IN_RANGE;
//
//  // Report an error, if the bitrate status has changed.
//  if (new_bitrate_status != _last_bitrate_status) {
//
//      std::string alarmMessage;
//
//      switch (new_bitrate_status) {
//          case LOWER:
//              alarmMessage = formatString("pid %u (0x%04X) - bitrate (%u bits/s) is lower than allowed minimum (%u bits/s)",
//                  _pid, _pid, bitrate, _min_bitrate);
//              break;
//          case IN_RANGE:
//              alarmMessage = formatString("pid %u (0x%04X) - bitrate (%u bits/s) is back in allowed range (%u-%u bits/s)",
//                  _pid, _pid, bitrate, _min_bitrate, _max_bitrate);
//              break;
//          case GREATER:
//              alarmMessage = formatString("pid %u (0x%04X) - bitrate (%u bits/s) is greater than allowed maximum (%u bits/s)",
//                  _pid, _pid, bitrate, _max_bitrate);
//              break;
//      }
//
//      tsp->warning(alarmMessage.c_str());
//
//      // Call alarm script if defined, and pass the alarm message as parameter.
//      runAlarmCommand(alarmMessage.c_str());
//
//      // Update status
//      _last_bitrate_status = new_bitrate_status;
//  }
//
//
//}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::BitrateMonitorPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
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
        _pkt_count_index = (_pkt_count_index + 1) % _window_size;
        _pkt_count[_pkt_count_index] = 0;

        // We are no more at startup if the index cycles.
        if (_startup) {
            _startup = !(_pkt_count_index == 0);
        }

        _last_second = now;
    }

    // If packet's PID matches, increment the number of packets
    // received during the current second.
    if (pkt.getPID() == _pid) {
        _pkt_count[_pkt_count_index]++;
    }

    // NOTE : the following lines implement an other way for bitrate
    // computation. This alternative can be used if packets are not
    // received smoothly, but in bulks, with more than 1 second between
    // two bulks.

    //// Compute bitrate after time interval is elapsed
    //if (now > _last_bitrate_date + _window_size) {

    //  // Do not compute bitrate at startup (incomplete values)
    //  if (_startup) _startup = false;
    //  else {
    //      computeBitrate(now - _last_bitrate_date);
    //  }

    //  // Reset counters for next computation
    //  _pkt_count_new = 0;
    //  _last_bitrate_date = now;
    //}

    //// If the received packet deals with the monitored pid,
    //// increment the packent counter.
    //if (pkt.getPID() == _pid) _pkt_count_new++;

    // Pass all packets
    return TSP_OK;
}
