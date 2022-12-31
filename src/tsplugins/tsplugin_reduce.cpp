//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  Reduce the bitrate of the TS by dropping null packets.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsPCRAnalyzer.h"

// Default mode: target bitrate with 10,000 packets window (620 ms at 24 Mb/s, 300 ms at 50 Mb/s)
#define DEFAULT_PACKET_WINDOW 10000


//----------------------------------------------------------------------------
// Plugin definition.
// Important: this plugin works in individual packet or packet window mode,
// depending on the command line parameters.
//----------------------------------------------------------------------------

namespace ts {
    class ReducePlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(ReducePlugin);
    public:
        // Implementation of plugin API
        ReducePlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual size_t getPacketWindowSize() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;
        virtual size_t processPacketWindow(TSPacketWindow& win) override;

    private:
        // Last error code (to avoid reporting the same error again and again).
        enum class Error {NONE, PKT_OVERFLOW, NO_BITRATE, USE_PREVIOUS, LOW_BITRATE};

        // Command line parameters:
        BitRate       _target_bitrate;   // Target bitrate to read, zero if fixed proportion is used.
        BitRate       _input_bitrate;    // User-sepcified input bitrate.
        MilliSecond   _window_ms;        // Packet window size in milliseconds.
        PacketCounter _window_pkts;      // Packet window size in packets.
        bool          _pcr_based;        // Use PCR's in packet window to compute the number f packets to remove.
        PIDSet        _pcr_pids;         // Reference PCR PID's.
        PacketCounter _fixed_rempkt;     // rempkt parameter, zero if target
        PacketCounter _fixed_inpkt;      // inpkt parameter

        // Working data:
        PacketCounter _pkt_to_remove;    // Current number of packets to remove
        uint64_t      _bits_to_remove;   // Current number of bits to remove
        BitRate       _previous_bitrate; // Bitrate from previous packet window.
        Error         _error;            // Last error code.

        // Compute bitrate in a packet window.
        BitRate computeBitRate(const TSPacketWindow& win) const;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"reduce", ts::ReducePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ReducePlugin::ReducePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Reduce the TS bitrate by removing stuffing packets", u"[options]"),
    _target_bitrate(0),
    _input_bitrate(0),
    _window_ms(0),
    _window_pkts(0),
    _pcr_based(false),
    _pcr_pids(),
    _fixed_rempkt(0),
    _fixed_inpkt(0),
    _pkt_to_remove(0),
    _bits_to_remove(0),
    _previous_bitrate(0),
    _error(Error::NONE)
{
    // Legacy parameters, now in --fixed-proportion.
    option(u"", 0, POSITIVE, 0, 2);
    help(u"",
         u"Legacy syntax: For compatibility, two integer parameters can be used to specify "
         u"'rempkt' and 'inpkt', the removal of packets in fixed proportion. "
         u"Now preferably use option --fixed-proportion.");

    option(u"fixed-proportion", 'f', STRING);
    help(u"fixed-proportion", u"rempkt/inpkt",
         u"Reduce the bitrate in fixed proportion: 'rempkt' TS packets are automatically "
         u"removed after every 'inpkt' input TS packets in the transport stream. "
         u"Only stuffing packets can be removed. "
         u"Both 'rempkt' and 'inpkt' must be non-zero integer values. "
         u"Exactly one of --target-bitrate or --fixed-proportion must be specified.");

    option<BitRate>(u"input-bitrate", 'i');
    help(u"input-bitrate",
         u"Specify the input bitrate in bits/second. "
         u"By default, the input bitrate is permanently evaluated by previous plugins.");

    option(u"packet-window", 0, POSITIVE);
    help(u"packet-window", u"packet-count",
         u"With --target-bitrate, define the number of packets over which they are analyzed and "
         u"extra packets are removed. The default is " + UString::Decimal(DEFAULT_PACKET_WINDOW) +
         u" packets. Options --time-window and --packet-window are mutually exclusive.");

    option(u"pcr-based", 'p');
    help(u"pcr-based",
         u"With --target-bitrate, use PCR's in each packet window to determine how many packets "
         u"should be removed in each window. By default, the input bitrate is used. In the case "
         u"of highly variable bitrate (VBR), using PCR's on each time window gives better results "
         u"but PCR's must be present and accurate and the window size must be large enough "
         u"to contain more than one PCR on at least one PID.");

    option(u"reference-pcr-pid", 'r', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"reference-pcr-pid", u"pid1[-pid2]",
         u"With --pcr-based, use PCR's from the specified reference PID's only. "
         u"The option --reference-pcr-pid can be present multiple time. "
         u"By default, PCR's are used from any PID.");

    option<BitRate>(u"target-bitrate", 't');
    help(u"target-bitrate",
         u"Reduce the bitrate to this target value in bits/second. "
         u"Only stuffing packets can be removed. "
         u"Exactly one of --target-bitrate or --fixed-proportion must be specified.\n\n"
         u"Using the target bitrate method introduces an uncompressable latency in the stream, "
         u"see options --time-window and --packet-window.");

    option(u"time-window", 0, POSITIVE);
    help(u"time-window", u"milli-seconds",
         u"With --target-bitrate, define the latency period over which packets are analyzed and "
         u"extra packets are removed. To use this method, the bitrate must be known during the "
         u"starting phase so that it can be turned into a number of packets. "
         u"Options --time-window and --packet-window are mutually exclusive.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::ReducePlugin::getOptions()
{
    bool ok = true;

    getValue(_target_bitrate, u"target-bitrate");
    getValue(_input_bitrate, u"input-bitrate");
    getIntValue(_window_pkts, u"packet-window", DEFAULT_PACKET_WINDOW);
    getIntValue(_window_ms, u"time-window");
    getIntValues(_pcr_pids, u"reference-pcr-pid", true);
    _pcr_based = present(u"pcr-based");

    // Legacy syntax for --fixed-proportion in parameters.
    getIntValue(_fixed_rempkt, u"", 0, 0);
    getIntValue(_fixed_inpkt, u"", 0, 1);
    const UString fixprop = value(u"fixed-proportion");
    if (!fixprop.empty()) {
        if (_fixed_rempkt > 0 || _fixed_inpkt > 0) {
            tsp->error(u"Specify either --fixed-proportion or legacy parameters but not both");
            ok = false;
        }
        else if (!fixprop.scan(u"%d/%d", {&_fixed_rempkt, &_fixed_inpkt}) || _fixed_rempkt <= 0 || _fixed_inpkt <= 0) {
            tsp->error(u"Invalid value '%s' for --fixed-proportion", {fixprop});
            ok = false;
        }
    }
    if (_target_bitrate > 0 && (_fixed_rempkt > 0 || _fixed_inpkt > 0)) {
        tsp->error(u"Specify either fixed proportion or target bitrate but not both");
        ok = false;
    }

    return ok;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::ReducePlugin::start()
{
    _pkt_to_remove = 0;
    _bits_to_remove = 0;
    _previous_bitrate = 0;
    _error = Error::NONE;
    return true;
}


//----------------------------------------------------------------------------
// Get requested window size, called between start() and first packet.
//----------------------------------------------------------------------------

size_t ts::ReducePlugin::getPacketWindowSize()
{
    if (_target_bitrate == 0) {
        // Fixed proportion mode: use individual packet mode.
        return 0;
    }

    if (_window_ms == 0) {
        // Packet window was specified in packets.
        assert(_window_pkts > 0);
        return size_t(_window_pkts);
    }

    const BitRate bitrate = tsp->bitrate();
    if (bitrate > 0) {
        // Compute packet window size based on bitrate, round up one packet.
        const PacketCounter count = PacketDistance(bitrate, _window_ms) + 1;
        tsp->verbose(u"bitrate analysis window size: %'d packets", {count});
        return size_t(count);
    }
    else {
        tsp->warning(u"bitrate is unknown in start phase, using the default window size (%'d packets)", {DEFAULT_PACKET_WINDOW});
        return size_t(DEFAULT_PACKET_WINDOW);
    }
}


//----------------------------------------------------------------------------
// Individual packet processing method. Call in fixed proportion mode.
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::ReducePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    assert(_fixed_inpkt > 0);
    assert(_fixed_rempkt > 0);

    if (tsp->pluginPackets() % _fixed_inpkt == 0) {
        // It is time to remove packets
        if (_pkt_to_remove > 2 * _fixed_rempkt) {
            // Overflow, we did not find enough stuffing packets to remove
            tsp->verbose(u"overflow: failed to remove %'d packets", {_pkt_to_remove});
        }
        _pkt_to_remove += _fixed_rempkt;
    }

    if (pkt.getPID() == PID_NULL && _pkt_to_remove > 0) {
        _pkt_to_remove--;
        return TSP_DROP;
    }
    else {
        return TSP_OK;
    }
}


//----------------------------------------------------------------------------
// Packet processing method. Called in bitrate adaptive mode.
//----------------------------------------------------------------------------

size_t ts::ReducePlugin::processPacketWindow(TSPacketWindow& win)
{
    assert(_target_bitrate > 0);

    // Get the input bitrate. Start with user-specified input bitrate.
    BitRate bitrate = _input_bitrate;
    if (bitrate == 0) {
        // No user-specified input bitrate, use current from tsp.
        bitrate = tsp->bitrate();
    }
    if (_pcr_based) {
        // Compute local bitrate from PCR's in packet window.
        bitrate = computeBitRate(win);
    }

    // Save bitrates for next packet window.
    if (bitrate > 0) {
        // Got a valid bitrate for this packet window.
        _previous_bitrate = bitrate;
        _error = Error::NONE;
    }
    else if (_previous_bitrate > 0) {
        // Could not get a bitrate this time, use same as previous.
        bitrate = _previous_bitrate;
        // Report this error once, not continuously.
        if (_error != Error::USE_PREVIOUS) {
            _error = Error::USE_PREVIOUS;
            tsp->warning(u"cannot get bitrate from packet window, using previous bitrate: %'d b/s", {bitrate});
        }
    }
    else {
        // No previous nor current bitrate, cannot do anything, let all packets pass.
        // Report this error once, not continuously.
        if (_error != Error::NO_BITRATE) {
            _error = Error::NO_BITRATE;
            tsp->warning(u"unknown bitrate, letting all packets pass");
        }
        return win.size();
    }

    // Cannot reduce less than input bitrate.
    if (bitrate < _target_bitrate) {
        // Report this error once, not continuously.
        if (_error != Error::LOW_BITRATE && _error != Error::USE_PREVIOUS) {
            _error = Error::LOW_BITRATE;
            tsp->warning(u"bitrate lower than target one, letting all packets pass");
        }
        return win.size();
    }

    // Bitrate to remove.
    const BitRate removed_bitrate = bitrate - _target_bitrate;

    // Compute how many bits should be removed from this window: window-size-in-bits * removed-bitrate / total-bitrate.
    // However, when BitRate is implemented as a fixed-point types, there is a risk of intermediate arithmetic overflow,
    // even on 64 bits for bitrate. This has been seen for window size of 30,000 packets and 45 Mb/s bitrate reduction.
    // To solve this, we compute a "sub-window size" which can be computed in bits without overflow. We start by
    // sub-window-size = window-size. In case of overflow, we use half size and iterate. This problem does not exist
    // with fractions instead of fixed-point.
    size_t subwin_size = win.size();
    bool overflow = true;
    while (overflow && subwin_size > 16) {
        const size_t subwin_bits = subwin_size * PKT_SIZE_BITS;
        overflow = removed_bitrate.mulOverflow(subwin_bits) || (removed_bitrate * subwin_bits).divOverflow(bitrate);
        if (overflow) {
            subwin_size /= 2;
        }
    }

    // Loop on each sub-window inside the window.
    size_t subwin_start = 0;
    while (subwin_start < win.size()) {

        // Reduce size of last sub-window.
        subwin_size = std::min(subwin_size, win.size() - subwin_start);

        // Compute how many bits should be removed from this sub-window and add them to remaining late bits.
        _bits_to_remove += (((subwin_size * PKT_SIZE_BITS) * removed_bitrate) / bitrate).toInt();

        // Remove as many packets as possible, regularly spaced over the packet sub-window.
        // We proceed in several passes. In each pass, we process equally-sized slices of the buffer.
        // In each slice, we remove at most one null packet. If there is at least one null packet per
        // slice, one pass is enough. Otherwise, re-iterate with larger slices for remaining packets
        // to remove. Stop when all required packets are removed or there is no more null packet in
        // the packet window.
        // To be improved: For drastic reduction, there are so many packets to remove than the slice
        // size is just one packet. Then, in each window, all removed null packets are at the beginning
        // of the window and the remaining null packets are at the end of the window. Is this a problem?
        size_t null_count = 1; // dummy non-null initial value
        size_t pass_count = 0;
        while (_bits_to_remove >= PKT_SIZE_BITS && null_count > 0) {
            // Number of null packets we would like to remove in this pass.
            size_t pkt_count = std::min(subwin_size, size_t(_bits_to_remove / PKT_SIZE_BITS));
            // Size of a slice, where one packet should be removed.
            const size_t slice_size = subwin_size / pkt_count;
            // Number of remaining null packets after this pass.
            null_count = 0;
            // In each slice, check if a packet was already dropped.
            bool slice_done = false;
            // Count passes.
            pass_count++;
            tsp->log(3, u"pass #%d, packets to remove: %'d, slice size: %'d packets", {pass_count, pkt_count, slice_size});
            // Perform the pass over the packet window.
            for (size_t i = 0; i < subwin_size && pkt_count > 0; ++i) {
                // Reset at start of slice.
                if (i % slice_size == 0) {
                    slice_done = false;
                }
                // Null packets are either dropped (first one in slice) or counted.
                if (win.isNullPacket(subwin_start + i)) {
                    if (slice_done) {
                        null_count++;
                    }
                    else {
                        slice_done = true;
                        win.drop(subwin_start + i);
                        pkt_count--;
                        assert(_bits_to_remove >= PKT_SIZE_BITS);
                        _bits_to_remove -= PKT_SIZE_BITS;
                    }
                }
            }
        }
        tsp->log(2, u"subwindow size: %'d packets, number of passes: %d, remaining null: %'d, remaining bits: %'d", {subwin_size, pass_count, null_count, _bits_to_remove});

        // Iterate to next sub-window.
        subwin_start += subwin_size;
    }

    // Report overflow if not enough null packets were found in the window.
    if (_bits_to_remove >= PKT_SIZE_BITS) {
        if (_error != Error::PKT_OVERFLOW) {
            _error = Error::PKT_OVERFLOW;
            tsp->error(u"overflow, late by %'d packets", {_bits_to_remove / PKT_SIZE_BITS});
        }
    }
    else {
        if (_error == Error::PKT_OVERFLOW) {
            _error = Error::NONE;
        }
    }
    return win.size();
}


//----------------------------------------------------------------------------
// Compute bitrate in a packet window.
//----------------------------------------------------------------------------

ts::BitRate ts::ReducePlugin::computeBitRate(const TSPacketWindow& win) const
{
    // Use a PCR analyzer. Need at least one PID with only 2 PCR's.
    PCRAnalyzer pa(1, 2);

    // Pass all packets in the window to the PCR analyzer.
    for (size_t i = 0; i < win.size(); ++i) {
        const TSPacket* pkt = win.packet(i);
        if (pkt != nullptr) {
            // Pass null packet when this packet is not in a reference PID.
            // We maintain the bitrate while avoiding excluded PID's with PCR's.
            pa.feedPacket(_pcr_pids.test(pkt->getPID()) ? *pkt : NullPacket);
        }
    }

    // Return extracted bitrate or zero if none found.
    return pa.bitrate188();
}
