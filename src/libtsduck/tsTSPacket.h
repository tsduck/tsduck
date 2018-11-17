//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Basic definition of an MPEG-2 transport packet.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"
#include "tsCerrReport.h"
#include "tsException.h"

namespace ts {
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
    struct TSDUCKDLL TSPacket
    {
        //!
        //! The public content is the 188-byte array representing the TS packet.
        //!
        uint8_t b[PKT_SIZE];

        //!
        //! Internal error: access a non-existent adaptation field.
        //!
        TS_DECLARE_EXCEPTION(AdaptationFieldError);

        //!
        //! Assigment operator.
        //! @param [in] p Other packet to copy.
        //! @return A reference to this object.
        //!
        inline TSPacket& operator=(const TSPacket& p)
        {
            ::memcpy(b, p.b, PKT_SIZE);
            return *this;
        }

        //!
        //! Equality operator.
        //! @param [in] p Other packet to compare.
        //! @return True is this object is equal to @a p.
        //!
        inline bool operator==(const TSPacket& p) const
        {
            return ::memcmp(b, p.b, PKT_SIZE) == 0;
        }

        //!
        //! Unequality operator.
        //! @param [in] p Other packet to compare.
        //! @return True is this object is different from @a p.
        //!
        inline bool operator!=(const TSPacket& p) const
        {
            return ::memcmp(b, p.b, PKT_SIZE) != 0;
        }

        //!
        //! Check if the sync byte is valid.
        //! @return True if the sync byte of the packet is valid.
        //!
        inline bool hasValidSync() const
        {
            return b[0] == SYNC_BYTE;
        }

        //!
        //! Extract PID - 13 bits.
        //! @return The PID value.
        //!
        inline PID getPID() const
        {
            return GetUInt16(b+1) & 0x1FFF;
        }

        //!
        //! Set PID - 13 bits.
        //! @param [in] pid The new PID.
        //!
        inline void setPID(PID pid)
        {
            b[1] = (b[1] & 0xE0) | ((pid >> 8) & 0x1F);
            b[2] = pid & 0x00FF;
        }

        //!
        //! Extract payload_unit_start_indicator (PUSI) - 1 bit
        //! @return The PUSI value.
        //!
        inline bool getPUSI() const
        {
            return (b[1] & 0x40) != 0;
        }

        //!
        //! Clear payload_unit_start_indicator (PUSI) - 1 bit
        //!
        inline void clearPUSI()
        {
            b[1] &= ~0x40;
        }

        //!
        //! Set payload_unit_start_indicator (PUSI) - 1 bit
        //!
        inline void setPUSI()
        {
            b[1] |= 0x40;
        }

        //!
        //! Extract transport_error_indicator (TEI) - 1 bit
        //! @return The TEI value.
        //!
        inline bool getTEI() const
        {
            return (b[1] & 0x80) != 0;
        }

        //!
        //! Clear transport_error_indicator (TEI) - 1 bit
        //!
        inline void clearTEI()
        {
            b[1] &= ~0x80;
        }

        //!
        //! Set transport_error_indicator (TEI) - 1 bit
        //!
        inline void setTEI()
        {
            b[1] |= 0x80;
        }

        //!
        //! Extract transport_priority - 1 bit
        //! @return The transport_priority value.
        //!
        inline bool getPriority() const
        {
            return (b[1] & 0x20) != 0;
        }

        //!
        //! Clear transport_priority - 1 bit
        //!
        inline void clearPriority()
        {
            b[1] &= ~0x20;
        }

        //!
        //! Set transport_priority - 1 bit
        //!
        inline void setPriority()
        {
            b[1] |= 0x20;
        }

        //!
        //! Extract transport_scrambling_control - 2 bits
        //! @return The transport_scrambling_control value.
        //!
        inline uint8_t getScrambling() const
        {
            return b[3] >> 6;
        }

        //!
        //! Check if the packet is clear (ie not scrambled).
        //! @return True if the packet is clear.
        //!
        inline bool isClear() const
        {
            return (b[3] >> 6) == 0;
        }

        //!
        //! Check if the packet is scrambled.
        //! @return True if the packet is scrambled.
        //!
        inline bool isScrambled() const
        {
            return (b[3] >> 6) != 0;
        }

        //!
        //! Set transport_scrambling_control - 2 bits.
        //! @param [in] sc New transport_scrambling_control value.
        //!
        inline void setScrambling(uint8_t sc)
        {
            b[3] = (b[3] & 0x3F) | (sc << 6);
        }

        //!
        //! Extract continuity_counter (CC) - 4 bits
        //! @return The CC value.
        //!
        inline uint8_t getCC() const
        {
            return b[3] & 0x0F;
        }

        //!
        //! Set continuity_counter (CC) - 4 bits
        //! @param [in] cc New continuity_counter value.
        //!
        inline void setCC(uint8_t cc)
        {
            b[3] = (b[3] & 0xF0) | (cc & 0x0F);
        }

        //!
        //! Check if packet has an adaptation_field (AF)
        //! @return True if the packet has an adaptation_field.
        //!
        inline bool hasAF() const
        {
            return (b[3] & 0x20) != 0;
        }

        //!
        //! Compute adaptation_field (AF) size.
        //! @return Size in bytes of the adaptation_field.
        //!
        inline size_t getAFSize() const
        {
            return hasAF() ? size_t(b[4]) : 0;
        }

        //!
        //! Compute the size of the TS header.
        //! @return Size in bytes of the TS header.
        //! This is also the index of the TS payload.
        //!
        inline size_t getHeaderSize() const
        {
            return std::min(4 + (hasAF() ? (size_t(b[4]) + 1) : 0), PKT_SIZE);
        }

        //!
        //! Check if packet has a payload
        //! @return True if the packet has a payload.
        //!
        inline bool hasPayload() const
        {
            return (b[3] & 0x10) != 0;
        }

        //!
        //! Get payload start address.
        //! @return The payload start address.
        //!
        inline const uint8_t* getPayload() const
        {
            return b + getHeaderSize();
        }

        //!
        //! Get payload start address.
        //! @return The payload start address.
        //!
        inline uint8_t* getPayload()
        {
            return b + getHeaderSize();
        }

        //!
        //! Compute payload size.
        //! @return The payload size in bytes.
        //!
        inline size_t getPayloadSize() const
        {
            return hasPayload() ? PKT_SIZE - getHeaderSize() : 0;
        }

        //!
        //! Check if packet has a discontinuity_indicator set - 1 bit
        //! @return True if packet has a discontinuity_indicator set.
        //!
        inline bool getDiscontinuityIndicator() const
        {
            return getAFSize() > 0 ? ((b[5] & 0x80) != 0) : false;
        }

        //!
        //! Check if packet has a random_access_indicator set - 1 bit
        //! @return True if packet has a random_access_indicator set.
        //!
        inline bool getRandomAccessIndicator() const
        {
            return getAFSize() > 0 ? ((b[5] & 0x40) != 0) : false;
        }

        //!
        //! Check if packet has a elementary_stream_priority_indicator (ESPI) set - 1 bit
        //! @return True if packet has an ESPI set.
        //!
        inline bool getESPI() const
        {
            return getAFSize() > 0 ? ((b[5] & 0x20) != 0) : false;
        }

        //!
        //! Check if packet has a Program Clock Reference (PCR)
        //! @return True if packet has a PCR.
        //!
        inline bool hasPCR() const
        {
            return getAFSize() > 0 && (b[5] & 0x10) != 0;
        }

        //!
        //! Check if packet has an Original Program Clock Reference (OPCR)
        //! @return True if packet has an OPCR.
        //!
        inline bool hasOPCR() const
        {
            return getAFSize() > 0 && (b[5] & 0x08) != 0;
        }

        //!
        //! Check if packet has splicing point countdown
        //! @return True if packet has a splicing point countdown.
        //!
        inline bool hasSpliceCountdown() const
        {
            return getAFSize() > 0 && (b[5] & 0x04) != 0;
        }

        //!
        //! Get the PCR - 42 bits.
        //! @return The PCR or 0 if not found.
        //!
        uint64_t getPCR() const;

        //!
        //! Get the OPCR - 42 bits.
        //! @return The OPCR or 0 if not found.
        //!
        uint64_t getOPCR() const;

        //!
        //! Get the splicing point countdown - 8 bits (signed).
        //! @return The splicing point countdown or 0 if not found.
        //!
        int8_t getSpliceCountdown() const;


        //!
        //! Replace the PCR value - 42 bits
        //! @param [in] pcr The new PCR value.
        //! @throw AdaptationFieldError if no PCR is present.
        //!
        void setPCR(const uint64_t& pcr);

        //!
        //! Replace the OPCR value - 42 bits
        //! @param [in] opcr The new OPCR value.
        //! @throw AdaptationFieldError if no OPCR is present.
        //!
        void setOPCR(const uint64_t& opcr);

        //!
        //! Check if the packet contains the start of a clear PES header.
        //! @return True if the packet contains the start of a clear PES header.
        //!
        bool startPES() const;

        //!
        //! Check if the TS packet contains a Presentation Time Stamp (PTS).
        //! Technically, the PTS and DTS are part of the PES packet, not the TS packet.
        //! If the TS packet is the first TS packet of a PES packet, it is possible
        //! that the PTS and/or DTS are present in the PES packet but outside the
        //! first TS packet. This is possible but rare. So, we provide here a fast
        //! way of getting PTS and/or DTS from the TS packet if available.
        //! @return True if the packet contains a PTS.
        //!
        inline bool hasPTS() const
        {
            return PTSOffset() > 0;
        }

        //!
        //! Check if the TS packet contains a Decoding Time Stamp (DTS).
        //! @return True if the packet contains a DTS.
        //! @see hasPTS()
        //!
        inline bool hasDTS() const
        {
            return DTSOffset() > 0;
        }

        //!
        //! Get the PTS - 33 bits.
        //! @return The PTS or 0 if not found.
        //!
        uint64_t getPTS() const
        {
            return getPDTS(PTSOffset());
        }

        //!
        //! Get the DTS - 33 bits.
        //! @return The DTS or 0 if not found.
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
            DUMP_PAYLOAD    = 0x00080000   //!< Payload in hexadecimal
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
        void copyFrom(const void* source)
        {
            assert(source != nullptr);
            ::memcpy(b, source, PKT_SIZE);
        }

        //!
        //! Copy packet content to a memory area.
        //! @param [out] dest Address of the memory area to write. Must contain at least PKT_SIZE bytes.
        //!
        void copyTo(void* dest) const
        {
            assert(dest != nullptr);
            ::memcpy(dest, b, PKT_SIZE);
        }

        //!
        //! Copy contiguous TS packets.
        //! @param [out] dest Address of the first contiguous TS packet to write.
        //! @param [in] source Address of the first contiguous TS packet to read.
        //! @param [in] count Number of TS packets to copy.
        //!
        static inline void Copy(TSPacket* dest, const TSPacket* source, size_t count = 1)
        {
            assert(dest != nullptr);
            assert(source != nullptr);
            ::memcpy(dest->b, source->b, count * PKT_SIZE);
        }

        //!
        //! Copy contiguous TS packets from raw memory.
        //! @param [out] dest Address of the first contiguous TS packet to write.
        //! @param [in] source Address of the memory area to read.
        //! @param [in] count Number of TS packets to copy.
        //!
        static inline void Copy(TSPacket* dest, const uint8_t* source, size_t count = 1)
        {
            assert(dest != nullptr);
            assert(source != nullptr);
            ::memcpy(dest->b, source, count * PKT_SIZE);
        }

        //!
        //! Copy contiguous TS packets into raw memory.
        //! @param [out] dest Address of the memory area to write.
        //! @param [in] source Address of the first contiguous TS packet to read.
        //! @param [in] count Number of TS packets to copy.
        //!
        static inline void Copy(uint8_t* dest, const TSPacket* source, size_t count = 1)
        {
            assert(dest != nullptr);
            assert(source != nullptr);
            ::memcpy(dest, source->b, count * PKT_SIZE);
        }

        //!
        //! Sanity check routine.
        //! Ensure that the TSPacket structure can
        //! be used in contiguous memory array and array of packets.
        //! Can be used once at startup time in paranoid applications.
        //! Abort application on error.
        //!
        static void SanityCheck();

    private:
        // These private methods compute the offset of PCR, OPCR, PTS, DTS.
        // Return 0 if there is none.
        size_t PCROffset() const;
        size_t OPCROffset() const;
        size_t PTSOffset() const;
        size_t DTSOffset() const;
        size_t spliceCountdownOffset() const;

        // Get or set PTS or DTS at specified offset. Return 0 if offset is zero.
        uint64_t getPDTS(size_t offset) const;
        void setPDTS(uint64_t pdts, size_t offset);
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
