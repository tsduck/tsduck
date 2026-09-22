//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to restamp PTS in SCTE-35 splice information.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsSectionDemux.h"
#include "tsSignalizationDemux.h"
#include "tsPacketizer.h"

namespace ts {
    //!
    //! Plugin to restamp PTS in SCTE-35 splice information.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL SpliceRestampPlugin: public ProcessorPlugin, private TableHandlerInterface, private SignalizationHandlerInterface, private SectionProviderInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SpliceRestampPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool     _replace = false;           // Replace pts_adjustment field without adding previous value.
        bool     _continuous = false;        // Continuously recompute the adjustment between the old and new PCR PID's.
        PID      _pid_arg = PID_NULL;        // The splice PID to restamp.
        PID      _old_pcr_pid = PID_NULL;    // Previous PCR reference.
        PID      _new_pcr_pid = PID_NULL;    // New PCR reference.
        uint64_t _pts_adjustment = 0;        // Raw PTS adjustment to apply.
        uint64_t _rebase_pts = INVALID_PTS;  // Assume that the first PTS in the stream will be set to this value.

        // Working data:
        PID                     _splice_pid = PID_NULL;              // The actual splice PID to restamp.
        std::optional<uint64_t> _current_adjustment {};              // Current PTS adjustment to apply.
        uint64_t                _old_pcr = INVALID_PCR;              // Last PCR value in old clock reference PID.
        PacketCounter           _old_pcr_packet = 0;                 // Packet index of _old_pcr.
        uint64_t                _new_pcr = INVALID_PCR;              // Last PCR value in new clock reference PID.
        PacketCounter           _new_pcr_packet = 0;                 // Packet index of _new_pcr.
        SectionDemux            _section_demux {duck, this};         // Section filter for splice information.
        SignalizationDemux      _sig_demux {duck, this};             // Signalization demux to get PMT's.
        Packetizer              _packetizer {duck, PID_NULL, this};  // Regenerate modified splice sections.
        std::list<SectionPtr>   _sections {};                        // List of sections to inject.
        std::map<PID,uint64_t>  _first_pts {};                       // First PTS in each PID.
        std::set<PID>           _service_pids {};                    // Set of PID's in the same service as the splice PID.

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Implementation of SignalizationHandlerInterface.
        virtual void handlePMT(const PMT&, PID) override;

        // Implementation of SectionProviderInterface.
        virtual void provideSection(SectionCounter, SectionPtr&) override;
        virtual bool doStuffing() override;
    };
}
