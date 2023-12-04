//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPcapStream.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PcapStream::DataBlock::DataBlock(const IPv4Packet& pkt, MicroSecond tstamp) :
    sequence(pkt.tcpSequenceNumber()),
    start(pkt.tcpSYN()),
    end(pkt.tcpFIN() || pkt.tcpRST()),
    timestamp(tstamp)
{
    if (pkt.isTCP()) {
        if (start) {
            // When a TCP packet has SYN set, the next sequence is +1.
            sequence++;
        }
        data.copy(pkt.protocolData(), pkt.protocolDataSize());
    }
}


//----------------------------------------------------------------------------
// Open the file, inherited method.
//----------------------------------------------------------------------------

bool ts::PcapStream::open(const fs::path& filename, Report& report)
{
    // Invoke superclass.
    const bool ok = PcapFilter::open(filename, report);
    if (ok) {
        // Force TCP filtering on one single stream (any stream, initially).
        PcapFilter::setProtocolFilterTCP();
        PcapFilter::setWildcardFilter(false);
        setBidirectionalFilter(IPv4SocketAddress(), IPv4SocketAddress());

        // Statistics on the stream.
        _max_queue_size = 0;
    }
    return ok;
}


//----------------------------------------------------------------------------
// Set a TCP/IP filter to select one bidirectional stream.
//----------------------------------------------------------------------------

void ts::PcapStream::setBidirectionalFilter(const IPv4SocketAddress& addr1, const IPv4SocketAddress& addr2)
{
    // Invoke superclass.
    PcapFilter::setBidirectionalFilter(addr1, addr2);

    // Client/server roles are initially unknown.
    _client.clear();
    _server.clear();

    // Reset data streams.
    _streams[0].packets.clear();
    _streams[1].packets.clear();
}



//----------------------------------------------------------------------------
// Check if data are immediately available in a stream.
//----------------------------------------------------------------------------

bool ts::PcapStream::Stream::dataAvailable() const
{
    // There must be one packet and it must not be empty.
    // It the first packet is empty, this means that more data are expected
    // after it, before the second packet if there is one.
    return !packets.empty() && packets.front()->index < packets.front()->data.size();
}


//----------------------------------------------------------------------------
// Store the content of an IP packet in a stream.
//----------------------------------------------------------------------------

void ts::PcapStream::Stream::store(const IPv4Packet& pkt, MicroSecond tstamp)
{
    // Allocate a new data block.
    const DataBlockPtr ptr(new DataBlock(pkt, tstamp));

    // Resolve wrap-up of TCP sequence number at 2^32.
    // Use the 32 upper bits of first queued block to get the order of magnitude.
    if (!packets.empty()) {
        // Mask / wrap-up boundary for sequence numbers:
        constexpr uint64_t max_seq = 0xFFFFFFFF;
        // If within that range around boundary, there is a wrap-up (say max 10 packets out-of-order):
        constexpr uint64_t range = 10 * TCP_MAX_PAYLOAD_SIZE;
        // Original sequence number in first queued block:
        const uint64_t first_full_seq = packets.front()->sequence;
        const uint64_t first_seq = first_full_seq & max_seq;
        uint64_t msb = first_full_seq >> 32;
        // Adjust captured sequence number.
        if (ptr->sequence > max_seq - range && first_seq < range) {
            // Captured block is before first block and wrap-up occured between the two.
            msb--;
        }
        else if (first_seq > max_seq - range && ptr->sequence < range) {
            // Captured block is after first block and wrap-up occured between the two.
            msb++;
        }
        ptr->sequence |= msb << 32;
    }

    // Find the right location in the queue.
    auto it = packets.begin();
    while (it != packets.end()) {
        DataBlock& db(**it);
        if (ptr->sequence == db.sequence) {
            // Same position. If the new packet has more data, add them to existing packet at that position.
            if (ptr->data.size() > db.data.size()) {
                db.data.append(ptr->data.data() + db.data.size(), ptr->data.size() - db.data.size());
            }
            return;
        }
        else if (ptr->sequence < db.sequence) {
            // Need to be inserted here, next packet is after.
            // Detect and truncate any overlap.
            const uint64_t diff = db.sequence - ptr->sequence;
            if (ptr->data.size() > diff) {
                ptr->data.resize(size_t(diff));
            }
            break;
        }
        else {
            // Need to be inserted after this one but check if there is any overlap.
            const uint64_t diff = ptr->sequence - db.sequence;
            if (db.data.size() > diff) {
                // The packet at **it overlaps on packet to insert.
                const uint64_t extra = db.data.size() - diff;
                if (ptr->data.size() <= extra) {
                    // Packet to insert is fully overlapped, drop it.
                    return;
                }
                else {
                    // Remove overlapping start of the packet to insert.
                    ptr->data.erase(0, size_t(extra));
                    ptr->sequence += extra;
                }
            }
        }
        ++it;
    }

    // Ignore empty packets without start/stop indicator (eg. keep-alive packets). They bring no value.
    // Do not store packet before the first one, it would be before already returned data.
    if ((!ptr->data.empty() || ptr->start || ptr->end) && (packets.empty() || it != packets.begin())) {

        // Actually insert the packet at its destination.
        it = packets.insert(it, ptr);

        // If the previous packet is empty and adjacent, it was waiting for the next
        // adjacent packet and we may remove it now.
        if (it != packets.begin()) {
            const uint64_t seq = (*it)->sequence;
            --it;
            const DataBlock& db(**it);
            if (db.index >= db.data.size() && db.sequence + db.data.size() == seq) {
                // Propagate start of stream from previous packet to this one.
                if (db.start && db.data.empty()) {
                    ptr->start = true;
                }
                // Remove previous packet.
                packets.erase(it);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Read IP packets and fill the two streams until one packet is read.
//----------------------------------------------------------------------------

bool ts::PcapStream::readStreams(size_t& source, Report& report)
{
    IPv4Packet pkt;
    MicroSecond timestamp = -1;
    size_t pkt_source = NPOS;

    // Loop on reading packet, return on error or packet found.
    for (;;) {

        // Get one IPv4 packet.
        if (!readIPv4(pkt, timestamp, report)) {
            return false;
        }

        // Ignore non-TCP packets.
        if (!pkt.isTCP()) {
            continue;
        }

        // Also ignored fragmented IP packets.
        if (pkt.fragmented()) {
            report.debug(u"got fragmented IP packet in TCP stream, sync lost on that TCP stream");
            continue;
        }

        // Check the direction of the IP packet in the filtered session.
        const IPv4SocketAddress src(pkt.sourceSocketAddress());
        const IPv4SocketAddress dst(pkt.destinationSocketAddress());
        if (src.match(sourceFilter()) && dst.match(destinationFilter())) {
            pkt_source = ISRC;
        }
        else if (src.match(destinationFilter()) && dst.match(sourceFilter())) {
            pkt_source = IDST;
        }
        else {
            // Not a packet from that TCP session. Shouldn't happen since the filter is set in the superclass.
            report.error(u"internal error in PcapStream::readStreams(), unexpected packet %s -> %s in stream %s <-> %s", {src, dst, sourceFilter(), destinationFilter()});
            return false;
        }

        // Determine client and server roles at the beginning of a TCP session.
        if (pkt.tcpSYN()) {
            if (pkt.tcpACK()) {
                // SYN/ACK: the source is the server.
                _client = dst;
                _server = src;
            }
            else {
                // SYN alone: the source is the client.
                _client = src;
                _server = dst;
            }
        }

        // Store the packet in the stream.
        _streams[pkt_source].store(pkt, timestamp);
        _max_queue_size = std::max(_max_queue_size, _streams[pkt_source].packets.size());

        // Stop when a packet was read from the specified peer.
        if ((source == pkt_source || (source != ISRC && source != IDST)) && !_streams[pkt_source].packets.empty()) {
            source = pkt_source;
            return true;
        }
    }
}


//----------------------------------------------------------------------------
// Read data from the TCP session, any direction.
//----------------------------------------------------------------------------

bool ts::PcapStream::readTCP(IPv4SocketAddress& source, ByteBlock& data, size_t& size, MicroSecond& timestamp, Report& report)
{
    size_t remain = size;
    size = 0;
    timestamp = -1;

    // Check the direction of the requested stream.
    size_t peer_number = NPOS;
    if (!indexOf(source, true, peer_number, report)) {
        return false;
    }

    // If the peer is unspecified, select which one we will use.
    if (peer_number != ISRC && peer_number != IDST) {
        // Loop until some data are available.
        for (;;) {
            const bool src_avail = _streams[ISRC].dataAvailable();
            const bool dst_avail = _streams[IDST].dataAvailable();
            if (src_avail && dst_avail) {
                // Data available in both, choose the one with older data.
                peer_number = _streams[ISRC].packets.front()->timestamp <= _streams[IDST].packets.front()->timestamp ? ISRC : IDST;
                break;
            }
            else if (src_avail) {
                peer_number = ISRC;
                break;
            }
            else if (dst_avail) {
                peer_number = IDST;
                break;
            }
            else if (!readStreams(peer_number, report)){
                // No data available, tried to read in first available direction, but failed.
                return false;
            }
        }
   }

    // Update source with full address, if was not or partially specified.
    assert(peer_number == ISRC || peer_number == IDST);
    source = peer_number == ISRC ? sourceFilter() : destinationFilter();

    // Read data from the selected stream.
    Stream& stream(_streams[peer_number]);
    while (remain > 0) {

        // If no buffered data are available, read more packets.
        while (!stream.dataAvailable()) {
            if (stream.packets.size() > TCP_MAX_FUTURE) {
                report.error(u"missing TCP segment, too many future out-of-sequence segments");
                return size > 0;
            }
            if (!readStreams(peer_number, report)) {
                return size > 0;
            }
        }
        assert(!stream.packets.empty());

        // Get data from the first packet.
        auto first = stream.packets.front();
        if (first->index < first->data.size()) {
            const size_t chunk = std::min(remain, first->data.size() - first->index);
            data.append(&first->data[first->index], chunk);
            remain -= chunk;
            size += chunk;
            first->index += chunk;
            timestamp = first->timestamp;
        }
        if (first->end) {
            return size > 0; // end of TCP stream
        }

        // Drop the first packet if adjacent to the second.
        auto second = stream.packets.begin();
        ++second;
        if (second != stream.packets.end() && (*second)->sequence == first->sequence + first->data.size()) {
            stream.packets.pop_front();
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Skip the end of the current TCP session and prepare for next session.
//----------------------------------------------------------------------------

bool ts::PcapStream::nextSession(Report& report)
{
    for (;;) {

        // Remove all leading packets on both sides up to an end of session.
        for (size_t num = 0; num < _streams.size(); ++num) {
            while (!_streams[num].packets.empty() && !_streams[num].packets.front()->end) {
                _streams[num].packets.pop_front();
            }
        }

        // Exit when explicit end of session is reached on both directions.
        if (!_streams[ISRC].packets.empty() && _streams[ISRC].packets.front()->end && !_streams[IDST].packets.empty() && _streams[IDST].packets.front()->end) {
            // Drop the ends of streams.
            _streams[ISRC].packets.pop_front();
            _streams[IDST].packets.pop_front();
            return true;
        }

        // Read packets from either direction (start of next session).
        size_t num = NPOS;
        if (!readStreams(num, report)) {
            return false; // end of file or error
        }
    }
}


//----------------------------------------------------------------------------
// Get index for source address. Return false if incorrect.
//----------------------------------------------------------------------------

bool ts::PcapStream::indexOf(const IPv4SocketAddress& source, bool allow_unspecified, size_t& index, Report& report) const
{
    const bool unspecified = !source.hasAddress() && !source.hasPort();
    if (allow_unspecified && unspecified) {
        index = NPOS;
        return true;
    }
    else if (!unspecified && source.match(sourceFilter())) {
        index = ISRC;
        return true;
    }
    else if (!unspecified && source.match(destinationFilter())) {
        index = IDST;
        return true;
    }
    else {
        report.error(u"invalid source address %s for TCP stream %s <-> %s", {source, sourceFilter(), destinationFilter()});
        index = NPOS;
        return false;
    }
}


//----------------------------------------------------------------------------
// Position of the next data to read.
//----------------------------------------------------------------------------

bool ts::PcapStream::startOfStream(Report& report)
{
    // Each side must be either empty or at start.
    if (!_streams[ISRC].packets.empty() && !_streams[IDST].packets.empty()) {
        return _streams[ISRC].packets.front()->start && _streams[IDST].packets.front()->start;
    }
    else if (!_streams[ISRC].packets.empty()) {
        return _streams[ISRC].packets.front()->start;
    }
    else if (!_streams[IDST].packets.empty()) {
        return _streams[IDST].packets.front()->start;
    }
    else {
        // Both sides are empty, need to read until the first packet of the session is found.
        size_t index = NPOS;
        return readStreams(index, report) && _streams[index].packets.front()->start;
    }
}

bool ts::PcapStream::startOfStream(const IPv4SocketAddress& source, Report& report)
{
    size_t index = NPOS;
    return indexOf(source, false, index, report) &&
           (!_streams[index].packets.empty() || readStreams(index, report)) &&
           _streams[index].packets.front()->start;
}

bool ts::PcapStream::endOfStream(const IPv4SocketAddress& source, Report& report)
{
    size_t index = NPOS;
    if (!indexOf(source, false, index, report) || (_streams[index].packets.empty() && !readStreams(index, report))) {
        return true; // error = end of stream
    }
    else {
        return _streams[index].packets.front()->end;
    }
}


//----------------------------------------------------------------------------
// These methods are disabled, the corresponding filtering is imposed.
//----------------------------------------------------------------------------

void ts::PcapStream::setProtocolFilterTCP()
{
    PcapFilter::setProtocolFilterTCP(); // enforce TCP
}

void ts::PcapStream::setProtocolFilterUDP()
{
    PcapFilter::setProtocolFilterTCP(); // enforce TCP
}

void ts::PcapStream::setProtocolFilter(const std::set<uint8_t>& protocols)
{
    PcapFilter::setProtocolFilterTCP(); // enforce TCP
}

void ts::PcapStream::clearProtocolFilter()
{
    PcapFilter::setProtocolFilterTCP(); // enforce TCP
}

void ts::PcapStream::setSourceFilter(const IPv4SocketAddress& addr)
{
    // Ignore "source" filter, must be bidirectional.
}

void ts::PcapStream::setDestinationFilter(const IPv4SocketAddress& addr)
{
    // Ignore "destination" filter, must be bidirectional.
}

void ts::PcapStream::setWildcardFilter(bool on)
{
    PcapFilter::setWildcardFilter(false); // no wildcard, always one single bidirectional stream
}
