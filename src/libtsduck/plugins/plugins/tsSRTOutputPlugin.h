//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2023, Anthony Delannoy
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Secure Reliable Transport (SRT) output plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"
#include "tsTSDatagramOutput.h"
#include "tsSRTSocket.h"

namespace ts {
    //!
    //! Secure Reliable Transport (SRT) output plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL SRTOutputPlugin: public OutputPlugin, private TSDatagramOutputHandlerInterface
    {
        TS_NOBUILD_NOCOPY(SRTOutputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        SRTOutputPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        bool             _multiple;       // Accept multiple (sequential) connections.
        MilliSecond      _restart_delay;  // If _multiple, wait before reconnecting.
        TSDatagramOutput _datagram;       // Buffering TS packets.
        SRTSocket        _sock;           // Outgoing SRT socket.

        // Implementation of TSDatagramOutputHandlerInterface.
        virtual bool sendDatagram(const void* address, size_t size, Report& report) override;
    };
}
