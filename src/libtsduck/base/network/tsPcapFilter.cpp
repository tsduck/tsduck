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

#include "tsPcapFilter.h"
#include "tsAlgorithm.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PcapFilter::PcapFilter() :
    PcapFile(),
    _protocols(),
    _source(),
    _destination(),
    _bidirectional_filter(false),
    _wildcard_filter(true),
    _display_addresses_severity(Severity::Debug),
    _first_packet(0),
    _last_packet(std::numeric_limits<size_t>::max()),
    _first_time_offset(0),
    _last_time_offset(std::numeric_limits<ts::MicroSecond>::max()),
    _first_time(0),
    _last_time(std::numeric_limits<ts::MicroSecond>::max())
{
}


//----------------------------------------------------------------------------
// Protocol filters.
//----------------------------------------------------------------------------

void ts::PcapFilter::setProtocolFilterTCP()
{
    _protocols.clear();
    _protocols.insert(IPv4_PROTO_TCP);
}

void ts::PcapFilter::setProtocolFilterUDP()
{
    _protocols.clear();
    _protocols.insert(IPv4_PROTO_UDP);
}

void ts::PcapFilter::setProtocolFilter(const std::set<uint8_t>& protocols)
{
    _protocols = protocols;
}

void ts::PcapFilter::clearProtocolFilter()
{
    _protocols.clear();
}


//----------------------------------------------------------------------------
// Address filters.
//----------------------------------------------------------------------------

void ts::PcapFilter::setSourceFilter(const IPv4SocketAddress& addr)
{
    _source = addr;
    _bidirectional_filter = false;
}

void ts::PcapFilter::setDestinationFilter(const IPv4SocketAddress& addr)
{
    _destination = addr;
    _bidirectional_filter = false;
}

void ts::PcapFilter::setBidirectionalFilter(const IPv4SocketAddress& addr1, const IPv4SocketAddress& addr2)
{
    _source = addr1;
    _destination = addr2;
    _bidirectional_filter = true;
}

void ts::PcapFilter::setWildcardFilter(bool on)
{
    _wildcard_filter = on;
}

bool ts::PcapFilter::addressFilterIsSet() const
{
    const bool use_port = _protocols.empty() || Contains(_protocols, IPv4_PROTO_TCP) || Contains(_protocols, IPv4_PROTO_UDP);
    return _source.hasAddress() &&
           (!use_port || _source.hasPort()) &&
           _destination.hasAddress() &&
           (!use_port || _destination.hasPort());
}


//----------------------------------------------------------------------------
// Open the file, inherited method.
//----------------------------------------------------------------------------

bool ts::PcapFilter::open(const UString& filename, Report& report)
{
    // Invoke superclass.
    const bool ok = PcapFile::open(filename, report);
    if (ok) {
        // Reinitialize filters.
        _protocols.clear();
        _source.clear();
        _destination.clear();
        _bidirectional_filter = false;
        _wildcard_filter = true;
        _first_packet = 0;
        _last_packet = std::numeric_limits<size_t>::max();
        _first_time_offset = 0;
        _last_time_offset = std::numeric_limits<ts::MicroSecond>::max();
        _first_time = 0;
        _last_time = std::numeric_limits<ts::MicroSecond>::max();
    }
    return ok;
}


//----------------------------------------------------------------------------
// Read an IPv4 packet, inherited method.
//----------------------------------------------------------------------------

bool ts::PcapFilter::readIPv4(IPv4Packet& packet, MicroSecond& timestamp, Report& report)
{
    // Read packets until one which matches all filters.
    for (;;) {
        // Invoke superclass to read next packet.
        if (!PcapFile::readIPv4(packet, timestamp, report)) {
            return false;
        }

        // Check final conditions (no need to read further in the file).
        if (packetCount() > _last_packet ||
            timestamp > _last_time ||
            timeOffset(timestamp) > _last_time_offset)
        {
            return false;
        }

        // Check if the packet matches all general filters.
        if ((!_protocols.empty() && !Contains(_protocols, packet.protocol())) ||
            packetCount() < _first_packet ||
            timestamp < _first_time ||
            timeOffset(timestamp) < _first_time_offset)
        {
            // Drop that packet.
            continue;
        }

        // Is there any unspecified field in current stream addresses (act as wildcard)?
        const IPv4SocketAddress src(packet.sourceSocketAddress());
        const IPv4SocketAddress dst(packet.destinationSocketAddress());
        const bool unspecified = !_wildcard_filter && !addressFilterIsSet();
        bool display_filter = false;

        // Check if the IP packet belongs to the filtered session.
        // By default, _source and _destination are empty and match everything.
        if (src.match(_source) && dst.match(_destination)) {
            if (unspecified) {
                _source = src;
                _destination = dst;
                display_filter = true;
            }
        }
        else if (_bidirectional_filter && src.match(_destination) && dst.match(_source)) {
            if (unspecified) {
                _source = dst;
                _destination = src;
                display_filter = true;
            }
        }
        else {
            // Not a packet from that TCP session.
            continue;
        }

        if (display_filter) {
            report.log(_display_addresses_severity, u"selected stream %s %s %s", {_source, _bidirectional_filter ? u"<->" : u"->", _destination});
        }

        report.debug(u"packet: ip size: %'d, data size: %'d, timestamp: %'d", {packet.size(), packet.protocolDataSize(), timestamp});
        return true;
    }
}
