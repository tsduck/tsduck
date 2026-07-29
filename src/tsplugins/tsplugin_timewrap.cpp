//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Detect PCR, PTS, DTS wrap-down to zero.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TimeWrapPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(TimeWrapPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of one PID
        struct PIDContext
        {
            PIDContext() = default;
            PacketCounter packets = 0;                // Packets in this PID.
            uint64_t      pcr = PCRTraits::INVALID;   // Last PCR value in this PID.
            uint64_t      pts = PTSTraits::INVALID;   // Last PTS value in this PID.
            uint64_t      dts = DTSTraits::INVALID;   // Last DTS value in this PID.
        };

        // Command line options.
        bool   _check_pcr = false;
        bool   _check_pts = false;
        bool   _check_dts = false;
        PIDSet _pid_list {};

        // Working data.
        std::map<PID,PIDContext> _stats {};
        std::set<PID>            _wrap_pids {};
        PacketCounter            _wrap_count = 0;

        // Process one type of time stamp.
        template <TimeSource T>
        void processTime(TSPacket& pkt, bool TimeWrapPlugin::* check_opt, uint64_t PIDContext::* field, uint64_t (TSPacket::* get)() const);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"timewrap", ts::TimeWrapPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TimeWrapPlugin::TimeWrapPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Detect PCR, PTS, DTS wrap-down to zero", u"[options]")
{
    option(u"pcr");
    help(u"pcr", u"Check wrap-down on PCR. By default, check all time stamps.");

    option(u"dts");
    help(u"dts", u"Check wrap-down on DTS. By default, check all time stamps.");

    option(u"pts");
    help(u"pts", u"Check wrap-down on PTS. By default, check all time stamps.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"PID filter: select packets with these PID values. "
         u"Several -p or --pid options may be specified. "
         u"Without -p or --pid option, all PID's are selected.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::TimeWrapPlugin::getOptions()
{
    _check_pcr = present(u"pcr");
    _check_pts = present(u"pts");
    _check_dts = present(u"dts");
    if (!_check_pcr && !_check_pts && !_check_dts) {
        _check_pcr = _check_pts = _check_dts = true; // all time stamps by default
    }
    getIntValues(_pid_list, u"pid", true); // all PID's set by default
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::TimeWrapPlugin::start()
{
    _stats.clear();
    _wrap_pids.clear();
    _wrap_count = 0;
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::TimeWrapPlugin::stop()
{
    // Display PCR summary
    info(u"found %'d wrap-downs on %'d PID's", _wrap_count, _wrap_pids.size());
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::PacketProcessStatus ts::TimeWrapPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    if (_pid_list[pid]) {
        processTime<TimeSource::PCR>(pkt, &TimeWrapPlugin::_check_pcr, &PIDContext::pcr, &TSPacket::getPCR);
        processTime<TimeSource::PTS>(pkt, &TimeWrapPlugin::_check_pts, &PIDContext::pts, &TSPacket::getPTS);
        processTime<TimeSource::DTS>(pkt, &TimeWrapPlugin::_check_dts, &PIDContext::dts, &TSPacket::getDTS);
        _stats[pid].packets++;
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Process one type of time stamp.
//----------------------------------------------------------------------------

template <ts::TimeSource T>
void ts::TimeWrapPlugin::processTime(TSPacket& pkt, bool TimeWrapPlugin::* check_opt, uint64_t PIDContext::* field, uint64_t (TSPacket::* get)() const)
{
    using Traits = TimeSourceTraits<T>;

    // If we need to analyze this type of time stamp.
    if (this->*check_opt) {

        // Get potential value from the packet (INVALID if there is none).
        const uint64_t value = (pkt.*get)();

        if (value != Traits::INVALID) {
            const PID pid = pkt.getPID();
            PIDContext& pc(_stats[pid]);
            if (pc.*field == Traits::INVALID) {
                // First time stamp in PID
                pc.*field = value;
            }
            else if (Traits::Wrap(pc.*field, value)) {
                // Found a wrap-down.
                _wrap_pids.insert(pid);
                _wrap_count++;
                info(u"TS packet %'d, PID %n, packet %'d, %s wrap-down", tsp->pluginPackets(), pid, pc.packets, TimeSourceEnum().name(Traits::TYPE));
            }
            pc.*field = value;
        }
    }
}
