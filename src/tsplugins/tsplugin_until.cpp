//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Copy TS packets until a specified condition is met.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTime.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class UntilPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(UntilPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool             _exclude_last = false;  // Exclude packet which triggers the condition
        PacketCounter    _pack_max = 0;          // Stop at Nth packet
        PacketCounter    _unit_start_max = 0;    // Stop at Nth packet with payload unit start
        PacketCounter    _null_seq_max = 0;      // Stop at Nth sequence of null packets
        cn::milliseconds _msec_max {};           // Stop after N milli-seconds

        // Working data:
        PacketCounter _unit_start_cnt = 0;       // Payload unit start counter
        PacketCounter _null_seq_cnt = 0;         // Sequence of null packets counter
        Time          _start_time {};            // Time of first packet reception
        PID           _previous_pid = PID_NULL;  // PID of previous packet
        bool          _terminated = false;       // Final condition is met
        bool          _transparent = false;      // Pass all packets, no longer check conditions
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"until", ts::UntilPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::UntilPlugin::UntilPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Copy packets until one of the specified conditions is met", u"[options]")
{
    option(u"bytes", 'b', UNSIGNED);
    help(u"bytes", u"Stop after processing the specified number of bytes.");

    option(u"exclude-last", 'e');
    help(u"exclude-last", u"Exclude the last packet (the one which triggers the final condition).");

    option(u"joint-termination", 'j');
    help(u"joint-termination",
         u"When the final condition is triggered, perform a \"joint termination\" instead of unconditional termination. "
         u"See \"tsp --help\" for more details on \"joint termination\".");

    option<cn::milliseconds>(u"milli-seconds", 'm');
    help(u"milli-seconds",
         u"Stop the specified number of milli-seconds after receiving the first packet.");

    option(u"null-sequence-count", 'n', UNSIGNED);
    help(u"null-sequence-count",
         u"Stop when the specified number of sequences of consecutive null packets is encountered.");

    option(u"packets", 'p', UNSIGNED);
    help(u"packets", u"Stop after the specified number of packets.");

    option<cn::seconds>(u"seconds", 's');
    help(u"seconds", u"Stop the specified number of seconds after receiving the first packet.");

    option(u"unit-start-count", 'u', UNSIGNED);
    help(u"unit-start-count",
         u"Stop when the specified number of packets containing a payload unit start indicator is encountered.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::UntilPlugin::getOptions()
{
    _exclude_last = present(u"exclude-last");
    getIntValue(_unit_start_max,u"unit-start-count");
    getIntValue(_null_seq_max, u"null-sequence-count");
    getIntValue(_pack_max, u"packets", (intValue<PacketCounter>(u"bytes") + PKT_SIZE - 1) / PKT_SIZE);
    cn::milliseconds sec, msec;
    getChronoValue(sec, u"seconds");
    getChronoValue(msec, u"milli-seconds");
    _msec_max = std::max(sec, msec);
    tsp->useJointTermination(present(u"joint-termination"));
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::UntilPlugin::start()
{
    _unit_start_cnt = 0;
    _null_seq_cnt = 0;
    _previous_pid = PID_MAX; // Invalid value
    _terminated = false;
    _transparent = false;
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::UntilPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Check if no longer need to check condition (typically in joint termination state).
    if (_transparent) {
        return TSP_OK;
    }

    // Check if already terminated.
    if (_terminated) {
        if (tsp->useJointTermination()) {
            tsp->jointTerminate();
            _transparent = true;
            return TSP_OK;
        }
        else {
            return TSP_END;
        }
    }

    // Record time of first packet.
    if (tsp->pluginPackets() == 0) {
        _start_time = Time::CurrentUTC();
    }

    // Update context information.
    if (pkt.getPID() == PID_NULL && _previous_pid != PID_NULL) {
        _null_seq_cnt++;
    }
    if (pkt.getPUSI()) {
        _unit_start_cnt++;
    }

    // Check if the packet matches one of the selected conditions.
    _terminated = (_pack_max > 0 && tsp->pluginPackets() + 1 >= _pack_max) ||
                  (_null_seq_max > 0 && _null_seq_cnt >= _null_seq_max) ||
                  (_unit_start_max > 0 && _unit_start_cnt >= _unit_start_max) ||
                  (_msec_max > cn::milliseconds::zero() && Time::CurrentUTC() >= _start_time + _msec_max);

    // Update context information for next packet.
    _previous_pid = pkt.getPID();

    // Finally report termination status.
    if (!_terminated || !_exclude_last) {
        return TSP_OK;
    }
    else if (tsp->useJointTermination()) {
        tsp->jointTerminate();
        _transparent = true;
        return TSP_OK;
    }
    else {
        return TSP_END;
    }
}
