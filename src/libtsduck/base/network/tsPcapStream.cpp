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
    _peers(2),
    _streams(2)
{
}

// Description of one data block from an IP packet.
ts::PcapStream::DataBlock::DataBlock() :
    data(),
    index(0),
    sequence(0),
    start(false),
    end(false)
{
}

ts::PcapStream::DataBlock::DataBlock(const IPv4Packet& pkt) :
    data(),
    index(0),
    sequence(pkt.tcpSequenceNumber()),
    start(pkt.tcpSYN()),
    end(pkt.tcpFIN() || pkt.tcpRST())
{
    if (pkt.isTCP()) {
        data.copy(pkt.protocolData(), pkt.protocolDataSize());
    }
}

// Description of a one-directional stream (there are two directions in a connection).
ts::PcapStream::Stream::Stream() :
    packets()
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

    // Ignore non-TCP packets. Also ignored fragmented IP packets.
    if (!pkt.isTCP() || pkt.fragmented()) {
        return false;
    }

    const IPv4SocketAddress src(pkt.sourceSocketAddress());
    const IPv4SocketAddress dst(pkt.destinationSocketAddress());

    // Is there any unspecified field in current stream addresses (act as wildcard)?
    // If yes and a wildcard match is found, this packet will define the stream end-points.
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
// Check if data are immediately available in a stream.
//----------------------------------------------------------------------------

bool ts::PcapStream::Stream::available(size_t size) const
{
    // Loop on packets until we get enough data.
    for (auto it = packets.begin(); size > 0 && it != packets.end(); ++it) {
        const DataBlock& db(**it);
        assert(db.index <= db.data.size());
        if (db.index + size <= db.data.size()) {
            // Enough data in that packet.
            return true;
        }
        size -= db.data.size() - db.index;
    }
    return size == 0;
}


//----------------------------------------------------------------------------
// Store the content of an IP packet in a stream.
//----------------------------------------------------------------------------

void ts::PcapStream::Stream::store(const IPv4Packet& pkt)
{
    // Allocate a new data block.
    const DataBlockPtr ptr(new DataBlock(pkt));

    // Find the right location in the queue.
    auto it = packets.begin();
    while (it != packets.end()) {
        const DataBlock& db(**it);
        if (ptr->sequence == db.sequence) {
            // Duplicate packet, drop it.
            return;
        }
        else if (TCPOrderedSequence(ptr->sequence, db.sequence)) {
            // Need to be inserted here, next packet is after.
            // Detect and truncate any overlap.
            const size_t diff = TCPSequenceDiff(ptr->sequence, db.sequence);
            if (ptr->data.size() > diff) {
                ptr->data.resize(diff);
            }
            break;
        }
        else {
            // Need to be inserted after this one but check if there is any overlap.
            const size_t diff = TCPSequenceDiff(db.sequence, ptr->sequence);
            if (db.data.size() > diff) {
                // The packet at **it overlaps on packet to insert.
                const size_t extra = db.data.size() - diff;
                if (ptr->data.size() <= extra) {
                    // Packet to insert is fully overlapped, drop it.
                    return;
                }
                else {
                    // Remove overlapping start of the packet to insert.
                    ptr->data.erase(0, extra);
                    ptr->sequence += uint32_t(extra);
                }
            }
        }
        ++it;
    }
    packets.insert(it, ptr);
}


//----------------------------------------------------------------------------
// Read data from the TCP session, any direction.
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
        assert(source_num == 1 || source_num == 2);

        // Determine client and server role at the beginning of a TCP session.
        if (pkt.tcpSYN()) {
            if (pkt.tcpACK()) {
                // SYN/ACK: the source is the server.
                _client_num = OtherPeer(source_num);
                _server_num = source_num;
            }
            else {
                // SYN alone: the source is the client.
                _client_num = source_num;
                _server_num = OtherPeer(source_num);
            }
        }

        //@@@
    }
    status = ReadStatus(tcp_syn, tcp_rst, tcp_fin, eof, tcp_sequence, timestamp);
    return !eof || size > 0;
}
