//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  Bitrate limiter.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsMonotonic.h"
#include "tsPAT.h"
#include "tsPMT.h"
TSDUCK_SOURCE;

#define DEFAULT_THRESHOLD1 10
#define DEFAULT_THRESHOLD2 100
#define DEFAULT_THRESHOLD3 1000


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class LimitPlugin:
        public ProcessorPlugin,
        private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        LimitPlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // Context per PID in the TS.
        struct PIDContext;
        typedef SafePtr<PIDContext, NullMutex> PIDContextPtr;
        typedef std::map<PID, PIDContextPtr> PIDContextMap;

        // Plugin fields.
        bool          _useWallClock;
        BitRate       _maxBitrate;
        PacketCounter _threshold1;
        PacketCounter _threshold2;
        PacketCounter _threshold3;
        PacketCounter _thresholdAV;   // Threshold for audio/video packets.
        BitRate       _curBitrate;    // Instant bitrate (between to consecutive PCR).
        PacketCounter _currentPacket; // Total number of packets so far in the TS.
        PacketCounter _excessPoint;   // Last packet from which we computed excess packets.
        PacketCounter _excessPackets; // Number of packets in excess (to drop).
        PacketCounter _excessBits;    // Number of bits in excess, in addition to packets.
        PIDSet        _pids1;         // PIDs to sacrifice at threshold 1.
        SectionDemux  _demux;         // Demux to collect PAT and PMT's.
        PIDContextMap _pidContexts;   // One context per PID in the TS.
        Monotonic     _clock;         // Monotonic clock for live streams.
        size_t        _bitsSecond;    // Number of bits in current second.

        // Context per PID in the TS.
        struct PIDContext
        {
            // Constructor.
            PIDContext() = delete;
            PIDContext(PID pid);

            PID           pid;         // PID value.
            bool          psi;         // The PID contains PSI/SI.
            bool          video;       // The PID contains video.
            bool          audio;       // The PID contains audio.
            uint64_t      pcrValue;    // Last PCR value.
            PacketCounter pcrPacket;   // Global packet index for pcrValue.
            PacketCounter dropCount;   // Number of dropped packets in this PID.
        };

        // Get or create the context for a PID.
        PIDContextPtr getContext(PID pid);

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;

        // Add bits in excess in counters.
        void addExcessBits(uint64_t bits);

        // Inaccessible operations
        LimitPlugin() = delete;
        LimitPlugin(const LimitPlugin&) = delete;
        LimitPlugin& operator=(const LimitPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(limit, ts::LimitPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::LimitPlugin::LimitPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Limit the global bitrate by dropping packets", u"[options]"),
    _useWallClock(false),
    _maxBitrate(0),
    _threshold1(0),
    _threshold2(0),
    _threshold3(0),
    _thresholdAV(0),
    _curBitrate(0),
    _currentPacket(0),
    _excessPoint(0),
    _excessPackets(0),
    _excessBits(0),
    _pids1(),
    _demux(this),
    _pidContexts(),
    _clock(),
    _bitsSecond(0)
{
    setIntro(u"This plugin limits the global bitrate of the transport stream. "
             u"Packets are dropped when necessary to maintain the overall bitrate "
             u"below a given maximum. The bitrate is computed from PCR's (the default) "
             u"or from the processing wall clock time.\n\n"
             u"Packets are not dropped randomly. Some packets are more likely to be "
             u"dropped than others. When the bitrate exceeds the maximum, the number "
             u"of packets in excess is permanently recomputed. The type of packets "
             u"to drop depends on the number of packets in excess. There are several "
             u"thresholds which are specified by the corresponding options:\n\n"
             u"- Below --threshold1, only null packets are dropped.\n"
             u"- Below --threshold2, if --pid options are specified, packets from "
             u"the specified PID's are dropped (except packets containing a PUSI or a PCR).\n"
             u"- Below --threshold3, packets from any video or audio PID are dropped "
             u"(except packets containing a PUSI or a PCR).\n"
             u"- Above the last threshold, any packet can be dropped.\n");

    option(u"bitrate", 'b', INTEGER, 1, 1, 100, UNLIMITED_VALUE);
    help(u"bitrate",
         u"Limit the overall bitrate of the transport stream to the specified value "
         u"in bits/second. This is a mandatory option, there is no default.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Specify PID's the content of which can be dropped when the maximum bitrate "
         u"is exceeded. Several --pid options can be specified.");

    option(u"threshold1", '1', UINT32);
    help(u"threshold1",
         u"Specify the first threshold for the number of packets in excess. "
         u"The default is " TS_STRINGIFY(DEFAULT_THRESHOLD1) u" packets.");

    option(u"threshold2", '2', UINT32);
    help(u"threshold2",
         u"Specify the second threshold for the number of packets in excess. "
         u"The default is " TS_STRINGIFY(DEFAULT_THRESHOLD2) u" packets.");

    option(u"threshold3", '3', UINT32);
    help(u"threshold3",
         u"Specify the third threshold for the number of packets in excess. "
         u"The default is " TS_STRINGIFY(DEFAULT_THRESHOLD3) u" packets.");

    option(u"wall-clock", 'w');
    help(u"wall-clock",
         u"Compute bitrates based on real wall-clock time. The option is meaningful "
         u"with live streams only. By default, compute bitrates based on PCR's.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::LimitPlugin::start()
{
    // Get option values
    _useWallClock = present(u"wall-clock");
    _maxBitrate = intValue<BitRate>(u"bitrate");
    _threshold1 = intValue<PacketCounter>(u"threshold1", DEFAULT_THRESHOLD1);
    _threshold2 = intValue<PacketCounter>(u"threshold2", DEFAULT_THRESHOLD2);
    _threshold3 = intValue<PacketCounter>(u"threshold3", DEFAULT_THRESHOLD3);
    getIntValues(_pids1, u"pid");

    // Threshold for audio/video packets. If a list of --pid is specified, we start
    // dropping a/v at --threshold2 only. But, without any --pid, we start at --threshold1.
    _thresholdAV = _pids1.any() ? _threshold3 : _threshold2;

    tsp->debug(u"threshold 1: %'d, threshold 2: %'d, threshold 3: %'d, audio/video threshold: %'d", {_threshold1, _threshold2, _threshold3, _thresholdAV});

    // Reset plugin state.
    _currentPacket = 0;
    _bitsSecond = 0;
    _excessPoint = 0;
    _excessPackets = 0;
    _excessBits = 0;
    _curBitrate = 0;
    _pidContexts.clear();
    _demux.reset();
    _demux.setPIDFilter(PID_PAT);

    return true;
}


//----------------------------------------------------------------------------
// Get or create the context for a PID.
//----------------------------------------------------------------------------

ts::LimitPlugin::PIDContextPtr ts::LimitPlugin::getContext(PID pid)
{
    PIDContextMap::iterator it = _pidContexts.find(pid);
    if (it != _pidContexts.end()) {
        return it->second;
    }
    else {
        PIDContextPtr pc(new PIDContext(pid));
        _pidContexts.insert(std::make_pair(pid, pc));
        return pc;
    }
}


//----------------------------------------------------------------------------
// Constructor for PID context.
//----------------------------------------------------------------------------

ts::LimitPlugin::PIDContext::PIDContext(PID pid_) :
    pid(pid_ <= PID_NULL ? pid_ : PID(PID_NULL)),
    psi(pid <= PID_DVB_LAST),
    video(false),
    audio(false),
    pcrValue(INVALID_PCR),
    pcrPacket(0),
    dropCount(0)
{
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
// Implementation of TableHandlerInterface.
//----------------------------------------------------------------------------

void ts::LimitPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_PAT: {
            const PAT pat(table);
            if (pat.isValid()) {
                // Collect all PMT PID's.
                for (auto it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                    const PID pid = it->second;
                    _demux.addPID(pid);
                    getContext(pid)->psi = true;
                    tsp->debug(u"Adding PMT PID 0x%X (%d)", {pid, pid});
                }
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(table);
            if (pmt.isValid()) {
                // Collect all component PID's.
                tsp->debug(u"Found PMT in PID 0x%X (%d)", {table.sourcePID(), table.sourcePID()});
                for (auto it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
                    const PID pid = it->first;
                    const PIDContextPtr pc(getContext(pid));
                    pc->audio = it->second.isAudio();
                    pc->video = it->second.isVideo();
                    tsp->debug(u"Found component PID 0x%X (%d)", {pid, pid});
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Add bits in excess in counters.
//----------------------------------------------------------------------------

void ts::LimitPlugin::addExcessBits(uint64_t bits)
{
    _excessBits += bits;
    _excessPackets += _excessBits / (8 * PKT_SIZE);
    _excessBits %= 8 * PKT_SIZE;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::LimitPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    Status status = TSP_OK;
    const PID pid = pkt.getPID();

    // Get system clock at first packet.
    if (_currentPacket == 0) {
        _clock.getSystemTime();
    }

    // Filter sections to process.
    _demux.feedPacket(pkt);

    // Get the PID context.
    const PIDContextPtr pc(getContext(pid));

    // Process bitrates.
    if (_useWallClock) {
        // Compute bitrates from wall clock.
        // Reset the monotonic clock every second.
        const NanoSecond duration = Monotonic(true) - _clock;
        if (duration >= NanoSecPerSec) {
            // More than one second elapsed, reset.
            _bitsSecond = 0;
            if (duration < 2 * NanoSecPerSec) {
                // Slightly more than 1 second, keep a monotonic behaviour.
                _clock += NanoSecPerSec;
            }
            else {
                // More than 1 second, probably a hole in broadcast, missed next
                // monotonic second => resync with current time.
                _clock += duration;
            }
        }

        // Accumulate packets in current second.
        _bitsSecond += PKT_SIZE_BITS;
        if (_bitsSecond > _maxBitrate) {
            // This packet is in excess, at least partially.
            const size_t excess = _bitsSecond - _maxBitrate;
            addExcessBits(excess < PKT_SIZE_BITS ? excess : PKT_SIZE_BITS);
        }
    }
    else if (pkt.hasPCR()) {
        // Compute bitrates from PCR's.
        // Process PCR in packet.
        const uint64_t pcr = pkt.getPCR();

        // Compute instant bitrate if the PID had a previous PCR.
        if (pc->pcrValue != INVALID_PCR && pc->pcrValue < pcr) {
            // We compute TS instant bitrate using only two consecutive PCR's
            // in one single PID. This can be not always precise. To be improved maybe.
            const BitRate newBitrate = BitRate(((_currentPacket - pc->pcrPacket) * 8 * PKT_SIZE * SYSTEM_CLOCK_FREQ) / (pcr - pc->pcrValue));

            // Report state change.
            if (_curBitrate > _maxBitrate && newBitrate <= _maxBitrate) {
                tsp->verbose(u"bitrate back to normal (%'d b/s)", {newBitrate});
            }
            else if (_curBitrate <= _maxBitrate && newBitrate > _maxBitrate) {
                tsp->verbose(u"bitrate exceeds maximum (%'d b/s), starting to drop packets", {newBitrate});
            }
            else if (_curBitrate != newBitrate && (_curBitrate > newBitrate ? _curBitrate - newBitrate : newBitrate - _curBitrate) > _curBitrate / 20) {
                // Report new bitrate when more than 5% change.
                tsp->debug(u"new bitrate: %'d b/s", {newBitrate});
            }

            // Save current bitrate.
            _curBitrate = newBitrate;

            if (_curBitrate <= _maxBitrate) {
                // Current bitrate is OK, no longer drop packets, even if a past excess is not yet absorbed.
                _excessPackets = 0;
                _excessBits = 0;
            }
            else {
                // The instant bitrate is too high.
                assert(_currentPacket > _excessPoint);
                assert(_curBitrate > 0);
                // Number of actual bits since the last "excess point":
                const uint64_t bits = (_currentPacket - _excessPoint) * 8 * PKT_SIZE;
                // Number of bits in excess, based on maximum bandwidth:
                addExcessBits((bits * (_curBitrate - _maxBitrate)) / _curBitrate);
                // Last time we computed the excess packets is remembered.
                _excessPoint = _currentPacket;
            }
        }

        // Remember last PCR.
        pc->pcrValue = pcr;
        pc->pcrPacket = _currentPacket;
    }

    // Decide to drop packet if needed.
    if (_excessPackets > 0) {
        // Packets with PCR or PUSI are more precious because they provide
        // synchronization to the receiver devices.
        const bool precious = pkt.hasPCR() || pkt.getPUSI();

        // Check all threshold to determine if the packet should be dropped.
        const bool drop =
            // Drop any packet above --threshold3.
            (_excessPackets >= _threshold3) ||
            // Drop non-precious audio/video packets above --threshold2 (or --threshold1 if there is no --pid).
            (_excessPackets >= _thresholdAV && !precious && (pc->audio || pc->video)) ||
            // Drop any --pid packet above --threshold1.
            (_excessPackets >= _threshold1 && !precious && _pids1.test(pid)) ||
            // Drop any null packet in all cases.
            (pid == PID_NULL);

        if (drop) {
            if (pc->dropCount++ == 0) {
                // First time we drop packets in this PID.
                tsp->verbose(u"starting to drop packets on PID 0x%X (%d)", {pid, pid});
            }
            _excessPackets--;
            status = TSP_DROP;
        }
    }

    // Count packets in input stream.
    _currentPacket++;

    return status;
}
