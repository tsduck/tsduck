//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Send live TS metrics to InfluxDB, typically as data source for Grafana.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsInfluxArgs.h"
#include "tsInfluxRequest.h"
#include "tsSignalizationDemux.h"
#include "tsDurationAnalyzer.h"
#include "tstr101290Analyzer.h"
#include "tsIATAnalyzer.h"
#include "tsCADescriptor.h"
#include "tsMessageQueue.h"
#include "tsThread.h"
#include "tsTime.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class InfluxPlugin: public ProcessorPlugin, private SignalizationHandlerInterface, private Thread
    {
        TS_PLUGIN_CONSTRUCTORS(InfluxPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

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
        bool        _pcr_based = false;
        bool        _timestamp_based = false;
        bool        _use_local_time = false;
        int         _max_severity = std::numeric_limits<int>::max();
        Time        _start_time {};
        size_t      _max_metrics = std::numeric_limits<size_t>::max();
        cn::seconds _log_interval {};
        PIDSet      _log_pids {};
        InfluxArgs  _influx_args {};

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
        Time               _first_time {};      // UTC time of first packet.
        Time               _due_time {};        // Next UTC time to report (without --pcr-based).
        Time               _last_time {};       // UTC time of last report.
        PCR                _due_pcr {};         // Next PCR to report (with --pcr-based or --time-stamp based).
        PCR                _last_pcr {};        // PCR of last report.
        size_t             _sent_metrics = 0;   // Number of sent metrics.
        SignalizationDemux _demux {duck};       // Analyze the stream.
        DurationAnalyzer   _ts_clock {*this};   // Compute playout time based on PCR or input timestamps.
        tr101290::Analyzer _tr_101_290 {duck};  // ETSI TR 101 290 analyzer.
        IATAnalyzer        _iat {*this};        // Inter-packet Arrival Time (IAT) analyzer.
        PacketCounter      _ts_packets = 0;     // All TS packets in period.
        PIDContextMap      _pids {};            // PID's description in period.
        ServiceContextMap  _services {};        // Services descriptions.
        MessageQueue<InfluxRequest> _metrics_queue {}; // Queue of metrics to send.

        // Get the representable name of a service, from an iterator in _service.
        UString serviceName(const ServiceContextMap::value_type&) const;

        // Report metrics to InfluxDB.
        void reportMetrics(bool force);
        void reportMetrics(Time timestamp, cn::milliseconds duration);

        // Build metrics string for a given type of timestamp.
        void addTimestampMetrics(InfluxRequest& req, const UChar* measurement, PID ServiceContext::* refpid, uint64_t PIDContext::* value, uint16_t tsid);

        // Implementation of SignalizationHandlerInterface.
        virtual void handleUTC(const Time&, TID) override;
        virtual void handleService(uint16_t, const Service&, const PMT&, bool) override;

        // Search PID's in a descriptor list.
        void searchPIDs(std::set<PID>&, const DescriptorList&);

        // There is one thread which asynchronously sends the metrics data to the InfluxDB server.
        // We cannot anticipate the response time of the server. Using a thread avoid slowing down
        // the packet transmission. The following method is the thread main code.
        virtual void main() override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"influx", ts::InfluxPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::InfluxPlugin::InfluxPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Send live TS metrics to InfluxDB, a data source for Grafana", u"[options]")
{
    // InfluxDB connection options.
    _influx_args.defineArgs(*this);

    // Types of monitoring.
    option(u"bitrate");
    help(u"bitrate",
         u"Send bitrate monitoring data. This is the default. "
         u"This option is only useful when any of --pcr, --pts, --dts, --tr-101-290 are also set.");

    option(u"pcr");
    help(u"pcr",
         u"Send the last PCR value in a set of PID and/or services. "
         u"Also specify at least one of --pid, --all-pids, --services.");

    option(u"pts");
    help(u"pts",
         u"Send the last PTS value in a set of PID and/or services. "
         u"Also specify at least one of --pid, --all-pids, --services.");

    option(u"dts");
    help(u"dts",
         u"Send the last DTS value in a set of PID and/or services. "
         u"Also specify at least one of --pid, --all-pids, --services.");

    option(u"tr-101-290");
    help(u"tr-101-290",
         u"Send error counters as defined by ETSI TR 101 290. "
         u"This plugin can detect a subset of ETSI TR 101 290 only: "
         u"all transport stream logical checks are performed but physical checks on modulation cannot be reported.");

    option(u"iat");
    help(u"iat",
         u"Send metrics on Inter-packet Arrival Time (IAT) for datagram-based inputs (ip, pcap, srt, rist). "
         u"Ignored if the input is not datagram-based.");

    // Subselection of types of monitoring.
    option(u"all-pids", 'a');
    help(u"all-pids",
         u"Send metrics data for all PID's. Equivalent to --pid 0-8191.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Send metrics data for the specified PID's. "
         u"The PID's are identified in InfluxDB by their value in decimal. "
         u"Several -p or --pid options may be specified.");

    option(u"services", 's');
    help(u"services",
         u"Send metrics data for services. "
         u"The services are identified in InfluxDB by their id in decimal.");

    option(u"names", 'n');
    help(u"names",
         u"With --services, the services are identified in InfluxDB by their name, when available.");

    option(u"max-severity", 0, INTEGER, 0, 0, 1, tr101290::INFO_SEVERITY);
    help(u"max-severity",
         u"With --tr-101-290, specify the maximum severity of error counters to send. "
         u"ETSI TR 101 290 defines severity from 1 (most severe) to 3 (less severe). "
         u"TSDuck adds informational counters at severity 4. "
         u"By default, all error counters are sent.");

    option(u"type");
    help(u"type",
         u"Send bitrate metrics for types of PID's. "
         u"The types are identified in InfluxDB as " +
         PIDClassIdentifier().nameList(u", ", u"\"", u"\"") +
         u".");

    // Timing options.
    option<cn::seconds>(u"interval", 'i');
    help(u"interval",
         u"Interval in seconds between metrics reports to InfluxDB. "
         u"The default is " + UString::Chrono(DEFAULT_INTERVAL) + u".");

    option(u"local-time");
    help(u"local-time",
         u"Transmit timestamps as local time, based on the current system configuration. "
         u"By default, timestamps are transmitted as UTC time.");

    option(u"pcr-based");
    help(u"pcr-based",
         u"Use playout time based on PCR values. "
         u"By default, the time is based on the wall-clock time (real time).");

    option(u"timestamp-based");
    help(u"timestamp-based",
         u"Use playout time based on timestamp values from the input plugin. "
         u"When input timestamps are not available or not monotonic, fallback to --pcr-based. "
         u"By default, the time is based on the wall-clock time (real time).");

    option(u"start-time", 0, STRING);
    help(u"start-time", u"year/month/day:hour:minute:second",
         u"With --pcr-based or --timestamp-based, specify the initial date & time reference. "
         u"By default, with --pcr-based or --timestamp-based, the activity starts at the first UTC time which is found in a DVB TDT or ATSC STT.");

    option(u"max-metrics", 0, UNSIGNED);
    help(u"max-metrics", u"count",
         u"Stop after sending that number of metrics. "
         u"This is a test option. Never stop by default.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::InfluxPlugin::getOptions()
{
    bool success = _influx_args.loadArgs(*this, true);
    _log_pcr = present(u"pcr");
    _log_pts = present(u"pts");
    _log_dts = present(u"dts");
    _log_timestamps = _log_pcr || _log_pts || _log_dts;
    _log_tr_101_290 = present(u"tr-101-290");
    _log_iat = present(u"iat");
    _log_bitrate = present(u"bitrate") || (!_log_timestamps && !_log_tr_101_290);
    _log_types = present(u"type");
    _log_services = present(u"services");
    _log_names = present(u"names");
    _pcr_based = present(u"pcr-based");
    _timestamp_based = present(u"timestamp-based");
    _use_local_time = present(u"local-time");
    getIntValue(_max_severity, u"max-severity", std::numeric_limits<int>::max());
    getIntValue(_max_metrics, u"max-metrics", std::numeric_limits<size_t>::max());
    getChronoValue(_log_interval, u"interval", DEFAULT_INTERVAL);
    getIntValues(_log_pids, u"pid");
    if (present(u"all-pids")) {
        _log_pids = AllPIDs();
    }

    if (_log_timestamps && (_log_pids.none() && !_log_services)) {
        error(u"with any of --pcr --pts --dts, at least one of --pid --all-pids --services is required");
        success = false;
    }

    _start_time = Time::Epoch;
    if (present(u"start-time")) {
        if (!_start_time.decode(value(u"start-time"))) {
            error(u"invalid --start-time value \"%s\" (use \"year/month/day:hour:minute:second\")", value(u"start-time"));
            success = false;
        }
        else if (_use_local_time) {
            // The specified time is local but we use UTC internally.
            _start_time = _start_time.localToUTC();
        }
    }

    return success;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::InfluxPlugin::start()
{
    _first_time = _due_time = _last_time = Time::Epoch;
    _due_pcr = _last_pcr = PCR::zero();
    _demux.reset();
    _demux.setHandler(this);
    _ts_clock.reset();
    _ts_clock.useInputTimestamps(_timestamp_based);
    _ts_packets = 0;
    _pids.clear();
    _services.clear();
    _iat.reset();

    // Reset the TR 101 290 analyzer.
    if (_log_tr_101_290) {
        _tr_101_290.reset();
        _tr_101_290.setCollectByPID(_log_services || _log_pids.any());
    }

    // Resize the inter-thread queue.
    _metrics_queue.clear();
    _metrics_queue.setMaxMessages(_influx_args.queue_size);

    // Start the internal thread which sends the metrics data.
    return Thread::start();
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::InfluxPlugin::stop()
{
    // Force a last set of metrics.
    reportMetrics(true);

    // Send a termination message and wait for actual thread termination.
    _metrics_queue.forceEnqueue(nullptr);
    Thread::waitForTermination();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::InfluxPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Start time is set on first packet.
    if (tsp->pluginPackets() == 0) {
        if (!_pcr_based && !_timestamp_based) {
            // Use wall clock time as reference.
            _first_time = _last_time = Time::CurrentUTC();
            _due_time = _first_time + _log_interval;
        }
        else if (_start_time != Time::Epoch) {
            // Use PCR or timestamp reference with a given start time.
            // Without a given start time, delay the reports until a UTC time is found in the stream.
            _first_time = _start_time;
            _due_pcr = cn::duration_cast<PCR>(_log_interval);
        }
    }

    // Feed the various analyzers.
    _demux.feedPacket(pkt);
    if (_pcr_based || _timestamp_based || _log_tr_101_290) {
        // Compute PCR-based time instead of real-time.
        _ts_clock.feedPacket(pkt, pkt_data);
    }
    if (_log_tr_101_290) {
        _tr_101_290.feedPacket(_ts_clock.duration(), pkt);
    }
    if (_log_iat) {
        _iat.feedPacket(pkt, pkt_data);
    }

    // Accumulate metrics.
    _ts_packets++;
    auto& ctx(_pids[pkt.getPID()]);
    ctx.packets++;
    if (_log_pcr && pkt.hasPCR()) {
        ctx.pcr = pkt.getPCR();
    }
    if (_log_pts && pkt.hasPTS()) {
        ctx.pts = pkt.getPTS();
    }
    if (_log_dts && pkt.hasDTS()) {
        ctx.dts = pkt.getDTS();
    }

    // Is it time to report metrics?
    reportMetrics(false);
    return _sent_metrics < _max_metrics ? TSP_OK : TSP_END;
}


//----------------------------------------------------------------------------
// Receive a new UTC time from the stream.
//----------------------------------------------------------------------------

void ts::InfluxPlugin::handleUTC(const Time& utc, TID tid)
{
    if ((_pcr_based || _timestamp_based) && _first_time == Time::Epoch) {
        // Use PCR time as reference and first TDT/TOT/STT as base.
        debug(u"first UTC time from stream: %s", utc);
        _first_time = utc - _ts_clock.duration();
        _due_pcr = cn::duration_cast<PCR>(_log_interval);
    }
}


//----------------------------------------------------------------------------
// Receive a service update.
//----------------------------------------------------------------------------

void ts::InfluxPlugin::handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed)
{
    debug(u"got service \"%s\", id %n, pmt valid: %s, removed: %s", service.getName(), service.getId(), pmt.isValid(), removed);
    if (_log_services) {
        if (removed) {
            _services.erase(service.getId());
        }
        else {
            auto& srv(_services[service.getId()]);
            const UString name(service.getName());
            if (!name.empty()) {
                srv.name = name;
                srv.inf_name = InfluxRequest::ToKey(name);
            }
            if (pmt.isValid()) {
                searchPIDs(srv.pids, pmt.descs);
                PID first_video_pid = PID_NULL;
                PID first_audio_pid = PID_NULL;
                for (const auto& it : pmt.streams) {
                    srv.pids.insert(it.first);
                    searchPIDs(srv.pids, it.second.descs);
                    if (first_video_pid == PID_NULL && it.second.isVideo(duck)) {
                        first_video_pid = it.first;
                    }
                    else if (first_audio_pid == PID_NULL && it.second.isAudio(duck)) {
                        first_audio_pid = it.first;
                    }
                }
                srv.pts_pid = first_video_pid != PID_NULL ? first_video_pid : first_audio_pid;
                srv.pcr_pid = pmt.pcr_pid != PID_NULL ? pmt.pcr_pid : srv.pts_pid;
            }
            debug(u"service \"%s\", id %n, PCR PID: %n, PTS PID: %n", srv.name, service.getId(), srv.pcr_pid, srv.pts_pid);
        }
    }
}


//----------------------------------------------------------------------------
// Search service PID's in a descriptor list.
//----------------------------------------------------------------------------

void ts::InfluxPlugin::searchPIDs(std::set<PID>& pids, const DescriptorList& dlist)
{
    // Search CA_descriptor only at this time.
    for (const auto& desc : dlist) {
        const CADescriptor ca(duck, desc);
        if (ca.isValid()) {
            pids.insert(ca.ca_pid);
        }
    }
}


//----------------------------------------------------------------------------
// Get the representable name of a service, from an iterator in _service.
//----------------------------------------------------------------------------

ts::UString ts::InfluxPlugin::serviceName(const ServiceContextMap::value_type& it) const
{
    return _log_names && !it.second.inf_name.empty() ? it.second.inf_name : UString::Decimal(it.first, 0, true, UString());
}


//----------------------------------------------------------------------------
// Report metrics to InfluxDB if time to do so.
//----------------------------------------------------------------------------

void ts::InfluxPlugin::reportMetrics(bool force)
{
    if (_sent_metrics < _max_metrics) {
        if (_pcr_based || _timestamp_based) {
            const PCR current = _ts_clock.duration();
            if (force || (_due_pcr > PCR::zero() && current >= _due_pcr)) {
                reportMetrics(_first_time + current, cn::duration_cast<cn::milliseconds>(current - _last_pcr));
                _last_pcr = current;
                _due_pcr += cn::duration_cast<PCR>(_log_interval);
                // Enforce monotonic time increase if late.
                if (_due_pcr <= current) {
                    // We are late, wait one second before next metrics.
                    _due_pcr = current + cn::duration_cast<PCR>(cn::seconds(1));
                }
            }
        }
        else {
            const Time current = Time::CurrentUTC();
            if (force || current >= _due_time) {
                reportMetrics(current, current - _last_time);
                _last_time = current;
                _due_time += _log_interval;
                // Enforce monotonic time increase if late.
                if (_due_time <= current) {
                    // We are late, wait one second before next metrics.
                    _due_time = current + cn::seconds(1);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Report metrics to InfluxDB using known timestamp and duration.
//----------------------------------------------------------------------------

void ts::InfluxPlugin::reportMetrics(Time timestamp, cn::milliseconds duration)
{
    // Convert timestamp in milliseconds since Unix Epoch for InfluxDB server.
    if (_use_local_time) {
        timestamp = timestamp.UTCToLocal();
    }

    // Build data to post. Use a shared pointer to send to the message queue.
    auto req = std::make_shared<InfluxRequest>(*this, _influx_args);
    req->start(timestamp - Time::UnixEpoch);

    // The total TS bitrate is always present and first.
    const uint16_t tsid = _demux.transportStreamId();
    req->add(u"bitrate", UString::Format(u"scope=ts,tsid=%d", tsid), PacketBitRate(_ts_packets, duration).toInt());

    // If we need to report metrics per service, determine the set of PID's which belong to a service.
    // All other PID's are "global".
    PIDSet allocated_pids;
    if (_log_services && (_log_bitrate || _log_tr_101_290)) {
        for (const auto& it : _services) {
            for (PID pid : it.second.pids) {
                allocated_pids.set(pid);
            }
        }
    }

    // Log bitrate per service.
    if (_log_bitrate && _log_services) {
        // Send bitrate info for services.
        for (const auto& it : _services) {
            // Count packets in this service.
            PacketCounter packets = 0;
            for (PID pid : it.second.pids) {
                packets += _pids[pid].packets;
            }
            // Send bitrate info by name or id.
            if (packets > 0) {
                req->add(u"bitrate", UString::Format(u"scope=service,tsid=%d,service=%s", tsid, serviceName(it)), PacketBitRate(packets, duration).toInt());
            }
        }
        // Send bitrate info for "global" PID's (unallocated to any service).
        PacketCounter globals = 0;
        for (const auto& it : _pids) {
            if (!allocated_pids.test(it.first)) {
                globals += it.second.packets;
            }
        }
        if (globals > 0) {
            req->add(u"bitrate", UString::Format(u"scope=service,tsid=%d,service=global", tsid), PacketBitRate(globals, duration).toInt());
        }
    }

    // Log bitrate per PID type.
    if (_log_bitrate && _log_types) {
        // Build a map of packet count per PID type (all PID's have a type).
        std::map<PIDClass,PacketCounter> by_type;
        for (const auto& it : _pids) {
            by_type[_demux.pidClass(it.first)] += it.second.packets;
        }
        // Send bitrate info for each type of PID.
        for (const auto& it : by_type) {
            if (it.second > 0) {
                const UString name(InfluxRequest::ToKey(PIDClassIdentifier().name(it.first)));
                req->add(u"bitrate", UString::Format(u"scope=type,tsid=%d,type=%s", tsid, name), PacketBitRate(it.second, duration).toInt());
            }
        }
    }

    // Log bitrate per PID.
    if (_log_bitrate && _log_pids.any()) {
        for (const auto& it : _pids) {
            if (_log_pids.test(it.first) && it.second.packets > 0) {
                req->add(u"bitrate", UString::Format(u"scope=pid,tsid=%d,pid=%d", tsid, it.first), PacketBitRate(it.second.packets, duration).toInt());
            }
        }
    }

    // Log PCR/PTS/DTS values.
    if (_log_pcr) {
        addTimestampMetrics(*req, u"pcr", &ServiceContext::pcr_pid, &PIDContext::pcr, tsid);
    }
    if (_log_pts) {
        addTimestampMetrics(*req, u"pts", &ServiceContext::pts_pid, &PIDContext::pts, tsid);
    }
    if (_log_dts) {
        addTimestampMetrics(*req, u"dts", &ServiceContext::pts_pid, &PIDContext::dts, tsid);
    }

    // Log ETSI TR 101 290 error counters.
    if (_log_tr_101_290) {
        // Get the error counters, global and by PID when necessary.
        tr101290::Counters counters;
        tr101290::CountersByPID counters_by_pid;
        _tr_101_290.getCountersRestart(counters, counters_by_pid);

        // Send metrics for each standard error counter.
        const auto& counter_descriptions(tr101290::GetCounterDescriptions());
        for (size_t cindex = 0; cindex < counters.size(); cindex++) {

            // Description of that error counter.
            const auto& desc(counter_descriptions[cindex]);
            if (desc.severity <= _max_severity) {

                // Name of that error counter, as InfluxDB tag.
                const UString name(desc.name.toLower());

                // Always log global counter, even if zero.
                req->add(u"counter", UString::Format(u"name=%s,severity=%d,scope=ts,tsid=%d", name, desc.severity, tsid), counters[cindex]);

                // Log the counter by service, if not zero.
                if (_log_services) {
                    // Loop on all known services.
                    for (const auto& it : _services) {
                        // Accumulate that counter for all PID's in this service.
                        size_t errcount = 0;
                        for (PID pid : it.second.pids) {
                            const auto xx = counters_by_pid.find(pid);
                            if (xx != counters_by_pid.end()) {
                                errcount += xx->second[cindex];
                            }
                        }
                        // Send counter for that service.
                        if (errcount > 0) {
                            req->add(u"counter", UString::Format(u"name=%s,severity=%d,scope=service,tsid=%d,service=%s", name, desc.severity, tsid, serviceName(it)), errcount);
                        }
                    }
                    // Send the error counter for "global" PID's (unallocated to any service).
                    size_t errcount = 0;
                    for (const auto& it : counters_by_pid) {
                        if (!allocated_pids.test(it.first)) {
                            errcount += it.second[cindex];
                        }
                    }
                    if (errcount > 0) {
                        req->add(u"counter", UString::Format(u"name=%s,severity=%d,scope=service,tsid=%d,service=global", name, desc.severity, tsid), errcount);
                    }
                }

                // Log the counter by selected PID, if not zero.
                if (_log_pids.any()) {
                    for (const auto& it : counters_by_pid) {
                        if (_log_pids.test(it.first) && it.second[cindex] > 0) {
                            req->add(u"counter", UString::Format(u"name=%s,severity=%d,scope=pid,tsid=%d,pid=%d", name, desc.severity, tsid, it.first), it.second[cindex]);
                        }
                    }
                }
            }
        }

        // Final synthetic error_count.
        if (tr101290::INFO_SEVERITY <= _max_severity) {
            req->add(u"counter", UString::Format(u"name=error_count,severity=%d,scope=ts,tsid=%d", tr101290::INFO_SEVERITY, tsid), counters.errorCount());
        }
    }

    // Log inter-packet arrival time.
    if (_log_iat && _iat.isValid()) {
        IATAnalyzer::Status status;
        if (_iat.getStatusRestart(status)) {
            req->add(u"iat", u"type=mean", status.mean_iat.count());
            req->add(u"iat", u"type=min", status.min_iat.count());
            req->add(u"iat", u"type=max", status.max_iat.count());
        }
    }

    // Debug output of the complete message to InfluxDB.
    debug(u"report at %s, for last %s, data: \"%s\"", timestamp, duration, req->currentContent());

    // Send the data to the outgoing thread. Use a zero timeout.
    // It the thread is so slow that the queue is full, just drop the metrics for this interval.
    if (_metrics_queue.enqueue(req, cn::milliseconds::zero())) {
        _sent_metrics++;
    }
    else {
        warning(u"lost metrics, consider increasing --queue-size (current: %d)", _influx_args.queue_size);
    }

    // Reset metrics.
    _ts_packets = 0;
    _pids.clear();
}


//----------------------------------------------------------------------------
// Build metrics string for a given type of timestamp.
//----------------------------------------------------------------------------

void ts::InfluxPlugin::addTimestampMetrics(InfluxRequest& req, const UChar* measurement, PID ServiceContext::* refpid, uint64_t PIDContext::* value, uint16_t tsid)
{
    // Log timestamp per service.
    if (_log_services) {
        for (const auto& it : _services) {
            if (it.second.*refpid != PID_NULL) {
                auto& ctx(_pids[it.second.*refpid]);
                if (ctx.*value != INVALID_PCR) {
                    req.add(measurement, UString::Format(u"scope=service,tsid=%d,service=%s", tsid, serviceName(it)), ctx.*value);
                }
            }
        }
    }

    // Log timestamp per PID.
    if (_log_pids.any()) {
        for (const auto& it : _pids) {
            if (_log_pids.test(it.first) && it.second.*value != INVALID_PCR) {
                req.add(measurement, UString::Format(u"scope=pid,tsid=%d,pid=%d", tsid, it.first), it.second.*value);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Thread which asynchronously sends the metrics data to the InfluxDB server.
//----------------------------------------------------------------------------

void ts::InfluxPlugin::main()
{
    debug(u"metrics output thread started");

    for (;;) {
        // Wait for one message, stop on null pointer.
        decltype(_metrics_queue)::MessagePtr msg;
        _metrics_queue.dequeue(msg);
        if (msg == nullptr) {
            break;
        }

        // Send the data to the InfluxDB server.
        msg->send();
    }

    debug(u"metrics output thread terminated");
}
