//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTLSServer.h"
#include "tsTLSServer.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveTLSServer::ReactiveTLSServer(Reactor& reactor, TCPServer& socket) :
    ReactiveTCPServer(reactor, socket)
{
    // The socket must be an instance of TCPServer, not an instance of TLSServer.
    // Detect and report trivial misusages.
    if (dynamic_cast<TLSServer*>(&socket) != nullptr) {
        report().fatal(u"internal error: ReactiveTLSServer needs a TCPServer, not a TLSServer");
    }
}

ts::ReactiveTLSServer::ReactiveTLSServer(Reactor& reactor, TCPServer& socket, const TLSArgs& args) :
    ReactiveTLSServer(reactor, socket)
{
    setArgs(args);
}

ts::ReactiveTLSServer::~ReactiveTLSServer()
{
}

ts::ReactiveTLSServer::AcceptRequest::~AcceptRequest()
{
}


//----------------------------------------------------------------------------
// Start the operation of accepting a TCP client.
//----------------------------------------------------------------------------

bool ts::ReactiveTLSServer::startAccept(ReactiveTCPServerHandlerInterface* handler, ReactiveTCPConnection& client, const ObjectPtr& user_data)
{
    const auto tls_client = dynamic_cast<ReactiveTLSConnection*>(&client);
    if (tls_client == nullptr) {
        report().error(u"internal programming error: ReactiveTLSServer::startAccept() needs a ReactiveTLSConnection");
        return false;
    }

    // Get or create the TLS server certificate the first time listen is called.
    if (!_cert.initServerCertificate(*this) || !tls_client->initServerContext(this, _cert.getCertificate())) {
        return false;
    }

    // Build an accept request.
    const auto req = std::make_shared<AcceptRequest>();
    req->handler = handler;
    req->user_data = user_data;

    // Wait for the TCP connection of a client.
    return ReactiveTCPServer::startAccept(this, client, req);
}


//----------------------------------------------------------------------------
// Called when a TCP client is accepted. Before TLS handshake.
//----------------------------------------------------------------------------

void ts::ReactiveTLSServer::handleTCPClientAccepted(ReactiveTCPServer& server, ReactiveTCPConnection& client, const IPSocketAddress& addr, int error_code, const ObjectPtr& user_data)
{
    // The user data must be an AcceptRequest.
    auto req = std::dynamic_pointer_cast<AcceptRequest>(user_data);
    assert(req != nullptr);

    // Notify the server object now. The connection object will be notified later, when the TLS handshake completes.
    if (req->handler != nullptr) {
        req->handler->handleTCPClientAccepted(server, client, addr, error_code, req->user_data);
    }
}
