//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  Adjust PCR values.
//
//  The adjustment works well with a constant bitrate. With variable bitrate,
//  the adjustment of PCR values can be counter-productive.
//
//  The other issue is the adjustment of PTS and DTS. If the PCR adjustment
//  only fixes the jitter of poorly muxed packets, the PCR and DTS shall
//  not be modified: the placement of packets in the TS has changed and their
//  clock time, relative to the system clock, shall be adjusted. But the
//  frames shall be decoded and presented at the same time as before, relative
//  to the reference system clock.
//
//  On the other hand, if the old PCR abruptly changes (because of a TS file
//  being looped for instance), then the PTS and DTS must be adjusted.
//  Otherwise, the decoding is completely out of sync from the system clock.
//
//  So, there are two difficulties:
//
//  1) When shall we adjust the PTS/DTS and when shall we keep them untouched?
//     --> When the difference between the PTS/DTS and the adjusted PCR
//         remains small (less than 500 ms), we assume that the PCR adjustment
//         was only the result of packet placement (bad muxing), we assume
//         that the reference clock is still valid and we keep the original
//         values of PTS/DTS.
//
//  2) When the PCR changed abruptly and the PTS/DTS must be adjusted, by
//     which amount shall we adjust the PTS/DTS?
//     --> We first compute the theoretical original PCR of the packet
//         containing the PTS/DTS (if the packet does not contain a PCR).
//         We compute the original difference between PCR and PTS/DTS.
//         Then we apply this difference to the adjusted PCR and we modify
//         the PTS/DTS accordingly.
//     Note: this is not ideal since it does not solve the problem of
//         simultaneous modified packet placement (bad muxin/merging) and
//         PCR warp (file loop for instance).
//  
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSafePtr.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PCRAdjustPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        PCRAdjustPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // Description of one PID.
        class PIDContext
        {
        public:
            PIDContext();                   // Constructor.
            PID           pcrPID;           // Associated PCR PID (if has PTS/DTS but no PCR).
            bool          scrambled;        // The PID contains scrambled packets.
            bool          syncPDTS;         // PTS and DTS are still synchronous with the PCR, do not modify them.
            uint64_t      lastOriginalPCR;  // Last PCR value, after modification.
            uint64_t      lastUpdatedPCR;   // Last PCR value, after modification.
            PacketCounter pktLastPCR;       // Last PCR packet index.
        };

        // Map of safe pointers to PID contexts, indexed by PID.
        typedef SafePtr<PIDContext> PIDContextPtr;
        typedef std::map<PID, PIDContextPtr> PIDContextMap;

        // PCRAdjustPlugin private members
        BitRate       _userBitrate;      // User-specified bitrate.
        PIDSet        _pids;             // User-specified list of PIDs.
        bool          _ignoreDTS;        // Do not modify DTS values.
        bool          _ignorePTS;        // Do not modify PTS values.
        bool          _ignoreScrambled;  // Do not modify scrambled PID's.
        PacketCounter _pktCount;         // Current packet counter.
        SectionDemux  _demux;            // Section demux to get service descriptions.
        PIDContextMap _pidContexts;      // Map of all PID contexts.

        // TableHandlerInterface implementation.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Get the context for a PID. Create one when necessary.
        PIDContextPtr getContext(PID pid);

        // Compute an updated PTS or DTS for the current packet.
        uint64_t updatedPDTS(const PIDContextPtr& ctx, uint64_t originalPDTS, BitRate bitrate);

        // Inaccessible operations
        PCRAdjustPlugin() = delete;
        PCRAdjustPlugin(const PCRAdjustPlugin&) = delete;
        PCRAdjustPlugin& operator=(const PCRAdjustPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(pcradjust, ts::PCRAdjustPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PCRAdjustPlugin::PCRAdjustPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Adjust PCR's according to a constant bitrate", u"[options]"),
    _userBitrate(0),
    _pids(),
    _ignoreDTS(false),
    _ignorePTS(false),
    _ignoreScrambled(false),
    _pktCount(0),
    _demux(duck, this),
    _pidContexts()
{
    option(u"bitrate", 'b', POSITIVE);
    help(u"bitrate",
         u"Specify a constant bitrate for the transport stream. "
         u"The PCR values will be adjusted according to this bitrate. "
         u"By default, use the input bitrate as reported by the input device or a previous plugin.");

    option(u"ignore-dts");
    help(u"ignore-dts",
         u"Do not modify DTS (decoding time stamps) values. "
         u"By default, the DTS are modified according to the PCR adjustment.");

    option(u"ignore-pts");
    help(u"ignore-pts",
         u"Do not modify PTS (presentation time stamps) values. "
         u"By default, the PTS are modified according to the PCR adjustment.");

    option(u"ignore-scrambled");
    help(u"ignore-scrambled",
         u"Do not modify PCR values on PID's containing scrambled packets. "
         u"By default, on scrambled PID's, the PCR's are modified but not the PTS and DTS since they are scrambled. "
         u"This may result in problems when playing video and audio.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Specifies PID's where PCR, DTS and PTS values shall be adjusted. "
         u"By default, all PID's are modified. Several --pid options may be specified.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::PCRAdjustPlugin::getOptions()
{
    getIntValues(_pids, u"pid", true);
    _userBitrate = intValue<BitRate>(u"bitrate");
    _ignoreDTS = present(u"ignore-dts");
    _ignorePTS = present(u"ignore-pts");
    _ignoreScrambled = present(u"ignore-scrambled");
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PCRAdjustPlugin::start()
{
    // Reset packet processing.
    _pktCount = 0;
    _pidContexts.clear();

    // Reset demux for service analysis.
    _demux.reset();
    _demux.addPID(PID_PAT);
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PCRAdjustPlugin::stop()
{
    return true;
}


//----------------------------------------------------------------------------
// Description of one PID.
//----------------------------------------------------------------------------

ts::PCRAdjustPlugin::PIDContext::PIDContext() :
    pcrPID(PID_NULL),
    scrambled(false),
    syncPDTS(true),
    lastOriginalPCR(INVALID_PCR),
    lastUpdatedPCR(INVALID_PCR),
    pktLastPCR(0)
{
}


//----------------------------------------------------------------------------
// Get the context for a PID.
//----------------------------------------------------------------------------

ts::PCRAdjustPlugin::PIDContextPtr ts::PCRAdjustPlugin::getContext(PID pid)
{
    const PIDContextPtr ptr = _pidContexts[pid];
    return ptr.isNull() ? (_pidContexts[pid] = new PIDContext()) : ptr;
}


//----------------------------------------------------------------------------
// TableHandlerInterface implementation.
//----------------------------------------------------------------------------

void ts::PCRAdjustPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_PAT: {
            const PAT pat(duck, table);
            if (pat.isValid()) {
                // Add all PMT PID's to the demux to grab all PMT's.
                for (auto it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                    _demux.addPID(it->second);
                }
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(duck, table);
            if (pmt.isValid()) {
                // Add all component PID's.
                for (auto it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
                    const PIDContextPtr ctx = getContext(it->first);
                    ctx->pcrPID = pmt.pcr_pid;
                }
            }
            break;
        }
        default: {
            // Ignore other tables.
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Compute an updated PTS or DTS for the current packet.
//----------------------------------------------------------------------------

uint64_t ts::PCRAdjustPlugin::updatedPDTS(const PIDContextPtr& ctx, uint64_t originalPDTS, BitRate bitrate)
{
    // If the PCR PID is unknown, we cannot compute anything and keep the original PTS/DTS.
    if (ctx->pcrPID == PID_NULL) {
        return originalPDTS;
    }

    // Get the context of the PCR PID.
    const PIDContextPtr ctxPCR(getContext(ctx->pcrPID));
    if (ctxPCR->lastUpdatedPCR == INVALID_PCR) {
        // There is no PCR found yet in the PCR PID, cannot compute a new PTS/DTS.
        return originalPDTS;
    }

    // Estimated updated PCR for the current packet:
    assert(ctxPCR->pktLastPCR <= _pktCount);
    const uint64_t updatedPCR = NextPCR(ctxPCR->lastUpdatedPCR, _pktCount - ctxPCR->pktLastPCR, bitrate);

    // Check if the PTS/DTS and the PCR are still more or less synchronous.
    if (ctx->syncPDTS) {
        // Difference between the PTS/DTS and the PCR, in PTS units.
        const int64_t diff = int64_t(originalPDTS) - int64_t(updatedPCR / SYSTEM_CLOCK_SUBFACTOR);
        // If the difference between the PTS/DTS and the PCR is less than 1/2 second, we are still sync.
        ctx->syncPDTS = std::abs(diff) < SYSTEM_CLOCK_SUBFREQ;
    }

    if (ctx->syncPDTS) {
        // The difference between the PTS/DTS and the PCR remains small, keep the original PTS/DTS.
        return originalPDTS;
    }
    else {
        // The difference between the PTS/DTS and the PCR is too high, update the PTS/DTS.
        // First, compute the original PCR for this packet:
        const uint64_t originalPCR = NextPCR(ctxPCR->lastOriginalPCR, _pktCount - ctxPCR->pktLastPCR, bitrate);

        // Compute the difference between the original PTS and the original PCR.
        const int64_t diff = int64_t(originalPDTS) - int64_t(originalPCR / SYSTEM_CLOCK_SUBFACTOR);

        // Apply the same difference, relative to the updated PCR.
        // WARNING: This is likely not a correct value because the placement of the TS packet may have changed.
        // If anyone has a better idea for the new PTS value, please suggest.
        return uint64_t(int64_t(updatedPCR / SYSTEM_CLOCK_SUBFACTOR) + diff);
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PCRAdjustPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Pass all packets to the demux.
    _demux.feedPacket(pkt);

    // Get PID context.
    const PID pid = pkt.getPID();
    const PIDContextPtr ctx(getContext(pid));

    // Keep track of scrambled PID's (or which contain at least one scrambled packet).
    if (pkt.isScrambled()) {
        ctx->scrambled = true;
    }

    // Get reference bitrate value (cannot do anything if zero).
    const BitRate bitrate = _userBitrate != 0 ? _userBitrate : tsp->bitrate();

    // Only process packets from selected PID's (all by default).
    if (bitrate != 0 && _pids.test(pid) && (!ctx->scrambled || !_ignoreScrambled)) {

        // Process PCR.
        if (pkt.hasPCR()) {
            // The PID is its own PCR reference.
            ctx->pcrPID = pid;
            ctx->lastOriginalPCR = pkt.getPCR();

            if (ctx->lastUpdatedPCR == INVALID_PCR) {
                // First packet in this PID with a PCR, use it as base.
                tsp->debug(u"starting fixing PCR in PID 0x%X (%d)", {pid, pid});
                ctx->lastUpdatedPCR = ctx->lastOriginalPCR;
            }
            else {
                // A previous PCR value was known in the PID. Compute the new PCR from the previous one.
                assert(_pktCount > ctx->pktLastPCR);
                const uint64_t pcr = NextPCR(ctx->lastUpdatedPCR, _pktCount - ctx->pktLastPCR, bitrate);
                pkt.setPCR(pcr);
                ctx->lastUpdatedPCR = pcr;
            }
            ctx->pktLastPCR = _pktCount;
        }

        // Process PTS.
        if (!_ignorePTS && pkt.hasPTS()) {
            pkt.setPTS(updatedPDTS(ctx, pkt.getPTS(), bitrate));
        }

        // Process DTS.
        if (!_ignoreDTS && pkt.hasDTS()) {
            pkt.setDTS(updatedPDTS(ctx, pkt.getDTS(), bitrate));
        }
    }

    // Count packets.
    _pktCount++;

    return TSP_OK;
}
