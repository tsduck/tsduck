//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS server - Windows specific parts with SChannel.
//
//----------------------------------------------------------------------------

#include "tsTLSServer.h"
#include "tsSChannelCertificate.h"


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSServer::SystemGuts
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    SChannelCertificate cert;
    SystemGuts(TLSServer* server) : cert(server) {}
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

void ts::TLSServer::allocateGuts()
{
    _guts = new SystemGuts(this);
}

void ts::TLSServer::deleteGuts()
{
    delete _guts;
}


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TLSServer::listen(int backlog)
{
    return _guts->cert.initServerCertificate(*this) && TCPServer::listen(backlog);
}


//----------------------------------------------------------------------------
// Wait for a TLS client.
//----------------------------------------------------------------------------

bool ts::TLSServer::acceptTLS(TLSConnection& client, IPSocketAddress& addr, IOSB* iosb)
{
    // Accept one TCP client.
    if (!TCPServer::accept(client, addr, iosb)) {
        return false;
    }

    // Perform handshake with the client.
    if (!client.setServerContext(_guts->cert.getCertificate())) {
        // Close the underlying TCP socket.
        client.close(true);
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Close the server resources.
//----------------------------------------------------------------------------

bool ts::TLSServer::closeImplementation(bool silent)
{
    _guts->cert.reset();
    return TCPServer::closeImplementation(silent);
}
