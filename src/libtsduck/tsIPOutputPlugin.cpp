//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
TSDUCK_SOURCE;

// Grouping TS packets in UDP packets

#define DEF_PACKET_BURST    7  // 1316 B, fits (with headers) in Ethernet MTU
#define MAX_PACKET_BURST  128  // ~ 48 kB


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::IPOutputPlugin::IPOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send TS packets using UDP/IP, multicast or unicast", u"[options] address:port"),
    _destination(),
    _local_addr(),
    _ttl(0),
    _tos(-1),
    _pkt_burst(DEF_PACKET_BURST),
    _enforce_burst(false),
    _sock(false, *tsp_),
    _out_count(0),
    _out_buffer()
{
    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"The parameter address:port describes the destination for UDP packets. "
         u"The 'address' specifies an IP address which can be either unicast or "
         u"multicast. It can be also a host name that translates to an IP address. "
         u"The 'port' specifies the destination UDP port.");

    option(u"enforce-burst", 'e');
    help(u"enforce-burst",
         u"Enforce that the number of TS packets per UDP packet is exactly what is specified "
         u"in option --packet-burst. By default, this is only a maximum value.");

    option(u"local-address", 'l', STRING);
    help(u"local-address",
         u"When the destination is a multicast address, specify the IP address "
         u"of the outgoing local interface. It can be also a host name that "
         u"translates to a local address.");

    option(u"packet-burst", 'p', INTEGER, 0, 1, 1, MAX_PACKET_BURST);
    help(u"packet-burst",
         u"Specifies the maximum number of TS packets per UDP packet. "
         u"The default is " TS_STRINGIFY(DEF_PACKET_BURST) u", the maximum is " TS_STRINGIFY(MAX_PACKET_BURST) u".");

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
// Output command line options method
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::getOptions()
{
    // Get command line arguments
    getValue(_destination, u"");
    getValue(_local_addr, u"local-address");
    _ttl = intValue<int>(u"ttl", 0);
    _tos = intValue<int>(u"tos", -1);
    _pkt_burst = intValue<size_t>(u"packet-burst", DEF_PACKET_BURST);
    _enforce_burst = present(u"enforce-burst");
    return true;
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::start()
{
    // Create UDP socket
    if (!_sock.open(*tsp)) {
        return false;
    }

    // Configure socket.
    if (!_sock.setDefaultDestination(_destination, *tsp) ||
        (!_local_addr.empty() && !_sock.setOutgoingMulticast(_local_addr, *tsp)) ||
        (_tos >= 0 && !_sock.setTOS(_tos, *tsp)) ||
        (_ttl > 0 && !_sock.setTTL(_ttl, *tsp)))
    {
        _sock.close(*tsp);
        return false;
    }

    // The output buffer is empty.
    if (_enforce_burst) {
        _out_buffer.resize(_pkt_burst);
        _out_count = 0;
    }
    return true;
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::stop()
{
    _sock.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::IPOutputPlugin::send(const TSPacket* pkt, size_t packet_count)
{
    // Send TS packets in UDP messages, grouped according to burst size.
    // Minimum number of TS packets per UDP packet.
    assert(_pkt_burst > 0);
    const size_t min_burst = _enforce_burst ? _pkt_burst - 1 : 0;

    // First, with --enforce-burst, fill partial output buffer.
    if (_out_count > 0) {
        assert(_enforce_burst);
        assert(_out_count < _pkt_burst);

        // Copy as many packets as possible in output buffer.
        const size_t count = std::min(packet_count, _pkt_burst - _out_count);
        TSPacket::Copy(&_out_buffer[_out_count], pkt, count);
        pkt += count;
        packet_count -= count;
        _out_count += count;

        // Send if the output buffer when full.
        if (_out_count == _pkt_burst) {
            if (!_sock.send(_out_buffer.data(), _out_count * PKT_SIZE, *tsp)) {
                return false;
            }
            _out_count = 0;
        }
    }

    // Send subsequent packets from the global buffer.
    while (packet_count > min_burst) {
        size_t count = std::min(packet_count, _pkt_burst);
        if (!_sock.send(pkt, count * PKT_SIZE, *tsp)) {
            return false;
        }
        pkt += count;
        packet_count -= count;
    }

    // If remaining packets are present, save them in output buffer.
    if (packet_count > 0) {
        assert(_enforce_burst);
        assert(_out_count == 0);
        assert(packet_count < _pkt_burst);
        TSPacket::Copy(_out_buffer.data(), pkt, packet_count);
        _out_count = packet_count;
    }
    return true;
}
