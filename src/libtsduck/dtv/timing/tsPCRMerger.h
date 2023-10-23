//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Adjust PCR clocks when a TS is merged into a larger one.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSignalizationHandlerInterface.h"
#include "tsSignalizationDemux.h"
#include "tsDuckContext.h"
#include "tsTSPacket.h"

namespace ts {
    //!
    //! Adjust PCR clocks when a TS is merged into a larger one.
    //! @ingroup mpeg
    //!
    //! In each PID with PCR's in the merged stream, we keep the first PCR
    //! value unchanged. Then, we need to adjust all subsequent PCR's.
    //! PCR's are system clock values. They must be synchronized with the
    //! transport stream rate. So, the difference between two PCR's shall
    //! be the transmission time in PCR units.
    //!
    //! We can compute new precise PCR values when the final bitrate is fixed.
    //! However, with a variable bitrate, our computed values will be inaccurate.
    //!
    //! Also note that we do not modify DTS and PTS. First, we can't access
    //! PTS and DTS in scrambled streams (unlike PCR's). Second, we MUST NOT
    //! change them because they indicate at which time the frame shall be
    //! _processed_, not _transmitted_.
    //!
    class TSDUCKDLL PCRMerger : private SignalizationHandlerInterface
    {
        TS_NOCOPY(PCRMerger);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! Contextual information (such as standards) are accumulated in the context from demuxed sections.
        //!
        PCRMerger(DuckContext& duck);

        //!
        //! Reset all collected information.
        //!
        void reset();

        //!
        //! Reset PCR progression when moving ahead of or far away from PTS or DTS.
        //!
        //! When restamping PCR's, the PCR adjustment is usually small and stays behind the PTS and DTS.
        //! But, after hours of continuous restamping, some inaccuracy my appear and the recomputed PCR
        //! may move ahead of PCR and DTS. Similarly, if there is a leap in the input PCR (such as a TS
        //! file looping back to the beginning), the difference between the adjusted PCR and input PTS/DTS
        //! become huge.
        //!
        //! With this option, as soon as a recomputed PCR is ahead of the PTS or DTS in the same packet,
        //! of if the difference between PCR and PTS/DTS is larger than one second, PCR restamping is reset
        //! and restarts from the original PCR value in this packet. Note that, of course, this creates a
        //! small PCR leap in the stream.
        //!
        //! The option has, of course, no effect on scrambled streams.
        //!
        //! @param [in] on If true, reset PCR progression when moving ahead of PTS or DTS.
        //! The initial default is false.
        //!
        void setResetBackwards(bool on) { _pcr_reset_backwards = on; }

        //!
        //! Restamp PCR in incremental fashion, not from the initial value.
        //!
        //! When restamping PCR's from the merged TS into the main TS, compute each new
        //! PCR from the last restampted one. By default, all PCR's are restampted from
        //! the initial PCR in the PID. The default method is more precise on constant
        //! bitrate (CBR) streams. The incremental method gives better results on variable
        //! bitrate (VBR) streams.
        //!
        //! @param [in] on If true, restamp PCR in incremental fashion, not from the initial value.
        //! The initial default is false.
        //!
        void setIncremental(bool on) { _incremental_pcr = on; }

        //!
        //! Process one packet from the TS to merge.
        //!
        //! This method may adjust the PCR of the packet for insertion in the main TS.
        //! For accurate adjustment of clocks, all packets from the stream to merge
        //! shall pass through this method, even if this packet will not be merged
        //! into the main TS, because this method also collects information in the
        //! stream to merge.
        //!
        //! @param [in,out] pkt One packet from the TS to merge. The PCR may be updated.
        //! @param [in] main_packet_index Current packet index in the main stream.
        //! @param [in] main_bitrate Current bitrate of the main stream. If the main
        //! bitrate is variable, the PCR adjustment may not be accurate.
        //!
        void processPacket(TSPacket& pkt, PacketCounter main_packet_index, const BitRate& main_bitrate);

    private:
        // Each PID in the merged stream is described by a structure like this. The map is indexed by PID.
        class PIDContext;
        typedef SafePtr<PIDContext> PIDContextPtr;
        typedef std::map<PID, PIDContextPtr> PIDContextMap;

        // Private fields.
        DuckContext&       _duck;
        bool               _incremental_pcr = false;      // Use incremental method to restamp PCR's.
        bool               _pcr_reset_backwards = false;  // Reset PCR restamping when DTS/PTD move backwards the PCR.
        PIDContextMap      _pid_ctx {};                   // Description of PID's from the merged stream.
        SignalizationDemux _demux;                        // Analyze the signalization in the merged stream.

        // Get the description of a PID inside the merged stream.
        PIDContextPtr getContext(PID pid);

        // Receives all PMT's of all services in the merged stream.
        virtual void handlePMT(const PMT& table, PID pid) override;

        // PID context in the merged stream.
        class PIDContext
        {
            TS_NOBUILD_NOCOPY(PIDContext);
        public:
            const PID     pid;                     // The described PID.
            PID           pcr_pid = PID_NULL;      // Associated PCR PID (can be the PID itself).
            uint64_t      first_pcr = INVALID_PCR; // First original PCR value in this PID.
            PacketCounter first_pcr_pkt = 0;       // Index in the main stream of the packet with the first PCR.
            uint64_t      last_pcr = INVALID_PCR;  // Last PCR value in this PID, after adjustment in main stream.
            PacketCounter last_pcr_pkt = 0;        // Index in the main stream of the packet with the last PCR.
            uint64_t      last_pts = INVALID_PTS;  // Last PTS value in this PID.
            PacketCounter last_pts_pkt = 0;        // Index in the main stream of the packet with the last PTS.
            uint64_t      last_dts = INVALID_DTS;  // Last DTS value in this PID.
            PacketCounter last_dts_pkt = 0;        // Index in the main stream of the packet with the last DTS.

            // Constructor.
            PIDContext(PID);

            // Get the DTS or PTS (whichever is defined and early).
            // Adjust it according to a bitrate and current packet.
            // Return INVALID_DTS if none defined.
            uint64_t adjustedPDTS(PacketCounter, const BitRate&) const;
        };
    };
}
