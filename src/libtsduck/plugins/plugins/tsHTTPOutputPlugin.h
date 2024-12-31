//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  HTTP output plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPlugin.h"
#include "tsTCPServer.h"
#include "tsTCPConnection.h"

namespace ts {
    //!
    //! HTTP output plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL HTTPOutputPlugin: public OutputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(HTTPOutputPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        // Command line options:
        IPSocketAddress _server_address {};
        bool            _reuse_port = false;
        bool            _multiple_clients = false;
        bool            _ignore_bad_request = false;
        size_t          _tcp_buffer_size = 0;

        // Working data:
        TCPServer     _server {};
        TCPConnection _client {};

        // Process request headers from new client, send response headers.
        bool startSession();

        // Send a response header.
        bool sendResponseHeader(const std::string& line);
    };
}
