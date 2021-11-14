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
//!
//!  @file
//!  Read a TCP/IP stream from a pcap or pcapng file.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPcapFile.h"
#include "tsSafePtr.h"

namespace ts {
    //!
    //! Read a TCP/IP stream from a pcap or pcapng file.
    //! @ingroup net
    //!
    //! Some effort is made to reorder repeated or re-ordered TCP packets.
    //!
    //! Fragmented IP packets are ignored. It is not possible to rebuild a
    //! TCP session with fragmented packets.
    //!
    class TSDUCKDLL PcapStream: public PcapFile
    {
        TS_NOCOPY(PcapStream);
    public:
        //!
        //! Default constructor.
        //!
        PcapStream();

        //!
        //! Definition of a peer number.
        //!
        //! A TCP/IP connection involves two peers, numbered 1 and 2. It is not always possible to
        //! determine which is the client and which is the server. When the pcap capture starts when
        //! the TCP connection is already established, we cannot observe the SYN/ACK sequence which
        //! determines the respective roles. This is why we just specify "peer 1" and "peer 2".
        //!
        //! The value 0 (or any value other than 1 or 2) means "any of the two peers".
        //!
        typedef uint32_t PeerNumber;

        //!
        //! Set a TCP/IP filter to select one bidirectional stream.
        //!
        //! Setting a new filter clears the current state of the previous filter.
        //! If any IPv4 address or TCP port is unspecified in @a peer1 or @a peer2, then the first
        //! TCP/IP packet matching the specified fields is used to determine the unspecified field.
        //! In short, if nothing is specified, the first TCP/IP packet will be used to specify the
        //! peers.
        //!
        //! @param [in] peer1 IPv4 address and TCP port of peer 1.
        //! @param [in] peer2 IPv4 address and TCP port of peer 2.
        //!
        void setFilter(const IPv4SocketAddress& peer1, const IPv4SocketAddress& peer2);

        //!
        //! Get the IPv4 address and TCP port of a peer.
        //! @param [in] number Peer number, must be 1 or 2.
        //! @return The IPv4 address and TCP port of the specified peer.
        //! Return an empty address is @a number is neither 1 nor 2.
        //!
        IPv4SocketAddress peer(PeerNumber number) const;

        //!
        //! Get the number of the client peer.
        //! @return Either 1 or 2 if the client is known, 0 if the client is unknown.
        //! When the pcap capture starts when the TCP connection is already established, the
        //! SYN/ACK sequence is not present and we do not know which peer is the client.
        //!
        PeerNumber clientPeer() const { return _client_num; }

        //!
        //! Get the number of the server peer.
        //! @return Either 1 or 2 if the server is known, 0 if the server is unknown.
        //! When the pcap capture starts when the TCP connection is already established, the
        //! SYN/ACK sequence is not present and we do not know which peer is the server.
        //!
        PeerNumber serverPeer() const { return _server_num; }

        //!
        //! Description of a readTCP() status.
        //!
        class TSDUCKDLL ReadStatus
        {
        private:
            uint8_t     _state;
            uint32_t    _sequence;
            MicroSecond _timestamp;
        public:
            //!
            //! Constructor.
            //! @param [in] syn True if a TCP SYN/ACK was present at the beginning of the returned data.
            //! @param [in] rst True is a TCP RST is present at the end of the returned data.
            //! @param [in] fin True is a TCP RST is present at the end of the returned data.
            //! @param [in] eof True if the end of the pcap file is reached.
            //! @param [in] sequence TCP sequence number at end of data.
            //! @param [in] timestamp Capture timestamp in microseconds since Unix epoch or -1 if none is available
            //!
            ReadStatus(bool syn = false, bool rst = false, bool fin = false, bool eof = false, uint32_t sequence = 0, MicroSecond timestamp = -1) :
                _state((syn ? 0x01 : 0x00) | (rst ? 0x02 : 0x00) | (fin ? 0x04 : 0x00) | (eof ? 0x08 : 0x00)),
                _sequence(sequence),
                _timestamp(timestamp)
            {}

            //!
            //! Check if the returned data start at the beginning of a TCP session.
            //! @return True if the returned data start at the beginning of a TCP session.
            //!
            bool startOfStream() const { return (_state & 0x01) != 0; }

            //!
            //! Check if the returned data terminate a TCP session.
            //! @return True if the returned data terminate a TCP session.
            //!
            bool endOfStream() const { return (_state & 0x06) != 0; }

            //!
            //! Check if the returned data terminate the pcap file.
            //! @return True if the returned data terminate the pcap file.
            //!
            bool endOfFile() const { return (_state & 0x08) != 0; }

            //!
            //! Get the TCP sequence number at the end of the returned data.
            //! @return The TCP sequence number at the end of the returned data.
            //!
            uint32_t sequence() const { return _sequence; }

            //!
            //! Get the capture timestamp at the end of the returned data.
            //! @return The capture timestamp in microseconds since Unix epoch at the end of the returned data.
            //! Return -1 if none available.
            //!
            MicroSecond timestamp() const { return _timestamp; }
        };

        //!
        //! Read data from the TCP stream either in one specific direction or any direction.
        //! @param [out] status A status object describing the state of the read operation.
        //! @param [in,out] peer Peer number to extract data from. If, on input, the value
        //! is neither 1 nor 2, then any data from any peer is read. On output, @a peer
        //! contains the number of the peer from which data were read.
        //! @param [in,out] data Byte block into which data is read. The byte block is not
        //! reinitialized, input data are appended to it.
        //! @param [in,out] size On input, this is the data size to read in bytes. In absence
        //! of error, that exact number of bytes is read. Reading can stop earlier in case of
        //! TCP reset (end of stream) or end of pcap file. On output, it contains the actual
        //! number of read bytes.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error or end of file.
        //!
        bool readTCP(ReadStatus& status, PeerNumber& peer, ByteBlock& data, size_t& size, Report& report);

    private:
        // Description of one data block from an IP packet.
        class DataBlock
        {
        public:
            DataBlock();
            DataBlock(const IPv4Packet& pkt);

            ByteBlock data;     // TCP payload
            size_t    index;    // index of next byte to read in data
            uint32_t  sequence; // TCP sequence number at start of data
            bool      start;    // Start of TCP stream.
            bool      end;      // End of TCP stream.
        };
        typedef SafePtr<DataBlock> DataBlockPtr;
        typedef std::list<DataBlockPtr> DataBlockQueue;

        // Description of a one-directional stream (there are two directions in a connection).
        class Stream
        {
        public:
            Stream();

            DataBlockQueue packets; // future packets to process

            // Check if data of the specified size are immediately available.
            bool available(size_t size) const;

            // Store the content of an IP packet.
            void store(const IPv4Packet& pkt);
        };

        // PcapStream private fields.
        PeerNumber              _client_num;
        PeerNumber              _server_num;
        IPv4SocketAddressVector _peers;
        std::vector<Stream>     _streams; // source has same index as _peers

        // Other peer number: turn 1 into 2 and 2 into 1.
        static PeerNumber OtherPeer(PeerNumber peer) { return 3 - peer; }

        // Check if an IP packet matches the current TCP stream.
        bool matchStream(const IPv4Packet& pkt, PeerNumber& source, Report& report);
    };
}
