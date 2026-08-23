//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtsduck mpeg
//!  Common definitions for MPEG Transport Stream layer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsBitRate.h"
#include "tsNames.h"

namespace ts {

    //---------------------------------------------------------------------
    // Transport stream packet and header constants.
    //---------------------------------------------------------------------

    //!
    //! PID value (13 bits).
    //!
    using PID = uint16_t;

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
    using PacketCounter = uint64_t;

    //!
    //! A impossible value for PacketCounter, meaning "undefined".
    //!
    constexpr PacketCounter INVALID_PACKET_COUNTER = std::numeric_limits<PacketCounter>::max();

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
    using PIDSet = std::bitset<PID_MAX>;

    //!
    //! PIDSet constant with no PID set.
    //! @return A constant reference to the PIDSet constant.
    //!
    TSDUCKDLL const PIDSet& NoPID();

    //!
    //! PIDSet constant with all PID's set.
    //! @return A constant reference to the PIDSet constant.
    //!
    TSDUCKDLL const PIDSet& AllPIDs();

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
    //! Compute the interval, in duration, between two bytes in the transport stream.
    //! @tparam DURATION An instance of std::chrono::duration (by default, milliseconds).
    //! @param [in] bitrate TS bitrate in bits/second, based on 188-byte packets.
    //! @param [in] distance Distance between the two bytes.
    //! @return Interval in DURATION units between the two bytes.
    //!
    template <class DURATION = cn::milliseconds>
        requires std::integral<typename DURATION::rep>
    inline DURATION ByteInterval(const BitRate& bitrate, std::intmax_t distance)
    {
        return DURATION(typename DURATION::rep(bitrate == 0 ? 0 : ((distance * 8 * DURATION::period::den) / (DURATION::period::num * bitrate)).toInt()));
    }

    //!
    //! Compute the interval, in duration, between two packets.
    //! @tparam DURATION An instance of std::chrono::duration (by default, milliseconds).
    //! @param [in] bitrate TS bitrate in bits/second, based on 188-byte packets.
    //! @param [in] distance Distance between the two packets: 0 for the same
    //! packet, 1 for the next packet, etc.
    //! @return Interval in DURATION units between the first byte of the first packet
    //! and the first byte of the second packet.
    //!
    template <class DURATION = cn::milliseconds>
        requires std::integral<typename DURATION::rep>
    inline DURATION PacketInterval(const BitRate& bitrate, PacketCounter distance)
    {
        return ByteInterval<DURATION>(bitrate, distance * PKT_SIZE);
    }

    //!
    //! Compute the number of "data structures" (bytes, packets, etc) transmitted during a given duration.
    //! @param [in] bits Number of bits in the data structure.
    //! @param [in] bitrate TS bitrate in bits/second, based on 188-byte packets.
    //! @param [in] duration A duration in any std::chrono::duration units.
    //! @return Number of data structures of size @a bits bits during @a duration.
    //!
    template <class Rep, class Period>
    inline int64_t BitDistance(size_t bits, const BitRate& bitrate, const cn::duration<Rep, Period>& duration)
    {
        return int64_t(((bitrate * Period::num * (duration.count() >= 0 ? duration.count() : -duration.count())) / (Period::den * bits)).toInt());
    }

    //!
    //! Compute the number of bytes transmitted during a given duration.
    //! @param [in] bitrate TS bitrate in bits/second, based on 188-byte packets.
    //! @param [in] duration A duration in any std::chrono::duration units.
    //! @return Number of bytes during @a duration.
    //!
    template <class Rep, class Period>
    inline int64_t ByteDistance(const BitRate& bitrate, const cn::duration<Rep, Period>& duration)
    {
        return BitDistance(8, bitrate, duration);
    }

    //!
    //! Compute the number of packets transmitted during a given duration.
    //! @param [in] bitrate TS bitrate in bits/second, based on 188-byte packets.
    //! @param [in] duration A duration in any std::chrono::duration units.
    //! @return Number of packets during @a duration.
    //!
    template <class Rep, class Period>
    inline PacketCounter PacketDistance(const BitRate& bitrate, const cn::duration<Rep, Period>& duration)
    {
        return PacketCounter(BitDistance(PKT_SIZE_BITS, bitrate, duration));
    }

    //!
    //! Compute the bitrate from a number of bytes transmitted during a given duration.
    //! @param [in] bytes Number of bytes during @a duration.
    //! @param [in] duration A duration in any std::chrono::duration units.
    //! @return TS bitrate in bits/second, based on 188-byte packets.
    //!
    template <class Rep, class Period>
    inline BitRate BytesBitRate(uint64_t bytes, const cn::duration<Rep, Period>& duration)
    {
        return duration.count() == 0 ? 0 : BitRate(bytes * 8 * Period::den) / (Period::num * BitRate(duration.count()));
    }

    //!
    //! Compute the bitrate from a number of packets transmitted during a given duration.
    //! @param [in] packets Number of packets during @a duration.
    //! @param [in] duration A duration in any std::chrono::duration units.
    //! @return TS bitrate in bits/second, based on 188-byte packets.
    //!
    template <class Rep, class Period>
    inline BitRate PacketBitRate(PacketCounter packets, const cn::duration<Rep, Period>& duration)
    {
        return BytesBitRate(packets * PKT_SIZE, duration);
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
        PID_IIP        = 0x1FF0, //!< PID for ISDB-T Information Packet (IIP)

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
        PCR_ONLY,   //!< PCR of a service, not otherwise used as video or audio.
        STUFFING,   //!< Null packets.
    };

    //!
    //! Enumeration description of ts::PIDClass with meaningful names.
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& PIDClassEnum();

    //!
    //! Enumeration description of ts::PIDClass with simple lowercase identifiers.
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& PIDClassIdentifier();

    //---------------------------------------------------------------------
    // Transport stream time and clocks.
    //---------------------------------------------------------------------

    //!
    //! Sources of time information for transport streams.
    //!
    enum class TimeSource: uint8_t {
        UNDEFINED = 0,  //!< Undefined source of time information.
        HARDWARE,       //!< Hardware-generated time, any local hardware (NIC for instance).
        KERNEL,         //!< OS kernel time stamp.
        TSP,            //!< Application time stamp, generated by tsp when the chunk of TS packets is received.
        RTP,            //!< RTP (Real Time Protocol) time stamp.
        SRT,            //!< SRT (Secure Reliable Transport) source time.
        M2TS,           //!< M2TS Bluray-style time stamp.
        PCR,            //!< PCR (Program Clock Reference), the transport stream system clock.
        OPCR,           //!< Original PCR (Program Clock Reference).
        DTS,            //!< DTS (Decoding Time Stamp), in a video or audio stream.
        PTS,            //!< PTS (Presentation Time Stamp), in a video or audio stream.
        PCAP,           //!< Timestamp from a pcap or pcap-ng file.
        RIST,           //!< RIST (Reliable Internet Stream Transport) source time.
    };

    //!
    //! Enumeration description of ts::TimeSource.
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& TimeSourceEnum();

    //!
    //! Check if a ts::TimeSource value is a monotonic clock.
    //! A monotonic clock always increases and never wraps up.
    //! @param [in] source The time source to check.
    //! @return True if @a source is a monotonic clock.
    //!
    TSDUCKDLL bool MonotonicTimeSource(TimeSource source);

    //!
    //! Traits superclass for TimeSource values.
    //! There is no generic definition, there are only specializations.
    //!
    template <TimeSource T, typename = void> class TimeSourceSuperTraits;

    //!
    //! TimeSourceSuperTraits specialization for PTS and DTS.
    //!
    template <TimeSource T>
    class TimeSourceSuperTraits<T, typename std::enable_if<T == TimeSource::PTS || T == TimeSource::DTS>::type>
    {
    public:
        //!
        //! Clock frequency in Hz for PTS and DTS.
        //!
        static constexpr uint64_t TICKS = 90'000;
        //!
        //! Size in bits of PTS and DTS.
        //!
        static constexpr size_t BIT_SIZE = 33;
        //!
        //! Scale factor for PTS and DTS (modular time source).
        //!
        static constexpr uint64_t SCALE = 1LL << BIT_SIZE;
        //!
        //! Bit mask for PTS and DTS values (wrap up at 2^33).
        //! The BIT_MASK value exists only for strictly binary time source which wrap up at 2^N.
        //!
        static constexpr uint64_t BIT_MASK = SCALE - 1;
    };

    //!
    //! TimeSourceSuperTraits specialization for PCR and OPCR.
    //! There is no BIT_MASK.
    //!
    template <TimeSource T>
    class TimeSourceSuperTraits<T, typename std::enable_if<T == TimeSource::PCR || T == TimeSource::OPCR>::type>
    {
    public:
        //!
        //! Clock frequency in Hz for PCR.
        //!
        static constexpr uint64_t TICKS = 27'000'000;
        //!
        //! Size in bits of the time source.
        //! Warning: A PCR value is not a linear value mod 2^42.
        //! It is split into PCR_base and PCR_ext (see ISO 13818-1, 2.4.2.2).
        //!
        static constexpr size_t BIT_SIZE = 42;
        //!
        //! Scale factor for the modular time source.
        //! For PCR, this is not a power of 2, it does not wrap up at a number of bits.
        //! The PCR_base part is equivalent to a PTS/DTS and wraps up at 2**33.
        //! The PCR_ext part is a mod 300 value. Note that, since this not a power of 2, there is no possible PCR_MASK value.
        //!
        static constexpr uint64_t SCALE = TimeSourceSuperTraits<TimeSource::PTS>::SCALE * (TICKS / TimeSourceSuperTraits<TimeSource::PTS>::TICKS);
    };

    //!
    //! Traits class for TimeSource values.
    //! The traits class is generic, based on a specialization of TimeSourceSuperTraits.
    //!
    template <TimeSource T>
    class TimeSourceTraits
    {
    public:
        //!
        //! Redefine the time source type as a local declaration.
        //!
        static constexpr TimeSource TYPE = T;
        //!
        //! Clock frequency in Hz for the time source.
        //!
        static constexpr uint64_t TICKS = TimeSourceSuperTraits<T>::TICKS;
        //!
        //! Size in bits of the time source.
        //!
        static constexpr size_t BIT_SIZE = TimeSourceSuperTraits<T>::BIT_SIZE;
        //!
        //! Scale factor for the modular time source.
        //!
        static constexpr uint64_t SCALE = TimeSourceSuperTraits<T>::SCALE;
        //!
        //! Definition of the time source as a std::chrono::duration type.
        //!
        using Duration = cn::duration<std::intmax_t, std::ratio<1, TICKS>>;
        //!
        //! The maximum possible value for the time source.
        //!
        static constexpr uint64_t MAX = SCALE - 1;
        //!
        //! An invalid PTS and DTS value, can be used as a marker.
        //!
        static constexpr uint64_t INVALID = 0xFFFFFFFFFFFFFFFF;
        //!
        //! Number of required hexadecimal digits to represent the time source.
        //!
        static constexpr size_t HEX_DIGITS = (BIT_SIZE + 3) / 4;
        //!
        //! Minimum distance between two time stamp values to consider them as wrapping up after the maximum value.
        //! Because time sources are modular values, they wrap up after MAX. It is common to consider, for instance,
        //! that value 4 "follows" MAX-4, even though 4 < MAX-4. The exact criteria is that two time stamp values
        //! are considered as following each other if their distance is within 20% of a full time source range.
        //!
        static constexpr uint64_t WRAPUP_THRESHOLD = (4 * SCALE) / 5;

        //!
        //! Check if time stamps @a t2 follows time @a t1 after wrap up.
        //! @param [in] t1 First time value.
        //! @param [in] t2 Second time value.
        //! @return True if @a t2 is probably following @a t1 after wrapping up. The exact criteria is that
        //! @a t2 wraps up after @a t1 and their distance is within 20% of a full time source range.
        //!
        static bool WrapUp(uint64_t t1, uint64_t t2)
        {
            return t2 < t1 && (t1 - t2) > WRAPUP_THRESHOLD;
        }

        //!
        //! Check if time stamps @a t1 and @a t2 follow each other, in any order, after wrap up.
        //! @param [in] t1 First time value.
        //! @param [in] t2 Second time value.
        //! @return True if @a t1 and @a t2 follow each other, in any order, after wrap up.
        //!
        static bool Wrap(uint64_t t1, uint64_t t2)
        {
            return (t1 > t2 ? t1 - t2 : t2 - t1) > WRAPUP_THRESHOLD;
        }

        //!
        //! Check if two time stamps are in sequence.
        //! Sample application: In MPEG video, B-frames are transported out-of-sequence. Their PTS is typically lower than the previous D-frame
        //! or I-frame in the transport. A "sequenced" PTS is one that is higher than the previous sequenced PTS (with possible wrap up).
        //! @param [in] t1 First time value.
        //! @param [in] t2 Second time value.
        //! @return True if @a t1 and @a t2 follow each other, possibly after wrapping down after max.
        //!
        static bool Sequenced(uint64_t t1, uint64_t t2)
        {
            return t1 > t2 ? t1 - t2 > WRAPUP_THRESHOLD : t2 - t1 < SCALE - WRAPUP_THRESHOLD;
        }

        //!
        //! Compute the time stamp of a packet, based on the equivalent time stamp of a previous packet.
        //! @param [in] last Time stamp in a previous packet.
        //! @param [in] distance Number of TS packets since the packet with @a last.
        //! @param [in] bitrate Constant bitrate of the stream in bits per second.
        //! @return The time stamp of the packet which is at the specified @a distance from the packet with @a last
        //! or INVALID if a parameter is incorrect.
        //!
        static uint64_t Next(uint64_t last, PacketCounter distance, const BitRate& bitrate);

        //!
        //! Add a signed offset to a time stamp.
        //! @param [in] t Initial time value.
        //! @param [in] offset Signed offset.
        //! @return The adjusted time value or INVALID if a parameter is incorrect.
        //!
        static uint64_t Add(uint64_t t, int64_t offset);

        //!
        //! Compute the difference between two time stamp values (t1 - t2).
        //! @param [in] t1 First time value.
        //! @param [in] t2 Second time value.
        //! @return The difference between the two values or INVALID if a parameter is incorrect.
        //!
        static int64_t Diff(uint64_t t1, uint64_t t2);

        //!
        //! Convert a time stamp value to a string.
        //! @param [in] t The time value.
        //! @param [in] hexa If true (the defaul), include hexadecimal value.
        //! @param [in] decimal If true (the defaul), include decimal value.
        //! @param [in] ms If true (the defaul), include the equivalent duration in milliseconds.
        //! @return The formatted string.
        //!
        static UString ToString(uint64_t t, bool hexa = true, bool decimal = true, bool ms = true);
    };

    // Convenience renammings.

    using PCRTraits = TimeSourceTraits<TimeSource::PCR>;  //!< PCR traits class.
    using PTSTraits = TimeSourceTraits<TimeSource::PTS>;  //!< PTS traits class.
    using DTSTraits = TimeSourceTraits<TimeSource::DTS>;  //!< DTS traits class.

    using PCR = PCRTraits::Duration;  //!< PCR units as a std::chrono::duration type.
    using PTS = PTSTraits::Duration;  //!< PTS units as a std::chrono::duration type.
    using DTS = DTSTraits::Duration;  //!< DTS units as a std::chrono::duration type.

    constexpr uint64_t INVALID_PCR = PCRTraits::INVALID;  //!< An invalid PCR value, can be used as a marker.
    constexpr uint64_t INVALID_PTS = PTSTraits::INVALID;  //!< An invalid PTS value, can be used as a marker.
    constexpr uint64_t INVALID_DTS = DTSTraits::INVALID;  //!< An invalid DTS value, can be used as a marker.

    //!
    //! MPEG-2 System Clock frequency in Hz, used by PCR (27 Mb/s).
    //!
    constexpr uint64_t SYSTEM_CLOCK_FREQ = PCRTraits::TICKS;

    //!
    //! MPEG-2 System Clock subfrequency in Hz, used by PTS and DTS (90 Kb/s).
    //!
    constexpr uint64_t SYSTEM_CLOCK_SUBFREQ = PTSTraits::TICKS;

    //!
    //! Subfactor of MPEG-2 System Clock subfrequency, used by PTS and DTS.
    //!
    constexpr uint64_t SYSTEM_CLOCK_SUBFACTOR = PCRTraits::TICKS / PTSTraits::TICKS;

    //!
    //! Mask for PTS and DTS values (wrap up at 2^33).
    //!
    constexpr uint64_t PTS_DTS_MASK = TimeSourceSuperTraits<TimeSource::PTS>::BIT_MASK;

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

    //---------------------------------------------------------------------
    // Transport stream packet processing.
    //---------------------------------------------------------------------

    //!
    //! Status of a packet processing.
    //! Typically returned by a packet processing plugin after processing one packet.
    //!
    enum PacketProcessStatus {
        TSP_OK   = 0,  //!< OK, pass packet to next processor or output.
        TSP_END  = 1,  //!< End of processing, tell everybody to terminate.
        TSP_DROP = 2,  //!< Drop this packet.
        TSP_NULL = 3   //!< Replace this packet with a null packet.
    };

    //!
    //! Names of packet procesing status values.
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& PacketProcessingStatusNames();
}

//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Compute the time stamp of a packet, based on the equivalent time stamp of a previous packet.
template <ts::TimeSource T>
uint64_t ts::TimeSourceTraits<T>::Next(uint64_t last, PacketCounter distance, const BitRate& bitrate)
{
    return last == INVALID || bitrate == 0 ? INVALID : (last + (BitRate(distance * PKT_SIZE_BITS * TICKS) / bitrate).toInt()) % SCALE;
}

// Add a signed offset to a time stamp.
template <ts::TimeSource T>
uint64_t ts::TimeSourceTraits<T>::Add(uint64_t t, int64_t offset)
{
    if (t > MAX) {
        return INVALID;
    }
    else {
        // Beware of signed / unsigned conversions.
        // If the final value is negative, the '%' operation differs on the signedness of the modulus type.
        // - If the modulus is unsigned (as SCALE is), the negative lhs is first converted to unsigned and the result is absurd.
        // - If the modulus is signed, the result is correct but also negative and must be adjusted.
        // So, let's compute everything in signed form and adjust negative results.
        const int64_t res = (int64_t(t) + offset) % int64_t(SCALE);
        return res < 0 ? uint64_t(int64_t(SCALE) + res) : uint64_t(res);
    }
}

// Compute the difference between two time stamp values (t1 - t2).
template <ts::TimeSource T>
int64_t ts::TimeSourceTraits<T>::Diff(uint64_t t1, uint64_t t2)
{
    if (!Wrap(t1, t2)) {
        return int64_t(t1) - int64_t(t2);
    }
    else if (t1 <= t2) {
        return int64_t(SCALE + t1) - int64_t(t2);
    }
    else {
        return int64_t(t1) - int64_t(SCALE + t2);
    }
}

// Convert a time stamp value to a string.
template <ts::TimeSource T>
ts::UString ts::TimeSourceTraits<T>::ToString(uint64_t value, bool hexa, bool decimal, bool ms)
{
    int count = 0;
    UString str;
    if (hexa) {
        str.format(u"0x%0*X", HEX_DIGITS, value);
        count++;
    }
    if (decimal && (value != 0 || count == 0)) {
        if (count == 1) {
            str.append(u" (");
        }
        str.format(u"%'d", value);
        count++;
    }
    if (ms && (value != 0 || count == 0)) {
        if (count == 1) {
            str.append(u" (");
        }
        else if (count > 1) {
            str.append(u", ");
        }
        str.format(u"%'d ms", value / (TICKS / 1000));
        count++;
    }
    if (count > 1) {
        str.append(u')');
    }
    return str;
}
