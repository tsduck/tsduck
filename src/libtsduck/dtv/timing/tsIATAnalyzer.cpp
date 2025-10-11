//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIATAnalyzer.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::IATAnalyzer::IATAnalyzer(Report& report) :
    _report(report)
{
}


//----------------------------------------------------------------------------
// Reset all collected information.
//----------------------------------------------------------------------------

void ts::IATAnalyzer::reset()
{
    _started = false;
    _invalid = false;
}


//----------------------------------------------------------------------------
// Get the IAT since start or the last getStatusRestart().
//----------------------------------------------------------------------------

bool ts::IATAnalyzer::getStatus(Status& status)
{
    if (_invalid || !_started) {
        return false;
    }
    else {
        status.mean_iat =_stats_iat.meanRound();
        status.dev_iat = cn::microseconds(cn::microseconds::rep(_stats_iat.standardDeviation()));
        status.min_iat =_stats_iat.minimum();
        status.max_iat =_stats_iat.maximum();
        status.mean_packets = _stats_packets.meanRound();
        status.dev_packets = size_t(_stats_packets.standardDeviation());
        status.min_packets = _stats_packets.minimum();
        status.max_packets = _stats_packets.maximum();
        status.source = _source;
        return true;
    }
}

bool ts::IATAnalyzer::getStatusRestart(Status& status)
{
    const bool ok = getStatus(status);
    if (ok) {
        _stats_packets.reset();
        _stats_iat.reset();
    }
    return ok;
}


//----------------------------------------------------------------------------
// Feeds the analyzer with a TS packet.
//----------------------------------------------------------------------------

void ts::IATAnalyzer::feedPacket(const TSPacket& pkt, const TSPacketMetadata& mdata)
{
    // Ignore artificial input stuffing, they are not part of input reception.
    if (_invalid || mdata.getInputStuffing()) {
        return;
    }

    const TimeSource source = mdata.getInputTimeSource();
    const PCR timestamp = mdata.getInputTimeStamp();

    // Initialize on first packet.
    if (!_started) {
        _started = true;

        _packets_since_last = 0;
        _last_timestamp = timestamp;
        _source = source;
        _stats_packets.reset();
        _stats_iat.reset();

        _report.verbose(u"using %s as timestamp source", TimeSourceEnum().name(source));
        if (source == TimeSource::RTP) {
            _report.warning(u"using %s timestamps, not appropriate for IAT, consider using '-I ip --timestamp-priority kernel-tsp'", TimeSourceEnum().name(source));
        }
    }

    // Detect invalidating conditions. Non recoverable.
    if (!mdata.getDatagram()) {
        _report.error(u"input packets are not in datagrams, cannot analyze IAT (time source: %s)", TimeSourceEnum().name(source));
        _invalid = true;
    }
    else if (!mdata.hasInputTimeStamp()) {
        _report.error(u"input packets have not timestamp, cannot analyze IAT");
        _invalid = true;
    }
    else if (source != _source) {
        _report.error(u"input timestamp source has changed, was %s, now %s, stopping IAT analysis", TimeSourceEnum().name(_source), TimeSourceEnum().name(source));
        _invalid = true;
    }
    else if (timestamp < _last_timestamp) {
        _report.error(u"non-monotonic input timestamp, resetting IAT analysis");
        _packets_since_last = 1;
        _last_timestamp = timestamp;
    }
    else if (timestamp == _last_timestamp) {
        // Most probably in the same datagram as previous TS packet.
        _packets_since_last++;
    }
    else {
        // In a new datagram.
        _stats_iat.feed(timestamp - _last_timestamp);
        _stats_packets.feed(_packets_since_last);
        _packets_since_last = 1;
        _last_timestamp = timestamp;
    }
}
