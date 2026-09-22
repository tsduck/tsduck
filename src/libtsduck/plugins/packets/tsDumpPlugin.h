//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to dump transport stream packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTSDumpArgs.h"

namespace ts {
    //!
    //! Plugin to dump transport stream packets.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL DumpPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(DumpPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        TSDumpArgs _dump {};
        fs::path   _outname {};

        // Working data.
        std::ofstream _outfile {};
        std::ostream* _out = &std::cout;
        bool          _add_endline = false;
    };
}
