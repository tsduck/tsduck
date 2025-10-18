//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsIPProtocols.h"

TS_REGISTER_INPUT_PLUGIN(u"ip", ts::IPInputPlugin);


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::IPInputPlugin::IPInputPlugin(TSP* tsp_) :
    AbstractDatagramInputPlugin(tsp_, IP_MAX_PACKET_SIZE, u"Receive TS packets from UDP/IP, multicast or unicast", u"[options] [address:]port",
                                u"kernel", u"A kernel-provided timestamp for the packet, when available (Linux only)",
                                TSDatagramInputOptions::REAL_TIME | TSDatagramInputOptions::ALLOW_RS204)
{
    // Add UDP receiver common options.
    _sock_args.defineArgs(*this, true, true);
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::getOptions()
{
    // Get command line arguments for superclass and socket.
    const bool ok = AbstractDatagramInputPlugin::getOptions() && _sock_args.loadArgs(*this, _sock.parameters().receive_timeout);
    _sock.setParameters(_sock_args);
    return ok;
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::start()
{
    // Initialize superclass and UDP socket.
    return AbstractDatagramInputPlugin::start() && _sock.open(*this);
}


//----------------------------------------------------------------------------
// Input stop method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::stop()
{
    _sock.close(*this);
    return AbstractDatagramInputPlugin::stop();
}


//----------------------------------------------------------------------------
// Input abort method
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::abortInput()
{
    debug(u"aborting IP input");
    _sock.close(*this);
    return true;
}


//----------------------------------------------------------------------------
// Set receive timeout from tsp.
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::setReceiveTimeout(cn::milliseconds timeout)
{
    if (timeout > cn::milliseconds::zero()) {
        _sock.setReceiveTimeoutArg(timeout);
    }
    return true;
}


//----------------------------------------------------------------------------
// Datagram reception method.
//----------------------------------------------------------------------------

bool ts::IPInputPlugin::receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp, TimeSource& timesource)
{
    IPSocketAddress sender;
    IPSocketAddress destination;
    UDPSocket::TimeStampType ts_type = UDPSocket::TimeStampType::NONE;
    const bool ok = _sock.receive(buffer, buffer_size, ret_size, sender, destination, tsp, *this, &timestamp, &ts_type);
    switch (ts_type) {
        case UDPSocket::TimeStampType::SOFTWARE:
            timesource = TimeSource::KERNEL;
            break;
        case UDPSocket::TimeStampType::HARDWARE:
            timesource = TimeSource::HARDWARE;
            break;
        case UDPSocket::TimeStampType::NONE:
        default:
            timesource = TimeSource::UNDEFINED;
            break;
    }
    return ok;
}
