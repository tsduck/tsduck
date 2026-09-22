//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to analyze the level of stuffing in tables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsCASSelectionArgs.h"
#include "tsSectionDemux.h"

namespace ts {
    //!
    //! Plugin to analyze the level of stuffing in tables.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL StuffAnalyzePlugin: public ProcessorPlugin, private TableHandlerInterface, private SectionHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(StuffAnalyzePlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Analysis context for a PID.
        class PIDContext
        {
        public:
            PIDContext() = default;          // Constructor.
            uint64_t total_sections = 0;     // Total number of sections.
            uint64_t stuffing_sections = 0;  // Number of stuffing sections.
            uint64_t total_bytes = 0;        // Total number of bytes in sections.
            uint64_t stuffing_bytes = 0;     // Total number of bytes in stuffing sections.

            // Format as a string.
            UString toString() const;
        };

        using PIDContextPtr = std::shared_ptr<PIDContext>;
        using PIDContextMap = std::map<PID, PIDContextPtr>;

        // Plugin private fields.
        fs::path         _output_name {};    // Output file name
        std::ofstream    _output_stream {};  // Output file stream
        std::ostream*    _output = nullptr;  // Actual output stream
        CASSelectionArgs _cas_args {};       // CAS selection
        PIDSet           _analyze_pids {};   // List of PIDs to pass
        SectionDemux     _analyze_demux {duck, nullptr, this};  // Demux for sections to analyze for stuffing
        SectionDemux     _psi_demux {duck, this, nullptr};      // Demux for PSI tables parsing
        PIDContext       _total {};          // Global context.
        PIDContextMap    _pid_contexts {};   // Contexts of analyzed PID's.

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;
    };
}
