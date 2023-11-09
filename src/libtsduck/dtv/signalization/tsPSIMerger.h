//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Merge the PSI/SI tables from two TS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsEnumUtils.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsSDT.h"
#include "tsBAT.h"
#include "tsNIT.h"

namespace ts {
    //!
    //! This class merges PSI/SI tables from two TS, replacing TS packets.
    //! @ingroup mpeg
    //!
    //! Definitions:
    //!  - Main stream: the main TS which will be modified.
    //!  - Merged stream: the additional TS the PSI of which will be merged in
    //!    the PSI of the main stream.
    //!
    //! The packets from the two streams are passed using two distinct methods:
    //! feedMainPacket() and feedMergedPacket(). The packets from the main stream
    //! can be modified to overwrite PSI/SI packets. Packets from the merged stream
    //! may be overwritten when they carry EIT's, depending on settings.
    //!
    //! The following tables can be merged: PAT, SDT, BAT, NIT. The EIT can also be
    //! merged, but in a different way. The PAT, SDT, BAT, NIT are fully merged.
    //! The two PAT, for instance, are merged into one single PAT containing all
    //! services from the two PAT's. The new PAT is cycled in replacement of the
    //! packets from the main stream containing the main PAT. All EIT's for the
    //! two streams are left unmodified but are mixed into one single PID. The
    //! mixed stream of EIT's is written in replacement of the EIT streams from
    //! the two streams.
    //!
    class TSDUCKDLL PSIMerger:
        private TableHandlerInterface,
        private SectionHandlerInterface,
        private SectionProviderInterface
    {
        TS_NOBUILD_NOCOPY(PSIMerger);
    public:
        //!
        //! Merging options. Can be used as bitmasks.
        //!
        enum Options: uint32_t {
            NONE           = 0x00000000,  //!< Do not merge anything.
            MERGE_PAT      = 0x00000001,  //!< Merge the two PAT's into one.
            MERGE_CAT      = 0x00000002,  //!< Merge the two CAT's into one.
            MERGE_NIT      = 0x00000004,  //!< Merge the two NIT's Actual into one. The NIT Others are mixed in the NIT PID.
            MERGE_SDT      = 0x00000008,  //!< Merge the two SDT's Actual into one. The SDT Others are mixed in the SDT/BAT PID.
            MERGE_BAT      = 0x00000010,  //!< Merge the BAT's from the same bouquet into one.
            MERGE_EIT      = 0x00000020,  //!< Mix the EIT's from the two streams.
            KEEP_MAIN_TDT  = 0x00000040,  //!< Keep TDT/TOT from main stream.
            KEEP_MERGE_TDT = 0x00000080,  //!< Keep TDT/TOT from merge stream. It is dangerous to use KEEP_MAIN_TDT and KEEP_MERGE_TDT at the same time.
            NULL_MERGED    = 0x00000100,  //!< Nullify packets from the merged stream when they carried merged PSI (PAT, NIT, SDT, BAT). EIT are merged, not nullified.
            NULL_UNMERGED  = 0x00000200,  //!< Nullify packets from the merged stream when they carry unmerged PSI (PAT, NIT, SDT, BAT, EIT).
            DEFAULT        = MERGE_PAT | MERGE_CAT | MERGE_NIT | MERGE_SDT | MERGE_BAT | MERGE_EIT | NULL_MERGED | NULL_UNMERGED,
                                          //!< Default options: merge all.
        };

        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! Contextual information (such as standards) are accumulated in the context from demuxed
        //! sections, from both streams.
        //! @param [in] options Bitmask of option values.
        //!
        explicit PSIMerger(DuckContext& duck, Options options = DEFAULT);

        //!
        //! Feed a packet from the main stream.
        //! @param [in,out] pkt A packet from the first stream. When the packet contains tables to merge, it is replaced.
        //! @return True on success, false if an error was reported.
        //!
        bool feedMainPacket(TSPacket& pkt);

        //!
        //! Feed a packet from the merged stream.
        //! @param [in,out] pkt A packet from the first stream. When the packet contains tables which can be merged,
        //! it can be replaced by null packets or EIT sections, depending on options.
        //! @return True on success, false if an error was reported.
        //!
        bool feedMergedPacket(TSPacket& pkt);

        //!
        //! Reset the PSI merger.
        //! All contexts are erased. The options are left unchanged.
        //!
        void reset();

        //!
        //! Reset the PSI merger with new options.
        //! All contexts are erased.
        //! @param [in] options Bitmask of option values.
        //!
        void reset(Options options);

    private:
        DuckContext&       _duck;                                    // Reference to TSDuck context.
        Options            _options = DEFAULT;                       // Merging options.
        SectionDemux       _main_demux {_duck, this};                // Demux on main transport stream. Complete table handler only.
        SectionDemux       _main_eit_demux {_duck, nullptr, this};   // Demux on main transport stream for EIT's. Section handler only.
        SectionDemux       _merge_demux {_duck, this};               // Demux on merged transport stream. Complete table handler only.
        SectionDemux       _merge_eit_demux {_duck, nullptr, this};  // Demux on merged transport stream for EIT's.
        CyclingPacketizer  _pat_pzer {_duck};                        // Packetizer for modified PAT in main TS.
        CyclingPacketizer  _cat_pzer {_duck};                        // Packetizer for modified CAT in main TS.
        CyclingPacketizer  _nit_pzer {_duck};                        // Packetizer for modified NIT's in main TS.
        CyclingPacketizer  _sdt_bat_pzer {_duck};                    // Packetizer for modified SDT/BAT in main TS.
        Packetizer         _eit_pzer {_duck, PID_EIT, this};         // Packetizer for the mixed EIT's.
        std::optional<uint16_t> _main_tsid {};     // TS id of the main stream.
        PAT                     _main_pat {};      // Last input PAT from main TS (version# is current output version).
        PAT                     _merge_pat {};     // Last input PAT from merged TS.
        CAT                     _main_cat {};      // Last input CAT from main TS (version# is current output version).
        CAT                     _merge_cat {};     // Last input CAT from merged TS.
        SDT                     _main_sdt {};      // Last input SDT Actual from main TS (version# is current output version).
        SDT                     _merge_sdt {};     // Last input SDT Actual from merged TS.
        NIT                     _main_nit {};      // Last input NIT Actual from main TS (version# is current output version).
        NIT                     _merge_nit {};     // Last input NIT Actual from merged TS.
        std::map<uint16_t, BAT> _main_bats {};     // Map of last input BAT/bouquet_it from main TS (version# is current output version).
        std::map<uint16_t, BAT> _merge_bats {};    // Map of last input BAT/bouquet_it from merged TS.
        std::list<SectionPtr>   _eits {};          // List of EIT sections to insert.
        size_t                  _max_eits = 128;   // Maximum number of buffered EIT sections (hard-coded for now).

        static constexpr int DEMUX_MAIN      = 1;  // Id of the demux from the main TS.
        static constexpr int DEMUX_MAIN_EIT  = 2;  // Id of the demux from the main TS for EIT's.
        static constexpr int DEMUX_MERGE     = 3;  // Id of the demux from the secondary TS to merge.
        static constexpr int DEMUX_MERGE_EIT = 4;  // Id of the demux from the secondary TS to merge for EIT's.

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;

        // Implementation of SectionHandlerInterface.
        virtual void handleSection(SectionDemux& demux, const Section& section) override;

        // Implementation of SectionProviderInterface (for EIT's only).
        virtual void provideSection(SectionCounter counter, SectionPtr& section) override;
        virtual bool doStuffing() override;

        // Check that the queue of EIT's does not overflow.
        bool checkEITs();

        // Get main and merged complete TS id. Return false if not yet known.
        bool getTransportStreamIds(TransportStreamId& main, TransportStreamId& merge) const;

        // Handle a table from the main or merged transport stream.
        void handleMainTable(const BinaryTable& table);
        void handleMergeTable(const BinaryTable& table);

        // Generate new/merged tables.
        void mergePAT();
        void mergeCAT();
        void mergeSDT();
        void mergeNIT();
        void mergeBAT(uint16_t bouquet_id);

        // Copy a table into another, preserving the previous version number if the table is valid.
        template<class TABLE, typename std::enable_if<std::is_base_of<AbstractLongTable, TABLE>::value>::type* = nullptr>
        void copyTableKeepVersion(TABLE& dest, const TABLE& src)
        {
            const bool was_valid = dest.isValid();
            const uint8_t version = dest.version;
            dest = src;
            if (was_valid) {
                dest.version = version;
            }
        }
    };
}

TS_ENABLE_BITMASK_OPERATORS(ts::PSIMerger::Options);
