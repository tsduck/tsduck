//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsIPOutputPlugin.h"
#include "tsPluginRepository.h"
#include "tsSystemRandomGenerator.h"

TS_REGISTER_OUTPUT_PLUGIN(u"ip", ts::IPOutputPlugin);


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::IPOutputPlugin::IPOutputPlugin(TSP* tsp_) :
    AbstractDatagramOutputPlugin(tsp_, u"Send TS packets using UDP/IP, multicast or unicast", u"[options] address:port", ALLOW_RTP),
    _destination(),
    _local_addr(),
    _local_port(IPv4SocketAddress::AnyPort),
    _ttl(0),
    _tos(-1),
    _mc_loopback(true),
    _force_mc_local(false),
    _sock(false, *tsp_)
{
    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"The parameter address:port describes the destination for UDP packets. "
         u"The 'address' specifies an IP address which can be either unicast or "
         u"multicast. It can be also a host name that translates to an IP address. "
         u"The 'port' specifies the destination UDP port.");

    option(u"disable-multicast-loop", 'd');
    help(u"disable-multicast-loop",
         u"Disable multicast loopback. By default, outgoing multicast packets are looped back on local interfaces, "
         u"if an application added membership on the same multicast group. This option disables this.\n"
         u"Warning: On output sockets, this option is effective only on Unix systems (Linux, macOS, BSD). "
         u"On Windows systems, this option applies only to input sockets.");

    option(u"force-local-multicast-outgoing", 'f');
    help(u"force-local-multicast-outgoing",
         u"When the destination is a multicast address and --local-address is specified, "
         u"force multicast outgoing traffic on this local interface (socket option IP_MULTICAST_IF). "
         u"Use this option with care. Its usage depends on the operating system. "
         u"If no route is declared for this destination address, this option may be necessary "
         u"to force the multicast to the specified local interface. On the other hand, if a route is "
         u"declared, this option may transport multicast IP packets in unicast Ethernet frames "
         u"to the gateway, preventing multicast reception on the local network (seen on Linux).");

    option(u"local-address", 'l', STRING);
    help(u"local-address",
         u"When the destination is a multicast address, specify the IP address "
         u"of the outgoing local interface. It can be also a host name that "
         u"translates to a local address.");

    option(u"local-port", 0, UINT16);
    help(u"local-port",
         u"Specify the local UDP source port for outgoing packets. "
         u"By default, a random source port is used.");

    option(u"rs204");
    help(u"rs204",
         u"Use 204-byte format for TS packets in UDP datagrams. "
         u"Each TS packet is followed by a zeroed placeholder for a 16-byte Reed-Solomon trailer.");

    option(u"tos", 's', INTEGER, 0, 1, 1, 255);
    help(u"tos",
         u"Specifies the TOS (Type-Of-Service) socket option. Setting this value "
         u"may depend on the user's privilege or operating system configuration.");

    option(u"ttl", 't', INTEGER, 0, 1, 1, 255);
    help(u"ttl",
         u"Specifies the TTL (Time-To-Live) socket option. The actual option "
         u"is either \"Unicast TTL\" or \"Multicast TTL\", depending on the "
         u"destination address. Remember that the default Multicast TTL is 1 "
         u"on most systems.");
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::isRealTime()
{
    return true;
}


//----------------------------------------------------------------------------
// Output command line options method
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::getOptions()
{
    // Call superclass first.
    bool success = AbstractDatagramOutputPlugin::getOptions();

    success = _destination.resolve(value(u""), *tsp) && success;
    const UString local(value(u"local-address"));
    _local_addr.clear();
    success = (local.empty() || _local_addr.resolve(local, *tsp)) && success;
    getIntValue(_local_port, u"local-port", IPv4SocketAddress::AnyPort);
    getIntValue(_ttl, u"ttl", 0);
    getIntValue(_tos, u"tos", -1);
    _mc_loopback = !present(u"disable-multicast-loop");
    _force_mc_local = present(u"force-local-multicast-outgoing");
    setRS204Format(present(u"rs204"));

    return success;
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::start()
{
    // Call superclass first, then initialize UDP socket.
    if (!AbstractDatagramOutputPlugin::start() || !_sock.open(*tsp)) {
        return false;
    }

    // Configure socket.
    const IPv4SocketAddress local(_local_addr, _local_port);
    if ((_local_port != IPv4SocketAddress::AnyPort && !_sock.reusePort(true, *tsp)) ||
        !_sock.bind(local, *tsp) ||
        !_sock.setDefaultDestination(_destination, *tsp) ||
        !_sock.setMulticastLoop(_mc_loopback, *tsp) ||
        (_force_mc_local && _destination.isMulticast() && _local_addr.hasAddress() && !_sock.setOutgoingMulticast(_local_addr, *tsp)) ||
        (_tos >= 0 && !_sock.setTOS(_tos, *tsp)) ||
        (_ttl > 0 && !_sock.setTTL(_ttl, *tsp)))
    {
        _sock.close(*tsp);
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::stop()
{
    // Call superclass first, then close UDP socket.
    AbstractDatagramOutputPlugin::stop();
    _sock.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Implementation of AbstractDatagramOutputPlugin: send one datagram.
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::sendDatagram(const void* address, size_t size)
{
    return _sock.send(address, size, *tsp);
}
