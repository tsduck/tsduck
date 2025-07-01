//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHTTPOutputPlugin.h"
#include "tsPluginRepository.h"
#include "tsLibTSCoreVersion.h"

TS_REGISTER_OUTPUT_PLUGIN(u"http", ts::HTTPOutputPlugin);

#define SERVER_BACKLOG  1  // One connection at a time


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::HTTPOutputPlugin::HTTPOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Act as an HTTP server and send TS packets to the incoming client", u"[options]")
{
    setIntro(u"The implemented HTTP server is rudimentary. "
             u"No SSL/TLS is supported, only the http: protocol is accepted.\n\n"
             u"Only one client is accepted at a time. "
             u"By default, tsp terminates if the client disconnects (see option --multiple-clients).\n\n"
             u"The request \"GET /\" returns the transport stream content. "
             u"All other requests are considered as invalid (see option --ignore-bad-request). "
             u"There is no Content-Length response header since the size of the returned TS is unknown. "
             u"The server disconnects at the end of the data. There is no Keep-Alive.");

    option(u"buffer-size", 0, UNSIGNED);
    help(u"buffer-size",
         u"Specifies the TCP socket send buffer size to the client connection (socket option).");

    option(u"ignore-bad-request");
    help(u"ignore-bad-request",
         u"Ignore invalid HTTP requests and unconditionally send the transport stream.");

    option(u"multiple-clients", 'm');
    help(u"multiple-clients",
         u"Specifies that the server handle multiple clients, one after the other. "
         u"By default, the plugin terminates the tsp session when the first client disconnects.");

    option(u"no-reuse-port");
    help(u"no-reuse-port",
         u"Disable the reuse port socket option. Do not use unless completely necessary.");

    option(u"server", 's', IPSOCKADDR_OA, 1, 1);
    help(u"server",
         u"Specifies the local TCP port on which the plugin listens for incoming HTTP connections. "
         u"This option is mandatory. "
         u"This plugin accepts only one HTTP connection at a time. "
         u"When present, the optional address shall specify a local IP address or host name. "
         u"By default, the server listens on all local interfaces.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::HTTPOutputPlugin::getOptions()
{
    _reuse_port = !present(u"no-reuse-port");
    _multiple_clients = present(u"multiple-clients");
    _ignore_bad_request = present(u"ignore-bad-request");
    getSocketValue(_server_address, u"server");
    getIntValue(_tcp_buffer_size, u"buffer-size");
    return true;
}


//----------------------------------------------------------------------------
// Start method.
//----------------------------------------------------------------------------

bool ts::HTTPOutputPlugin::start()
{
    if (!_server.open(IP::Any, *this)) {
        return false;
    }
    if (!_server.reusePort(_reuse_port, *this) ||
        (_tcp_buffer_size > 0 && !_server.setSendBufferSize(_tcp_buffer_size, *this)) ||
        !_server.bind(_server_address, *this) ||
        !_server.listen(SERVER_BACKLOG, *this))
    {
        _server.close(*this);
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Stop method.
//----------------------------------------------------------------------------

bool ts::HTTPOutputPlugin::stop()
{
    if (_client.isConnected()) {
        _client.disconnect(*this);
    }
    if (_client.isOpen()) {
        _client.close(*this);
    }
    _server.close(*this);
    return true;
}


//----------------------------------------------------------------------------
// Send packets.
//----------------------------------------------------------------------------

bool ts::HTTPOutputPlugin::send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    // Loop over multiple clients if necessary.
    for (;;) {
        // Establish one client connection, if none is connected.
        while (!_client.isConnected()) {
            // Wait for a new incoming client.
            IPSocketAddress client_address;
            debug(u"waiting for incoming client connection");
            if (!_server.accept(_client, client_address, *this)) {
                // Error while accepting a client is fatal.
                return false;
            }
            verbose(u"client connected from %s", client_address);

            // Initialize the session, process request, send response headers.
            if (startSession()) {
                // Session initialized, we can start sending data.
                break;
            }

            // Session initialization error, close the connection.
            _client.disconnect(*this);
            _client.close(*this);
            if (!_multiple_clients) {
                return false;
            }
        }

        // Send the TS packets to the client.
        if (_client.send(buffer, packet_count * PKT_SIZE, *this)) {
            return true;
        }

        // Send error, close the connection.
        _client.disconnect(*this);
        _client.close(*this);
        if (!_multiple_clients) {
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Send a response header.
//----------------------------------------------------------------------------

bool ts::HTTPOutputPlugin::sendResponseHeader(const std::string& line)
{
    debug(u"response header: %s", line);
    std::string data(line);
    data += "\r\n";
    return _client.send(data.data(), data.size(), *this);
}


//----------------------------------------------------------------------------
// Process request headers, send response headers.
//----------------------------------------------------------------------------

bool ts::HTTPOutputPlugin::startSession()
{
    UString request;
    UString header(1, SPACE); // Need an initial non-empty value
    ByteBlock data;
    data.reserve(1024);

    // Read request header lines, until an empty line is read.
    do {
        // Read a chunk of data.
        const size_t previous = data.size();
        size_t ret_size = 0;
        data.resize(previous + 512);
        if (!_client.receive(data.data() + previous, data.size() - previous, ret_size, nullptr, *this)) {
            return false; // receive error
        }
        data.resize(previous + ret_size);

        // Look for a header line.
        size_t eol = 0;
        while (!header.empty() && (eol = data.find('\n')) != NPOS) {
            // Extract the header line from the buffer.
            header.assignFromUTF8(reinterpret_cast<const char*>(data.data()), eol);
            header.trim();
            data.erase(0, eol + 1);
            debug(u"request header: %s", header);

            // The first header is the request.
            if (request.empty()) {
                request = header;
            }
        }
    } while (!header.empty());

    // Expected request: "GET / HTTP/1.1"
    UStringVector fields;
    request.split(fields, ' ', true, true);
    const bool is_get = fields.size() >= 1 && fields[0] == u"GET";
    const UString& resource(fields.size() >= 2 ? fields[1] : UString::EMPTY());
    const UString& protocol(fields.size() >= 3 ? fields[2] : UString::EMPTY());
    const bool valid = is_get && resource == u"/" && protocol.starts_with(u"HTTP/");

    if (!valid && !_ignore_bad_request) {
        error(u"invalid client request: %s", request);
        sendResponseHeader(is_get ? "HTTP/1.1 404 Not Found" : "HTTP/1.1 400 Bad Request");
        sendResponseHeader("");
        return false;
    }
    else {
        // Send the HTTP response headers.
        sendResponseHeader("HTTP/1.1 200 OK");
        sendResponseHeader("Server: TSDuck/" TS_VERSION_STRING);
        sendResponseHeader("Content-Type: video/mp2t");
        sendResponseHeader("Connection: close");
        sendResponseHeader("");
        return true;
    }
}
