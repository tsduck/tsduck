//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Read a TCP/IP stream from a pcap or pcapng file.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPcapFilter.h"
#include "tsSafePtr.h"

namespace ts {
    //!
    //! Read a TCP/IP session from a pcap or pcapng file.
    //! @ingroup net
    //!
    //! A TCP session uses two continuous streams, one in each direction,
    //! between two socket addresses.
    //!
    //! Filtering a specified TCP stream shall be set using setBidirectionalFilter().
    //! If not set, the first TCP packet defines the TCP session to follow.
    //!
    //! Setting a new filter clears the current state of the previous filter but does not
    //! change the current position inside the pcap file. If any IPv4 address or TCP port is
    //! unspecified in filtered addresses, then the first TCP/IP packet matching the specified
    //! fields is used to determine the unspecified field.
    //!
    //! Once the peers are defined, either because they were fully specified using
    //! setBidirectionalFilter() or the first packet resolved them, use sourceFilter()
    //! and destinationFilter() to get the peers addresses. In that case, "source" and
    //! "destination" are interchangeable since a TCP session is bidirectional.
    //!
    //! Use addressFilterIsSet() to check if the peers are fully specified.
    //!
    //! Some effort is made to reassemble repeated or re-ordered TCP packets.
    //! Fragmented IP packets are ignored. It is not possible to rebuild a
    //! TCP session with fragmented packets.
    //!
    class TSDUCKDLL PcapStream: public PcapFilter
    {
        TS_NOCOPY(PcapStream);
    public:
        //!
        //! Default constructor.
        //!
        PcapStream() = default;

        //!
        //! Get the address of the client peer.
        //! @return A constant reference to the client socket address if the client is known,
        //! no addres/port if the client is unknown. When the pcap capture starts when the TCP
        //! connection is already established, the SYN/ACK sequence is not present and we do
        //! not know which peer is the client.
        //!
        const IPv4SocketAddress& clientPeer() const { return _client; }

        //!
        //! Get the address of the server peer.
        //! @return A constant reference to the server socket address if the server is known,
        //! no addres/port if the server is unknown. When the pcap capture starts when the TCP
        //! connection is already established, the SYN/ACK sequence is not present and we do
        //! not know which peer is the client.
        //!
        const IPv4SocketAddress& serverPeer() const { return _server; }

        //!
        //! Read data from the TCP session either in one specific direction or any direction.
        //!
        //! Reading stops at end of TCP session or end of pcap file. To move to next TCP session,
        //! use nextSession().
        //!
        //! @param [in,out] source Source address of the TCP stream to extract data from. If, on input,
        //! the value is unset (no address, no port), then data from any direction are read. On output,
        //! @a source contains the address of the peer from which data were read.
        //! @param [in,out] data Byte block into which data is read. The byte block is not
        //! reinitialized, input data are appended to it.
        //! @param [in,out] size On input, this is the data size to read in bytes. In absence of
        //! error, that exact number of bytes is read. Reading can stop earlier in case of end of
        //! TCP stream or end of pcap file. On output, it contains the actual number of read bytes.
        //! @param [out] timestamp Capture timestamp in microseconds since Unix epoch or -1 if none is available.
        //! If the data has been reassembled from several IP packets, this is the timestamp of the last part.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error or end of file.
        //!
        bool readTCP(IPv4SocketAddress& source, ByteBlock& data, size_t& size, MicroSecond& timestamp, Report& report);

        //!
        //! Check if the next data to read is at start of TCP session.
        //! @param [in,out] report Where to report errors.
        //! @return True on success and if the next data to read is at start of TCP session, false otherwise.
        //!
        bool startOfStream(Report& report);

        //!
        //! Check if the next data to read is at start of TCP session.
        //! @param [in] source Source address of the TCP stream. It must match one of the peers of the TCP session.
        //! @param [in,out] report Where to report errors.
        //! @return True on success and if the next data to read is at start of TCP session, false otherwise.
        //!
        bool startOfStream(const IPv4SocketAddress& source, Report& report);

        //!
        //! Check if the next data to read is at end of TCP session.
        //! @param [in] source Source address of the TCP stream. It must match one of the peers of the TCP session.
        //! @param [in,out] report Where to report errors.
        //! @return True if the next data to read is at end of TCP session or on error, false otherwise.
        //!
        bool endOfStream(const IPv4SocketAddress& source, Report& report);

        //!
        //! Check if the TCP session is fully terminated on both sides.
        //! @param [in,out] report Where to report errors.
        //! @return True if the TCP session is fully terminated on both sides or on error, false otherwise.
        //!
        bool endOfSession(Report& report) { return endOfStream(0, report) && endOfStream(1, report); }

        //!
        //! Skip the end of the current TCP session and prepare for next session.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error or end of file before an explicit end of current session.
        //!
        bool nextSession(Report& report);

        //!
        //! Get the maximum number of queued data blocks to reassemble TCP streams.
        //! This value gives an idea of how packets were reordered during transmission.
        //! @return The maximum number of queued data blocks to reassemble TCP streams.
        //!
        size_t maxReassemblyQueueSize() const { return _max_queue_size; }

        // Inherited methods.
        virtual bool open(const fs::path& filename, Report& report) override;
        virtual void setBidirectionalFilter(const IPv4SocketAddress& addr1, const IPv4SocketAddress& addr2) override;

    private:
        // Description of one data block from an IP packet.
        class DataBlock
        {
        public:
            DataBlock() = default;
            DataBlock(const IPv4Packet& pkt, MicroSecond tstamp);

            ByteBlock   data {};         // TCP payload
            size_t      index = 0;       // index of next byte to read in data
            uint64_t    sequence = 0;    // TCP sequence number at start of data, expanded for
            bool        start = false;   // start of TCP stream.
            bool        end = false;     // end of TCP stream.
            MicroSecond timestamp = -1;  // capture time stamp.
        };
        typedef SafePtr<DataBlock> DataBlockPtr;
        typedef std::list<DataBlockPtr> DataBlockQueue;

        // Description of a one-directional stream.
        class Stream
        {
        public:
            // Constructor.
            Stream() = default;

            // Future packets to process.
            DataBlockQueue packets {};

            // Check if data are immediately available.
            bool dataAvailable() const;

            // Store the content of an IP packet at the right place in the queue.
            void store(const IPv4Packet& pkt, MicroSecond tstamp);
        };

        // Maximum number of out-of-sequence TCP segments after a segment is declared missing.
        static constexpr size_t TCP_MAX_FUTURE = 10;

        // There are two streams, two directions in a connection.
        // The source filter is at index 0, the destination filter is at index 1.
        static constexpr size_t ISRC = 0;
        static constexpr size_t IDST = 1;

        // PcapStream private fields.
        IPv4SocketAddress     _client {};
        IPv4SocketAddress     _server {};
        std::array<Stream, 2> _streams {};
        size_t                _max_queue_size = 0; // Maximum observed size of TCP reassembly queues.

        // Read IP packets and fill the two streams until one packet is read from the specified peer.
        // Index must be either ISRC, IDST or NPOS (any direction). Updated with actual index.
        bool readStreams(size_t& index, Report& report);

        // Get index for source address. Report an error and return false if incorrect.
        bool indexOf(const IPv4SocketAddress& source, bool allow_unspecified, size_t& index, Report& report) const;

        // These methods are disabled, the corresponding filtering is imposed in this subclass.
        virtual void setProtocolFilterTCP() override;
        virtual void setProtocolFilterUDP() override;
        virtual void setProtocolFilter(const std::set<uint8_t>& protocols) override;
        virtual void clearProtocolFilter() override;
        virtual void setSourceFilter(const IPv4SocketAddress& addr) override;
        virtual void setDestinationFilter(const IPv4SocketAddress& addr) override;
        virtual void setWildcardFilter(bool on) override;
    };
}
