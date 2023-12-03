//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of DVB T2-MI (DVB-T2 Modulator Interface) packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDemuxedData.h"
#include "tsT2MI.h"
#include "tsTS.h"

namespace ts {

    class T2MIPacket;

    //!
    //! Safe pointer for T2MIPacket (not thread-safe).
    //!
    typedef SafePtr<T2MIPacket, ts::null_mutex> T2MIPacketPtr;

    //!
    //! Vector of T2MIPacket safe pointers.
    //!
    typedef std::vector<T2MIPacketPtr> T2MIPacketPtrVector;

    //!
    //! Representation of a DVB T2-MI (DVB-T2 Modulator Interface) packet.
    //! @ingroup mpeg
    //! @see ETSI TS 102 773 V1.4.1, section 5.1
    //!
    class TSDUCKDLL T2MIPacket : public DemuxedData
    {
    public:
        //!
        //! Explicit identification of super class.
        //!
        typedef DemuxedData SuperClass;

        //!
        //! Default constructor.
        //! The T2MIPacket is initially marked invalid.
        //!
        T2MIPacket() = default;

        //!
        //! Copy constructor.
        //! @param [in] other Another instance to copy.
        //! @param [in] mode The packet's data are either shared (ShareMode::SHARE) between the
        //! two instances or duplicated (ShareMode::COPY).
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

        // Inherited methods.
        virtual void clear() override;
        virtual void reload(const void* content, size_t content_size, PID source_pid = PID_NULL) override;
        virtual void reload(const ByteBlock& content, PID source_pid = PID_NULL) override;
        virtual void reload(const ByteBlockPtr& content_ptr, PID source_pid = PID_NULL) override;

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
        bool isValid() const { return _is_valid; }

        //!
        //! Equality operator.
        //! The source PID's are ignored, only the packet contents are compared.
        //! Invalid packets are never identical.
        //! @param [in] other Other packet to compare.
        //! @return True if the two packets are identical. False otherwise.
        //!
        bool operator==(const T2MIPacket& other) const;
        TS_UNEQUAL_OPERATOR(T2MIPacket)

        //!
        //! Access to the payload of the packet.
        //! @return Address of the payload of the packet.
        //!
        const uint8_t* payload() const { return _is_valid ? content() + T2MI_HEADER_SIZE : nullptr; }

        //!
        //! Size of the payload of the packet in bits.
        //! A T2-MI packet may stop in the middle of a byte.
        //! The payload size in bytes is rounded to the next byte.
        //! @return Size of the payload of the packet in bits.
        //!
        size_t payloadSizeInBits() const { return _is_valid ? GetUInt16(content() + 4) : 0; }

        //!
        //! Size of the payload of the packet in bytes.
        //! @return Size of the payload of the packet in bytes.
        //!
        size_t payloadSize() const;

        //!
        //! Get the T2-MI packet type.
        //! @return The T2-MI packet type or T2MI_INVALID_TYPE if the packet is invalid.
        //!
        T2MIPacketType packetType() const { return _is_valid ? T2MIPacketType(*content()) : T2MIPacketType::INVALID_TYPE; }

        //!
        //! Get the T2-MI packet count (from the packet header).
        //! @return The T2-MI packet count.
        //!
        uint8_t packetCount() const { return _is_valid ? content()[1] : 0; }

        //!
        //! Get the T2-MI superframe index (from the packet header).
        //! @return The T2-MI superframe index (4 bits).
        //!
        uint8_t superframeIndex() const { return _is_valid ? ((content()[2] >> 4) & 0x0F) : 0; }

        //!
        //! Get the T2-MI frame index.
        //! This is valid only for some packet types (see ETSI TS 102 773, section 5.2).
        //! @return The T2-MI frame index.
        //!
        uint8_t frameIndex() const { return payloadSize() >= 1 ? content()[T2MI_HEADER_SIZE] : 0; }

        //!
        //! Check if the packet has a valid PLP (Physical Layer Pipe) identifier.
        //! @return True if the packet has a valid PLP.
        //!
        bool plpValid() const { return packetType() == T2MIPacketType::BASEBAND_FRAME && payloadSize() >= 2; }

        //!
        //! Get the PLP (Physical Layer Pipe) identifier.
        //! This is valid only for baseband frames (packet type T2MI_BASEBAND_FRAME).
        //! @return The PLP identifier.
        //!
        uint8_t plp() const { return plpValid() ? content()[T2MI_HEADER_SIZE + 1] : 0; }

        //!
        //! Get the interleaving frame start flag.
        //! This is valid only for baseband frames (packet type T2MI_BASEBAND_FRAME).
        //! @return True if the T2-MI packet contains the first baseband frame of an interleaving frame for a particular PLP.
        //!
        bool interleavingFrameStart() const;

        //!
        //! Access to the baseband frame inside the packet.
        //! This is valid only for baseband frames (packet type T2MI_BASEBAND_FRAME).
        //! @return Address of the baseband frame in the packet or zero if invalid.
        //!
        const uint8_t* basebandFrame() const;

        //!
        //! Size of the baseband frame in bytes.
        //! This is valid only for baseband frames (packet type T2MI_BASEBAND_FRAME).
        //! @return Size of the baseband frame in bytes.
        //!
        size_t basebandFrameSize() const;

    private:
        // Private fields
        bool _is_valid = false;

        // Validate binary content.
        void validate();

        // Inaccessible operations
        T2MIPacket(const T2MIPacket&) = delete;
    };
}
