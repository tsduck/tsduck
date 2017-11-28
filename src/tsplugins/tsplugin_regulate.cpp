//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Regulate (slow down) the packet flow according to a bitrate.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsMonotonic.h"
TSDUCK_SOURCE;

#define DEF_PACKET_BURST 16


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class RegulatePlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        RegulatePlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // Regulation state
        enum State {INITIAL, REGULATED, UNREGULATED};

        // Private members
        State         _state;           // Current regulation state
        BitRate       _opt_bitrate;     // Bitrate option, zero means use input
        BitRate       _cur_bitrate;     // Current bitrate
        PacketCounter _opt_burst;       // Number of packets to burst at a time
        PacketCounter _burst_pkt_max;   // Total packets in current burst
        PacketCounter _burst_pkt_cnt;   // Countdown of packets in current burst
        NanoSecond    _burst_min;       // Minimum delay between two bursts (ns)
        NanoSecond    _burst_duration;  // Delay between two bursts (nano-seconds)
        Monotonic     _burst_end;       // End of current burst
        Monotonic     _bitrate_start;   // Time of last bitrate change
        PacketCounter _bitrate_pkt_cnt; // Passed packets since last bitrate change

        // Compute burst duration (_burst_duration and _burst_pkt_max), based on
        // required packets/burst (command line option) and current bitrate.
        void handleNewBitrate();

        // Process one packet in a regulated burst. Wait at end of burst.
        Status regulatePacket(bool& flush, bool smoothen);

        // Inaccessible operations
        RegulatePlugin() = delete;
        RegulatePlugin(const RegulatePlugin&) = delete;
        RegulatePlugin& operator=(const RegulatePlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::RegulatePlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::RegulatePlugin::RegulatePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Regulate the TS packets flow to a specified bitrate.", u"[options]"),
    _state(INITIAL),
    _opt_bitrate(0),
    _cur_bitrate(0),
    _opt_burst(0),
    _burst_pkt_max(0),
    _burst_pkt_cnt(0),
    _burst_min(0),
    _burst_duration(0),
    _burst_end(),
    _bitrate_start(),
    _bitrate_pkt_cnt(0)
{
    option(u"bitrate",      'b', POSITIVE);
    option(u"packet-burst", 'p', POSITIVE);

    setHelp(u"Regulate (slow down only) the TS packets flow according to a specified\n"
            u"bitrate. Useful to play a non-regulated input (such as a TS file) to a\n"
            u"non-regulated output device such as IP multicast.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -b value\n"
            u"  --bitrate value\n"
            u"      Specify the bitrate in b/s. By default, use the \"input\" bitrate,\n"
            u"      typically resulting from the PCR analysis of the input file.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -p value\n"
            u"  --packet-burst value\n"
            u"      Number of packets to burst at a time. Does not modify the average\n"
            u"      output bitrate but influence smoothing and CPU load. The default\n"
            u"      is " TS_STRINGIFY(DEF_PACKET_BURST) " packets.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::RegulatePlugin::start()
{
    // Get command line arguments
    _opt_bitrate = intValue<BitRate>(u"bitrate", 0);
    _opt_burst = intValue<PacketCounter>(u"packet-burst", DEF_PACKET_BURST);

    // Compute the minimum delay between two bursts, in nano-seconds.
    // This is a limitation of the operating system. If we try to use
    // wait on durations lower than the minimum, this will introduce
    // latencies which mess up the regulation. We try to request 2
    // milliseconds as time precision and we keep what the operating
    // system gives.

    _burst_min = Monotonic::SetPrecision(2000000); // 2 milliseconds in nanoseconds

    tsp->verbose(u"minimum packet burst duration is %'d nano-seconds", {_burst_min});

    // Reset state
    _state = INITIAL;
    _cur_bitrate = 0;
    _burst_pkt_max = 0;
    _burst_pkt_cnt = 0;
    _burst_duration = 0;

    return true;
}


//----------------------------------------------------------------------------
// Handle bitrate change, compute burst duration.
//----------------------------------------------------------------------------

void ts::RegulatePlugin::handleNewBitrate()
{
    // Assume that the packets/burst is the one specified on the command line.
    _burst_pkt_max = _opt_burst;

    // Compute corresponding duration (in nano-seconds) between two bursts.
    assert(_cur_bitrate > 0);
    _burst_duration = (NanoSecPerSec * PKT_SIZE * 8 * _burst_pkt_max) / _cur_bitrate;

    // If the result is too small for the time precision of the operating
    // system, recompute a larger burst duration
    if (_burst_duration < _burst_min) {
        _burst_duration = _burst_min;
        _burst_pkt_max = (_burst_duration * _cur_bitrate) / (NanoSecPerSec * PKT_SIZE * 8);
    }

    tsp->debug(u"new regulation, burst: %'d nano-seconds, %'d packets", {_burst_duration, _burst_pkt_max});
    
    // Register start of bitrate sequence.
    _bitrate_pkt_cnt = 0;
    _bitrate_start.getSystemTime();
}


//----------------------------------------------------------------------------
// Process one packet in a regulated burst. Wait at end of burst.
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::RegulatePlugin::regulatePacket(bool& flush, bool smoothen)
{
    // Check if end of current burst. Take care, _burst_pkt_cnt may be already zero.
    if ((_burst_pkt_cnt == 0 || --_burst_pkt_cnt == 0) && smoothen && _bitrate_pkt_cnt > 0) {
        // In the middle of a sequence with same bitrate, we try to smoothen the regulation.
        // Because of rounding, we tend to pass slightly less packets than requested.
        // See if we need to add some packets from time to time.
        Monotonic now;
        now.getSystemTime();
        // Number of packets we should have passed since beginning of sequence of this bitrate:
        const PacketCounter expected = PacketDistance(_cur_bitrate, (now - _bitrate_start) / NanoSecPerMilliSec);
        if (expected > _bitrate_pkt_cnt) {
            // We should have passed more than we did, increase this burst size.
            _burst_pkt_cnt = expected - _bitrate_pkt_cnt;
        }
    }

    // Recheck end of burst, just in case we added some more packets to smoothen.
    if (_burst_pkt_cnt == 0) {
        // Wait until scheduled end of burst.
        _burst_end.wait();
        // Restart a new burst, use monotonic time
        _burst_pkt_cnt = _burst_pkt_max;
        _burst_end += _burst_duration;
        // Flush current burst
        flush = true;
    }

    // One more regulated packet at this bitrate.
    _bitrate_pkt_cnt++;

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::RegulatePlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Compute old and new bitrate (most often the same)
    BitRate old_bitrate = _cur_bitrate;
    _cur_bitrate = _opt_bitrate != 0 ? _opt_bitrate : tsp->bitrate();

    if (tsp->verbose() && (_cur_bitrate != old_bitrate || _state == INITIAL)) {
        // Initial state or new bitrate
        if (_cur_bitrate == 0) {
            tsp->verbose(u"unknown bitrate, cannot regulate.");
        }
        else {
            tsp->verbose(u"regulated at bitrate %'d b/s", {_cur_bitrate});
        }
    }

    // Process with state machine
    switch (_state) {

        case INITIAL: {
            // Initial state, will become either regulated or unregulated
            if (_cur_bitrate == 0) {
                // No bitrate -> unregulated
                _state = UNREGULATED;
                return TSP_OK; // transmit without regulation
            }
            else {
                // Got a non-zero bitrate -> start regulation
                _state = REGULATED;
                // Compute initial burst duration: first, compute burst time
                handleNewBitrate();
                // Get initial clock
                _burst_end.getSystemTime();
                // Compute end time of next burst
                _burst_end += _burst_duration;
                // We are at the start of the burst, initialize countdown
                _burst_pkt_cnt = _burst_pkt_max;
                // Transmit first packet of burst
                bitrate_changed = true;
                return regulatePacket(flush, false);
            }
            break;
        }

        case UNREGULATED: {
            // We had no bitrate, we did not regulate
            if (_cur_bitrate == 0) {
                // Still no bitrate, transmit without regulation, remain unregulated
                return TSP_OK;
            }
            else {
                // Finally got a bitrate.
                // Transmit this one without regulation and flush.
                // Will regulate next time, starting with empty (flushed) buffer.
                _state = INITIAL;
                bitrate_changed = true;
                flush = true;
                return TSP_OK;
            }
            break;
        }

        case REGULATED: {
            // We previously had a bitrate and we regulated the flow.
            if (_cur_bitrate == 0) {
                // No more bitrate, become unregulated
                _state = UNREGULATED;
                return TSP_OK;
            }
            else if (_cur_bitrate == old_bitrate) {
                // Still the same bitrate, continue to burst
                return regulatePacket(flush, true);
            }
            else {
                // Got a new non-zero bitrate. Compute new burst.
                // Revert to start time of current burst, based on previous burst duration.
                _burst_end -= _burst_duration;
                // Compute the estimated due elapsed time in current burst, based on
                // previous burst duration and proportion of passed packets.
                const NanoSecond elapsed = _burst_duration - (_burst_duration * _burst_pkt_max) / _burst_pkt_cnt;
                // Compute new burst duration, based on new bitrate
                handleNewBitrate();
                // Adjust end time of current burst. We want to close the current burst
                // as soon as possible to restart based on new bitrate.
                if (elapsed >= _burst_min) {
                    // We already passed more packets that required for minimum delay.
                    // Close current burst now.
                    _burst_end += elapsed;
                    _burst_pkt_cnt = 0;
                }
                else {
                    // We have to wait a bit more to respect the minimum delay.
                    // Compute haw many packets we should pass for the remaining time,
                    // based on the new bitrate.
                    _burst_end += _burst_min;
                    _burst_pkt_cnt = ((_burst_min - elapsed) * _cur_bitrate) / (NanoSecPerSec * PKT_SIZE * 8);
                }
                // Report that the bitrate has changed
                bitrate_changed = true;
                return regulatePacket(flush, false);
            }
            break;
        }

        default: {
            assert(false);
        }
    }

    // Should never get there...
    tsp->error(u"internal error, invalid regulator state");
    return TSP_END;
}
