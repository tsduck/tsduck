//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to trace packets with a custom message.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"

namespace ts {
    //!
    //! Plugin to trace packets with a custom message.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL TracePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(TracePlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options
        UString          _format {};        // Message format
        PIDSet           _pids {};          // Trace packets in these PID's
        TSPacketLabelSet _labels {};        // Trace packets with any of these labels
        fs::path         _outfile_name {};  // Output file name

        // Working data
        std::ofstream _outfile {};          // User-specified output file
    };
}
