//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPCRRegulator.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PCRRegulator::PCRRegulator(Report* report, int log_level) :
    _report(report == nullptr ? &NULLREP : report),
    _log_level(log_level)
{
}


//----------------------------------------------------------------------------
// Set a new report.
//----------------------------------------------------------------------------

void ts::PCRRegulator::setReport(ts::Report *report, int log_level)
{
    _report = report == nullptr ? &NULLREP : report;
    _log_level = log_level;
}


//----------------------------------------------------------------------------
// Set the PCR reference PID.
//----------------------------------------------------------------------------

void ts::PCRRegulator::setReferencePID(PID pid)
{
    _user_pid = pid;
    if (pid != _pid) {
        reset();
        _pid = pid;
    }
}


//----------------------------------------------------------------------------
// Set the minimum wait interval.
//----------------------------------------------------------------------------

void ts::PCRRegulator::setMinimimWait(NanoSecond ns)
{
    if (ns != _wait_min && ns > 0) {
        // Request at least this precision.
        const NanoSecond precision = Monotonic::SetPrecision(2000000); // 2 milliseconds in nanoseconds

        // We must wait at least the returned precision.
        _wait_min = std::max(ns, precision);

        _report->log(_log_level, u"minimum wait: %'d nano-seconds, using %'d ns", {precision, _wait_min});
    }
}


//----------------------------------------------------------------------------
// Re-initialize state.
//----------------------------------------------------------------------------

void ts::PCRRegulator::reset()
{
    _pid = _user_pid;
    _burst_pkt_cnt = 0;
    _started = false;
}


//----------------------------------------------------------------------------
// Regulate the flow, to be called at each packet.
//----------------------------------------------------------------------------

bool ts::PCRRegulator::regulate(const TSPacket& pkt)
{
    const PID pid = pkt.getPID();
    const bool has_pcr = pkt.hasPCR();
    bool flush = false;

    // Select first PID with PCR's when unspecified by user.
    if (has_pcr && _pid == PID_NULL) {
        _pid = pid;
        _report->log(_log_level, u"using PID 0x%X (%d) for PCR reference", {pid, pid});
    }

    // Do something only on PCR's from the reference PID.
    if (has_pcr && pid == _pid) {

        // PCR value, this is the reference system clock.
        const uint64_t pcr = pkt.getPCR();

        // Check if the PCR sequence seems valid.
        // We check that the difference between two PCR's is less than 2 seconds.
        // Normally, adjacent PCR's are way much closer, but let's be tolerant.
        constexpr uint64_t max_pcr_diff = 2 * SYSTEM_CLOCK_FREQ; // 2 seconds in PCR units
        const bool valid_pcr_seq = _started &&
            (_pcr_last == INVALID_PCR ||
             (pcr < _pcr_last && pcr + PCR_SCALE < _pcr_last + max_pcr_diff) ||
             (pcr > _pcr_last && pcr < _pcr_last + max_pcr_diff));

        // Try to detect incorrect PCR sequences (such as cycling input).
        if (_started && !valid_pcr_seq) {
            _report->warning(u"out of sequence PCR, maybe source was cycling, restarting regulation");
            _started = false;
        }

        if (!_started) {
            // Initialize regulation at the first PCR.
            _started = true;
            _clock_first.getSystemTime();
            _clock_last = _clock_first;
            _pcr_first = pcr;
            _pcr_offset = 0;

            // Compute minimum wait is none is set.
            if (_wait_min <= 0) {
                setMinimimWait();
            }
        }
        else {
            // Got a PCR after start, need to regulate.

            // Accumulate all PCR wrap-down sequences so that the distance with _pcr_first is a valid duration.
            // One complete PCR round is only 26.5 hours. So, it it realistic to go through more than one round.
            // In an uint64_t value, we can accumulate 21664 years in PCR units. So, we can safely assume that
            // there will be no overflow when accumulating PCR's on 64 bits.
            if (_pcr_last != INVALID_PCR && pcr < _pcr_last) {
                _pcr_offset += PCR_SCALE;
            }

            // Compute the number of PCR units since the first PCR.
            const uint64_t pcru = _pcr_offset + pcr - _pcr_first;

            // Compute the number of nano-seconds since the first PCR.
            // In an uint64_t value, we can accumulate 292 years in nano-seconds units.
            // Coded to avoid arithmetic overflow, don't change without thinking twice.
            const NanoSecond ns = (NanoSecPerMicroSec * pcru) / (SYSTEM_CLOCK_FREQ / MicroSecPerSec);

            // Compute due system clock, the expected system time for this PCR.
            Monotonic clock_due(_clock_first);
            clock_due += ns;

            // Do not wait less than the user-specified minimum.
            if (clock_due - _clock_last >= _wait_min) {
                // Wait until system time for current PCR.
                _clock_last = clock_due;
                _clock_last.wait();
                // Always flush after wait.
                flush = true;
            }
        }

        // Always keep last PCR value.
        _pcr_last = pcr;
    }

    // One more packet in current burst.
    if (++_burst_pkt_cnt >= _opt_burst) {
        flush = true;
    }

    // Reset packet counter at end of each burst.
    if (flush) {
        _burst_pkt_cnt = 0;
    }

    // Return true when packets should be flushed to next plugin.
    return flush;
}
