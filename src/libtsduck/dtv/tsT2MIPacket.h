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
//!  Representation of DVB T2-MI (DVB-T2 Modulator Interface) packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsMPEG.h"

namespace ts {

    class T2MIPacket;

    //!
    //! Safe pointer for T2MIPacket (not thread-safe).
    //!
    typedef SafePtr<T2MIPacket, NullMutex> T2MIPacketPtr;

    //!
    //! Vector of T2MIPacket safe pointers.
    //!
    typedef std::vector<T2MIPacketPtr> T2MIPacketPtrVector;

    //!
    //! Representation of a DVB T2-MI (DVB-T2 Modulator Interface) packet.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL T2MIPacket
    {
    public:
        //!
        //! Default constructor.
        //! The T2MIPacket is initially marked invalid.
        //!
        T2MIPacket();

        //!
        //! Copy constructor.
        //! @param [in] other Another instance to copy.
        //! @param [in] mode The packet's data are either shared (ts::SHARE) between the
        //! two instances or duplicated (ts::COPY).
        //!
        T2MIPacket(const T2MIPacket& other, ShareMode mode);

        //!
        //! Move constructor.
        //! @param [in,out] other Another instance to move.
        //!
        T2MIPacket(T2MIPacket&& other) noexcept;

        //!
        //! Constructor from full binary content.
        //! The content is copied into the packet if valid.
        //! @param [in] content Address of the binary packet data.
        //! @param [in] content_size Size in bytes of the packet.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        T2MIPacket(const void* content, size_t content_size, PID source_pid = PID_NULL);

        //!
        //! Constructor from full binary content.
        //! The content is copied into the packet if valid.
        //! @param [in] content Binary packet data.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        T2MIPacket(const ByteBlock& content, PID source_pid = PID_NULL);

        //!
        //! Constructor from full binary content.
        //! The content is copied into the packet if valid.
        //! @param [in] content_ptr Safe pointer to the binary packet data.
        //! The content is referenced, and thus shared.
        //! Do not modify the referenced ByteBlock from outside the T2MIPacket.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        T2MIPacket(const ByteBlockPtr& content_ptr, PID source_pid = PID_NULL);

        //!
        //! Reload from full binary content.
        //! The content is copied into the packet if valid.
        //! @param [in] content Address of the binary packet data.
        //! @param [in] content_size Size in bytes of the packet.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        void reload(const void* content, size_t content_size, PID source_pid = PID_NULL)
        {
            _source_pid = source_pid;
            initialize(new ByteBlock(content, content_size));
        }

        //!
        //! Reload from full binary content.
        //! The content is copied into the packet if valid.
        //! @param [in] content Binary packet data.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        void reload(const ByteBlock& content, PID source_pid = PID_NULL)
        {
            _source_pid = source_pid;
            initialize(new ByteBlock(content));
        }

        //!
        //! Reload from full binary content.
        //! The content is copied into the packet if valid.
        //! @param [in] content_ptr Safe pointer to the binary packet data.
        //! The content is referenced, and thus shared.
        //! Do not modify the referenced ByteBlock from outside the T2MIPacket.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        void reload(const ByteBlockPtr& content_ptr, PID source_pid = PID_NULL)
        {
            _source_pid = source_pid;
            initialize(content_ptr);
        }

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
        T2MIPacket& operator=(const T2MIPacket& other);

        //!
        //! Move assignment operator.
        //! @param [in,out] other Other packet to move into this object.
        //! @return A reference to this object.
        //!
        T2MIPacket& operator=(T2MIPacket&& other) noexcept;

        //!
        //! Duplication.
        //! Similar to assignment but the packets are duplicated.
        //! @param [in] other Other packet to duplicate into this object.
        //! @return A reference to this object.
        //!
        T2MIPacket& copy(const T2MIPacket& other);

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
        bool operator==(const T2MIPacket& other) const;

        //!
        //! Unequality operator.
        //! The source PID's are ignored, only the packet contents are compared.
        //! Invalid packets are never identical.
        //! @param [in] other Other packet to compare.
        //! @return True if the two packets are different. False otherwise.
        //!
        bool operator!=(const T2MIPacket& other) const
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
        size_t size() const
        {
            return _data->size();
        }

        //!
        //! Access to the payload of the packet.
        //! @return Address of the payload of the packet.
        //!
        const uint8_t* payload() const
        {
            return _is_valid ? _data->data() + T2MI_HEADER_SIZE : nullptr;
        }

        //!
        //! Size of the payload of the packet in bits.
        //! A T2-MI packet may stop in the middle of a byte.
        //! The payload size in bytes is rounded to the next byte.
        //! @return Size of the payload of the packet in bits.
        //!
        size_t payloadSizeInBits() const
        {
            return _is_valid ? GetUInt16(_data->data() + 4) : 0;
        }

        //!
        //! Size of the payload of the packet in bytes.
        //! @return Size of the payload of the packet in bytes.
        //!
        size_t payloadSize() const;

        //!
        //! Get the T2-MI packet type.
        //! @return The T2-MI packet type or T2MI_INVALID_TYPE if the packet is invalid.
        //!
        uint8_t packetType() const
        {
            return _is_valid ? (*_data)[0] : uint8_t(T2MI_INVALID_TYPE);
        }

        //!
        //! Get the T2-MI packet count (from the packet header).
        //! @return The T2-MI packet count.
        //!
        uint8_t packetCount() const
        {
            return _is_valid ? (*_data)[1] : 0;
        }

        //!
        //! Get the T2-MI superframe index (from the packet header).
        //! @return The T2-MI superframe index (4 bits).
        //!
        uint8_t superframeIndex() const
        {
            return _is_valid ? (((*_data)[2] >> 4) & 0x0F) : 0;
        }

        //!
        //! Get the T2-MI frame index.
        //! This is valid only for some packet types (see ETSI TS 102 773, section 5.2).
        //! @return The T2-MI frame index.
        //!
        uint8_t frameIndex() const
        {
            return payloadSize() >= 1 ? (*_data)[T2MI_HEADER_SIZE] : 0;
        }

        //!
        //! Check if the packet has a valid PLP (Physical Layer Pipe) identifier.
        //! @return True if the packet has a valid PLP.
        //!
        bool plpValid() const
        {
            return packetType() == T2MI_BASEBAND_FRAME && payloadSize() >= 2;
        }

        //!
        //! Get the PLP (Physical Layer Pipe) identifier.
        //! This is valid only for baseband frames (packet type T2MI_BASEBAND_FRAME).
        //! @return The PLP identifier.
        //!
        uint8_t plp() const
        {
            return plpValid() ? (*_data)[T2MI_HEADER_SIZE + 1] : 0;
        }

        //!
        //! Get the interleaving frame start flag.
        //! This is valid only for baseband frames (packet type T2MI_BASEBAND_FRAME).
        //! @return True if the T2-MI packet contains the first baseband frame of an interleaving frame for a particular PLP.
        //!
        bool interleavingFrameStart() const
        {
            return packetType() == T2MI_BASEBAND_FRAME && payloadSize() >= 3 && ((*_data)[T2MI_HEADER_SIZE + 2] & 0x80) != 0;
        }

        //!
        //! Access to the baseband frame inside the packet.
        //! This is valid only for baseband frames (packet type T2MI_BASEBAND_FRAME).
        //! @return Address of the baseband frame in the packet or zero if invalid.
        //!
        const uint8_t* basebandFrame() const
        {
            return packetType() == T2MI_BASEBAND_FRAME && payloadSize() >= 3 ? _data->data() + T2MI_HEADER_SIZE + 3 : nullptr;
        }

        //!
        //! Size of the baseband frame in bytes.
        //! This is valid only for baseband frames (packet type T2MI_BASEBAND_FRAME).
        //! @return Size of the baseband frame in bytes.
        //!
        size_t basebandFrameSize() const
        {
            return packetType() == T2MI_BASEBAND_FRAME && payloadSize() >= 3 ? payloadSize() - 3 : 0;
        }

    private:
        // Private fields
        bool          _is_valid;     // Content of *_data is a valid packet
        PID           _source_pid;   // Source PID (informational)
        ByteBlockPtr  _data;         // Full binary content of the packet

        // Initialize from a binary content.
        void initialize(const ByteBlockPtr&);

        // Inaccessible operations
        T2MIPacket(const T2MIPacket&) = delete;
    };
}
