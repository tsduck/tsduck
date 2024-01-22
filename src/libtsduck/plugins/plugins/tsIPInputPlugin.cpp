//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsIPProtocols.h"
#include "tsSysUtils.h"

TS_REGISTER_INPUT_PLUGIN(u"ip", ts::IPInputPlugin);


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::IPInputPlugin::IPInputPlugin(TSP* tsp_) :
    AbstractDatagramInputPlugin(tsp_, IP_MAX_PACKET_SIZE, u"Receive TS packets from UDP/IP, multicast or unicast", u"[options] [address:]port",
                                u"kernel", u"A kernel-provided time-stamp for the packet, when available (Linux only)",
                                true), // real-time network reception
    _sock(*tsp_)
{
    // Add UDP receiver common options.
    _sock.defineArgs(*this, true, true, false);
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::getOptions()
{
    // Get command line arguments for superclass and socket.
    return AbstractDatagramInputPlugin::getOptions() && _sock.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::start()
{
    // Initialize superclass and UDP socket.
    return AbstractDatagramInputPlugin::start() && _sock.open(*tsp);
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::stop()
{
    _sock.close(*tsp);
    return AbstractDatagramInputPlugin::stop();
}


//----------------------------------------------------------------------------
// Input abort method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::abortInput()
{
    tsp->debug(u"aborting IP input");
    _sock.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Set receive timeout from tsp.
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::setReceiveTimeout(cn::milliseconds timeout)
{
    if (timeout.count() > 0) {
        _sock.setReceiveTimeoutArg(timeout);
    }
    return true;
}


//----------------------------------------------------------------------------
// Datagram reception method.
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp)
{
    IPv4SocketAddress sender;
    IPv4SocketAddress destination;
    return _sock.receive(buffer, buffer_size, ret_size, sender, destination, tsp, *tsp, &timestamp);
}
