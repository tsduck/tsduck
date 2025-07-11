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
#include "tsMessageQueue.h"
#include "tsThread.h"
#include "tsTime.h"

#define DEFAULT_INTERVAL    5  // default logging interval in seconds
#define DEFAULT_QUEUE_SIZE 10  // default maximum queued metrics messages


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
        // Command line options.
        bool        _log_types = false;
        bool        _log_services = false;
        bool        _log_names = false;
        bool        _log_tr_101_290 = false;
        bool        _pcr_based = false;
        bool        _timestamp_based = false;
        bool        _use_local_time = false;
        int         _max_severity = std::numeric_limits<int>::max();
        Time        _start_time {};
        size_t      _queue_size = DEFAULT_QUEUE_SIZE;
        size_t      _max_metrics = std::numeric_limits<size_t>::max();
        cn::seconds _log_interval {};
        PIDSet      _log_pids {};
        InfluxArgs  _influx_args {};

        // Description of a service. Not reset in each period.
        class ServiceContext
        {
        public:
            ServiceContext() = default;
            UString name {};
            std::set<PID> pids {};
        };

        // Working data.
        Time                  _first_time {};            // UTC time of first packet.
        Time                  _due_time {};              // Next UTC time to report (without --pcr-based).
        Time                  _last_time {};             // UTC time of last report.
        PCR                   _due_pcr {};               // Next PCR to report (with --pcr-based or --time-stamp based).
        PCR                   _last_pcr {};              // PCR of last report.
        size_t                _sent_metrics = 0;         // Number of sent metrics.
        SignalizationDemux    _demux {duck};             // Analyze the stream.
        DurationAnalyzer      _ts_clock {*this};         // Compute playout time based on PCR or input timestamps.
        tr101290::Analyzer    _tr_101_290 {duck};        // ETSI TR 101 290 analyzer.
        InfluxRequest         _request {*this};          // Web request to InfluxDB server.
        MessageQueue<UString> _metrics_queue {};         // Queue of metrics to send.
        PacketCounter         _ts_packets = 0;           // All TS packets in period.
        std::map<PID,PacketCounter> _pids_packets {};    // Packets per PID in period.
        std::map<uint16_t,ServiceContext> _services {};  // Service descriptions.

        // Report metrics to InfluxDB.
        void reportMetrics(bool force);
        void reportMetrics(Time timestamp, cn::milliseconds duration);

        // Implementation of SignalizationHandlerInterface.
        virtual void handleUTC(const Time&, TID) override;
        virtual void handleService(uint16_t, const Service&, const PMT&, bool) override;

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
    option(u"all-pids", 'a');
    help(u"all-pids",
         u"Send bitrate monitoring data for all PID's. Equivalent to --pid 0-8191.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Send bitrate monitoring data for the specified PID's. "
         u"The PID's are identified in InfluxDB by their value in decimal. "
         u"Several -p or --pid options may be specified.");

    option(u"services", 's');
    help(u"services",
         u"Send bitrate monitoring data for services. "
         u"The services are identified in InfluxDB by their id in decimal.");

    option(u"names", 'n');
    help(u"names",
         u"With --services, the services are identified in InfluxDB by their name, when available.");

    option(u"tr-101-290");
    help(u"tr-101-290",
         u"Send error counters as defined by ETSI TR 101 290. "
         u"This plugin can detect a subset of ETSI TR 101 290 only: "
         u"all transport stream logical checks are performed but physical checks on modulation cannot be reported.");

    option(u"max-severity", 0, INTEGER, 0, 0, 1, tr101290::INFO_SEVERITY);
    help(u"max-severity",
         u"With --tr-101-290, specify the maximum severity of error counters to send. "
         u"ETSI TR 101 290 defines severity from 1 (most severe) to 3 (less severe). "
         u"TSDuck adds informational counters at severity 4. "
         u"By default, all error counters are sent.");

    option(u"type");
    help(u"type",
         u"Send bitrate monitoring for types of PID's. "
         u"The types are identified in InfluxDB as " +
         PIDClassIdentifier().nameList(u", ", u"\"", u"\"") +
         u".");

    // Timing options.
    option<cn::seconds>(u"interval", 'i');
    help(u"interval",
         u"Interval in seconds between metrics reports to InfluxDB. "
         u"The default is " TS_USTRINGIFY(DEFAULT_INTERVAL) u" seconds.");

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

    option(u"queue-size", 0, POSITIVE);
    help(u"queue-size", u"count",
         u"Maximum number of queued metrics between the plugin thread and the communication thread with InfluxDB. "
         u"With --pcr-based or --timestamp-based, on off-line streams which are processed at high speed, increase this value if some metrics are lost. "
         u"The default queue size is " TS_USTRINGIFY(DEFAULT_QUEUE_SIZE) u" messages.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::InfluxPlugin::getOptions()
{
    bool success = _influx_args.loadArgs(*this, true);
    _log_types = present(u"type");
    _log_services = present(u"services");
    _log_names = present(u"names");
    _log_tr_101_290 = present(u"tr-101-290");
    _pcr_based = present(u"pcr-based");
    _timestamp_based = present(u"timestamp-based");
    _use_local_time = present(u"local-time");
    getIntValue(_max_severity, u"max-severity", std::numeric_limits<int>::max());
    getIntValue(_max_metrics, u"max-metrics", std::numeric_limits<size_t>::max());
    getIntValue(_queue_size, u"queue-size", DEFAULT_QUEUE_SIZE);
    getChronoValue(_log_interval, u"interval", cn::seconds(DEFAULT_INTERVAL));
    if (present(u"all-pids")) {
        _log_pids = AllPIDs();
    }
    else {
        getIntValues(_log_pids, u"pid");
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
    _tr_101_290.reset();
    _ts_packets = 0;
    _pids_packets.clear();
    _services.clear();

    // Resize the inter-thread queue.
    _metrics_queue.clear();
    _metrics_queue.setMaxMessages(_queue_size);

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

    // Accumulate metrics.
    _ts_packets++;
    if (_log_types || _log_services || _log_pids.any()) {
        _pids_packets[pkt.getPID()]++;
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
    debug(u"got service \"%s\", id %n, removed: %s", service.getName(), service.getId(), removed);
    if (_log_services) {
        if (removed) {
            _services.erase(service.getId());
        }
        else {
            auto& srv(_services[service.getId()]);
            if (_log_names) {
                const UString name(service.getName());
                if (!name.empty()) {
                    srv.name = name;
                }
            }
            if (pmt.isValid()) {
                for (const auto& it : pmt.streams) {
                    srv.pids.insert(it.first);
                }
            }
        }
    }
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
    cn::milliseconds::rep timestamp_ms = (timestamp - Time::UnixEpoch).count();

    // Build data to post. Use a shared pointer to send to the message queue.
    auto data = std::make_shared<UString>();
    data->format(u"bitrate,scope=ts,tsid=%d value=%d %d", _demux.transportStreamId(), PacketBitRate(_ts_packets, duration), timestamp_ms);
    if (_log_services) {
        // PID's which belong to services.
        PIDSet allocated;
        // Send bitrate info for services.
        for (const auto& it : _services) {
            // Count packets in this service.
            PacketCounter packets = 0;
            for (PID pid : it.second.pids) {
                packets += _pids_packets[pid];
                allocated.set(pid);
            }
            // Send bitrate info by name or id.
            if (packets > 0) {
                if (_log_names && !it.second.name.empty()) {
                    const UString name(InfluxRequest::ToKey(it.second.name));
                    data->format(u"\nbitrate,scope=service,service=%s value=%d %d", name, PacketBitRate(packets, duration), timestamp_ms);
                }
                else {
                    data->format(u"\nbitrate,scope=service,service=%d value=%d %d", it.first, PacketBitRate(packets, duration), timestamp_ms);
                }
            }
        }
        // Send bitrate info for "global" PID's (unallocated to any service).
        PacketCounter globals = 0;
        for (const auto& it : _pids_packets) {
            if (!allocated.test(it.first)) {
                globals += it.second;
            }
        }
        if (globals > 0) {
            data->format(u"\nbitrate,scope=service,service=global value=%d %d", PacketBitRate(globals, duration), timestamp_ms);
        }
    }
    if (_log_types) {
        // Build a map of packet count per PID type (all PID's have a type).
        std::map<PIDClass,PacketCounter> by_type;
        for (const auto& it : _pids_packets) {
            by_type[_demux.pidClass(it.first)] += it.second;
        }
        // Send bitrate info for each type of PID.
        for (const auto& it : by_type) {
            if (it.second > 0) {
                const UString name(InfluxRequest::ToKey(PIDClassIdentifier().name(it.first)));
                data->format(u"\nbitrate,scope=type,type=%s value=%d %d", name, PacketBitRate(it.second, duration), timestamp_ms);
            }
        }
    }
    if (_log_pids.any()) {
        for (const auto& it : _pids_packets) {
            if (_log_pids.test(it.first) && it.second > 0) {
                data->format(u"\nbitrate,scope=pid,pid=%d value=%d %d", it.first, PacketBitRate(it.second, duration), timestamp_ms);
            }
        }
    }
    if (_log_tr_101_290) {
        tr101290::Counters counters;
        _tr_101_290.getCountersRestart(counters);
        const auto& desc(tr101290::GetCounterDescriptions());
        for (size_t i = 0; i < counters.size(); i++) {
            if (desc[i].severity <= _max_severity) {
                data->format(u"\ncounter,name=%s,severity=%d value=%d %d", desc[i].name.toLower(), desc[i].severity, counters[i], timestamp_ms);
            }
        }
        if (tr101290::INFO_SEVERITY <= _max_severity) {
            data->format(u"\ncounter,name=error_count,severity=%d value=%d %d", tr101290::INFO_SEVERITY, counters.errorCount(), timestamp_ms);
        }
    }
    debug(u"report at %s, for last %s, data: \"%s\"", timestamp, duration, *data);

    // Send the data to the outgoing thread. Use a zero timeout.
    // It the thread is so slow that the queue is full, just drop the metrics for this interval.
    if (_metrics_queue.enqueue(data, cn::milliseconds::zero())) {
        _sent_metrics++;
    }
    else {
        warning(u"lost metrics, consider increasing --queue-size (current: %d)", _queue_size);
    }

    // Reset metrics.
    _ts_packets = 0;
    _pids_packets.clear();
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
        _request.write(_influx_args, *msg, u"ms");
    }

    debug(u"metrics output thread terminated");
}
