//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Schedule packets pass or drop, based on packet numbers.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTSClock.h"
#include "tsTSValve.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SlicePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(SlicePlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Event description
        struct SliceEvent
        {
            // Public fields
            PacketProcessStatus status;   // Packet status to return ...
            uint64_t            value;    // ... after this packet number or milli-seconds

            // Constructor
            SliceEvent(const PacketProcessStatus& s, const uint64_t& v) : status(s), value(v) {}

            // Comparison, for sort algorithm
            bool operator<(const SliceEvent& e) const { return value < e.value; }
        };
        using SliceEventVector = std::vector<SliceEvent>;

        // Command line options:
        bool                _use_time = false;      // Use milliseconds in SliceEvent::value
        bool                _ignore_pcr = false;    // Do not use PCR's, rely on previous plugins' bitrate
        PacketProcessStatus _first_status = TSP_OK; // Current packet status to return
        uint64_t            _time_factor = 0;       // Factor to apply to get milli-seconds
        SliceEventVector    _events {};             // Sorted list of events to apply
        TSClockArgs         _clock_args {};         // Command-line arguments for _clock (fixed option --pcr-based).
        TSValveArgs         _valve_args {};         // Command-line arguments for _valve.

        // Working data:
        size_t              _next_index = 0;        // Index of next SliceEvent to apply
        TSClock             _clock {duck};          // Transport stream clock.
        TSValve             _valve {*this};         // Open/close packet flow.
        const Names&        _names {PacketProcessingStatusNames()};

        // Add event in the list from one option.
        void addEvents(const UChar* option, PacketProcessStatus status);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"slice", ts::SlicePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SlicePlugin::SlicePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Pass or drop packets based on packet numbers", u"[options]")
{
    _valve_args.defineArgs(*this);

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
// Get command line options.
//----------------------------------------------------------------------------

bool ts::SlicePlugin::getOptions()
{
    _use_time = present(u"milli-seconds") || present(u"seconds");
    _time_factor = present(u"seconds") ? 1000 : 1;
    _ignore_pcr = present(u"ignore-pcr");

    // Get list of events, sort them by value.
    _events.clear();
    _first_status = TSP_OK;
    addEvents(u"drop", TSP_DROP);
    addEvents(u"null", TSP_NULL);
    addEvents(u"pass", TSP_OK);
    addEvents(u"stop", TSP_END);
    std::sort(_events.begin(), _events.end());

    // Always use PCR to evaluate the clock.
    _clock_args.pcr_based = true;

    return _valve_args.loadArgs(*this);
}


//----------------------------------------------------------------------------
// Add events in the list from one option.
//----------------------------------------------------------------------------

void ts::SlicePlugin::addEvents(const UChar* opt, PacketProcessStatus status)
{
    for (size_t index = 0; index < count(opt); ++index) {
        uint64_t value = intValue<uint64_t>(opt, 0, index);
        if (value == 0) {
            // First packet, this is the initial action
            _first_status = status;
        }
        else {
            _events.push_back(SliceEvent(status, value * _time_factor));
        }
    }
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SlicePlugin::start()
{
    _next_index = 0;
    _clock.reset(_clock_args);
    _valve.reset(_valve_args, TSP_DROP, _first_status);

    if (verbose()) {
        verbose(u"initial packet processing: %s", _names.name(_first_status));
        for (auto& it : _events) {
            verbose(u"packet %s after %'d %s", _names.name(it.status), it.value, _use_time ? u"ms" : u"packets");
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::PacketProcessStatus ts::SlicePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Feed the clock if necessary
    if (_use_time && !_ignore_pcr) {
        _clock.feedPacket(pkt, pkt_data);
    }

    // Compute current "value" (depends on interpretation);
    uint64_t current_value = 0;
    if (!_use_time) {
        // By default, use packet count
        current_value = tsp->pluginPackets();
    }
    else if (!_ignore_pcr) {
        // Use milliseconds duration from the clock using PCR.
        current_value = _clock.durationMS().count();
    }
    else {
        // Get current bitrate
        const BitRate bitrate = tsp->bitrate();
        if (bitrate == 0) {
            error(u"unknown bitrate, cannot compute time offset");
            return TSP_END;
        }
        // Compute time in milli-seconds since beginning
        current_value = PacketInterval(bitrate, tsp->pluginPackets()).count();
    }

    // Is it time to change the action?
    while (_next_index < _events.size() && _events[_next_index].value <= current_value) {
        // Yes, we just passed a schedule
        _valve.change(_events[_next_index].status);
        _next_index++;
        verbose(u"new packet processing: %s after %'d packets", _names.name(_valve.current()), tsp->pluginPackets());
    }

    return _valve.processPacket(pkt);
}
