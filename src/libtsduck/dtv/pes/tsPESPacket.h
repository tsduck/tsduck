//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of MPEG PES packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDemuxedData.h"
#include "tsCodecType.h"
#include "tsTS.h"
#include "tsPSI.h"

namespace ts {

    class PESPacket;

    //!
    //! Safe pointer for PESPacket (not thread-safe).
    //!
    typedef SafePtr<PESPacket, ts::null_mutex> PESPacketPtr;

    //!
    //! Vector of PESPacket safe pointers.
    //!
    typedef std::vector<PESPacketPtr> PESPacketPtrVector;

    //!
    //! Representation of MPEG PES packets.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL PESPacket : public DemuxedData
    {
    public:
        //!
        //! Explicit identification of super class.
        //!
        typedef DemuxedData SuperClass;

        //!
        //! Default constructor.
        //! The PESPacket is initially marked invalid.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        PESPacket(PID source_pid = PID_NULL);

        //!
        //! Copy constructor.
        //! @param [in] other Another instance to copy.
        //! @param [in] mode The packet's data are either shared (ShareMode::SHARE) between the
        //! two instances or duplicated (ShareMode::COPY).
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

        // Inherited methods.
        virtual void clear() override;
        virtual void reload(const void* content, size_t content_size, PID source_pid = PID_NULL) override;
        virtual void reload(const ByteBlock& content, PID source_pid = PID_NULL) override;
        virtual void reload(const ByteBlockPtr& content_ptr, PID source_pid = PID_NULL) override;
        virtual size_t size() const override;

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
        bool isValid() const { return _is_valid; }

        //!
        //! Equality operator.
        //! The source PID's are ignored, only the packet contents are compared.
        //! Invalid packets are never identical.
        //! @param [in] other Other packet to compare.
        //! @return True if the two packets are identical. False otherwise.
        //!
        bool operator==(const PESPacket& other) const;
        TS_UNEQUAL_OPERATOR(PESPacket)

        //!
        //! Get the optional PCR value which was associated to the PES packets.
        //! It was typically extracted from the first TS packet of the PES packet.
        //! @return The 42-bit PCR or INVALID_PCR if there is none.
        //!
        uint64_t getPCR() const { return _pcr; }

        //!
        //! Set the PCR value for this PES packet.
        //! @param [in] pcr The new 42-bit PCR value. Specify INVALID_PCR to clear the PCR.
        //!
        void setPCR(uint64_t pcr);

        //!
        //! Get the stream type, as specified in the PMT (optional).
        //! @return The stream type.
        //!
        uint8_t getStreamType() const { return _stream_type; }

        //!
        //! Set the stream type, as specified in the PMT.
        //! @param [in] type The stream type.
        //!
        void setStreamType(uint8_t type) { _stream_type = type; }

        //!
        //! Get the codec type, as specified by the user (optional).
        //! @return The codec type.
        //!
        CodecType getCodec() const { return _codec; }

        //!
        //! Set the codec type (informational only).
        //! @param [in] codec The codec type.
        //!
        void setCodec(CodecType codec) { _codec = codec; }

        //!
        //! Set a default codec type.
        //! If the codec is not already known and if the data in the PES packet
        //! looks compatible with @a codec, then this codec type is set.
        //! @param [in] default_codec The default codec type.
        //!
        void setDefaultCodec(CodecType default_codec);

        //!
        //! Stream id of the PES packet.
        //! @return The stream id of the PES packet.
        //!
        uint8_t getStreamId() const;

        //!
        //! Set the stream id of the PES packet.
        //! @param [in] sid The stream id of the PES packet.
        //!
        void setStreamId(uint8_t sid);

        //!
        //! Check if the packet has a long header.
        //! @return True if the packet has a long header.
        //!
        bool hasLongHeader() const;

        //!
        //! Access to the PES header of the packet.
        //! @return Address of the PES header of the packet.
        //!
        const uint8_t* header() const { return _is_valid ? content() : nullptr; }

        //!
        //! Size of the PES header of the packet.
        //! @return Size of the PES header of the packet.
        //!
        size_t headerSize() const { return _is_valid ? _header_size : 0; }

        //!
        //! Access to the payload of the packet.
        //! @return Address of the payload of the packet.
        //!
        const uint8_t* payload() const { return _is_valid ? content() + _header_size : nullptr; }

        //!
        //! Size of the payload of the packet.
        //! @return Size of the payload of the packet.
        //!
        size_t payloadSize() const { return _is_valid ? size() - _header_size : 0; }

        //!
        //! Number of spurious data bytes after the packet.
        //! @return Size of the spurious data bytes after the packet.
        //!
        size_t spuriousDataSize() const { return _is_valid ? SuperClass::size() - size() : 0; }

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
        //! Check if the PES packet contains VVC / H.266 video.
        //! @return True if the PES packet contains VVC / H.266 video.
        //!
        bool isVVC() const;

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

        //!
        //! Check if the PES packet contains an intra-coded image.
        //! The stream type and/or codec type must have been set.
        //! @return If the PES packet contains the start of an intra-coded image, return the
        //! offset inside the PES packet where the intra-image starts. This value is informational only,
        //! the exact semantics depends on the video codec. Return NPOS if no intra-image was found.
        //!
        size_t findIntraImage() const;

        //!
        //! Get the header size of the start of a PES packet.
        //! @param [in] data Address of data to check, typically the start of a PES packet.
        //! @param [in] size Data size in bytes.
        //! @return The PES header size in bytes, if one is found, or 0 on error.
        static size_t HeaderSize(const uint8_t* data, size_t size);

        //!
        //! Check if a truncated PES packet may contain MPEG-2 or MPEG-1 video.
        //! @param [in] data Address of data to check, typically the start of a PES packet.
        //! @param [in] size Data size in bytes.
        //! @param [in] stream_type Optional stream type, as found in the PMT. Used as a hint.
        //! @return True if the PES data may contain MPEG-2 or MPEG-1 video.
        //!
        static bool IsMPEG2Video(const uint8_t* data, size_t size, uint8_t stream_type = ST_NULL);

        //!
        //! Check if a truncated PES packet may contain AVC / H.264 video.
        //! @param [in] data Address of data to check, typically the start of a PES packet.
        //! @param [in] size Data size in bytes.
        //! @param [in] stream_type Optional stream type, as found in the PMT. Used as a hint.
        //! @return True if the PES data may contain AVC / H.264 video.
        //!
        static bool IsAVC(const uint8_t* data, size_t size, uint8_t stream_type = ST_NULL)
        {
            return IsXVC(StreamTypeIsAVC, data, size, stream_type);
        }

        //!
        //! Check if a truncated PES packet may contain HEVC / H.265 video.
        //! @param [in] data Address of data to check, typically the start of a PES packet.
        //! @param [in] size Data size in bytes.
        //! @param [in] stream_type Optional stream type, as found in the PMT. Used as a hint.
        //! @return True if the PES data may contain HEVC / H.265 video.
        //!
        static bool IsHEVC(const uint8_t* data, size_t size, uint8_t stream_type = ST_NULL)
        {
            return IsXVC(StreamTypeIsHEVC, data, size, stream_type);
        }

        //!
        //! Check if a truncated PES packet may contain VVC / H.266 video.
        //! @param [in] data Address of data to check, typically the start of a PES packet.
        //! @param [in] size Data size in bytes.
        //! @param [in] stream_type Optional stream type, as found in the PMT. Used as a hint.
        //! @return True if the PES data may contain VVC / H.266 video.
        //!
        static bool IsVVC(const uint8_t* data, size_t size, uint8_t stream_type = ST_NULL)
        {
            return IsXVC(StreamTypeIsVVC, data, size, stream_type);
        }

        //!
        //! Check if a truncated PES packet may contain AC-3 or Enhanced-AC-3 audio.
        //! @param [in] data Address of data to check, typically the start of a PES packet.
        //! @param [in] size Data size in bytes.
        //! @param [in] stream_type Optional stream type, as found in the PMT. Used as a hint.
        //! @return True if the PES data may contain AC-3 or Enhanced-AC-3 audio.
        //!
        static bool IsAC3(const uint8_t* data, size_t size, uint8_t stream_type = ST_NULL);

        //!
        //! Check if a truncated PES packet starts with 00 00 00 [00...] 01, common header for AVC, HEVC and VVC.
        //! @param [in] data Address of data to check, typically the start of a PES packet.
        //! @param [in] size Data size in bytes.
        //! @return True if the PES data starts with a common header.
        //!
        static bool HasCommonVideoHeader(const uint8_t* data, size_t size);

        //!
        //! Check if a truncated PES packet may contain the start of an intra-coded image.
        //! @param [in] data Address of data to check, typically the start of a PES packet.
        //! @param [in] size Data size in bytes.
        //! @param [in] stream_type Optional stream type, as found in the PMT. Used as a hint.
        //! @param [in] default_format Default encoding format if it cannot be determined from @a stream_type.
        //! If @a stream_type and @a default_format are both unspecified, intra-image cannot be detected.
        //! @return If the PES data may contain the start of an intra-coded image, return the
        //! offset inside @a data where the intra-image starts. This value is informational only,
        //! the exact semantics depends on the video codec. Return NPOS if no intra-image was found.
        //! If the data is not sufficient to determine the presence of an intra-image,
        //! return NPOS, even though a larger piece of information may contain one.
        //!
        static size_t FindIntraImage(const uint8_t* data, size_t size, uint8_t stream_type = ST_NULL, CodecType default_format = CodecType::UNDEFINED);

    private:
        // Private fields
        bool      _is_valid = false;              // Content of *_data is a valid packet
        size_t    _header_size = 0;               // PES header size in bytes
        uint8_t   _stream_type {ST_NULL};         // Stream type from PMT (informational)
        CodecType _codec {CodecType::UNDEFINED};  // Data format (informational)
        uint64_t  _pcr {INVALID_PCR};             // PCR value from TS packets (informational)

        // Validate binary content.
        void validate();

        //! Check if a truncated PES packet may contain AVC, HEVC or VVC.
        static bool IsXVC(bool (*StreamTypeCheck)(uint8_t), const uint8_t* data, size_t size, uint8_t stream_type);

        // Inaccessible operations
        PESPacket(const PESPacket&) = delete;
    };
}
