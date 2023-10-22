//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Schedule packets pass or drop, based on packet numbers.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsPCRAnalyzer.h"
#include "tsEnumeration.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SlicePlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(SlicePlugin);
    public:
        // Implementation of plugin API
        SlicePlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Event description
        struct SliceEvent
        {
            // Public fields
            Status   status;   // Packet status to return ...
            uint64_t value;    // ... after this packet or milli-second

            // Constructor
            SliceEvent(const Status& s, const uint64_t& v) : status(s), value(v) {}

            // Comparison, for sort algorithm
            bool operator< (const SliceEvent& e) const {return value < e.value;}
        };
        typedef std::vector<SliceEvent> SliceEventVector;

        // SlicePlugin private members
        bool              _use_time = false;     // Use milliseconds in SliceEvent::value
        bool              _ignore_pcr = false;   // Do not use PCR's, rely on previous plugins' bitrate
        Status            _status = TSP_OK;       // Current packet status to return
        PacketCounter     _packet_cnt = 0;   // Packet counter
        uint64_t          _time_factor = 0;  // Factor to apply to get milli-seconds
        PCRAnalyzer       _pcr_analyzer {}; // PCR analyzer for time stamping
        SliceEventVector  _events {};       // Sorted list of time events to apply
        size_t            _next_index = 0;   // Index of next SliceEvent to apply

        // Add event in the list from one option.
        void addEvents(const UChar* option, Status status);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"slice", ts::SlicePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SlicePlugin::SlicePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Pass or drop packets based on packet numbers", u"[options]")
{
    option(u"drop",'d', UNSIGNED, 0, UNLIMITED_COUNT);
    help(u"drop",
         u"All packets are dropped after the specified packet number. "
         u"Several --drop options may be specified.");

    option(u"ignore-pcr",'i');
    help(u"ignore-pcr",
         u"When --seconds or --milli-seconds is used, do not use PCR's to "
         u"compute time values. Only rely on bitrate as determined by previous "
         u"plugins in the chain.");

    option(u"milli-seconds", 'm');
    help(u"milli-seconds",
         u"With options --drop, --null, --pass and --stop, interpret the integer "
         u"values as milli-seconds from the beginning, not as packet numbers. "
         u"Time is measured based on bitrate and packet count, not on real time.");

    option(u"null",'n', UNSIGNED, 0, UNLIMITED_COUNT);
    help(u"null",
         u"All packets are replaced by null packets after the specified packet "
         u"number. Several --null options may be specified.");

    option(u"pass",'p', UNSIGNED, 0, UNLIMITED_COUNT);
    help(u"pass",
         u"All packets are passed unmodified after the specified packet number. "
         u"Several --pass options may be specified. This is the default for the "
         u"initial packets.");

    option(u"seconds",0);
    help(u"seconds",
         u"With options --drop, --null, --pass and --stop, interpret the integer "
         u"values as seconds from the beginning, not as packet numbers. "
         u"Time is measured based on bitrate and packet count, not on real time.");

    option(u"stop",'s', UNSIGNED);
    help(u"stop",
         u"Packet transmission stops after the specified packet number and tsp "
         u"terminates.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SlicePlugin::start()
{
    // Get command line options
    _status = TSP_OK;
    _packet_cnt = 0;
    _use_time = present(u"milli-seconds") || present(u"seconds");
    _time_factor = present(u"seconds") ? 1000 : 1;
    _ignore_pcr = present(u"ignore-pcr");
    _pcr_analyzer.reset();

    // Get list of time events
    _events.clear();
    addEvents(u"drop", TSP_DROP);
    addEvents(u"null", TSP_NULL);
    addEvents(u"pass", TSP_OK);
    addEvents(u"stop", TSP_END);

    // Sort events by value
    std::sort(_events.begin(), _events.end());
    _next_index = 0;

    if (tsp->verbose()) {
        tsp->verbose(u"initial packet processing: %s", {StatusNames.name(_status)});
        for (auto& it : _events) {
            tsp->verbose(u"packet %s after %'d %s", {StatusNames.name(it.status), it.value, _use_time ? u"ms" : u"packets"});
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Add events in the list fro one option.
//----------------------------------------------------------------------------

void ts::SlicePlugin::addEvents(const UChar* opt, Status status)
{
    for (size_t index = 0; index < count(opt); ++index) {
        uint64_t value = intValue<uint64_t>(opt, 0, index);
        if (value == 0) {
            // First packet, this is the initial action
            _status = status;
        }
        else {
            _events.push_back(SliceEvent(status, value * _time_factor));
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SlicePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Feed PCR analyzer if necessary
    if (_use_time && !_ignore_pcr) {
        _pcr_analyzer.feedPacket(pkt);
    }

    // Compute current "value" (depends on interpretation);
    uint64_t current_value;
    if (!_use_time) {
        // By default, use packet count
        current_value = _packet_cnt;
    }
    else {
        // Get current bitrate
        BitRate bitrate;
        if (_ignore_pcr || !_pcr_analyzer.bitrateIsValid()) {
            // Do not use PCR, use previous plugins' bitrate
            bitrate = tsp->bitrate();
        }
        else {
            bitrate = _pcr_analyzer.bitrate188();
        }
        if (bitrate == 0) {
            tsp->error(u"unknown bitrate, cannot compute time offset");
            return TSP_END;
        }
        // Compute time in milli-seconds since beginning
        current_value = PacketInterval(bitrate, _packet_cnt);
    }

    // Is it time to change the action?
    while (_next_index < _events.size() && _events[_next_index].value <= current_value) {
        // Yes, we just passed a schedule
        _status = _events[_next_index].status;
        _next_index++;
        tsp->verbose(u"new packet processing: %s after %'d packets", {StatusNames.name(_status), _packet_cnt});
    }

    // Count packets
    _packet_cnt++;
    return _status;
}
