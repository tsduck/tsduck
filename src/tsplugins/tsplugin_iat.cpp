//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Analyze Inter-packet Arrival Time (IAT) for datagram-based inputs.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsIATAnalyzer.h"
#include "tsTime.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class IATPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(IATPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Default values.
        static constexpr cn::seconds DEFAULT_INTERVAL = cn::seconds(5);  // Default logging interval in seconds.

        // Command line options:
        cn::seconds _log_interval {};

        // Working data:
        Time        _due_time {};
        IATAnalyzer _iat_analyzer {*this};
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"iat", ts::IATPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::IATPlugin::IATPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Analyze Inter-packet Arrival Time (IAT) for datagram-based inputs", u"[options]")
{
    option<cn::seconds>(u"interval", 'i');
    help(u"interval",
         u"Interval in seconds between evaluations of the intra-packet arrival time. "
         u"The default is " + UString::Chrono(DEFAULT_INTERVAL) + u".");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::IATPlugin::getOptions()
{
    getChronoValue(_log_interval, u"interval", DEFAULT_INTERVAL);
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::IATPlugin::start()
{
    _iat_analyzer.reset();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::IATPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Give up immediately if analysis is impossible.
    if (!_iat_analyzer.isValid()) {
        return TSP_OK;
    }

    // Start time is set on first packet.
    const Time current = Time::CurrentUTC();
    if (tsp->pluginPackets() == 0) {
        _due_time = current + _log_interval;
    }

    // Analyze all packets.
    _iat_analyzer.feedPacket(pkt, pkt_data);

    // Report on due time.
    if (current >= _due_time) {

        IATAnalyzer::Status status;
        _iat_analyzer.getStatusRestart(status);
        info(u"IAT: %s (std.dev: %d, min: %d, max: %d), source: %s, pkt/dgram: %d (min: %d, max: %d)",
             status.mean_iat, status.dev_iat.count(), status.min_iat.count(), status.max_iat.count(),
             TimeSourceEnum().name(status.source),
             status.mean_packets, status.min_packets, status.max_packets);

        // Enforce monotonic time increase if late.
        _due_time += _log_interval;
        if (_due_time <= current) {
            // We are late, wait one second before next metrics.
            _due_time = current + cn::seconds(1);
        }
    }
    return TSP_OK;
}
