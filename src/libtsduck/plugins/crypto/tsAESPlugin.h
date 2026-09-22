//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  AES scrambling experimental plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsService.h"
#include "tsSectionDemux.h"
#include "tsBinaryTable.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsBlockCipher.h"

namespace ts {
    //!
    //! AES scrambling experimental plugin for tsp.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL AESPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(AESPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        using CipherPtr = std::shared_ptr<BlockCipher>;

        // Command line options:
        bool      _descramble = false; // Descramble instead of scramble
        Service   _service_arg {};     // Service name & id
        PIDSet    _scrambled {};       // List of PID's to (de)scramble
        CipherPtr _chain {};           // Selected cipher chaining mode

        // Working data:
        bool         _abort = false;      // Error (service not found, etc)
        Service      _service {};         // Service name & id
        SectionDemux _demux {duck, this}; // Section demux

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables
        void processPAT(PAT&);
        void processPMT(PMT&);
        void processSDT(SDT&);
    };
}
