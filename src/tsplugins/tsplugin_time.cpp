//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Schedule packets pass or drop, based on time.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsBinaryTable.h"
#include "tsEnumeration.h"
#include "tsTime.h"
#include "tsTDT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TimePlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TimePlugin);
    public:
        // Implementation of plugin API
        TimePlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Time event description
        struct TimeEvent
        {
            // Public fields
            Status status;   // Packet status to return ...
            Time   time;     // ... after this UTC time

            // Constructor
            TimeEvent(const Status& s, const Time& t) : status (s), time (t) {}

            // Comparison, for sort algorithm
            bool operator<(const TimeEvent& t) const {return time < t.time;}
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
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Add time events in the list fro one option. Return false if a time string is invalid
        bool addEvents(const UChar* option, Status status);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"time", ts::TimePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TimePlugin::TimePlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Schedule packets pass or drop, based on time", u"[options]"),
    _status(TSP_DROP),
    _relative(false),
    _use_utc(false),
    _use_tdt(false),
    _last_time(Time::Epoch),
    _status_names({{u"pass", TSP_OK}, {u"stop", TSP_END}, {u"drop", TSP_DROP}, {u"null", TSP_NULL}}),
    _demux(duck, this),
    _events(),
    _next_index(0)
{
    option(u"drop", 'd', STRING, 0, UNLIMITED_COUNT);
    help(u"drop",
         u"All packets are dropped after the specified time. "
         u"Several --drop options may be specified.\n\n"
         u"Specifying time values:\n\n"
         u"A time value must be in the format \"year/month/day:hour:minute:second\" "
         u"(unless --relative is specified, in which case it is a number of seconds). "
         u"An empty value (\"\") means \"from the beginning\", that is to say when "
         u"tsp starts. By default, packets are passed when tsp starts.");

    option(u"null", 'n', STRING, 0, UNLIMITED_COUNT);
    help(u"null",
         u"All packets are replaced by null packets after the specified time. "
         u"Several --null options may be specified.");

    option(u"pass", 'p', STRING, 0, UNLIMITED_COUNT);
    help(u"pass",
         u"All packets are passed unmodified after the specified time. "
         u"Several --pass options may be specified.");

    option(u"relative", 'r');
    help(u"relative",
         u"All time values are interpreted as a number of seconds relative to the "
         u"tsp start time. By default, all time values are interpreted as an "
         u"absolute time in the format \"year/month/day:hour:minute:second\". "
         u"Option --relative is incompatible with --tdt or --utc.");

    option(u"stop", 's', STRING);
    help(u"stop", u"Packet transmission stops after the specified time and tsp terminates.");

    option(u"tdt", 't');
    help(u"tdt",
         u"Use the Time & Date Table (TDT) from the transport stream as time "
         u"reference instead of the system clock. Since the TDT contains UTC "
         u"time, all time values in the command line must be UTC also.");

    option(u"utc", 'u');
    help(u"utc",
         u"Specifies that all time values in the command line are in UTC. "
         u"By default, the time values are interpreted as system local time.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::TimePlugin::start()
{
    // Get command line options
    _status = TSP_OK;
    _relative = present(u"relative");
    _use_tdt = present(u"tdt");
    _use_utc = present(u"utc");

    if (_relative + _use_tdt + _use_utc > 1) {
        tsp->error(u"options --relative, --tdt and --utc are mutually exclusive");
        return false;
    }

    // Get list of time events
    _events.clear();
    if (!addEvents(u"drop", TSP_DROP) ||
        !addEvents(u"null", TSP_NULL) ||
        !addEvents(u"pass", TSP_OK) ||
        !addEvents(u"stop", TSP_END))
    {
        return false;
    }

    // Sort events by time
    std::sort(_events.begin(), _events.end());

    if (tsp->verbose()) {
        tsp->verbose(u"initial packet processing: %s", {_status_names.name(_status)});
        for (auto& it : _events) {
            tsp->verbose(u"packet %s after %s", {_status_names.name(it.status), it.time.format(Time::DATETIME)});
        }
    }

    // Reinitialize the demux
    _demux.reset();
    if (_use_tdt) {
        _demux.addPID(PID_TDT);
    }

    _last_time = Time::Epoch;
    _next_index = 0;

    return true;
}


//----------------------------------------------------------------------------
// Add time events in the list fro one option.
// Return false if a time string is invalid
//----------------------------------------------------------------------------

bool ts::TimePlugin::addEvents(const UChar* opt, Status status)
{
    const Time start_time(Time::CurrentLocalTime());

    for (size_t index = 0; index < count(opt); ++index) {
        const UString timeString(value(opt, u"", index));
        if (timeString.empty()) {
            // If the time string is empty, this is the initial action
            _status = status;
        }
        else if (_relative) {
            // Decode relative time string (a number of seconds)
            MilliSecond second = 0;
            if (!timeString.toInteger(second)) {
                tsp->error(u"invalid relative number of seconds: %s", {timeString});
                return false;
            }
            _events.push_back(TimeEvent(status, start_time + second * MilliSecPerSec));
        }
        else {
            // Decode an absolute time string
            Time absTime;
            if (!absTime.decode(timeString)) {
                tsp->error(u"invalid time value \"%s\" (use \"year/month/day:hour:minute:second\")", {timeString});
                return false;
            }
            _events.push_back(TimeEvent(status, absTime));
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::TimePlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    if (table.tableId() == TID_TDT) {
        if (table.sourcePID() == PID_TDT) {
            // Use TDT as clock reference
            TDT tdt(duck, table);
            if (tdt.isValid()) {
                _last_time = tdt.utc_time;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TimePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Filter sections
    _demux.feedPacket(pkt);

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
            tsp->verbose(u"%s: new packet processing: %s", {_last_time.format(Time::DATETIME), _status_names.name(_status)});
        }
    }

    return _status;
}
