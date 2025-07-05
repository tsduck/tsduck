//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDurationAnalyzer.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::DurationAnalyzer::DurationAnalyzer(Report& report, int severity) :
    _report(report),
    _severity(severity)
{
}


//----------------------------------------------------------------------------
// Reset all collected information.
//----------------------------------------------------------------------------

void ts::DurationAnalyzer::reset()
{
    _use_timestamps = false;
    _total_duration = _switch_duration = _switch_timestamp = PCR::zero();
    _last_source = TimeSource::UNDEFINED;
    _pcr_analyzer.reset();
}


//----------------------------------------------------------------------------
// The following method feeds the analyzer with a TS packet.
//----------------------------------------------------------------------------

void ts::DurationAnalyzer::feedPacket(const TSPacket& pkt, const TSPacketMetadata& mdata)
{
    // Always feed the PCR analyzer to have a fallback.
    _pcr_analyzer.feedPacket(pkt);

    // Do we need to switch source? This is only possible when we prefer input timestamps.
    bool same_source = true;
    if (_prefer_timestamps) {
        const TimeSource source = mdata.getInputTimeSource();
        const bool valid = mdata.hasInputTimeStamp() && ((_use_timestamps && source == _last_source) || (source != _last_source && MonotonicTimeSource(source)));
        if (valid && source != _last_source) {
            // Switch type of input timestamp (source has changed) or switching from PCR-based to input timestamp.
            _report.log(_severity, u"using %s input timestamps to compute durations", TimeSourceEnum().name(source));
            _switch_duration = _total_duration;
            _switch_timestamp = mdata.getInputTimeStamp();
            _last_source = source;
            _use_timestamps = true;
            same_source = false;
        }
        else if (!valid && _use_timestamps) {
            // Invalid input timestamp, switch to PCR-based.
            _report.log(_severity, u"%s input timestamps are not monotonic, fallback to PCR-based", TimeSourceEnum().name(source));
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
            _report.log(_severity, u"found non-monotonic %s input timestamps, fallback to PCR-based", TimeSourceEnum().name(_last_source));
            _switch_duration = _total_duration;
            _switch_timestamp = _pcr_analyzer.duration();
            _last_source = TimeSource::UNDEFINED;
            _use_timestamps = false;
        }
    }
}
