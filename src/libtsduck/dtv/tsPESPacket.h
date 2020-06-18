//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  Representation of MPEG PES packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsMPEG.h"

namespace ts {

    class PESPacket;

    //!
    //! Safe pointer for PESPacket (not thread-safe).
    //!
    typedef SafePtr<PESPacket, NullMutex> PESPacketPtr;

    //!
    //! Vector of PESPacket safe pointers.
    //!
    typedef std::vector<PESPacketPtr> PESPacketPtrVector;

    //!
    //! Representation of MPEG PES packets.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL PESPacket
    {
    public:
        //!
        //! Default constructor.
        //! The PESPacket is initially marked invalid.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        PESPacket(PID source_pid = PID_NULL);

        //!
        //! Copy constructor.
        //! @param [in] other Another instance to copy.
        //! @param [in] mode The packet's data are either shared (ts::SHARE) between the
        //! two instances or duplicated (ts::COPY).
        //!
        PESPacket(const PESPacket& other, ShareMode mode);

        //!
        //! Move constructor.
        //! @param [in,out] other Another instance to move.
        //!
        PESPacket(PESPacket&& other) noexcept;

        //!
        //! Constructor from full binary content.
        //! The content is copied into the packet if valid.
        //! @param [in] content Address of the binary packet data.
        //! @param [in] content_size Size in bytes of the packet.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        PESPacket(const void* content, size_t content_size, PID source_pid = PID_NULL);

        //!
        //! Constructor from full binary content.
        //! The content is copied into the packet if valid.
        //! @param [in] content Binary packet data.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        PESPacket(const ByteBlock& content, PID source_pid = PID_NULL);

        //!
        //! Constructor from full binary content.
        //! The content is copied into the packet if valid.
        //! @param [in] content_ptr Safe pointer to the binary packet data.
        //! The content is referenced, and thus shared.
        //! Do not modify the referenced ByteBlock from outside the PESPacket.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        PESPacket(const ByteBlockPtr& content_ptr, PID source_pid = PID_NULL);

        //!
        //! Reload from full binary content.
        //! The content is copied into the packet if valid.
        //! @param [in] content Address of the binary packet data.
        //! @param [in] content_size Size in bytes of the packet.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        void reload(const void* content, size_t content_size, PID source_pid = PID_NULL);

        //!
        //! Reload from full binary content.
        //! The content is copied into the packet if valid.
        //! @param [in] content Binary packet data.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        void reload(const ByteBlock& content, PID source_pid = PID_NULL);

        //!
        //! Reload from full binary content.
        //! The content is copied into the packet if valid.
        //! @param [in] content_ptr Safe pointer to the binary packet data.
        //! The content is referenced, and thus shared.
        //! Do not modify the referenced ByteBlock from outside the PESPacket.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        void reload(const ByteBlockPtr& content_ptr, PID source_pid = PID_NULL);

        //!
        //! Clear packet content.
        //! Becomes an invalid packet.
        //!
        void clear();

        //!
        //! Assignment operator.
        //! The packets are referenced, and thus shared between the two packet objects.
        //! @param [in] other Other packet to assign to this object.
        //! @return A reference to this object.
        //!
        PESPacket& operator=(const PESPacket& other);

        //!
        //! Move assignment operator.
        //! @param [in,out] other Other packet to assign to this object.
        //! @return A reference to this object.
        //!
        PESPacket& operator=(PESPacket&& other) noexcept;

        //!
        //! Duplication.
        //! Similar to assignment but the packets are duplicated.
        //! @param [in] other Other packet to duplicate into this object.
        //! @return A reference to this object.
        //!
        PESPacket& copy(const PESPacket& other);

        //!
        //! Check if the packet has valid content.
        //! @return True if the packet has valid content.
        //!
        bool isValid() const {return _is_valid;}

        //!
        //! Equality operator.
        //! The source PID's are ignored, only the packet contents are compared.
        //! Invalid packets are never identical.
        //! @param [in] other Other packet to compare.
        //! @return True if the two packets are identical. False otherwise.
        //!
        bool operator==(const PESPacket& other) const;

        //!
        //! Unequality operator.
        //! The source PID's are ignored, only the packet contents are compared.
        //! Invalid packets are never identical.
        //! @param [in] other Other packet to compare.
        //! @return True if the two packets are different. False otherwise.
        //!
        bool operator!=(const PESPacket& other) const
        {
            return !(*this == other);
        }

        //!
        //! Get the source PID.
        //! @return The source PID.
        //!
        PID getSourcePID() const
        {
            return _source_pid;
        }

        //!
        //! Set the source PID.
        //! @param [in] pid The source PID.
        //!
        void setSourcePID(PID pid)
        {
            _source_pid = pid;
        }

        //!
        //! Get the stream type, as specified in the PMT (optional).
        //! @return The stream type.
        //!
        uint8_t getStreamType() const
        {
            return _stream_type;
        }

        //!
        //! Set the stream type, as specified in the PMT.
        //! @param [in] type The stream type.
        //!
        void setStreamType(uint8_t type)
        {
            _stream_type = type;
        }

        //!
        //! Index of first TS packet of the PES packet in the demultiplexed stream.
        //! Usually valid only if the PES packet was extracted by a PES demux.
        //! @return The first TS packet of the PES packet in the demultiplexed stream.
        //!
        PacketCounter getFirstTSPacketIndex() const
        {
            return _first_pkt;
        }

        //!
        //! Index of last TS packet of the PES packet in the demultiplexed stream.
        //! Usually valid only if the PES packet was extracted by a PES demux.
        //! @return The last TS packet of the PES packet in the demultiplexed stream.
        //!
        PacketCounter getLastTSPacketIndex() const
        {
            return _last_pkt;
        }

        //!
        //! Set the first TS packet of the PES packet in the demultiplexed stream.
        //! @param [in] i The first TS packet of the PES packet in the demultiplexed stream.
        //!
        void setFirstTSPacketIndex(PacketCounter i)
        {
            _first_pkt = i;
        }

        //!
        //! Set the last TS packet of the PES packet in the demultiplexed stream.
        //! @param [in] i The last TS packet of the PES packet in the demultiplexed stream.
        //!
        void setLastTSPacketIndex(PacketCounter i)
        {
            _last_pkt = i;
        }

        //!
        //! Stream id of the PES packet.
        //! @return The stream id of the PES packet.
        //!
        uint8_t getStreamId() const
        {
            return _is_valid ? (*_data)[3] : 0;
        }

        //!
        //! Set the stream id of the PES packet.
        //! @param [in] sid The stream id of the PES packet.
        //!
        void setStreamId(uint8_t sid);

        //!
        //! Check if the packet has a long header.
        //! @return True if the packet has a long header.
        //!
        bool hasLongHeader() const
        {
            return _is_valid && IsLongHeaderSID((*_data)[3]);
        }

        //!
        //! Access to the full binary content of the packet.
        //! Do not modify content.
        //! @return Address of the full binary content of the packet.
        //! May be invalidated after modification in packet.
        //!
        const uint8_t* content() const
        {
            return _data->data();
        }

        //!
        //! Size of the binary content of the packet.
        //! @return Size of the binary content of the packet.
        //!
        size_t size() const;

        //!
        //! Access to the PES header of the packet.
        //! @return Address of the PES header of the packet.
        //!
        const uint8_t* header() const
        {
            return _is_valid ? _data->data() : nullptr;
        }

        //!
        //! Size of the PES header of the packet.
        //! @return Size of the PES header of the packet.
        //!
        size_t headerSize() const
        {
            return _is_valid ? _header_size : 0;
        }

        //!
        //! Access to the payload of the packet.
        //! @return Address of the payload of the packet.
        //!
        const uint8_t* payload() const
        {
            return _is_valid ? _data->data() + _header_size : nullptr;
        }

        //!
        //! Size of the payload of the packet.
        //! @return Size of the payload of the packet.
        //!
        size_t payloadSize() const
        {
            return _is_valid ? size() - _header_size : 0;
        }

        //!
        //! Number of spurious data bytes after the packet.
        //! @return Size of the spurious data bytes after the packet.
        //!
        size_t spuriousDataSize() const
        {
            return _is_valid ? _data->size() - size() : 0;
        }

        //!
        //! Check if the PES packet contains MPEG-2 video.
        //! Also applies to MPEG-1 video.
        //! @return True if the PES packet contains MPEG-2 video.
        //!
        bool isMPEG2Video() const;

        //!
        //! Check if the PES packet contains AVC / H.264 video.
        //! @return True if the PES packet contains AVC / H.264 video.
        //!
        bool isAVC() const;

        //!
        //! Check if the PES packet contains HEVC / H.265 video.
        //! @return True if the PES packet contains HEVC / H.265 video.
        //!
        bool isHEVC() const;

        //!
        //! Check if the PES packet contains AC-3 or Enhanced-AC-3 audio.
        //!
        //! Warning: As specified in ETSI TS 102 366, an AC-3 audio frame always
        //! starts with 0x0B77. This is what we check here. However, it is still
        //! possible that other encodings may start from time to time with 0x0B77.
        //! Thus, it is safe to say that a PID in which all PES packets start with
        //! 0x0B77 (ie isAC3() returns true) contains AC-3. However, if only
        //! a few PES packets start with 0x0B77, it is safe to say that it should be
        //! something else.
        //!
        //! @return True if the PES packet contains AC-3 or Enhanced-AC-3 audio.
        //!
        bool isAC3() const;

    private:
        // Private fields
        bool          _is_valid;     // Content of *_data is a valid packet
        size_t        _header_size;  // PES header size in bytes
        PID           _source_pid;   // Source PID (informational)
        uint8_t       _stream_type;  // Stream type from PMT (informational)
        PacketCounter _first_pkt;    // Index of first packet in stream
        PacketCounter _last_pkt;     // Index of last packet in stream
        ByteBlockPtr  _data;         // Full binary content of the packet

        // Initialize from a binary content.
        void initialize(const ByteBlockPtr&);

        // Inaccessible operations
        PESPacket(const PESPacket&) = delete;
    };
}
