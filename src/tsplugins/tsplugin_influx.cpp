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
#include "tsPCRAnalyzer.h"
#include "tsTime.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class InfluxPlugin: public ProcessorPlugin, private SignalizationHandlerInterface
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
        bool        _pcr_based = false;
        bool        _use_local_time = false;
        Time        _start_time {};
        cn::seconds _log_interval {};
        InfluxArgs  _influx_args {};

        // Working data.
        Time               _first_time {};         // UTC time of first packet.
        Time               _due_time {};           // Next UTC time to report (without --pcr-based).
        Time               _last_time {};          // UTC time of last report.
        PCR                _due_pcr {};            // Next PCR to report (with --pcr-based).
        PCR                _last_pcr {};           // PCR of last report.
        SignalizationDemux _demux {duck};          // Analyze the stream.
        PCRAnalyzer        _pcr_analyzer {1, 1};   // Compute playout time based on PCR.
        InfluxRequest      _request {*this};       // Web request to InfluxDB server.
        PacketCounter      _ts_packets = 0;        // All TS packets in period.
        PacketCounter      _null_packets = 0;      // Null packets in period.
        uint16_t           _ts_id = INVALID_TS_ID; // Transport stream id.

        // Report metrics to InfluxDB.
        void reportMetrics(Time timestamp, cn::milliseconds duration);

        // Implementation of SignalizationHandlerInterface.
        virtual void handlePAT(const PAT&, PID) override;
        virtual void handleUTC(const Time&, TID) override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"influx", ts::InfluxPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::InfluxPlugin::InfluxPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Send live TS metrics to InfluxDB, a data source for Grafana", u"[options]")
{
    _influx_args.defineArgs(*this);

    option<cn::seconds>(u"interval", 'i');
    help(u"interval",
         u"Interval in seconds between metrics reports to InfluxDB. "
         u"The default is one second.");

    option(u"local-time");
    help(u"local-time",
         u"Transmit timestamps as local time, based on the current system configuration. "
         u"By default, timestamps are transmitted as UTC time.");

    option(u"pcr-based");
    help(u"pcr-based",
         u"Use playout time based on PCR values. "
         u"By default, the time is based on the wall-clock time (real time).");

    option(u"start-time", 0, STRING);
    help(u"start-time", u"year/month/day:hour:minute:second",
         u"With --pcr-based, specify the initial date & time reference. "
         u"By default, with --pcr-based, the activity starts at the first UTC time which is found in a DVB TDT or ATSC STT.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::InfluxPlugin::getOptions()
{
    bool success = _influx_args.loadArgs(duck, *this, true);
    getChronoValue(_log_interval, u"interval", cn::seconds(1));
    _pcr_based = present(u"pcr-based");
    _use_local_time = present(u"local-time");
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
    _pcr_analyzer.reset();
    _ts_packets = _null_packets = 0;
    _ts_id = INVALID_TS_ID;
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::InfluxPlugin::stop()
{
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::InfluxPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Start time on first packet.
    if (tsp->pluginPackets() == 0) {
        if (!_pcr_based) {
            // Use wall clock time as reference.
            _first_time = _last_time = Time::CurrentUTC();
            _due_time = _first_time + _log_interval;
        }
        else if (_start_time != Time::Epoch) {
            // Use PCR time reference with a given base.
            _first_time = _start_time;
            _due_pcr = cn::duration_cast<PCR>(_log_interval);
        }
    }

    // Feed the various analyzers.
    _demux.feedPacket(pkt);
    if (_pcr_based) {
        // Compute PCR-based time instead of real-time.
        _pcr_analyzer.feedPacket(pkt);
    }

    // Accumulate metrics.
    _ts_packets++;
    if (pkt.getPID() == PID_NULL) {
        _null_packets++;
    }

    // Is it time to report metrics?
    if (_pcr_based) {
        const PCR current = _pcr_analyzer.duration();
        if (_due_pcr > PCR::zero() && current >= _due_pcr) {
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
        if (current >= _due_time) {
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

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Receive a new UTC time from the stream.
//----------------------------------------------------------------------------

void ts::InfluxPlugin::handleUTC(const Time& utc, TID tid)
{
    if (_pcr_based && _first_time == Time::Epoch) {
        // Use PCR time as reference and first TDT/TOT/STT as base.
        debug(u"first UTC time from stream: %s", utc);
        _first_time = utc - _pcr_analyzer.duration();
        _due_pcr = cn::duration_cast<PCR>(_log_interval);
    }
}


//----------------------------------------------------------------------------
// Receive a PAT from the TS.
//----------------------------------------------------------------------------

void ts::InfluxPlugin::handlePAT(const PAT& pat, PID pid)
{
    _ts_id = pat.ts_id;
}


//----------------------------------------------------------------------------
// Report metrics to InfluxDB.
//----------------------------------------------------------------------------

void ts::InfluxPlugin::reportMetrics(Time timestamp, cn::milliseconds duration)
{
    // Convert timestamp in milliseconds since Unix Epoch for InfluxDB server.
    if (_use_local_time) {
        timestamp = timestamp.UTCToLocal();
    }
    cn::milliseconds::rep timestamp_ms = (timestamp - Time::UnixEpoch).count();

    // Build data to post.
    UString data;
    data.format(u"bitrate,tsid=%d ts=%d,null=%d %d",
                _ts_id,
                PacketBitRate(_ts_packets, duration),
                PacketBitRate(_null_packets, duration),
                timestamp_ms);

    // Send the data to the InfluxDB server.
    debug(u"report at %s, for last %s, data: \"%s\"", timestamp, duration, data);
    _request.write(_influx_args, data, u"ms");

    // Reset metrics.
    _ts_packets = _null_packets = 0;
}
