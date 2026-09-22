//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Send live TS metrics to InfluxDB plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsInfluxArgs.h"
#include "tsInfluxRequest.h"
#include "tsInfluxSender.h"
#include "tsSignalizationDemux.h"
#include "tsTSClock.h"
#include "tsTSClockArgs.h"
#include "tstr101290Analyzer.h"
#include "tsIATAnalyzer.h"
#include "tsCADescriptor.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Plugin to send live TS metrics to InfluxDB, typically as data source for Grafana.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL InfluxPlugin: public ProcessorPlugin, private SignalizationHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(InfluxPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Default values.
        static constexpr cn::seconds DEFAULT_INTERVAL = cn::seconds(5);  // Default logging interval in seconds.

        // Command line options.
        bool        _log_bitrate = false;
        bool        _log_pcr = false;
        bool        _log_pts = false;
        bool        _log_dts = false;
        bool        _log_timestamps = false;  // any of --pcr --pts --dts
        bool        _log_tr_101_290 = false;
        bool        _log_iat = false;
        bool        _log_types = false;
        bool        _log_services = false;
        bool        _log_names = false;
        int         _max_severity = std::numeric_limits<int>::max();
        size_t      _max_metrics = std::numeric_limits<size_t>::max();
        cn::seconds _log_interval {};
        PIDSet      _log_pids {};
        TSClockArgs _ts_clock_args {};
        InfluxArgs  _influx_args {false, true};

        // Description of a service. Not reset in each period.
        class ServiceContext
        {
        public:
            ServiceContext() = default;
            PID           pcr_pid = PID_NULL;  // Declared PCR PID or video PID.
            PID           pts_pid = PID_NULL;  // First PID where we expect PTS and DTS.
            UString       name {};             // Service name.
            UString       inf_name {};         // Service name with escaped characters, compatible with InfluxDB message.
            std::set<PID> pids {};             // Set of PID's in this service.
        };
        using ServiceContextMap = std::map<uint16_t,ServiceContext>;

        // Description of a PID. Reset in each period.
        class PIDContext
        {
        public:
            PIDContext() = default;
            PacketCounter packets = 0;         // Number of TS packets in period.
            uint64_t      pcr = INVALID_PCR;   // Last PCR found in period.
            uint64_t      pts = INVALID_PTS;   // Last PTS found in period.
            uint64_t      dts = INVALID_DTS;   // Last DTS found in period.
        };
        using PIDContextMap = std::map<PID,PIDContext>;

        // Working data.
        Time               _due_time {};        // Next UTC time to report (without --pcr-based).
        Time               _last_time {};       // UTC time of last report.
        size_t             _sent_metrics = 0;   // Number of sent metrics.
        SignalizationDemux _demux {duck};       // Analyze the stream.
        TSClock            _ts_clock {duck};    // Compute playout time based on real time, PCR or input timestamps.
        tr101290::Analyzer _tr_101_290 {duck};  // ETSI TR 101 290 analyzer.
        IATAnalyzer        _iat {*this};        // Inter-packet Arrival Time (IAT) analyzer.
        PacketCounter      _ts_packets = 0;     // All TS packets in period.
        PIDContextMap      _pids {};            // PID's description in period.
        ServiceContextMap  _services {};        // Services descriptions.
        InfluxSender       _server {this};      // Send requests to InfluxDB server.

        // Get the representable name of a service, from an iterator in _service.
        UString serviceName(const ServiceContextMap::value_type&) const;

        // Report metrics to InfluxDB.
        void reportMetrics(bool force);
        void reportMetrics(const Time& timestamp, cn::milliseconds duration);

        // Build metrics string for a given type of timestamp.
        void addTimestampMetrics(InfluxRequest& req, const UChar* measurement, PID ServiceContext::* refpid, uint64_t PIDContext::* value, uint16_t tsid);

        // Implementation of SignalizationHandlerInterface.
        virtual void handleService(uint16_t, const Service&, const PMT&, bool) override;

        // Search PID's in a descriptor list.
        void searchPIDs(std::set<PID>&, const DescriptorList&);
    };
}
