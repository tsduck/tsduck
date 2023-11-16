//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPCRMerger.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PCRMerger::PCRMerger(DuckContext& duck) :
    _duck(duck),
    _demux(duck, this)

{
    // Capture all PMT's from the merged stream.
    _demux.addFilteredTableId(TID_PMT);
}

ts::PCRMerger::PIDContext::PIDContext(PID p) :
    pid(p),
    pcr_pid(p)  // each PID is its own PCR PID until proven otherwise in a PMT
{
}


//----------------------------------------------------------------------------
// Reset all collected information.
//----------------------------------------------------------------------------

void ts::PCRMerger::reset()
{
    _demux.reset();
    _demux.addFilteredTableId(TID_PMT);
    _pid_ctx.clear();
}


//----------------------------------------------------------------------------
// Process one packet from the TS to merge.
//----------------------------------------------------------------------------

void ts::PCRMerger::processPacket(ts::TSPacket& pkt, ts::PacketCounter main_packet_index, const BitRate& main_bitrate)
{
    // Collect PMT's from the merged TS.
    _demux.feedPacket(pkt);

    // Collect information on this packet.
    const PID pid = pkt.getPID();
    const PIDContextPtr ctx(getContext(pid));
    const uint64_t pcr = pkt.getPCR();
    const uint64_t dts = pkt.getDTS();
    const uint64_t pts = pkt.getPTS();

    // The last DTS and PTS are stored for all PID's.
    if (dts != INVALID_DTS) {
        ctx->last_dts = dts;
        ctx->last_dts_pkt = main_packet_index;
    }
    if (pts != INVALID_PTS) {
        ctx->last_pts = pts;
        ctx->last_pts_pkt = main_packet_index;
    }

    // PCR's are stored, modified or reset.
    if (pcr == INVALID_PCR) {
        // No PCR, do nothing.
    }
    else if (ctx->last_pcr == INVALID_PCR) {
        // First time we see a PCR in this PID.
        // Save the initial PCR value but do not modify it.
        ctx->first_pcr = ctx->last_pcr = pcr;
        ctx->first_pcr_pkt = ctx->last_pcr_pkt = main_packet_index;
    }
    else if (main_bitrate > 0) {
        // This is not the first PCR in this PID.
        // Compute the transmission time since some previous PCR in PCR units.
        // We base the result on the main stream bitrate and the number of packets.
        // By default, compute PCR based on distance from first PCR.
        // On the long run, this is more precise on CBR but can be devastating on VBR.
        uint64_t base_pcr = ctx->first_pcr;
        PacketCounter base_pkt = ctx->first_pcr_pkt;
        if (_incremental_pcr) {
            // Compute PCR based in increment from the last one. Small errors may accumulate.
            base_pcr = ctx->last_pcr;
            base_pkt = ctx->last_pcr_pkt;
        }
        assert(base_pkt < main_packet_index);
        ctx->last_pcr = base_pcr + ((BitRate(main_packet_index - base_pkt) * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ) / main_bitrate).toInt();
        ctx->last_pcr_pkt = main_packet_index;

        // When --pcr-reset-backwards is specified, check if DTS or PTS have moved backwards PCR.
        // This may occur after slow drift in PCR restamping.
        bool update_pcr = true;
        if (_pcr_reset_backwards) {
            // Restamped PCR value in PTS/DTS units:
            const uint64_t subpcr = ctx->last_pcr / SYSTEM_CLOCK_SUBFACTOR;
            // Loop on all PID's which use current PID as PCR PID, searching for a reason not to update the PCR.
            for (auto it = _pid_ctx.begin(); update_pcr && it != _pid_ctx.end(); ++it) {
                if (it->second->pcr_pid == pid) {
                    // Extrapolated current PTS/DTS of this PID at current packet.
                    const uint64_t pdts = it->second->adjustedPDTS(main_packet_index, main_bitrate);
                    if (pdts != INVALID_DTS && (pdts <= subpcr || (pdts - subpcr) > SYSTEM_CLOCK_SUBFREQ)) {
                        // PTS/DTS moved backwards PCR or PCR is far behind PTS/DTS (more than one second).
                        // Reset PCR restamping.
                        update_pcr = false;
                        ctx->first_pcr = ctx->last_pcr = pcr;
                        ctx->first_pcr_pkt = ctx->last_pcr_pkt = main_packet_index;
                        _duck.report().verbose(u"resetting PCR restamping in PID 0x%X (%<d) after DTS/PTS moved backwards restamped PCR", {pid});
                    }
                }
            }
        }

        // Update the PCR in the packet if required.
        if (update_pcr) {
            // Compute the offset between the adjusted PCR (ctx->last_pcr) and the PCR from the packet (pcr).
            const SubSecond moved = SubSecond(ctx->last_pcr) - SubSecond(pcr);
            // If the jump is too high (1 second), there must be some discontinuity in the original PCR.
            if (std::abs(moved) >= SubSecond(SYSTEM_CLOCK_FREQ)) {
                // Too high, reset PCR adjustment.
                ctx->first_pcr = ctx->last_pcr = pcr;
                ctx->first_pcr_pkt = ctx->last_pcr_pkt = main_packet_index;
                _duck.report().verbose(u"resetting PCR restamping in PID 0x%X (%<d) after possible discontinuity in original PCR", {pid});
            }
            else {
                pkt.setPCR(ctx->last_pcr);
                // In debug mode, report the displacement of the PCR.
                // This may go back and forth around zero but should never diverge (--pcr-reset-backwards case).
                // Report it at debug level 2 only since it occurs on almost all merged packets with PCR.
                if (_duck.report().maxSeverity() >= 2) {
                    _duck.report().log(2, u"adjusted PCR by %+'d (%+'d ms) in PID 0x%X (%<d)", {moved, (moved * MilliSecPerSec) / SYSTEM_CLOCK_FREQ, pid});
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Receives all PMT's of all services in the merged stream.
//----------------------------------------------------------------------------

void ts::PCRMerger::handlePMT(const PMT& pmt, PID pid)
{
    _duck.report().debug(u"got PMT for service 0x%X (%<d), PMT PID 0x%X (%<d), PCR PID 0x%X (%<d)", {pmt.service_id, pid, pmt.pcr_pid});

    // Record the PCR PID for each component in the service.
    if (pmt.pcr_pid != PID_NULL) {
        for (const auto& it : pmt.streams) {
            // it.first is the PID of the component
            getContext(it.first)->pcr_pid = pmt.pcr_pid;
            _duck.report().debug(u"associating PID 0x%X (%<d) to PCR PID 0x%X (%<d)", {it.first, pmt.pcr_pid});
        }
    }
}


//----------------------------------------------------------------------------
// Get the description of a PID inside the merged stream.
//----------------------------------------------------------------------------

ts::PCRMerger::PIDContextPtr ts::PCRMerger::getContext(PID pid)
{
    const auto ctx = _pid_ctx.find(pid);
    if (ctx != _pid_ctx.end()) {
        return ctx->second;
    }
    else {
        PIDContextPtr ptr(new PIDContext(pid));
        CheckNonNull(ptr.pointer());
        _pid_ctx[pid] = ptr;
        return ptr;
    }
}


//----------------------------------------------------------------------------
// Get the adjusted DTS or PTS according to a bitrate and current packet.
//----------------------------------------------------------------------------

uint64_t ts::PCRMerger::PIDContext::adjustedPDTS(PacketCounter current_pkt, const BitRate& bitrate) const
{
    // Compute adjusted DTS and PTS.
    uint64_t dts = last_dts;
    uint64_t pts = last_pts;
    if (bitrate != 0) {
        if (dts != INVALID_DTS) {
            dts += (((current_pkt - last_dts_pkt) * PKT_SIZE_BITS * SYSTEM_CLOCK_SUBFREQ) / bitrate).toInt();
        }
        if (pts != INVALID_PTS) {
            pts += (((current_pkt - last_pts_pkt) * PKT_SIZE_BITS * SYSTEM_CLOCK_SUBFREQ) / bitrate).toInt();
        }
    }

    if (dts == INVALID_DTS) {
        return pts; // can be INVALID_PTS
    }
    else if (pts == INVALID_PTS) {
        return dts; // only DTS is valid
    }
    else {
        return std::min(pts, dts);
    }
}
