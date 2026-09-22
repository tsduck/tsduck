//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to remove or merge sections from various PID's.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTablePatchXML.h"
#include "tsSectionDemux.h"
#include "tsPacketizer.h"
#include "tsBoolPredicate.h"

namespace ts {
    //!
    //! Plugin to remove or merge sections from various PID's.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL SectionsPlugin: public ProcessorPlugin, private SectionHandlerInterface, private SectionProviderInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SectionsPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options.
        bool                   _section_stuffing = false;
        bool                   _use_null_pid = false;
        bool                   _reverse_eitd = false;
        bool                   _keep_selected = false;
        bool                   _selections_present = false;    // there are selection options in the command line
        MultiBoolPredicate     _predicate = nullptr;           // global "and" / "or" on all criteria, see option --and
        MonoBoolPredicate      _valid_predicate = nullptr;     // see method condition()
        BoolPredicate          _cond_predicate = nullptr;      // see method condition()
        size_t                 _max_buffered_sections = 1024;  // hard-coded for now
        PIDSet                 _input_pids {};
        PID                    _output_pid = PID_NULL;
        std::set<TID>          _tids {};
        std::set<uint16_t>     _exts {};
        std::set<uint32_t>     _etids {};
        std::set<uint8_t>      _versions {};
        std::set<uint8_t>      _section_numbers {};
        std::vector<ByteBlock> _contents {};
        std::vector<ByteBlock> _contents_masks {};

        // Working data.
        std::list<SectionPtr> _sections {};
        SectionDemux          _demux {duck, nullptr, this};
        Packetizer            _packetizer {duck, PID_NULL, this};
        TablePatchXML         _patch_xml {duck};

        // Compute a condition in the chain of _predicate.
        // - valid: the condition needs to be checked (eg. there are some tids to remove).
        // - cond: the condition itself (eg. this section has a tid to remove).
        bool condition(bool valid, bool cond) const { return _cond_predicate(_valid_predicate(valid), cond); }

        // Check if a section matches any selected leading content.
        bool matchContent(const Section& section) const;

        // Implementation of SectionHandlerInterface.
        virtual void handleSection(SectionDemux& demux, const Section& section) override;

        // Implementation of SectionProviderInterface.
        virtual void provideSection(SectionCounter counter, SectionPtr& section) override;
        virtual bool doStuffing() override;
    };
}
