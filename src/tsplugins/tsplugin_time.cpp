//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Schedule packets pass or drop, based on time.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsSectionDemux.h"
#include "tsEnumeration.h"
#include "tsToInteger.h"
#include "tsTime.h"
#include "tsTDT.h"



//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TimePlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        TimePlugin (TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        // Time event description
        struct TimeEvent
        {
            // Public fields
            Status status;   // Packet status to return ...
            Time   time;     // ... after this UTC time

            // Constructor
            TimeEvent (const Status& s, const Time& t) : status (s), time (t) {}

            // Comparison, for sort algorithm
            bool operator< (const TimeEvent& t) const {return time < t.time;}
        };
        typedef std::vector<TimeEvent> TimeEventVector;

        // TimePlugin private members
        Status            _status;       // Packet status to return
        bool              _relative;     // Use relative time from the beginning
        bool              _use_utc;      // Use UTC time
        bool              _use_tdt;      // Use TDT as time reference
        Time              _last_time;    // Last measured time
        const Enumeration _status_names; // Names of packet status
        SectionDemux      _demux;        // Section filter
        TimeEventVector   _events;       // Sorted list of time events to apply
        size_t            _next_index;   // Index of next TimeEvent to apply

        // Invoked by the demux when a complete table is available.
        virtual void handleTable (SectionDemux&, const BinaryTable&);

        // Add time events in the list fro one option. Return false if a time string is invalid
        bool addEvents (const char* option, Status status);
    };
}

TSPLUGIN_DECLARE_VERSION;
TSPLUGIN_DECLARE_PROCESSOR (ts::TimePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TimePlugin::TimePlugin (TSP* tsp_) :
    ProcessorPlugin (tsp_, "Schedule packets pass or drop, based on time.", "[options]"),
    _status_names ("pass", TSP_OK, "stop", TSP_END, "drop", TSP_DROP, "null", TSP_NULL, TS_NULL),
    _demux (this)
{
    option ("drop",     'd', STRING, 0, UNLIMITED_COUNT);
    option ("null",     'n', STRING, 0, UNLIMITED_COUNT);
    option ("pass",     'p', STRING, 0, UNLIMITED_COUNT);
    option ("relative", 'r');
    option ("stop",     's', STRING);
    option ("tdt",      't');
    option ("utc",      'u');

    setHelp ("Options:\n"
             "\n"
             "  -d time\n"
             "  --drop time\n"
             "      All packets are dropped after the specified time.\n"
             "      Several --drop options may be specified\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -n time\n"
             "  --null time\n"
             "      All packets are replaced by null packets after the specified time.\n"
             "      Several --null options may be specified\n"
             "\n"
             "  -p time\n"
             "  --pass time\n"
             "      All packets are passed unmodified after the specified time.\n"
             "      Several --pass options may be specified\n"
             "\n"
             "  -s time\n"
             "  --stop time\n"
             "      Packet transmission stops after the specified time and tsp terminates.\n"
             "\n"
             "  -r\n"
             "  --relative\n"
             "      All time values are interpreted as a number of seconds relative to the\n"
             "      tsp start time. By default, all time values are interpreted as an\n"
             "      absolute time in the format \"year/month/day:hour:minute:second\".\n"
             "      Option --relative is incompatible with --tdt or --utc.\n"
             "\n"
             "  -t\n"
             "  --tdt\n"
             "      Use the Time & Date Table (TDT) from the transport stream as time\n"
             "      reference instead of the system clock. Since the TDT contains UTC\n"
             "      time, all time values in the command line must be UTC also.\n"
             "\n"
             "  -u\n"
             "  --utc\n"
             "      Specifies that all time values in the command line are in UTC.\n"
             "      By default, the time values are interpreted as system local time.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n"
             "\n"
             "Specifying time values:\n"
             "\n"
             "  A time value must be in the format \"year/month/day:hour:minute:second\"\n"
             "  (unless --relative is specified, in which case it is a number of seconds).\n"
             "  An empty value (\"\") means \"from the beginning\", that is to say when\n"
             "  tsp starts. By default, packets are passed when tsp starts.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::TimePlugin::start()
{
    // Get command line options
    _status = TSP_OK;
    _relative = present ("relative");
    _use_tdt = present ("tdt");
    _use_utc = present ("utc");

    if (_relative + _use_tdt + _use_utc > 1) {
        tsp->error ("options --relative, --tdt and --utc are mutually exclusive");
        return false;
    }

    // Get list of time events
    _events.clear();
    if (!addEvents ("drop", TSP_DROP) ||
        !addEvents ("null", TSP_NULL) ||
        !addEvents ("pass", TSP_OK) ||
        !addEvents ("stop", TSP_END)) {
        return false;
    }

    // Sort events by time
    std::sort (_events.begin(), _events.end());

    if (tsp->verbose()) {
        tsp->log (Severity::Verbose, "initial packet processing: " + _status_names.name (_status));
        for (TimeEventVector::iterator it = _events.begin(); it != _events.end(); ++it) {
            tsp->log (Severity::Verbose, "packet " + _status_names.name (it->status) +
                      " after " + it->time.format (Time::DATE | Time::TIME));
        }
    }

    // Reinitialize the demux
    _demux.reset();
    if (_use_tdt) {
        _demux.addPID (PID_TDT);
    }

    _last_time = Time::Epoch;
    _next_index = 0;

    return true;
}


//----------------------------------------------------------------------------
// Add time events in the list fro one option.
// Return false if a time string is invalid
//----------------------------------------------------------------------------

bool ts::TimePlugin::addEvents (const char* option, Status status)
{
    const Time start_time (Time::CurrentLocalTime());

    for (size_t index = 0; index < count (option); ++index) {
        const std::string time (value (option, "", index));
        try {
            if (time.empty()) {
                // If the time string is empty, this is the initial action
                _status = status;
            }
            else if (_relative) {
                // Decode relative time string (a number of seconds)
                MilliSecond second;
                if (!ToInteger (second, time)) {
                    tsp->error ("invalid relative number of seconds: %s", time.c_str());
                    return false;
                }
                _events.push_back (TimeEvent (status, start_time + second * MilliSecPerSec));
            }
            else {
                // Decode an absolute time string
                int year, month, day, hour, minute, second;
                char unused;
                if (::sscanf (time.c_str(), "%d/%d/%d:%d:%d:%d%c", &year, &month, &day, &hour, &minute, &second, &unused) != 6) {
                    tsp->error ("invalid time value \"%s\" (use \"year/month/day:hour:minute:second\")", time.c_str());
                    return false;
                }
                _events.push_back (TimeEvent (status, Time (year, month, day, hour, minute, second)));
            }
        }
        catch (Time::TimeError) {
            tsp->error ("at least one invalid value in \"%s\"", time.c_str());
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::TimePlugin::handleTable (SectionDemux& demux, const BinaryTable& table)
{
    if (table.tableId() == TID_TDT) {
        if (table.sourcePID() == PID_TDT) {
            // Use TDT as clock reference
            TDT tdt (table);
            if (tdt.isValid()) {
                _last_time = tdt.utc_time;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TimePlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Filter sections
    _demux.feedPacket (pkt);

    // Get current system time (unless TDT is used as reference)
    if (!_use_tdt) {
        _last_time = _use_utc ? Time::CurrentUTC() : Time::CurrentLocalTime();
    }

    // Is it time to change the action?

    while (_next_index < _events.size() && _events[_next_index].time <= _last_time) {
        // Yes, we just passed a schedule
        _status = _events[_next_index].status;
        _next_index++;

        if (tsp->verbose()) {
            tsp->log (Severity::Verbose, _last_time.format (Time::DATE | Time::TIME) +
                      ": new packet processing: " + _status_names.name (_status));
        }
    }

    return _status;
}
