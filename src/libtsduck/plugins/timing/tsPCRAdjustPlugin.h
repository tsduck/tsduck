//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to adjust PCR values.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSectionDemux.h"
#include "tsBinaryTable.h"

namespace ts {
    //!
    //! Plugin to adjust PCR values.
    //! @ingroup libtsduck plugin
    //!
    //! The adjustment works well with a constant bitrate. With variable bitrate,
    //! the adjustment of PCR values can be counter-productive.
    //!
    //! The other issue is the adjustment of PTS and DTS. If the PCR adjustment
    //! only fixes the jitter of poorly muxed packets, the PCR and DTS shall
    //! not be modified: the placement of packets in the TS has changed and their
    //! clock time, relative to the system clock, shall be adjusted. But the
    //! frames shall be decoded and presented at the same time as before, relative
    //! to the reference system clock.
    //!
    //! On the other hand, if the old PCR abruptly changes (because of a TS file
    //! being looped for instance), then the PTS and DTS must be adjusted.
    //! Otherwise, the decoding is completely out of sync from the system clock.
    //!
    //! So, there are two difficulties:
    //!
    //! 1) When shall we adjust the PTS/DTS and when shall we keep them untouched?
    //!    --> When the difference between the PTS/DTS and the adjusted PCR
    //!        remains small (less than 500 ms), we assume that the PCR adjustment
    //!        was only the result of packet placement (bad muxing), we assume
    //!        that the reference clock is still valid and we keep the original
    //!        values of PTS/DTS.
    //!
    //! 2) When the PCR changed abruptly and the PTS/DTS must be adjusted, by
    //!    which amount shall we adjust the PTS/DTS?
    //!    --> We first compute the theoretical original PCR of the packet
    //!        containing the PTS/DTS (if the packet does not contain a PCR).
    //!        We compute the original difference between PCR and PTS/DTS.
    //!        Then we apply this difference to the adjusted PCR and we modify
    //!        the PTS/DTS accordingly.
    //!    Note: this is not ideal since it does not solve the problem of
    //!        simultaneous modified packet placement (bad muxin/merging) and
    //!        PCR warp (file loop for instance).
    //!
    class TSDUCKDLL PCRAdjustPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(PCRAdjustPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of PID's. Map of safe pointers to PID contexts, indexed by PID.
        class PIDContext;
        using PIDContextPtr = std::shared_ptr<PIDContext>;
        using PIDContextMap = std::map<PID, PIDContextPtr>;

        // PCRAdjustPlugin private members
        BitRate       _user_bitrate = 0;          // User-specified bitrate.
        PIDSet        _pids {};                   // User-specified list of PIDs.
        bool          _ignore_dts = false;        // Do not modify DTS values.
        bool          _ignore_pts = false;        // Do not modify PTS values.
        bool          _ignore_scrambled = false;  // Do not modify scrambled PID's.
        uint64_t      _min_pcr_interval = 0;      // Minimum interval between two PCR's. Ignored if zero.
        SectionDemux  _demux {duck, this};        // Section demux to get service descriptions.
        PIDContextMap _pid_contexts {};           // Map of all PID contexts.

        // TableHandlerInterface implementation.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Get the context for a PID. Create one when necessary.
        PIDContextPtr getContext(PID pid);

        // Description of one PID. One structure is created per PID in the TS.
        class PIDContext
        {
            TS_NOBUILD_NOCOPY(PIDContext);
        public:
            const PID     pid;                             // PID value.
            PIDContextPtr pcr_ctx {};                      // Context for associated PCR PID.
            bool          scrambled = false;               // The PID contains scrambled packets.
            bool          sync_pdts = false;               // PTS and DTS are still synchronous with the PCR, do not modify them.
            uint8_t       last_cc = 0;                     // Last continuity counter in this PID.
            uint64_t      last_original_pcr = INVALID_PCR; // Last PCR value, before modification.
            uint64_t      last_updated_pcr = INVALID_PCR;  // Last PCR value, after modification.
            PacketCounter last_pcr_packet = 0;             // Last PCR packet index.
            uint64_t      last_created_pcr = INVALID_PCR;  // Last created PCR value in a null packet.
            PacketCounter last_created_packet = 0;         // Packet index of the last created PCR.

            // Constructor.
            PIDContext(PID p) : pid(p) {}

            // Retrieve the last updated PCR. INVALID_PCR if unknown.
            uint64_t lastPCR() const;

            // Compute the theoretical updated PCR at the given packet index. INVALID_PCR if unknown.
            uint64_t updatedPCR(PacketCounter packet_index, const BitRate& bitrate) const;

            // Compute an updated PTS or DTS at the given packet index. Unchanged if unknown.
            uint64_t updatedPDTS(PacketCounter packet_index, const BitRate& bitrate, uint64_t original_pdts);
        };
    };
}
