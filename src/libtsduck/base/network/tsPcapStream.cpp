//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tsPcapStream.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PcapStream::PcapStream() :
    PcapFile(),
    _client_num(0),
    _server_num(0),
    _peers(2)
{
}


//----------------------------------------------------------------------------
// Get the IPv4 address and TCP port of a peer.
//----------------------------------------------------------------------------

ts::IPv4SocketAddress ts::PcapStream::peer(PeerNumber number) const
{
    return number == 1 || number == 2 ? _peers[number - 1] : IPv4SocketAddress();
}


//----------------------------------------------------------------------------
// Set a TCP/IP filter to select one bidirectional stream.
//----------------------------------------------------------------------------

void ts::PcapStream::setFilter(const IPv4SocketAddress& peer1, const IPv4SocketAddress& peer2)
{
    _client_num = _server_num = 0;
    _peers[0] = peer1;
    _peers[1] = peer2;
}


//----------------------------------------------------------------------------
// Check if an IP packet matches the current TCP stream.
//----------------------------------------------------------------------------

bool ts::PcapStream::matchStream(const IPv4Packet& pkt, PeerNumber& source, Report& report)
{
    source = 0;

    if (!pkt.isTCP()) {
        return false;
    }

    const IPv4SocketAddress src(pkt.sourceSocketAddress());
    const IPv4SocketAddress dst(pkt.destinationSocketAddress());

    // Is there any unspecified field in current stream addresses (act as wildcard).
    const bool unspecified = !_peers[0].hasAddress() || !_peers[0].hasPort() || !_peers[1].hasAddress() || !_peers[1].hasPort();

    if (src.match(_peers[0]) && dst.match(_peers[1])) {
        source = 1;
        if (unspecified) {
            _peers[0] = src;
            _peers[1] = dst;
            report.debug(u"using %s <-> %s TCP stream", {src, dst});
        }
        return true;
    }
    if (dst.match(_peers[0]) && src.match(_peers[1])) {
        source = 2;
        if (unspecified) {
            _peers[0] = dst;
            _peers[1] = src;
            report.debug(u"using %s <-> %s TCP stream", {dst, src});
        }
        return true;
    }
    return false;
}


//----------------------------------------------------------------------------
// Read data from the TCP stream.
//----------------------------------------------------------------------------

bool ts::PcapStream::readTCP(ReadStatus& status, PeerNumber& peer_num, ByteBlock& data, size_t& size, Report& report)
{
    IPv4Packet pkt;
    bool tcp_syn = false;
    bool tcp_rst = false;
    bool tcp_fin = false;
    bool eof = false;
    uint32_t tcp_sequence = 0;
    MicroSecond timestamp = -1;
    PeerNumber source_num = 0;

    // Size to read.
    size_t remain = 0;
    size = 0;

    // Loop on IPv4 packets until error or enough data.
    while (remain > 0) {

        // Get one IPv4 packet.
        if (!readIPv4(pkt, timestamp, report)) {
            eof = true;
            break;
        }

        // Skip packet if not from the requested sequence.
        if (!matchStream(pkt, source_num, report)) {
            continue;
        }

        //@@@
    }
    status = ReadStatus(tcp_syn, tcp_rst, tcp_fin, eof, tcp_sequence, timestamp);
    return !eof || size > 0;
}
