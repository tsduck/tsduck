//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPcapFilter.h"
#include "tsAlgorithm.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Define command line filtering options.
//----------------------------------------------------------------------------

void ts::PcapFilter::defineArgs(Args& args)
{
    args.option(u"first-packet", 0, Args::POSITIVE);
    args.help(u"first-packet",
         u"Filter packets starting at the specified number. "
         u"The packet numbering counts all captured packets from the beginning of the file, starting at 1. "
         u"This is the same value as seen on Wireshark in the leftmost column.");

    args.option(u"first-timestamp", 0, Args::UNSIGNED);
    args.help(u"first-timestamp", u"micro-seconds",
         u"Filter packets starting at the specified timestamp in micro-seconds from the beginning of the capture. "
         u"This is the same value as seen on Wireshark in the \"Time\" column (in seconds).");

    args.option(u"first-date", 0, Args::STRING);
    args.help(u"first-date", u"date-time",
         u"Filter packets starting at the specified date. Use format YYYY/MM/DD:hh:mm:ss.mmm.");

    args.option(u"last-packet", 0, Args::POSITIVE);
    args.help(u"last-packet",
         u"Filter packets up to the specified number. "
         u"The packet numbering counts all captured packets from the beginning of the file, starting at 1. "
         u"This is the same value as seen on Wireshark in the leftmost column.");

    args.option(u"last-timestamp", 0, Args::UNSIGNED);
    args.help(u"last-timestamp", u"micro-seconds",
         u"Filter packets up to the specified timestamp in micro-seconds from the beginning of the capture. "
         u"This is the same value as seen on Wireshark in the \"Time\" column (in seconds).");

    args.option(u"last-date", 0, Args::STRING);
    args.help(u"last-date", u"date-time",
         u"Filter packets up to the specified date. Use format YYYY/MM/DD:hh:mm:ss.mmm.");
}


//----------------------------------------------------------------------------
// Get a date option and return it as micro-seconds since Unix epoch.
//----------------------------------------------------------------------------

ts::MicroSecond ts::PcapFilter::getDate(Args& args, const ts::UChar* arg_name, ts::MicroSecond def_value)
{
    ts::Time date;
    const ts::UString str(args.value(arg_name));
    if (str.empty()) {
        return def_value;
    }
    else if (!date.decode(str, ts::Time::ALL)) {
        args.error(u"invalid date \"%s\", use format \"YYYY/MM/DD:hh:mm:ss.mmm\"", {str});
        return def_value;
    }
    else if (date < ts::Time::UnixEpoch) {
        args.error(u"invalid date %s, must be after %s", {str, ts::Time::UnixEpoch});
        return def_value;
    }
    else {
        return (date - ts::Time::UnixEpoch) * ts::MicroSecPerMilliSec;
    }
}


//----------------------------------------------------------------------------
// Load command line filtering options.
//----------------------------------------------------------------------------

bool ts::PcapFilter::loadArgs(DuckContext& duck, Args& args)
{
    args.getIntValue(_opt_first_packet, u"first-packet", 0);
    args.getIntValue(_opt_last_packet, u"last-packet", std::numeric_limits<size_t>::max());
    args.getIntValue(_opt_first_time_offset, u"first-timestamp", 0);
    args.getIntValue(_opt_last_time_offset, u"last-timestamp", std::numeric_limits<ts::MicroSecond>::max());
    _opt_first_time = getDate(args, u"first-date", 0);
    _opt_last_time = getDate(args, u"last-date", std::numeric_limits<ts::MicroSecond>::max());
    return true;
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

const ts::IPv4SocketAddress& ts::PcapFilter::otherFilter(const IPv4SocketAddress& addr) const
{
    if (addr.match(_source)) {
        return _destination;
    }
    else if (addr.match(_destination)) {
        return _source;
    }
    else {
        return IPv4SocketAddress::AnySocketAddress;
    }
}


//----------------------------------------------------------------------------
// Open the file, inherited method.
//----------------------------------------------------------------------------

bool ts::PcapFilter::open(const fs::path& filename, Report& report)
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
        _first_packet = _opt_first_packet;
        _last_packet = _opt_last_packet;
        _first_time_offset = _opt_first_time_offset;
        _last_time_offset = _opt_last_time_offset;
        _first_time = _opt_first_time;
        _last_time = _opt_last_time;
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

        report.log(2, u"packet: ip size: %'d, data size: %'d, timestamp: %'d", {packet.size(), packet.protocolDataSize(), timestamp});
        return true;
    }
}
