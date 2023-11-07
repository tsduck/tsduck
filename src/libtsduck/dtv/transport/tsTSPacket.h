//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Basic definition of an MPEG-2 transport packet.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"
#include "tsMemory.h"
#include "tsCerrReport.h"
#include "tsException.h"
#include "tsResidentBuffer.h"

namespace ts {

    class ByteBlock;

    //!
    //! Basic definition of an MPEG-2 transport packet.
    //! @ingroup mpeg
    //!
    //! Physically, an object of this class is exactly implemented as a 188-byte TS packets.
    //! It is safe to consider that arrays or vectors of this class have the physical layout
    //! of a transport stream.
    //!
    //! For performance reason, there is no constructor. Uninitialized packets have undefined
    //! binary content.
    //!
    class TSDUCKDLL TSPacket final
    {
    public:
        //!
        //! The public content is the 188-byte array representing the TS packet.
        //!
        uint8_t b[PKT_SIZE];

        //!
        //! Internal error: access a non-existent adaptation field.
        //!
        TS_DECLARE_EXCEPTION(AdaptationFieldError);

        //!
        //! Equality operator.
        //! @param [in] p Other packet to compare.
        //! @return True if this object is equal to @a p.
        //!
        bool operator==(const TSPacket& p) const
        {
            return std::memcmp(b, p.b, PKT_SIZE) == 0;
        }
        TS_UNEQUAL_OPERATOR(TSPacket)

        //!
        //! Initialize a TS packet.
        //! This method should be used when initializing with NullPacket or EmptyPacket is not appropriate.
        //! The packet payload is 184 bytes long and filled with the @a data byte.
        //! @param [in] pid PID value.
        //! @param [in] cc Continuity counter.
        //! @param [in] data Byte value to fill the payload with.
        //!
        void init(PID pid = PID_NULL, uint8_t cc = 0, uint8_t data = 0xFF);

        //!
        //! Check if the sync byte is valid.
        //! @return True if the sync byte of the packet is valid.
        //!
        bool hasValidSync() const
        {
            return b[0] == SYNC_BYTE;
        }

        //!
        //! Extract PID - 13 bits.
        //! @return The PID value.
        //!
        PID getPID() const
        {
            return GetUInt16(b+1) & 0x1FFF;
        }

        //!
        //! Set PID - 13 bits.
        //! @param [in] pid The new PID.
        //!
        void setPID(PID pid)
        {
            b[1] = (b[1] & 0xE0) | ((pid >> 8) & 0x1F);
            b[2] = pid & 0x00FF;
        }

        //!
        //! Extract payload_unit_start_indicator (PUSI) - 1 bit
        //! @return The PUSI value.
        //!
        bool getPUSI() const
        {
            return (b[1] & 0x40) != 0;
        }

        //!
        //! Clear payload_unit_start_indicator (PUSI) - 1 bit
        //!
        void clearPUSI()
        {
            b[1] &= ~0x40;
        }

        //!
        //! Set payload_unit_start_indicator (PUSI) - 1 bit
        //!
        void setPUSI()
        {
            b[1] |= 0x40;
        }

        //!
        //! Set payload_unit_start_indicator (PUSI) - 1 bit
        //! @param [in] on The value to set.
        //!
        void setPUSI(bool on)
        {
            b[1] = (b[1] & ~0x40) | (on ? 0x40 : 0x00);
        }

        //!
        //! Extract transport_error_indicator (TEI) - 1 bit
        //! @return The TEI value.
        //!
        bool getTEI() const
        {
            return (b[1] & 0x80) != 0;
        }

        //!
        //! Clear transport_error_indicator (TEI) - 1 bit
        //!
        void clearTEI()
        {
            b[1] &= ~0x80;
        }

        //!
        //! Set transport_error_indicator (TEI) - 1 bit
        //!
        void setTEI()
        {
            b[1] |= 0x80;
        }

        //!
        //! Set transport_error_indicator (TEI) - 1 bit
        //! @param [in] on The value to set.
        //!
        void setTEI(bool on)
        {
            b[1] = (b[1] & ~0x80) | (on ? 0x80 : 0x00);
        }

        //!
        //! Extract transport_priority - 1 bit
        //! @return The transport_priority value.
        //!
        bool getPriority() const
        {
            return (b[1] & 0x20) != 0;
        }

        //!
        //! Clear transport_priority - 1 bit
        //!
        void clearPriority()
        {
            b[1] &= ~0x20;
        }

        //!
        //! Set transport_priority - 1 bit
        //!
        void setPriority()
        {
            b[1] |= 0x20;
        }

        //!
        //! Set transport_priority - 1 bit
        //! @param [in] on The value to set.
        //!
        void setPriority(bool on)
        {
            b[1] = (b[1] & ~0x20) | (on ? 0x20 : 0x00);
        }

        //!
        //! Extract transport_scrambling_control - 2 bits
        //! @return The transport_scrambling_control value.
        //!
        uint8_t getScrambling() const
        {
            return b[3] >> 6;
        }

        //!
        //! Check if the packet is clear (ie not scrambled).
        //! @return True if the packet is clear.
        //!
        bool isClear() const
        {
            return (b[3] >> 6) == 0;
        }

        //!
        //! Check if the packet is scrambled.
        //! @return True if the packet is scrambled.
        //!
        bool isScrambled() const
        {
            return (b[3] >> 6) != 0;
        }

        //!
        //! Set transport_scrambling_control - 2 bits.
        //! @param [in] sc New transport_scrambling_control value.
        //!
        void setScrambling(uint8_t sc)
        {
            b[3] = (b[3] & 0x3F) | uint8_t(sc << 6);
        }

        //!
        //! Extract continuity_counter (CC) - 4 bits
        //! @return The CC value.
        //!
        uint8_t getCC() const
        {
            return b[3] & 0x0F;
        }

        //!
        //! Set continuity_counter (CC) - 4 bits
        //! @param [in] cc New continuity_counter value.
        //!
        void setCC(uint8_t cc)
        {
            b[3] = (b[3] & 0xF0) | (cc & 0x0F);
        }

        //!
        //! Check if packet has an adaptation_field (AF)
        //! @return True if the packet has an adaptation_field.
        //!
        bool hasAF() const
        {
            return (b[3] & 0x20) != 0;
        }

        //!
        //! Compute adaptation_field (AF) size.
        //! @return Total size in bytes of the adaptation_field, including the length field.
        //!
        size_t getAFSize() const
        {
            return hasAF() ? size_t(b[4]) + 1 : 0;
        }

        //!
        //! Compute the size of the stuffing part in the adaptation_field.
        //! @return Size in bytes of the stuffing part in the adaptation_field.
        //!
        size_t getAFStuffingSize() const;

        //!
        //! Compute the size of the TS header.
        //! @return Size in bytes of the TS header.
        //! This is also the index of the TS payload.
        //!
        size_t getHeaderSize() const
        {
            return std::min(4 + (hasAF() ? (size_t(b[4]) + 1) : 0), PKT_SIZE);
        }

        //!
        //! Check if packet has a payload
        //! @return True if the packet has a payload.
        //!
        bool hasPayload() const
        {
            return (b[3] & 0x10) != 0;
        }

        //!
        //! Get payload start address.
        //! @return The payload start address.
        //!
        const uint8_t* getPayload() const
        {
            return b + getHeaderSize();
        }

        //!
        //! Get payload start address.
        //! @return The payload start address.
        //!
        uint8_t* getPayload()
        {
            return b + getHeaderSize();
        }

        //!
        //! Compute payload size.
        //! @return The payload size in bytes.
        //!
        size_t getPayloadSize() const
        {
            return hasPayload() ? PKT_SIZE - getHeaderSize() : 0;
        }

        //!
        //! Set the payload size.
        //! If the payload shall be shrunk, the adaptation field is enlarged
        //! with stuffing. If the payload shall be enlarged, reduce the amount
        //! of stuffing in the adaptation field.
        //! This method should be used only when creating a packet from scratch,
        //! before filling the payload.
        //! @param [in] size The requested payload size.
        //! @param [in] shift_payload If true, the payload is shifted so
        //! that the start of its content remains the same. When the payload
        //! is shrunk, its end is truncated. When the paylaod is enlarged, it
        //! is padded with @a pad values. When @a shift_payload is false,
        //! the data in the memory area of the payload is not modified. In that
        //! case, the memory is silently overwritten, losing the payload content.
        //! @param [in] pad Byte value to use when padding the adaptation field or payload.
        //! @return True on success, false when the requested size is too large.
        //!
        bool setPayloadSize(size_t size, bool shift_payload = false, uint8_t pad = 0xFF);

        //!
        //! Check if packet has a discontinuity_indicator set - 1 bit
        //! @return True if packet has a discontinuity_indicator set.
        //!
        bool getDiscontinuityIndicator() const
        {
            return getAFSize() > 1 ? ((b[5] & 0x80) != 0) : false;
        }

        //!
        //! Clear discontinuity_indicator - 1 bit
        //!
        void clearDiscontinuityIndicator()
        {
            if (getAFSize() > 1) {
                b[5] &= ~0x80;
            }
        }

        //!
        //! Set discontinuity_indicator - 1 bit
        //! @param [in] shift_payload If true, the payload can be shifted and
        //! truncated when the adaptation field needs to be created.
        //! @return True if the flag was correctly set. False when the adaptation
        //! needed to be enlarged but could not because @a shift_payload was false.
        //!
        bool setDiscontinuityIndicator(bool shift_payload = false)
        {
            return setFlagsInAF(0x80, shift_payload);
        }

        //!
        //! Check if packet has a random_access_indicator set - 1 bit
        //! @return True if packet has a random_access_indicator set.
        //!
        bool getRandomAccessIndicator() const
        {
            return getAFSize() > 1 ? ((b[5] & 0x40) != 0) : false;
        }

        //!
        //! Clear random_access_indicator - 1 bit
        //!
        void clearRandomAccessIndicator()
        {
            if (getAFSize() > 1) {
                b[5] &= ~0x40;
            }
        }

        //!
        //! Set random_access_indicator - 1 bit
        //! @param [in] shift_payload If true, the payload can be shifted and
        //! truncated when the adaptation field needs to be created.
        //! @return True if the flag was correctly set. False when the adaptation
        //! needed to be enlarged but could not because @a shift_payload was false.
        //!
        bool setRandomAccessIndicator(bool shift_payload = false)
        {
            return setFlagsInAF(0x40, shift_payload);
        }

        //!
        //! Check if packet has a elementary_stream_priority_indicator (ESPI) set - 1 bit
        //! @return True if packet has an ESPI set.
        //!
        bool getESPI() const
        {
            return getAFSize() > 1 ? ((b[5] & 0x20) != 0) : false;
        }

        //!
        //! Clear elementary_stream_priority_indicator (ESPI) - 1 bit
        //!
        void clearESPI()
        {
            if (getAFSize() > 1) {
                b[5] &= ~0x20;
            }
        }

        //!
        //! Set elementary_stream_priority_indicator (ESPI) - 1 bit
        //! @param [in] shift_payload If true, the payload can be shifted and
        //! truncated when the adaptation field needs to be created.
        //! @return True if the flag was correctly set. False when the adaptation
        //! needed to be enlarged but could not because @a shift_payload was false.
        //!
        bool setESPI(bool shift_payload = false)
        {
            return setFlagsInAF(0x20, shift_payload);
        }

        //!
        //! Check if packet has a Program Clock Reference (PCR)
        //! @return True if packet has a PCR.
        //!
        bool hasPCR() const
        {
            return getAFSize() > 1 && (b[5] & 0x10) != 0;
        }

        //!
        //! Get the PCR - 42 bits.
        //! @return The PCR or INVALID_PCR if not found.
        //!
        uint64_t getPCR() const;

        //!
        //! Create or replace the PCR value - 42 bits.
        //! @param [in] pcr The new PCR value.
        //! @param [in] shift_payload If true, the payload can be shifted and
        //! truncated when the adaptation field needs to be enlarged.
        //! @return True if the PCR was correctly created. False when the adaptation
        //! needed to be enlarged but could not because @a shift_payload was false.
        //!
        bool setPCR(const uint64_t& pcr, bool shift_payload = false);

        //!
        //! Remove the Program Clock Reference (PCR) from the packet, if there is one.
        //! The adaptation field size is unchanged, its stuffing part is enlarged.
        //!
        void removePCR()
        {
            deleteFieldFromAF(PCROffset(), 6, 0x10);
        }

        //!
        //! Check if packet has an Original Program Clock Reference (OPCR)
        //! @return True if packet has an OPCR.
        //!
        bool hasOPCR() const
        {
            return getAFSize() > 1 && (b[5] & 0x08) != 0;
        }

        //!
        //! Get the OPCR - 42 bits.
        //! @return The OPCR or INVALID_PCR if not found.
        //!
        uint64_t getOPCR() const;

        //!
        //! Create or replace the OPCR value - 42 bits.
        //! @param [in] opcr The new OPCR value.
        //! @param [in] shift_payload If true, the payload can be shifted and
        //! truncated when the adaptation field needs to be enlarged.
        //! @return True if the OPCR was correctly created. False when the adaptation
        //! needed to be enlarged but could not because @a shift_payload was false.
        //!
        bool setOPCR(const uint64_t& opcr, bool shift_payload = false);

        //!
        //! Remove the Original Program Clock Reference (OPCR) from the packet, if there is one.
        //! The adaptation field size is unchanged, its stuffing part is enlarged.
        //!
        void removeOPCR()
        {
            deleteFieldFromAF(OPCROffset(), 6, 0x08);
        }

        //!
        //! Size in bytes of a Program Clock Reference (PCR) as stored in a TS packet.
        //!
        static constexpr size_t PCR_BYTES = 6;

        //!
        //! This static method extracts a PCR from a stream.
        //! @param [in] b Address of a 6-byte memory area containing a PCR binary value.
        //! @return A 42-bit PCR value.
        //!
        static uint64_t GetPCR(const uint8_t* b);

        //!
        //! This routine inserts a PCR in a stream.
        //! @param [out] b Address of a 6-byte memory area to write the PCR binary value.
        //! @param [in] pcr A 42-bit PCR value.
        //!
        static void PutPCR(uint8_t* b, const uint64_t& pcr);

        //!
        //! Check if packet has splicing point countdown
        //! @return True if packet has a splicing point countdown.
        //!
        bool hasSpliceCountdown() const
        {
            return getAFSize() > 1 && (b[5] & 0x04) != 0;
        }

        //!
        //! Get the splicing point countdown - 8 bits (signed).
        //! @return The splicing point countdown or 0 if not found.
        //!
        int8_t getSpliceCountdown() const;

        //!
        //! Create or replace the splicing point countdown - 8 bits.
        //! @param [in] count The new splicing point countdown value.
        //! @param [in] shift_payload If true, the payload can be shifted and
        //! truncated when the adaptation field needs to be enlarged.
        //! @return True if the splicing point countdown was correctly created. False when
        //! the adaptation needed to be enlarged but could not because @a shift_payload was false.
        //!
        bool setSpliceCountdown(int8_t count, bool shift_payload = false);

        //!
        //! Remove the splicing point countdown from the packet, if there is one.
        //! The adaptation field size is unchanged, its stuffing part is enlarged.
        //!
        void removeSpliceCountdown()
        {
            deleteFieldFromAF(spliceCountdownOffset(), 1, 0x04);
        }

        //!
        //! Check if packet has private data in adaptation field.
        //! @return True if packet has private data in adaptation field.
        //!
        bool hasPrivateData() const
        {
            return privateDataOffset() > 0;
        }

        //!
        //! Get size in bytes of private data from adaptation field.
        //! @return Size in bytes of private data (not including its length field).
        //!
        size_t getPrivateDataSize() const;

        //!
        //! Get address of private data in adaptation field.
        //! @return Address of private data in adaptation field or a null pointer if there is no private data.
        //!
        const uint8_t* getPrivateData() const;

        //!
        //! Get address of private data in adaptation field.
        //! @return Address of private data in adaptation field or a null pointer if there is no private data.
        //!
        uint8_t* getPrivateData();

        //!
        //! Get private data from adaptation field.
        //! @param [out] data Private data from adaptation field.
        //!
        void getPrivateData(ByteBlock& data) const;

        //!
        //! Set private data in adaptation field.
        //! @param [in] data Address of private data to set in the packet.
        //! @param [in] size Size in bytes of private data to set in the packet.
        //! @param [in] shift_payload If true, the payload can be shifted and
        //! truncated when the adaptation field needs to be created or enlarged.
        //! @return True if the flag was correctly set. False when the adaptation
        //! needed to be enlarged but could not because @a shift_payload was false.
        //!
        bool setPrivateData(const void* data, size_t size, bool shift_payload = false);

        //!
        //! Set private data in adaptation field.
        //! @param [in] data Private data to set in the packet.
        //! @param [in] shift_payload If true, the payload can be shifted and
        //! truncated when the adaptation field needs to be created or enlarged.
        //! @return True if the flag was correctly set. False when the adaptation
        //! needed to be enlarged but could not because @a shift_payload was false.
        //!
        bool setPrivateData(const ByteBlock& data, bool shift_payload = false);

        //!
        //! Remove the private data from the adaptation field, if there is one.
        //! The adaptation field size is unchanged, its stuffing part is enlarged.
        //!
        void removePrivateData();

        //!
        //! Reserve some given space in the stuffing part of the adaptation field.
        //! @param [in] size The expected stuffing size in bytes, in the adaptation field.
        //! If the AF stuffing is already that size or larger, do nothing. Otherwise,
        //! attempt to increase the AF size by shifting and truncating the payload.
        //! @param [in] shift_payload If true, the payload can be shifted and
        //! truncated when the adaptation field needs to be enlarged.
        //! @param [in] enforce_af When true, try to create the AF anyway, even if @a size
        //! is zero, making sure that the flags field of the AF is present.
        //! @return True if the expected stuffing size is available. False when the adaptation
        //! needed to be enlarged but could not because @a shift_payload was false.
        //!
        bool reserveStuffing(size_t size, bool shift_payload = false, bool enforce_af = false);

        //!
        //! Check if the packet contains the start of a clear PES header.
        //! @return True if the packet contains the start of a clear PES header.
        //!
        bool startPES() const;

        //!
        //! Get the size of the PES header in the packet, if one is present.
        //! @return The size of the PES header in bytes or zero if there is no PES header.
        //! It is not guaranteed that the complete PES header fits inside the TS packet.
        //!
        size_t getPESHeaderSize() const;

        //!
        //! Get the address and size of the stuffing area of the PES header in the TS packet.
        //! @param [out] addr Address of the PES header stuffing area. This address points
        //! inside the TS packet payload.
        //! @param [out] pes_size Size in bytes of the PES header stuffing area. This is the
        //! complete size, some of which can be outside the TS packet.
        //! @param [out] ts_size Size in bytes of the PES header stuffing area which is in the
        //! TS packet. This size can be lower than the returned @a pes_size if the stuffing
        //! area continues in another TS packet.
        //! @return True when the PES header stuffing area was found. False otherwise.
        //!
        bool getPESHeaderStuffingArea(const uint8_t*& addr, size_t& pes_size, size_t& ts_size) const;

        //!
        //! Get the address and size of the stuffing area of the PES header in the TS packet.
        //! @param [out] addr Address of the PES header stuffing area. This address points
        //! inside the TS packet payload.
        //! @param [out] pes_size Size in bytes of the PES header stuffing area. This is the
        //! complete size, some of which can be outside the TS packet.
        //! @param [out] ts_size Size in bytes of the PES header stuffing area which is in the
        //! TS packet. This size can be lower than the returned @a pes_size if the stuffing
        //! area continues in another TS packet.
        //! @return True when the PES header stuffing area was found. False otherwise.
        //!
        bool getPESHeaderStuffingArea(uint8_t*& addr, size_t& pes_size, size_t& ts_size);

        //!
        //! Check if the TS packet contains a Presentation Time Stamp (PTS).
        //! Technically, the PTS and DTS are part of the PES packet, not the TS packet.
        //! If the TS packet is the first TS packet of a PES packet, it is possible
        //! that the PTS and/or DTS are present in the PES packet but outside the
        //! first TS packet. This is possible but rare. So, we provide here a fast
        //! way of getting PTS and/or DTS from the TS packet if available.
        //! @return True if the packet contains a PTS.
        //!
        bool hasPTS() const
        {
            return PTSOffset() > 0;
        }

        //!
        //! Check if the TS packet contains a Decoding Time Stamp (DTS).
        //! @return True if the packet contains a DTS.
        //! @see hasPTS()
        //!
        bool hasDTS() const
        {
            return DTSOffset() > 0;
        }

        //!
        //! Get the PTS - 33 bits.
        //! @return The PTS or INVALID_PTS if not found.
        //!
        uint64_t getPTS() const
        {
            return getPDTS(PTSOffset());
        }

        //!
        //! Get the DTS - 33 bits.
        //! @return The DTS or INVALID_DTS if not found.
        //!
        uint64_t getDTS() const
        {
            return getPDTS(DTSOffset());
        }

        //!
        //! Replace the PTS value - 33 bits
        //! @param [in] pts The new PTS value.
        //!
        void setPTS(const uint64_t& pts)
        {
            setPDTS(pts, PTSOffset());
        }

        //!
        //! Replace the DTS value - 33 bits
        //! @param [in] dts The new DTS value.
        //!
        void setDTS(const uint64_t& dts)
        {
            setPDTS(dts, DTSOffset());
        }

        //!
        //! Check if this packet has the same payload as another one.
        //! @param [in] other The other packet to compare.
        //! @return True if the two packets have a payload and these payloads are identical.
        //!
        bool samePayload(const TSPacket& other) const;

        //!
        //! Check if this packet is a duplicate as another one.
        //! A valid "true" pair of duplicate packets is made of two consecutive packets with
        //! same continuity counter and same payload. It must also have the same adaptation field,
        //! with the exception of the PCR which can (should?) be different.
        //! @param [in] other The other packet to compare.
        //! @return True if the two packets have same PID, same CC and same payload.
        //!
        bool isDuplicate(const TSPacket& other) const;

        //!
        //! Read a packet from standard streams (binary mode).
        //! @param [in,out] strm A standard stream in input mode.
        //! @param [in] check_sync If true, the sync byte of the input packet is checked.
        //! If it is not valid, set the failbit of the stream.
        //! @param [in,out] report Where to report errors.
        //! @return A reference to the @a strm object.
        //!
        std::istream& read(std::istream& strm, bool check_sync = true, Report& report = CERR);

        //!
        //! Write a packet to standard streams (binary mode).
        //! @param [in,out] strm A standard stream in output mode.
        //! @param [in,out] report Where to report errors.
        //! @return A reference to the @a strm object.
        //!
        std::ostream& write(std::ostream& strm, Report& report = CERR) const;

        //!
        //! Options for packet display.
        //!
        enum {
            DUMP_RAW        = 0x00010000,  //!< Complete packet in hexadecimal (default)
            DUMP_TS_HEADER  = 0x00020000,  //!< Decode/format TS header
            DUMP_PES_HEADER = 0x00040000,  //!< Decode/format PES header
            DUMP_PAYLOAD    = 0x00080000,  //!< Payload in hexadecimal
            DUMP_AF         = 0x00100000,  //!< Decode/format adaptation field
        };

        //!
        //! This method displays the content of a transport packet.
        //! @param [in,out] strm A standard stream in output mode (text mode).
        //! @param [in] flags Indicate which part must be dumped. If DUMP_RAW or
        //! DUMP_PAYLOAD is specified, flags from ts::UString::HexaFlags may also be used.
        //! @param [in] indent Indicates the base indentation of lines.
        //! @param [in] size Maximum size to display in the packet.
        //! @return A reference to the @a strm object.
        //!
        std::ostream& display(std::ostream& strm, uint32_t flags = 0, size_t indent = 0, size_t size = PKT_SIZE) const;

        //!
        //! Init packet from a memory area.
        //! @param [in] source Address of the memory area to read. Must contain at least PKT_SIZE bytes.
        //!
        void copyFrom(const void* source);

        //!
        //! Copy packet content to a memory area.
        //! @param [out] dest Address of the memory area to write. Must contain at least PKT_SIZE bytes.
        //!
        void copyTo(void* dest) const;

        //!
        //! Copy contiguous TS packets.
        //! @param [out] dest Address of the first contiguous TS packet to write.
        //! @param [in] source Address of the first contiguous TS packet to read.
        //! @param [in] count Number of TS packets to copy.
        //!
        static void Copy(TSPacket* dest, const TSPacket* source, size_t count = 1);

        //!
        //! Copy contiguous TS packets from raw memory.
        //! @param [out] dest Address of the first contiguous TS packet to write.
        //! @param [in] source Address of the memory area to read.
        //! @param [in] count Number of TS packets to copy.
        //!
        static void Copy(TSPacket* dest, const uint8_t* source, size_t count = 1);

        //!
        //! Copy contiguous TS packets into raw memory.
        //! @param [out] dest Address of the memory area to write.
        //! @param [in] source Address of the first contiguous TS packet to read.
        //! @param [in] count Number of TS packets to copy.
        //!
        static void Copy(uint8_t* dest, const TSPacket* source, size_t count = 1);

        //!
        //! Locate contiguous TS packets into a buffer.
        //!
        //! This static method is typically used to locate useful packets in a UDP datagram.
        //! Basically, we expect the message to contain only TS packets. However, we
        //! also face the following situations:
        //! - Presence of a header preceeding the first TS packet (typically when the
        //!   TS packets are encapsulated in RTP).
        //! - Presence of a truncated packet at the end of message.
        //!
        //! To face the first situation, we look backward from the end of the message,
        //! looking for a 0x47 sync byte every 188 bytes, going backward.
        //!
        //! If no TS packet is found using the first method, we restart from
        //! the beginning of the message, looking for a 0x47 sync byte every
        //! 188 bytes, going forward. If we find this pattern, followed by
        //! less than 188 bytes, then we have found a sequence of TS packets.
        //!
        //! @param [in] buffer Address of a message buffer containing TS packets.
        //! @param [in] buffer_size Size in bytes of the buffer.
        //! @param [out] start_index Start index in bytes of the first TS packet in the buffer.
        //! @param [out] packet_count Number of TS packets in the buffer.
        //! @return True if at least one packet was found, false if there is no packet in the buffer.
        //!
        static bool Locate(const uint8_t* buffer, size_t buffer_size, size_t& start_index, size_t& packet_count);

        //!
        //! Sanity check routine.
        //! Ensure that the TSPacket structure can
        //! be used in contiguous memory array and array of packets.
        //! Can be used once at startup time in paranoid applications.
        //! Abort application on error.
        //!
        static void SanityCheck();

    private:
        // These private methods compute the offset of PCR, OPCR, etc.
        // Return 0 if there is none.
        size_t PCROffset() const;
        size_t OPCROffset() const;
        size_t PTSOffset() const;
        size_t DTSOffset() const;
        size_t spliceCountdownOffset() const;
        size_t privateDataOffset() const;

        // Get or set PTS or DTS at specified offset. Return INVALID_PTS if offset is zero.
        uint64_t getPDTS(size_t offset) const;
        void setPDTS(uint64_t pdts, size_t offset);

        // Set flags in the adaptation field.
        bool setFlagsInAF(uint8_t flags, bool shift_payload);

        // Erase an AF field at specified offset (if not zero), clear corresponding AF flag.
        void deleteFieldFromAF(size_t offset, size_t size, uint32_t flag);
    };

    //!
    //! This constant is a null (or stuffing) packet.
    //!
    TSDUCKDLL extern const TSPacket NullPacket;

    //!
    //! This constant is an empty packet (no payload).
    //! PID and CC shall be updated for use in specific PID's.
    //!
    TSDUCKDLL extern const TSPacket EmptyPacket;

    //!
    //! Vector of packets.
    //!
    typedef std::vector<TSPacket> TSPacketVector;

    //!
    //! TS packet are accessed in a memory-resident buffer.
    //!
    typedef ResidentBuffer<TSPacket> PacketBuffer;
}

//!
//! Output operator for the class @link ts::TSPacket @endlink on standard text streams.
//! @param [in,out] strm A standard stream in output mode.
//! @param [in] pkt TS packet object.
//! @return A reference to the @a strm object.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::TSPacket& pkt)
{
    return pkt.display(strm);
}
