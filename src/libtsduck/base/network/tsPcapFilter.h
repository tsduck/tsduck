//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Read a pcap or pcapng file with packet filtering.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPcapFile.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Read a pcap or pcapng file with packet filtering.
    //!
    //! This class also sets filtering options from the command line:
    //! @c -\-first-packet, @c -\-first-timestamp, @c -\-first-date, @c -\-last-packet, @c -\-last-timestamp, @c -\-last-date.
    //!
    //! @ingroup net
    //!
    class TSDUCKDLL PcapFilter: public PcapFile
    {
        TS_NOCOPY(PcapFilter);
    public:
        //!
        //! Default constructor.
        //!
        PcapFilter() = default;

        //!
        //! Filter packets starting at the specified number.
        //! The packet numbering counts all captured packets from the beginning of the file, starting at 1.
        //! This is the same value as seen on Wireshark in the leftmost column.
        //! @param [in] count Number of first captured packet to read.
        //!
        void setFirstPacketFilter(size_t count) { _first_packet = count; }

        //!
        //! Filter packets up to the specified number.
        //! The packet numbering counts all captured packets from the beginning of the file, starting at 1.
        //! This is the same value as seen on Wireshark in the leftmost column.
        //! @param [in] count Number of last captured packet to read.
        //!
        void setLastPacketFilter(size_t count) { _last_packet = count; }

        //!
        //! Filter packets starting at the specified time offset from the beginning of the file.
        //! This is the same value as seen on Wireshark in the "Time" column (in seconds).
        //! @param [in] time First time offset in microseconds from the beginning of the capture.
        //!
        void setFirstTimeOffset(MicroSecond time) { _first_time_offset = time; }

        //!
        //! Filter packets up to the specified time offset from the beginning of the file.
        //! This is the same value as seen on Wireshark in the "Time" column (in seconds).
        //! @param [in] time Last time offset in microseconds from the beginning of the capture.
        //!
        void setLastTimeOffset(MicroSecond time) { _last_time_offset = time; }

        //!
        //! Filter packets starting at the specified timestamp.
        //! @param [in] time First timestamp, in microseconds from the UNIX epoch.
        //! @see ts::Time::UnixEpoch
        //!
        void setFirstTimestamp(MicroSecond time) { _first_time = time; }

        //!
        //! Filter packets up to the specified timestamp.
        //! @param [in] time Last timestamp, in microseconds from the UNIX epoch.
        //! @see ts::Time::UnixEpoch
        //!
        void setLastTimestamp(MicroSecond time) { _last_time = time; }

        //!
        //! Filter TCP packets only.
        //!
        virtual void setProtocolFilterTCP();

        //!
        //! Filter UDP packets only.
        //!
        virtual void setProtocolFilterUDP();

        //!
        //! Filter packets with the specified set of protocols.
        //! @param [in] protocols A set of 8-bit protocol values (eg. IPv4_PROTO_TCP, IPv4_PROTO_ICMP, etc.)
        //! If the filter is empty, all packets are passed (same as all protocol values set).
        //!
        virtual void setProtocolFilter(const std::set<uint8_t>& protocols);

        //!
        //! Clear the set of protocols to filter, all protocols are accepted.
        //!
        virtual void clearProtocolFilter();

        //!
        //! Set a source address filter.
        //! @param [in] addr Source address to filter. The port is meaningful only with TCP and UDP.
        //! @see setWildcardFilter()
        //!
        virtual void setSourceFilter(const IPv4SocketAddress& addr);

        //!
        //! Set a destination address filter.
        //! @param [in] addr Destination address to filter. The port is meaningful only with TCP and UDP.
        //! @see setWildcardFilter()
        //!
        virtual void setDestinationFilter(const IPv4SocketAddress& addr);

        //!
        //! Set a bidirectional address filter.
        //! Select packets where the source and destination addresses match the pair
        //! of filtered address, in any direction.
        //! @param [in] addr1 First address to filter, either source or destination.
        //! The port is meaningful only with TCP and UDP.
        //! @param [in] addr2 Second address to filter, either source or destination.
        //! The port is meaningful only with TCP and UDP.
        //! @see setWildcardFilter()
        //!
        virtual void setBidirectionalFilter(const IPv4SocketAddress& addr1, const IPv4SocketAddress& addr2);

        //!
        //! Get the current source filter.
        //!
        //! In the case of non-wildcard filtering, after returning the first packet,
        //! this is the actual socket address of the filtered stream.
        //!
        //! In the case of bidirectional filtering (for instance a TCP session), there
        //! is no real "source" or "destination". They are the two endpoints of the stream.
        //!
        //! @return A constant reference to the current source filter.
        //! @see setWildcardFilter()
        //!
        const IPv4SocketAddress& sourceFilter() const { return _source; }

        //!
        //! Get the current destination filter.
        //!
        //! In the case of non-wildcard filtering, after returning the first packet,
        //! this is the actual socket address of the filtered stream.
        //!
        //! In the case of bidirectional filtering (for instance a TCP session), there
        //! is no real "source" or "destination". They are the two endpoints of the stream.
        //!
        //! @return A constant reference to the current source filter.
        //! @see setWildcardFilter()
        //!
        const IPv4SocketAddress& destinationFilter() const { return _destination; }

        //!
        //! Set the source and destination address filter in wildcard mode.
        //!
        //! When the address filter is in wildcard mode (the default), the unspecified parts
        //! of source and destination, address or port, act as wildcard for all packets.
        //!
        //! When the wildcard mode is set to false, the first packet which matches the
        //! unspecified parts forces the addresses of that packet. In other words, the
        //! first packet which matches the wildcard forces a specific stream and all
        //! subsequent packets are filtered for that specific stream.
        //!
        //! @param [in] on If true, use wildcard mode for all packets. If false, the
        //! first packet which matches the wildcard forces a specific stream.
        //!
        virtual void setWildcardFilter(bool on);

        //!
        //! Check if the address filter is precisely set.
        //! Using address filtering, some addresses or port may be unspecified, acting as a wildcard.
        //! In non-wildcard mode, the first packet matching this wildcard is selected and the
        //! corresponding stream is then exclusively filtered. At this point, the filter is precisely set.
        //! @return True if the address filter is precisely set.
        //!
        bool addressFilterIsSet() const;

        //!
        //! Get the "other" filter (source or destination) based on the other one.
        //! @param [in] addr A socket address, typically matching the source or destination filter.
        //! @return A constant reference to the other filter. If @a addr matches neither the source
        //! nor the destination filter, return an empty socket address.
        //!
        const IPv4SocketAddress& otherFilter(const IPv4SocketAddress& addr) const;

        //!
        //! Specify the severity to report the filtered addresses once (in non-wildcard mode).
        //!
        //! In non-wildcard mode, when the filtered addresses contain non-specified fields,
        //! the first packet which matches the filters define the stream. At this point, the
        //! selected stream is displayed (in debug mode by default). This function redefines
        //! the severity level (info or verbose for instance).
        //!
        //! @param [in] level Severity level to use.
        //!
        void setReportAddressesFilterSeverity(int level) { _display_addresses_severity = level; }

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);

        // Inherited methods.
        virtual bool open(const fs::path& filename, Report& report) override;
        virtual bool readIPv4(IPv4Packet& packet, MicroSecond& timestamp, Report& report) override;

    private:
        std::set<uint8_t> _protocols {};
        IPv4SocketAddress _source {};
        IPv4SocketAddress _destination {};
        bool              _bidirectional_filter = false;
        bool              _wildcard_filter = true;
        int               _display_addresses_severity {Severity::Debug};
        size_t            _first_packet = 0;
        size_t            _last_packet {std::numeric_limits<size_t>::max()};
        MicroSecond       _first_time_offset = 0;
        MicroSecond       _last_time_offset {std::numeric_limits<ts::MicroSecond>::max()};
        MicroSecond       _first_time = 0;
        MicroSecond       _last_time {std::numeric_limits<ts::MicroSecond>::max()};
        size_t            _opt_first_packet = 0;
        size_t            _opt_last_packet {std::numeric_limits<size_t>::max()};
        MicroSecond       _opt_first_time_offset = 0;
        MicroSecond       _opt_last_time_offset {std::numeric_limits<ts::MicroSecond>::max()};
        MicroSecond       _opt_first_time = 0;
        MicroSecond       _opt_last_time {std::numeric_limits<ts::MicroSecond>::max()};

        // Get a date option and return it as micro-seconds since Unix epoch.
        ts::MicroSecond getDate(Args& args, const ts::UChar* arg_name, ts::MicroSecond def_value);
    };
}
