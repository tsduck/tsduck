//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSocketHandlerInterface.h"
#include "tsSocket.h"


//----------------------------------------------------------------------------
// Register / deregister the suscription to a socket.
//----------------------------------------------------------------------------

void ts::SocketHandlerInterface::registerSocket(Socket* sock)
{
    if (sock != nullptr) {
        _registered_sockets.insert(sock);
    }
}

void ts::SocketHandlerInterface::deregisterSocket(Socket* sock)
{
    // Avoid modifying _registered_sockets during destructor.
    if (!_destructing) {
        _registered_sockets.erase(sock);
    }
}


//----------------------------------------------------------------------------
// The destructor removes all subscriptions.
//----------------------------------------------------------------------------

ts::SocketHandlerInterface::~SocketHandlerInterface()
{
    // Tell all subscribers to forget us.
    _destructing = true;
    for (auto sock : _registered_sockets) {
        sock->cancelSubscription(this);
    }
    _registered_sockets.clear();
}
