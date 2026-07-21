//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTLSServer.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TLSServer::TLSServer(Report* report) :
    TCPServer(report, false)
{
}

ts::TLSServer::TLSServer(ReporterBase* delegate) :
    TCPServer(delegate, false)
{
}

ts::TLSServer::~TLSServer()
{
}


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TLSServer::listen(int backlog)
{
    return _cert.initServerCertificate(*this) && TCPServer::listen(backlog);
}


//----------------------------------------------------------------------------
// Wait for a client (inherited version).
//----------------------------------------------------------------------------

bool ts::TLSServer::accept(TCPConnection& client, IPSocketAddress& addr, IOSB* iosb)
{
    TLSConnection* tls = dynamic_cast<TLSConnection*>(&client);

    if (isNonBlocking()) {
        report().error(u"internal error: TLSServer called in non-blocking mode, use ReactiveTLSServer");
        return false;
    }
    else if (tls == nullptr) {
        report().error(u"internal programming error: TLSServer::accept() needs a TLSConnection");
        return false;
    }
    else if (!TCPServer::accept(client, addr, iosb)) {
        return false;
    }
    else if (!tls->setServerContext(_cert.getCertificate())) {
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
    _cert.reset();
    return TCPServer::closeImplementation(silent);
}
