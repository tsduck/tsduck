//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  File packet processor plugin for tsp.
//!  Fork a process and send TS packets to its standard input (pipe).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTSForkPipe.h"

namespace ts {
    //!
    //! File packet processor plugin for tsp.
    //! Fork a process and send TS packets to its standard input (pipe).
    //! @ingroup plugin
    //!
    class TSDUCKDLL ForkPacketPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(ForkPacketPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        UString                _command {};        // The command to run.
        bool                   _nowait = false;    // Don't wait for children termination.
        TSPacketFormat         _format = TSPacketFormat::TS;  // Packet format on the pipe
        size_t                 _buffer_size = 0;   // Max number of packets in buffer.
        size_t                 _buffer_count = 0;  // Number of packets currently in buffer.
        TSPacketVector         _buffer {};         // Packet buffer.
        TSPacketMetadataVector _mdata {};          // Metadata for packets in buffer.
        TSForkPipe             _pipe {};           // The pipe device.
    };
}
