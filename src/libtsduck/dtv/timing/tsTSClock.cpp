//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSClock.h"
#include "tsDuckContext.h"
#include "tsBinaryTable.h"
#include "tsTDT.h"
#include "tsTOT.h"
#include "tsSTT.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSClock::TSClock(DuckContext& duck, int severity) :
    _duck(duck),
    _severity(severity)
{
}


//----------------------------------------------------------------------------
// Reset all collected information.
//----------------------------------------------------------------------------

void ts::TSClock::reset(const TSClockArgs& args)
{
    _args = args;
    _first_time = Time::Epoch;
    _use_timestamps = false;
    _total_duration = _switch_duration = _switch_timestamp = PCR::zero();
    _last_source = TimeSource::UNDEFINED;
    _pcr_analyzer.reset();
    _demux.reset();
    _demux.addPID(PID_TDT);
    _demux.addPID(PID_PSIP); // for ATSC STT
}


//----------------------------------------------------------------------------
// Get the current or initial clock.
//----------------------------------------------------------------------------

ts::Time ts::TSClock::clockUTC() const
{
    if (!_args.timestamp_based && !_args.pcr_based) {
        // Based on real time, not TS time.
        return Time::CurrentUTC();
    }
    else if (_first_time != Time::Epoch) {
        // Based on stream timestamps.
        return _first_time + _total_duration;
    }
    else {
        return Time::Epoch;
    }
}

ts::Time ts::TSClock::clock() const
{
    const Time clk = clockUTC();
    return clk != Time::Epoch && _args.use_local_time ? clk.UTCToLocal() : clk;
}

ts::Time ts::TSClock::initialClock() const
{
    return _first_time != Time::Epoch && _args.use_local_time ? _first_time.UTCToLocal() : _first_time;
}


//----------------------------------------------------------------------------
// Get the estimated playout duration since first packets in milliseconds.
//----------------------------------------------------------------------------

cn::milliseconds ts::TSClock::durationMS() const
{
    if (_args.timestamp_based || _args.pcr_based) {
        // Use timestamps from the stream.
        return cn::duration_cast<cn::milliseconds>(_total_duration);
    }
    else if (_first_time != Time::Epoch) {
        // UTC time of first packet is kown.
        return Time::CurrentUTC() - _first_time;
    }
    else {
        return cn::milliseconds::zero();
    }
}


//----------------------------------------------------------------------------
// Handle UTC time from the stream.
//----------------------------------------------------------------------------

void ts::TSClock::handleUTC(const Time& time)
{
    _duck.report().debug(u"first UTC time from stream: %s", time);
    _first_time = time - _total_duration;

}

void ts::TSClock::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    if (table.tableId() == TID_TDT) {
        const TDT tdt(_duck, table);
        if (tdt.isValid()) {
            handleUTC(tdt.utc_time);
        }
    }
    else if (table.tableId() == TID_TOT) {
        const TOT tot(_duck, table);
        if (tot.isValid()) {
            handleUTC(tot.utc_time);
        }
    }
}

void ts::TSClock::handleSection(SectionDemux& demux, const Section& section)
{
    // We use the section handler for ATSC System Time Table (STT) only.
    // This table violates the common usage rules of MPEG sections, see file tsSTT.h.
    if (section.tableId() == TID_STT) {
        const STT stt(_duck, section);
        if (stt.isValid()) {
            handleUTC(stt.utcTime());
        }
    }
}


//----------------------------------------------------------------------------
// The following method feeds the analyzer with a TS packet.
//----------------------------------------------------------------------------

void ts::TSClock::feedPacket(const TSPacket& pkt, const TSPacketMetadata& mdata)
{
    // Get time of the first packet.
    if (_first_time == Time::Epoch) {
        if (!_args.timestamp_based && !_args.pcr_based) {
            // Based on real time, not TS time.
            _first_time = Time::CurrentUTC();
        }
        else if (_args.start_time != Time::Epoch) {
            // Explicit start time of first packet.
            _first_time = _args.start_time;
        }
        else {
            // Get UTC time from the stream, when needed, only once.
            // After getting the first UTC time, no longer use the demux.
            _demux.feedPacket(pkt);
        }
    }

    // Always feed the PCR analyzer to have a fallback.
    _pcr_analyzer.feedPacket(pkt);

    // Do we need to switch source? This is only possible when we prefer input timestamps.
    bool same_source = true;
    if (_args.timestamp_based) {
        const TimeSource source = mdata.getInputTimeSource();
        const bool valid = mdata.hasInputTimeStamp() && ((_use_timestamps && source == _last_source) || (source != _last_source && MonotonicTimeSource(source)));
        if (valid && source != _last_source) {
            // Switch type of input timestamp (source has changed) or switching from PCR-based to input timestamp.
            _duck.report().log(_severity, u"using %s input timestamps to compute durations", TimeSourceEnum().name(source));
            _switch_duration = _total_duration;
            _switch_timestamp = mdata.getInputTimeStamp();
            _last_source = source;
            _use_timestamps = true;
            same_source = false;
        }
        else if (!valid && _use_timestamps) {
            // Invalid input timestamp, switch to PCR-based.
            _duck.report().log(_severity, u"%s input timestamps are not monotonic, fallback to PCR-based", TimeSourceEnum().name(source));
            _switch_duration = _total_duration;
            _switch_timestamp = _pcr_analyzer.duration();
            _last_source = TimeSource::UNDEFINED;
            _use_timestamps = false;
            same_source = false;
        }
    }

    // Update duration only when we don't switch time source.
    if (same_source) {
        const PCR current = _use_timestamps ? mdata.getInputTimeStamp() : _pcr_analyzer.duration();
        const PCR new_duration = _switch_duration + (current - _switch_timestamp);
        if (new_duration >= _total_duration) {
            _total_duration = new_duration;
        }
        else {
            // The source is not monotonic, switch to PCR-based.
            _duck.report().log(_severity, u"found non-monotonic %s input timestamps, fallback to PCR-based", TimeSourceEnum().name(_last_source));
            _switch_duration = _total_duration;
            _switch_timestamp = _pcr_analyzer.duration();
            _last_source = TimeSource::UNDEFINED;
            _use_timestamps = false;
        }
    }
}
