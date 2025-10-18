//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Skip TS packets until a specified condition is met.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTSClock.h"
#include "tsTime.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SkipPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(SkipPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool             _use_stuffing = false;
        PacketCounter    _skip_packets = 0;
        PacketCounter    _skip_unit_start = 0;
        PacketCounter    _skip_null_seq = 0;
        cn::milliseconds _skip_msec {};
        TSClockArgs      _ts_clock_args {};

        // Working data:
        bool             _started = false;          // Condition is met, pass packets
        PID              _previous_pid = PID_NULL;  // PID of previous packet
        PacketCounter    _unit_start_cnt = 0;       // Payload unit start counter
        PacketCounter    _null_seq_cnt = 0;         // Sequence of null packets counter
        TSClock          _ts_clock {duck};          // Compute playout time
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"skip", ts::SkipPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SkipPlugin::SkipPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Skip leading TS packets of a stream", u"[options] count")
{
    option(u"", 0, UNSIGNED, 0, 1);
    help(u"", u" Legacy parameter, now use --packets.");

    option(u"bytes", 'b', UNSIGNED);
    help(u"bytes", u"Number of leading bytes to skip (rounded up to the next TS packet).");

    option<cn::milliseconds>(u"milli-seconds", 'm');
    help(u"milli-seconds",
         u"Skip the specified number of leading milli-seconds. "
         u"By default, this is wall-clock time (real time). "
         u"See also option --pcr-based.");

    option(u"null-sequence-count", 'n', UNSIGNED);
    help(u"null-sequence-count",
         u"Skip packets until the specified number of sequences of consecutive null packets is encountered.");

    option(u"packets", 'p', UNSIGNED);
    help(u"packets", u"Number of leading packets to skip.");

    option(u"pcr-based");
    help(u"pcr-based",
         u"With --seconds or --milli-seconds, use playout time based on PCR values. "
         u"By default, the time is based on the wall-clock time (real time).");

    option(u"timestamp-based");
    help(u"timestamp-based",
         u"With --seconds or --milli-seconds, use playout time based on timestamp values from the input plugin. "
         u"When input timestamps are not available or not monotonic, fallback to --pcr-based. "
         u"By default, the time is based on the wall-clock time (real time).");

    option<cn::seconds>(u"seconds");
    help(u"seconds",
         u"Skip the specified number of leading seconds. "
         u"By default, this is wall-clock time (real time). "
         u"See also option --pcr-based.");

    option(u"stuffing", 's');
    help(u"stuffing", u"Replace excluded leading packets with stuffing (null packets) instead of removing them.\n");

    option(u"unit-start-count", 'u', UNSIGNED);
    help(u"unit-start-count",
         u"Skip packets until the specified number of packets containing a payload unit start indicator is encountered.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::SkipPlugin::getOptions()
{
    _use_stuffing = present(u"stuffing");
    _ts_clock_args.pcr_based = present(u"pcr-based");
    _ts_clock_args.timestamp_based = present(u"timestamp-based");
    getIntValue(_skip_unit_start, u"unit-start-count");
    getIntValue(_skip_null_seq, u"null-sequence-count");
    getIntValue(_skip_packets, u"packets", intValue<PacketCounter>(u"", (intValue<PacketCounter>(u"bytes") + PKT_SIZE - 1) / PKT_SIZE));
    cn::milliseconds sec, msec;
    getChronoValue(sec, u"seconds");
    getChronoValue(msec, u"milli-seconds");
    _skip_msec = std::max(sec, msec);
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SkipPlugin::start()
{
    // Directly start if no condition is set.
    _started = _skip_packets == 0 && _skip_unit_start == 0 && _skip_null_seq == 0 && _skip_msec == cn::milliseconds::zero();
    _unit_start_cnt = 0;
    _null_seq_cnt = 0;
    _previous_pid = PID_MAX; // Invalid value
    _ts_clock.reset(_ts_clock_args);
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SkipPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Evaluate start condition.
    if (!_started) {

        // Compute time.
        _ts_clock.feedPacket(pkt, pkt_data);

        // Update context information.
        if (pkt.getPID() != PID_NULL && _previous_pid == PID_NULL) {
            _null_seq_cnt++;
        }
        if (pkt.getPUSI()) {
            _unit_start_cnt++;
        }

        // Check if the packet matches one of the selected conditions.
        _started = (_skip_packets > 0 && tsp->pluginPackets() >= _skip_packets) ||
                   (_skip_null_seq > 0 && _null_seq_cnt >= _skip_null_seq) ||
                   (_skip_unit_start > 0 && _unit_start_cnt >= _skip_unit_start) ||
                   (_skip_msec > cn::milliseconds::zero() && _ts_clock.durationMS() >= _skip_msec);

        // Update context information for next packet.
        _previous_pid = pkt.getPID();

        if (_started) {
            verbose(u"starting transmission at packet %'d", tsp->pluginPackets());
        }
    }

    // Final packet status.
    if (_started) {
        return TSP_OK;
    }
    else if (_use_stuffing) {
        return TSP_NULL;
    }
    else {
        return TSP_DROP;
    }
}
