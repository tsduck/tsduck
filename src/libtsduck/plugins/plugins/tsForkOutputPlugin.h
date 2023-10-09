//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  File output plugin for tsp.
//!  Fork a process and send TS packets to its standard input (pipe).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"
#include "tsTSForkPipe.h"

namespace ts {
    //!
    //! File output plugin for tsp.
    //! Fork a process and send TS packets to its standard input (pipe).
    //! @ingroup plugin
    //!
    class TSDUCKDLL ForkOutputPlugin: public OutputPlugin
    {
        TS_NOBUILD_NOCOPY(ForkOutputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        ForkOutputPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        UString        _command;      // The command to run.
        bool           _nowait;       // Don't wait for children termination.
        TSPacketFormat _format;       // Packet format on the pipe
        size_t         _buffer_size;  // Pipe buffer size in packets.
        TSForkPipe     _pipe;         // The pipe device.
    };
}
