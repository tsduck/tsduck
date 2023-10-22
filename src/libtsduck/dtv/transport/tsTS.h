//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup mpeg
//!  Common definitions for MPEG Transport Stream layer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsBitRate.h"
#include "tsEnumeration.h"

namespace ts {
    //!
    //! PID value (13 bits).
    //!
    typedef uint16_t PID;

    //!
    //! MPEG TS packet size in bytes.
    //!
    constexpr size_t PKT_SIZE = 188;

    //!
    //! MPEG TS packet size in bits.
    //!
    constexpr size_t PKT_SIZE_BITS = 8 * PKT_SIZE;

    //!
    //! MPEG TS packet header size in bytes.
    //!
    constexpr size_t PKT_HEADER_SIZE = 4;

    //!
    //! MPEG TS packet maximum payload size in bytes.
    //!
    constexpr size_t PKT_MAX_PAYLOAD_SIZE = PKT_SIZE - PKT_HEADER_SIZE;

    //!
    //! Size in bytes of a Reed-Solomon outer FEC.
    //!
    constexpr size_t RS_SIZE = 16;

    //!
    //! Size in bytes of a TS packet with trailing Reed-Solomon outer FEC.
    //!
    constexpr size_t PKT_RS_SIZE = PKT_SIZE + RS_SIZE;

    //!
    //! Size in bits of a TS packet with trailing Reed-Solomon outer FEC.
    //!
    constexpr size_t PKT_RS_SIZE_BITS = 8 * PKT_RS_SIZE;

    //!
    //! Size in bytes of a timestamp preceeding a TS packet in M2TS files (Blu-ray disc).
    //!
    constexpr size_t M2TS_HEADER_SIZE = 4;

    //!
    //! Size in bytes of an TS packet in M2TS files (Blu-ray disc).
    //! There is a leading 4-byte timestamp before the TS packet.
    //!
    constexpr size_t PKT_M2TS_SIZE = M2TS_HEADER_SIZE + PKT_SIZE;

    //!
    //! Number of Transport Stream packets.
    //!
    //! TS packets are counted using 64-bit integers.
    //! Thus, PacketCounter will never overflow: at 100 Mb/s, 2^64 188-byte
    //! packets will take 8.7 million years to transmit. No process will
    //! ever run that long. On the contrary, using 32-bit integer would
    //! be insufficient: at 100 Mb/s, 2^32 188-byte packets will take
    //! only 17 hours to transmit.
    //!
    typedef uint64_t PacketCounter;

    //!
    //! A impossible value for PacketCounter, meaning "undefined".
    //!
    constexpr PacketCounter INVALID_PACKET_COUNTER = std::numeric_limits<PacketCounter>::max();

    //!
    //! Number of sections.
    //!
    typedef uint64_t SectionCounter;

    //!
    //! Value of a sync byte (first byte in a TS packet).
    //!
    constexpr uint8_t SYNC_BYTE = 0x47;

    //!
    //! Size (in bits) of a PID field.
    //!
    constexpr size_t PID_BITS = 13;

    //!
    //! Maximum number of PID's (8192).
    //!
    constexpr PID PID_MAX = 1 << PID_BITS;

    //!
    //! A bit mask for PID values.
    //! Useful to implement PID filtering.
    //!
    typedef std::bitset<PID_MAX> PIDSet;

    //!
    //! PIDSet constant with no PID set.
    //!
    TSDUCKDLL extern const PIDSet NoPID;

    //!
    //! PIDSet constant with all PID's set.
    //!
    TSDUCKDLL extern const PIDSet AllPIDs;

    //!
    //! Size (in bits) of a Continuity Counter (CC) field.
    //!
    constexpr size_t CC_BITS = 4;

    //!
    //! Mask to wrap a Continuity Counter (CC) value.
    //! CC values wrap at 16.
    //!
    constexpr uint8_t CC_MASK = 0x0F;

    //!
    //! Maximum value of a Continuity Counter (CC).
    //!
    constexpr uint8_t CC_MAX = 1 << CC_BITS;

    //!
    //! An invalid Continuity Counter (CC) value, typically meaning "undefined".
    //!
    constexpr uint8_t INVALID_CC = 0xFF;

    //!
    //! Scrambling_control values (used in TS and PES packets headers)
    //!
    enum : uint8_t {
        SC_CLEAR        = 0,  //!< Not scrambled (MPEG-defined).
        SC_DVB_RESERVED = 1,  //!< Reserved for future use by DVB.
        SC_EVEN_KEY     = 2,  //!< Scrambled with even key (DVB-defined).
        SC_ODD_KEY      = 3   //!< Scrambled with odd key (DVB-defined).
    };

    //---------------------------------------------------------------------
    // Bitrates computations.
    //---------------------------------------------------------------------

    //!
    //! Confidence in a bitrate value.
    //!
    //! Bitrates can be provided by various sources, some being more reliable than others.
    //! Each bitrate value or computation is associated with a "level of confidence".
    //! This enumeration type lists various levels of confidence in increasing order.
    //! When evaluating a bitrate from several values, the one with highest confidence
    //! is used.
    //!
    enum class BitRateConfidence {
        LOW,              //!< Low confidence, used as last resort.
        PCR_CONTINUOUS,   //!< Evaluated from PCR's, continuously adjusted.
        PCR_AVERAGE,      //!< Evaluated from PCR's, average all over the stream.
        CLOCK,            //!< Evaluated using the system clock on a real-time stream.
        HARDWARE,         //!< Reported by hardware input device (demodulator, ASI).
        OVERRIDE,         //!< Highest level, overrides any other value (user-defined for instance).
    };

    //!
    //! Select a bitrate from two input values with potentially different levels of confidence.
    //! @param [in] bitrate1 First bitrate.
    //! @param [in] brc1 Level of confidence for @a bitrate1.
    //! @param [in] bitrate2 Second bitrate.
    //! @param [in] brc2 Level of confidence for @a bitrate2.
    //! @return The selected bitrate value.
    //!
    TSDUCKDLL BitRate SelectBitrate(const BitRate& bitrate1, BitRateConfidence brc1, const BitRate& bitrate2, BitRateConfidence brc2);

    //!
    //! Convert 188-byte packet bitrate into 204-byte packet bitrate.
    //! @param [in] bitrate188 Bitrate using 188-byte packet as reference.
    //! @return Corresponding bitrate using 204-byte packet as reference.
    //!
    TSDUCKDLL inline BitRate ToBitrate204(const BitRate& bitrate188)
    {
        return (bitrate188 * 204) / 188;
    }

    //!
    //! Convert 204-byte packet bitrate into 188-byte packet bitrate.
    //! @param [in] bitrate204 Bitrate using 204-byte packet as reference.
    //! @return Corresponding bitrate using 188-byte packet as reference.
    //!
    TSDUCKDLL inline BitRate ToBitrate188(const BitRate& bitrate204)
    {
        return (bitrate204 * 188) / 204;
    }

    //!
    //! Compute the interval, in milliseconds, between two packets.
    //! @param [in] bitrate TS bitrate in bits/second, based on 188-byte packets.
    //! @param [in] distance Distance between the two packets: 0 for the same
    //! packet, 1 for the next packet (the default), etc.
    //! @return Interval in milliseconds between the first byte of the first packet
    //! and the first byte of the second packet.
    //!
    TSDUCKDLL inline MilliSecond PacketInterval(const BitRate& bitrate, PacketCounter distance = 1)
    {
        return bitrate == 0 ? 0 : ((distance * PKT_SIZE_BITS * MilliSecPerSec) / bitrate).toInt();
    }

    //!
    //! Compute the number of packets transmitted during a given duration in milliseconds.
    //! @param [in] bitrate TS bitrate in bits/second, based on 188-byte packets.
    //! @param [in] duration Number of milliseconds.
    //! @return Number of packets during @a duration milliseconds.
    //!
    TSDUCKDLL inline PacketCounter PacketDistance(const BitRate& bitrate, MilliSecond duration)
    {
        return PacketCounter(((bitrate * (duration >= 0 ? duration : -duration)) / (MilliSecPerSec * PKT_SIZE_BITS)).toInt());
    }

    //!
    //! Compute the bitrate from a number of packets transmitted during a given duration in milliseconds.
    //! @param [in] packets Number of packets during @a duration milliseconds.
    //! @param [in] duration Number of milliseconds.
    //! @return TS bitrate in bits/second, based on 188-byte packets.
    //!
    TSDUCKDLL inline BitRate PacketBitRate(PacketCounter packets, MilliSecond duration)
    {
        return duration == 0 ? 0 : BitRate(packets * MilliSecPerSec * PKT_SIZE_BITS) / BitRate(duration);
    }

    //!
    //! Compute the minimum number of TS packets required to transport a section.
    //! @param [in] section_size Total section size in bytes.
    //! @return Number of packets required for the section.
    //!
    TSDUCKDLL inline PacketCounter SectionPacketCount(size_t section_size)
    {
        // The required size for a section is section_size + 1 (1 for pointer_field
        // in first packet). In each packet, the useable size is 184 bytes.
        return PacketCounter((section_size + 184) / 184);
    }

    //---------------------------------------------------------------------
    //! Predefined PID values
    //---------------------------------------------------------------------

    enum : PID {

        // Valid in all MPEG contexts:

        PID_PAT        = 0x0000, //!< PID for Program Association Table PAT
        PID_CAT        = 0x0001, //!< PID for Conditional Access Table
        PID_TSDT       = 0x0002, //!< PID for Transport Stream Description Table
        PID_MPEG_LAST  = 0x000F, //!< Last reserved PID for MPEG

        // Valid in DVB context:

        PID_DVB_FIRST  = 0x0010, //!< First reserved PID for DVB
        PID_NIT        = 0x0010, //!< PID for Network Information Table
        PID_SDT        = 0x0011, //!< PID for Service Description Table
        PID_BAT        = 0x0011, //!< PID for Bouquet Association Table
        PID_EIT        = 0x0012, //!< PID for Event Information Table
        PID_CIT        = 0x0012, //!< PID for Content Identifier Table (TV-Anytime)
        PID_RST        = 0x0013, //!< PID for Running Status Table
        PID_TDT        = 0x0014, //!< PID for Time & Date Table
        PID_TOT        = 0x0014, //!< PID for Time Offset Table
        PID_NETSYNC    = 0x0015, //!< PID for Network synchronization
        PID_RNT        = 0x0016, //!< PID for Resolution Notification Table (TV-Anytime)
        PID_SAT        = 0x001B, //!< PID for Satellite Access Table
        PID_INBSIGN    = 0x001C, //!< PID for Inband Signalling
        PID_MEASURE    = 0x001D, //!< PID for Measurement
        PID_DIT        = 0x001E, //!< PID for Discontinuity Information Table
        PID_SIT        = 0x001F, //!< PID for Selection Information Table
        PID_DVB_LAST   = 0x001F, //!< Last reserved PID for DVB

        // Valid in ISDB context:

        PID_DCT        = 0x0017, //!< PID for ISDB Download Control Table
        PID_ISDB_FIRST = 0x0020, //!< First reserved PID for ISDB
        PID_LIT        = 0x0020, //!< PID for ISDB Local Event Information Table
        PID_ERT        = 0x0021, //!< PID for ISDB Event Relation Table
        PID_PCAT       = 0x0022, //!< PID for ISDB Partial Content Announcement Table
        PID_SDTT       = 0x0023, //!< PID for ISDB Software Download Trigger Table
        PID_BIT        = 0x0024, //!< PID for ISDB Broadcaster Information Table
        PID_NBIT       = 0x0025, //!< PID for ISDB Network Board Information Table
        PID_LDT        = 0x0025, //!< PID for ISDB Linked Description Table
        PID_ISDB_EIT_2 = 0x0026, //!< Additional PID for ISDB Event Information Table
        PID_ISDB_EIT_3 = 0x0027, //!< Additional PID for ISDB Event Information Table
        PID_SDTT_TER   = 0x0028, //!< PID for ISDB Software Download Trigger Table (terrestrial)
        PID_CDT        = 0x0029, //!< PID for ISDB Common Data Table
        PID_AMT        = 0x002E, //!< PID for ISDB Address Map Table
        PID_ISDB_LAST  = 0x002F, //!< Last reserved PID for ISDB

        // Valid in ATSC context:

        PID_ATSC_FIRST = 0x1FF0, //!< First reserved PID for ATSC.
        PID_ATSC_PAT_E = 0x1FF7, //!< PID for ATSC PAT-E
        PID_PSIP_TS_E  = 0x1FF9, //!< PID for ATSC Program and System Information Protocol in TS-E
        PID_PSIP       = 0x1FFB, //!< PID for ATSC Program and System Information Protocol (contains most ATSC tables)
        PID_ATSC_LAST  = 0x1FFE, //!< Last reserved PID for ATSC.

        // Valid in all MPEG contexts:

        PID_NULL       = 0x1FFF, //!< PID for Null packets (stuffing)
    };

    //---------------------------------------------------------------------
    //! Classification of PID's.
    //---------------------------------------------------------------------

    enum class PIDClass {
        UNDEFINED,  //!< Undefined PID class.
        PSI,        //!< Signalization (PAT, CAT, PMT, etc).
        EMM,        //!< PID carrying EMM's.
        ECM,        //!< PID carrying ECM's.
        VIDEO,      //!< Video component of a service.
        AUDIO,      //!< Audio component of a service.
        SUBTITLES,  //!< Subtitles component of a service.
        DATA,       //!< Data component of a service.
        STUFFING,   //!< Null packets.
    };

    //!
    //! Enumeration description of ts::PIDClass.
    //!
    TSDUCKDLL extern const Enumeration PIDClassEnum;

    //---------------------------------------------------------------------
    // MPEG clock representation:
    // - PCR (Program Clock Reference)
    // - PTS (Presentation Time Stamp)
    // - DTS (Decoding Time Stamp)
    //---------------------------------------------------------------------

    //!
    //! MPEG-2 System Clock frequency in Hz, used by PCR (27 Mb/s).
    //!
    constexpr uint32_t SYSTEM_CLOCK_FREQ = 27000000;

    //!
    //! Subfactor of MPEG-2 System Clock subfrequency, used by PTS and DTS.
    //!
    constexpr uint32_t SYSTEM_CLOCK_SUBFACTOR = 300;

    //!
    //! MPEG-2 System Clock subfrequency in Hz, used by PTS and DTS (90 Kb/s).
    //!
    constexpr uint32_t SYSTEM_CLOCK_SUBFREQ = SYSTEM_CLOCK_FREQ / SYSTEM_CLOCK_SUBFACTOR;

    //!
    //! Size in bits of a PCR (Program Clock Reference).
    //! Warning: A PCR value is not a linear value mod 2^42.
    //! It is split into PCR_base and PCR_ext (see ISO 13818-1, 2.4.2.2).
    //!
    constexpr size_t PCR_BIT_SIZE = 42;

    //!
    //! Size in bits of a PTS (Presentation Time Stamp) or DTS (Decoding Time Stamp).
    //! Unlike PCR, PTS and DTS are regular 33-bit binary values, wrapping up at 2^33.
    //!
    constexpr size_t PTS_DTS_BIT_SIZE = 33;

    //!
    //! Scale factor for PTS and DTS values (wrap up at 2^33).
    //!
    constexpr uint64_t PTS_DTS_SCALE = 1LL << PTS_DTS_BIT_SIZE;

    //!
    //! Mask for PTS and DTS values (wrap up at 2^33).
    //!
    constexpr uint64_t PTS_DTS_MASK = PTS_DTS_SCALE - 1;

    //!
    //! The maximum value possible for a PTS/DTS value.
    //!
    constexpr uint64_t MAX_PTS_DTS = PTS_DTS_SCALE - 1;

    //!
    //! Scale factor for PCR values.
    //! This is not a power of 2, it does not wrap up at a number of bits.
    //! The PCR_base part is equivalent to a PTS/DTS and wraps up at 2**33.
    //! The PCR_ext part is a mod 300 value. Note that, since this not a
    //! power of 2, there is no possible PCR_MASK value.
    //!
    constexpr uint64_t PCR_SCALE = PTS_DTS_SCALE * SYSTEM_CLOCK_SUBFACTOR;

    //!
    //! The maximum value possible for a PCR (Program Clock Reference) value.
    //!
    constexpr uint64_t MAX_PCR = PCR_SCALE - 1;

    //!
    //! An invalid PCR (Program Clock Reference) value, can be used as a marker.
    //!
    constexpr uint64_t INVALID_PCR = 0xFFFFFFFFFFFFFFFF;

    //!
    //! An invalid PTS value, can be used as a marker.
    //!
    constexpr uint64_t INVALID_PTS = 0xFFFFFFFFFFFFFFFF;

    //!
    //! An invalid DTS value, can be used as a marker.
    //!
    constexpr uint64_t INVALID_DTS = 0xFFFFFFFFFFFFFFFF;

    //!
    //! Check if PCR2 follows PCR1 after wrap up.
    //! @param [in] pcr1 First PCR.
    //! @param [in] pcr2 Second PCR.
    //! @return True if @a pcr2 is probably following @a pcr1 after wrapping up.
    //! The exact criteria is that @a pcr2 wraps up after @a pcr1 and their
    //! distance is within 20% of a full PCR range.
    //!
    TSDUCKDLL inline bool WrapUpPCR(uint64_t pcr1, uint64_t pcr2)
    {
        return pcr2 < pcr1 && (pcr1 - pcr2) > ((4 * PCR_SCALE) / 5);
    }

    //!
    //! Compute the PCR of a packet, based on the PCR of a previous packet.
    //! @param [in] last_pcr PCR in a previous packet.
    //! @param [in] distance Number of TS packets since the packet with @a last_pcr.
    //! @param [in] bitrate Constant bitrate of the stream in bits per second.
    //! @return The PCR of the packet which is at the specified @a distance from the packet with @a last_pcr
    //! or INVALID_PCR if a parameter is incorrect.
    //!
    TSDUCKDLL uint64_t NextPCR(uint64_t last_pcr, PacketCounter distance, const BitRate& bitrate);

    //!
    //! Compute the difference between PCR2 and PCR1.
    //! @param [in] pcr1 First PCR.
    //! @param [in] pcr2 Second PCR.
    //! @return The difference between the two values or INVALID_PCR if a parameter is incorrect.
    //!
    TSDUCKDLL uint64_t DiffPCR(uint64_t pcr1, uint64_t pcr2);

    //!
    //! Compute the absolute value of the difference between two PCR's, regardless of their order.
    //! @param [in] pcr1 First PCR.
    //! @param [in] pcr2 Second PCR.
    //! @return The difference between the two values or INVALID_PCR if a parameter is incorrect.
    //!
    TSDUCKDLL uint64_t AbsDiffPCR(uint64_t pcr1, uint64_t pcr2);

    //!
    //! Compute the number of packets transmitted during a given duration in PCR units.
    //! @param [in] bitrate TS bitrate in bits/second, based on 188-byte packets.
    //! @param [in] pcr Number of PCR units.
    //! @return Number of packets during @a pcr time.
    //!
    TSDUCKDLL inline PacketCounter PacketDistanceFromPCR(BitRate bitrate, uint64_t pcr)
    {
        return PacketCounter(((bitrate * pcr) / (SYSTEM_CLOCK_FREQ * PKT_SIZE_BITS)).toInt());
    }

    //!
    //! Check if PTS2 follows PTS1 after wrap up.
    //! @param [in] pts1 First PTS.
    //! @param [in] pts2 Second PTS.
    //! @return True is @a pts2 is probably following @a pts1 after wrapping up at 2^33.
    //!
    TSDUCKDLL inline bool WrapUpPTS(uint64_t pts1, uint64_t pts2)
    {
        return pts2 < pts1 && (pts1 - pts2) > 0x00000001F0000000LL;
    }

    //!
    //! Check if two Presentation Time Stamps are in sequence.
    //!
    //! In MPEG video, B-frames are transported out-of-sequence.
    //! Their PTS is typically lower than the previous D-frame or I-frame
    //! in the transport. A "sequenced" PTS is one that is higher than
    //! the previous sequenced PTS (with possible wrap up).
    //! @param [in] pts1 First PTS.
    //! @param [in] pts2 Second PTS.
    //! @return True is @a pts2 is after @a pts1, possibly after wrapping up at 2**33.
    //!
    TSDUCKDLL inline bool SequencedPTS(uint64_t pts1, uint64_t pts2)
    {
        return pts1 <= pts2 || WrapUpPTS(pts1, pts2);
    }

    //!
    //! Compute the difference between PTS2 and PTS1.
    //! @param [in] pts1 First PTS.
    //! @param [in] pts2 Second PTS.
    //! @return The difference between the two values.
    //! or INVALID_PTS if a parameter is incorrect.
    //!
    TSDUCKDLL uint64_t DiffPTS(uint64_t pts1, uint64_t pts2);

    //!
    //! Convert a PCR value to a string.
    //! @param [in] pcr The PCR value.
    //! @param [in] hexa If true (the defaul), include hexadecimal value.
    //! @param [in] decimal If true (the defaul), include decimal value.
    //! @param [in] ms If true (the defaul), include the equivalent duration in milliseconds.
    //! @return The formatted string.
    //!
    TSDUCKDLL UString PCRToString(uint64_t pcr, bool hexa = true, bool decimal = true, bool ms = true);

    //!
    //! Convert a PTS or DTS value to a string.
    //! @param [in] pts The PTS or DTS value.
    //! @param [in] hexa If true (the defaul), include hexadecimal value.
    //! @param [in] decimal If true (the defaul), include decimal value.
    //! @param [in] ms If true (the defaul), include the equivalent duration in milliseconds.
    //! @return The formatted string.
    //!
    TSDUCKDLL UString PTSToString(uint64_t pts, bool hexa = true, bool decimal = true, bool ms = true);

    //!
    //! Convert a PCR value to milliseconds.
    //! @param [in] pcr The PCR value.
    //! @return The corresponding number of milliseconds or -1 on invalid value.
    //!
    TSDUCKDLL MilliSecond PCRToMilliSecond(uint64_t pcr);

    //!
    //! Convert a PTS or DTS value to milliseconds.
    //! @param [in] pts The PTS or DTS value.
    //! @return The corresponding number of milliseconds or -1 on invalid value.
    //!
    TSDUCKDLL MilliSecond PTSToMilliSecond(uint64_t pts);

    //---------------------------------------------------------------------
    //! Adaptation field descriptor tags.
    //! @see ISO 13818-1 / ITU-T Rec. H.262.0, section U.3
    //---------------------------------------------------------------------

    enum : uint8_t {
        AFDID_TIMELINE        = 0x04,  //!< Timeline descriptor
        AFDID_LOCATION        = 0x05,  //!< Location descriptor
        AFDID_BASEURL         = 0x06,  //!< BaseURL descriptor
        AFDID_CETS_BRANGE     = 0x07,  //!< Cets_byte_range_descriptor
        AFDID_3DA_EXTSTREAM   = 0x08,  //!< AF_MPEG-H_3dAudio_extStreamID_descriptor
        AFDID_3DA_MULTISTREAM = 0x09,  //!< AF_MPEG-H_3dAudio_multi-stream_descriptor
        AFDID_3DA_COMMAND     = 0x0A,  //!< AF_MPEG-H_3dAudio_command_descriptor
        AFDID_BOUNDARY        = 0x0B,  //!< Boundary Descriptor
        AFDID_LABELING        = 0x0C,  //!< Labeling Descriptor
        AFDID_HEVC_TILE       = 0x0D,  //!< HEVC_tile_substream_af_descriptor
    };
}
