//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPCRAdjustPlugin.h"
#include "tsPluginRepository.h"
#include "tsPAT.h"
#include "tsPMT.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"pcradjust", ts::PCRAdjustPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PCRAdjustPlugin::PCRAdjustPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Adjust PCR's according to a constant bitrate", u"[options]")
{
    option<BitRate>(u"bitrate", 'b');
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

    option<cn::milliseconds>(u"min-ms-interval");
    help(u"min-ms-interval",
         u"Specify the minimum interval between two PCR's in milliseconds. "
         u"On a given PID, if the interval between two PCR's is larger than the minimum, "
         u"the next null packet will be replaced with an empty packet with a PCR for that PID.");

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
    getValue(_user_bitrate, u"bitrate");
    _ignore_dts = present(u"ignore-dts");
    _ignore_pts = present(u"ignore-pts");
    _ignore_scrambled = present(u"ignore-scrambled");
    PCR min_pcr;
    getChronoValue(min_pcr, u"min-ms-interval");
    _min_pcr_interval = min_pcr.count();
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PCRAdjustPlugin::start()
{
    // Reset packet processing.
    _pid_contexts.clear();

    // Reset demux for service analysis.
    _demux.reset();
    _demux.addPID(PID_PAT);
    return true;
}


//----------------------------------------------------------------------------
// Retrieve the last updated PCR. INVALID_PCR if unknown.
//----------------------------------------------------------------------------

uint64_t ts::PCRAdjustPlugin::PIDContext::lastPCR() const
{
    if (last_updated_pcr != INVALID_PCR && (last_created_pcr == INVALID_PCR || last_created_packet < last_pcr_packet)) {
        // The most recent is an original packet with a previous PCR.
        return last_updated_pcr;
    }
    else if (last_created_pcr != INVALID_PCR && (last_updated_pcr == INVALID_PCR || last_pcr_packet < last_created_packet)) {
        // The most recent is a PCR we created in a null packet.
        return last_created_pcr;
    }
    else {
        // No previous PCR was found.
        return INVALID_PCR;
    }
}


//----------------------------------------------------------------------------
// Compute the theoretical updated PCR at the given packet index.
//----------------------------------------------------------------------------

uint64_t ts::PCRAdjustPlugin::PIDContext::updatedPCR(PacketCounter packet_index, const BitRate& bitrate) const
{
    if (last_updated_pcr != INVALID_PCR && (last_created_pcr == INVALID_PCR || last_created_packet < last_pcr_packet)) {
        // The most recent is an original packet with a previous PCR.
        return PCRTraits::Next(last_updated_pcr, packet_index - last_pcr_packet, bitrate);
    }
    else if (last_created_pcr != INVALID_PCR && (last_updated_pcr == INVALID_PCR || last_pcr_packet < last_created_packet)) {
        // The most recent is a PCR we created in a null packet.
        return PCRTraits::Next(last_created_pcr, packet_index - last_created_packet, bitrate);
    }
    else {
        // No previous PCR was found, no reference.
        return INVALID_PCR;
    }
}


//----------------------------------------------------------------------------
// Compute an updated PTS or DTS at the given packet index.
//----------------------------------------------------------------------------

uint64_t ts::PCRAdjustPlugin::PIDContext::updatedPDTS(PacketCounter packet_index, const BitRate& bitrate, uint64_t original_pdts)
{
    // If the PCR PID is unknown, we cannot compute anything and keep the original PTS/DTS.
    if (pcr_ctx == nullptr) {
        return original_pdts;
    }

    // Estimated updated PCR for the current packet:
    const uint64_t updated_pcr = pcr_ctx->updatedPCR(packet_index, bitrate);
    if (updated_pcr == INVALID_PCR) {
        // There is no PCR found yet in the PCR PID, cannot compute a new PTS/DTS.
        return original_pdts;
    }

    // Check if the PTS/DTS and the PCR are still more or less synchronous.
    if (sync_pdts) {
        // Difference between the PTS/DTS and the PCR, in PTS units.
        const uint64_t diff = std::abs(int64_t(original_pdts) - int64_t(updated_pcr / SYSTEM_CLOCK_SUBFACTOR));
        // If the difference between the PTS/DTS and the PCR is less than 10 second, we are still sync.
        // Take in account the case where there is a wrapup at PTS_DTS_SCALE.
        const uint64_t max_diff = SYSTEM_CLOCK_SUBFREQ * 10;
        sync_pdts = diff < max_diff || diff > PTSTraits::SCALE - max_diff;
    }

    if (sync_pdts) {
        // The difference between the PTS/DTS and the PCR remains small, keep the original PTS/DTS.
        return original_pdts;
    }
    else {
        // The difference between the PTS/DTS and the PCR is too high, update the PTS/DTS.
        // First, compute the original PCR for this packet:
        const uint64_t original_pcr = PCRTraits::Next(pcr_ctx->last_original_pcr, packet_index - pcr_ctx->last_pcr_packet, bitrate);

        // Compute the difference between the original PTS and the original PCR.
        const int64_t diff = int64_t(original_pdts) - int64_t(original_pcr / SYSTEM_CLOCK_SUBFACTOR);

        // Apply the same difference, relative to the updated PCR.
        // WARNING: This is likely not a correct value because the placement of the TS packet may have changed.
        // If anyone has a better idea for the new PTS value, please suggest.
        return uint64_t(int64_t(updated_pcr / SYSTEM_CLOCK_SUBFACTOR) + diff);
    }
}


//----------------------------------------------------------------------------
// Get the context for a PID.
//----------------------------------------------------------------------------

ts::PCRAdjustPlugin::PIDContextPtr ts::PCRAdjustPlugin::getContext(PID pid)
{
    const PIDContextPtr ptr = _pid_contexts[pid];
    return ptr == nullptr ? (_pid_contexts[pid] = std::make_shared<PIDContext>(pid)) : ptr;
}


//----------------------------------------------------------------------------
// TableHandlerIntrface implementation.
//----------------------------------------------------------------------------

void ts::PCRAdjustPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_PAT: {
            const PAT pat(duck, table);
            if (pat.isValid()) {
                // Add all PMT PID's to the demux to grab all PMT's.
                for (const auto& it : pat.pmts) {
                    _demux.addPID(it.second);
                }
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(duck, table);
            if (pmt.isValid() && pmt.pcr_pid != PID_NULL) {
                // Remember PCR PID for all components.
                for (const auto& it : pmt.streams) {
                    getContext(it.first)->pcr_ctx = getContext(pmt.pcr_pid);
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
// Packet processing method
//----------------------------------------------------------------------------

ts::PacketProcessStatus ts::PCRAdjustPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Pass all packets to the demux.
    _demux.feedPacket(pkt);

    // Get PID context.
    const PID pid = pkt.getPID();
    const PIDContextPtr ctx(getContext(pid));
    const PacketCounter current_packet = tsp->pluginPackets();

    // Keep track of scrambled PID's (or which contain at least one scrambled packet).
    if (pkt.isScrambled()) {
        ctx->scrambled = true;
    }

    // Keep track of last continuity counter in case we have to create an empty packet with PCR later.
    ctx->last_cc = pkt.getCC();

    // Get reference bitrate value (cannot do anything if zero).
    const BitRate bitrate = _user_bitrate != 0 ? _user_bitrate : tsp->bitrate();

    // Only process packets from selected PID's (all by default).
    if (bitrate != 0 && _pids.test(pid) && (!ctx->scrambled || !_ignore_scrambled)) {

        // Process PCR.
        if (pkt.hasPCR()) {
            // The PID is its own PCR reference.
            ctx->pcr_ctx = ctx;
            ctx->last_original_pcr = pkt.getPCR();

            if (ctx->last_updated_pcr == INVALID_PCR) {
                // First packet in this PID with a PCR, use it as base.
                debug(u"starting fixing PCR in PID %n", pid);
                ctx->last_updated_pcr = ctx->last_original_pcr;
            }
            else {
                // A previous PCR value was known in the PID. Compute the new PCR from the previous one.
                const uint64_t pcr = ctx->updatedPCR(current_packet, bitrate);
                pkt.setPCR(pcr);
                ctx->last_updated_pcr = pcr;
            }
            ctx->last_pcr_packet = current_packet;
        }

        // Process PTS.
        if (!_ignore_pts && pkt.hasPTS()) {
            pkt.setPTS(ctx->updatedPDTS(current_packet, bitrate, pkt.getPTS()));
        }

        // Process DTS.
        if (!_ignore_dts && pkt.hasDTS()) {
            pkt.setDTS(ctx->updatedPDTS(current_packet, bitrate, pkt.getDTS()));
        }
    }

    // Replace null packets with an empty packet containing a PCR when necessary.
    if (_min_pcr_interval > 0 && pid == PID_NULL && bitrate != 0) {

        // Look for PID's with PCR for which the PCR are outdated.
        // Keep the "most urgent" PID, ie. the one which is the most late.
        PIDContextPtr pcr_ctx;
        uint64_t pcr_delay = 0;
        uint64_t pcr_value = INVALID_PCR;
        for (const auto& it : _pid_contexts) {
            const PIDContextPtr& cur_ctx(it.second);
            // Consider only PID's which contain PCR, ie. which are their own PCR reference.
            if (cur_ctx != nullptr && cur_ctx->pcr_ctx != nullptr && cur_ctx->pid == cur_ctx->pcr_ctx->pid) {
                const uint64_t last_pcr = cur_ctx->lastPCR();
                const uint64_t updated_pcr = cur_ctx->updatedPCR(current_packet, bitrate);
                if (last_pcr != INVALID_PCR && updated_pcr != INVALID_PCR && updated_pcr > last_pcr) {
                    const uint64_t delay = updated_pcr - last_pcr;
                    if (delay > _min_pcr_interval && delay > pcr_delay) {
                        // This is the "most late" PID so far.
                        pcr_ctx = cur_ctx;
                        pcr_delay = delay;
                        pcr_value = updated_pcr;
                    }
                }
            }
        }

        // Create an empty packet if a PID is late.
        if (pcr_ctx != nullptr) {
            debug(u"adding PCR in PID %n", pcr_ctx->pid);

            // Build an empty packet with a PCR.
            pkt = EmptyPacket;
            pkt.setPID(pcr_ctx->pid);
            pkt.setCC(pcr_ctx->last_cc); // Don't increment CC since there is no payload
            pkt.setPCR(pcr_value, true);

            // Remember we inserted the packet.
            pcr_ctx->last_created_pcr = pcr_value;
            pcr_ctx->last_created_packet = current_packet;
        }
    }

    return TSP_OK;
}
