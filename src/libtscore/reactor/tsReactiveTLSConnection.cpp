//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveTLSConnection.h"
#include "tsTLSConnection.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::ReactiveTLSConnection::ReactiveTLSConnection(Reactor& reactor, TCPConnection& socket, Object* owner) :
    ReactiveTCPConnection(reactor, socket, owner)
{
    allocateGuts();
    CheckNonNull(_guts);

    // The socket must be an instance of TCPConnection, not an instance of TLSConnection.
    // Detect and report trivial misusages.
    if (dynamic_cast<TLSConnection*>(&socket) != nullptr) {
        report().fatal(u"internal error: ReactiveTLSConnection needs a TCPConnection, not a TLSConnection");
    }
}

ts::ReactiveTLSConnection::ReactiveTLSConnection(Reactor& reactor, TCPConnection& socket, const TLSArgs& args, Object* owner) :
    ReactiveTLSConnection(reactor, socket, owner)
{
    setArgs(args);
}

ts::ReactiveTLSConnection::~ReactiveTLSConnection()
{
    if (_guts != nullptr) {
        deleteGuts();
        _guts = nullptr;
    }
}
