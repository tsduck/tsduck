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

ts::TLSServer::TLSServer(Report* report, Object* owner) :
    TCPServer(report, false,owner)
{
    allocateGuts();
}

ts::TLSServer::TLSServer(ReporterBase* delegate, Object* owner) :
    TCPServer(delegate, false, owner)
{
    allocateGuts();
}

ts::TLSServer::~TLSServer()
{
    if (_guts != nullptr) {
        deleteGuts();
        _guts = nullptr;
    }
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
    else {
        return acceptTLS(*tls, addr, iosb);
    }
}
