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
//!  Pcap and pcapng file.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsMemory.h"

namespace ts {
    //!
    //! Read a pcap or pcapng capture file format.
    //! @ingroup net
    //!
    //! This is the type of files which is created by Wireshark.
    //! This class reads a pcap or pcapng file and extracts IPv4 frames.
    //! All metadata and all other types of frames are ignored.
    //!
    //! @see https://pcapng.github.io/pcapng/draft-gharris-opsawg-pcap.html (PCAP)
    //! @see https://pcapng.github.io/pcapng/draft-tuexen-opsawg-pcapng.html (PCAP-ng)
    //!
    class TSDUCKDLL PcapFile
    {
        TS_NOCOPY(PcapFile);
    public:
        //!
        //! Default constructor.
        //!
        PcapFile();

        //!
        //! Destructor.
        //!
        ~PcapFile();

        //!
        //! Open the file for read.
        //! @param [in] filename File name. If empty or "-", use standard input.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool open(const UString& filename, Report& report);

        //!
        //! Check if the file is open.
        //! @return True if the file is open, false otherwise.
        //!
        bool isOpen() const { return _in != nullptr; }

        //!
        //! Read the next IPv4 packet (headers included).
        //! Skip intermediate metadata and other types of packets.
        //!
        //! @param [out] data Address of the buffer for the received IPv4 packet.
        //! @param [in] max_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received message. Will never be larger than @a max_size.
        //! @param [out] timestamp Capture timestamp in microseconds since Unix epoch or -1 if none is available.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool readIPv4(uint8_t* data, size_t max_size, size_t& ret_size, MicroSecond& timestamp, Report& report);

        //!
        //! Get the number of captured packets so far.
        //! This includes all packets, not only IPv4 packets.
        //! This value is the number of the last returned packet, as seen in the left-most column in Wireshark interface.
        //! @return The number of captured packets so far.
        //!
        size_t packetCount() const { return _packet_count; }

        //!
        //! Close the file.
        //!
        void close();

    private:
        // Descriptioon of one capture interface.
        // Pcap files have only one interface, pcap-ng files may have more.
        class InterfaceDesc
        {
        public:
            InterfaceDesc();          // Constructor.
            uint16_t    link_type;    // A pcap LINKTYPE_ value.
            size_t      fcs_size;     // Number of Frame Cyclic Sequences bytes after each packet.
            SubSecond   time_units;   // Time units per second.
            MicroSecond time_offset;  // Offset to add to all time stamps.
        };

        bool          _error;            // Error was set, may be logical error, not a file error.
        std::istream* _in;               // Point to actual input stream.
        std::ifstream _file;             // Input file (when it is a named file).
        UString       _name;             // Saved file name for messages.
        bool          _be;               // The file use a big-endian representation.
        bool          _ng;               // Pcapng format (not pcap).
        uint16_t      _major;            // File format major version.
        uint16_t      _minor;            // File format minor version.
        size_t        _packet_count;     // Count of captured packets.
        std::vector<InterfaceDesc> _if;  // Capture interfaces by index, only one in pcap files.

        // Report an error (if fmt is not empty), set error indicator, return false.
        bool error(Report& report, const UString& fmt = UString(), const std::initializer_list<ArgMixIn>& args = std::initializer_list<ArgMixIn>());

        // Read exactly "size" bytes. Return false if not enough bytes before eof.
        bool readall(uint8_t* data, size_t size, Report& report);

        // Read a file / section header, starting from a magic number which was read as big endian.
        bool readHeader(uint32_t magic, Report& report);

        // Analyze a pcap-ng interface description.
        bool analyzeNgInterface(const uint8_t* data, size_t size, Report& report);

        // Read a pcap-ng block. The 32-bit block type has already been read.
        // Start at "Block total length". Read complete block, including the two length fields.
        // Return only the block body.
        bool readNgBlockBody(uint32_t block_type, ByteBlock& body, Report& report);

        // Read 32 or 16 bits using the endianness.
        uint16_t get16(const void* addr) const { return _be ? GetUInt16BE(addr) : GetUInt16LE(addr); }
        uint32_t get32(const void* addr) const { return _be ? GetUInt32BE(addr) : GetUInt32LE(addr); }
        uint64_t get64(const void* addr) const { return _be ? GetUInt64BE(addr) : GetUInt64LE(addr); }
    };
}
