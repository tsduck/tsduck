//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  File input plugin for tsp.
//!  Fork a process and receive packets from its standard output (pipe).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"
#include "tsTSForkPipe.h"

namespace ts {
    //!
    //! File input plugin for tsp.
    //! Fork a process and receive packets from its standard output (pipe).
    //! @ingroup plugin
    //!
    class TSDUCKDLL ForkInputPlugin: public InputPlugin
    {
        TS_NOBUILD_NOCOPY(ForkInputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        ForkInputPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool abortInput() override;

    private:
        UString        _command {};       // The command to run.
        bool           _nowait = false;   // Don't wait for children termination.
        TSPacketFormat _format = TSPacketFormat::AUTODETECT;  // Packet format on the pipe
        size_t         _buffer_size = 0;  // Pipe buffer size in packets.
        TSForkPipe     _pipe {};          // The pipe device.
    };
}
