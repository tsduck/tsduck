//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Common definition for MPEG level
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    // Base types

    typedef uint16_t PID;  // PID value
    typedef uint8_t  TID;  // Table identifier
    typedef uint8_t  DID;  // Descriptor identifier
    typedef uint32_t PDS;  // Private data specifier

    // MPEG TS packet size

    const size_t PKT_SIZE = 188;

    // TS packet with trailing Reed-Solomon outer FEC

    const size_t RS_SIZE = 16; // Size of Reed-Solomon outer FEC
    const size_t PKT_RS_SIZE = PKT_SIZE + RS_SIZE;

    // MPEG packet size in M2TS files (Blu-ray disc): use a leading 4-byte timestamp

    const size_t M2TS_HEADER_SIZE = 4;  // 4-byte timestamp before TS packet
    const size_t PKT_M2TS_SIZE = M2TS_HEADER_SIZE + PKT_SIZE;

    // A bitrate is specified in bits/second.

    typedef uint32_t BitRate;

    // Transport Stream packets are counted using 64-bit integers.
    // Thus, PacketCounter will never overflow: at 100 Mb/s, 2^64 188-byte
    // packets will take 8.7 million years to transmit. No process will
    // ever run that long. On the contrary, using 32-bit integer would
    // be insufficient: at 100 Mb/s, 2^32 188-byte packets will take
    // only 17 hours to transmit.

    typedef uint64_t PacketCounter;
    typedef uint64_t SectionCounter;

    // Convert 188-byte packet bitrate into 204-byte packet bitrate and vice-versa.

    TSDUCKDLL inline BitRate ToBitrate204 (BitRate bitrate188)
    {
        return BitRate ((uint64_t (bitrate188) * 204L) / 188L);
    }
    TSDUCKDLL inline BitRate ToBitrate188 (BitRate bitrate204)
    {
        return BitRate ((uint64_t (bitrate204) * 188L) / 204L);
    }

    // Compute the interval, in milliseconds, between two packets.

    TSDUCKDLL inline MilliSecond PacketInterval (BitRate bitrate, PacketCounter distance = 1)
    {
        return bitrate == 0 ? 0 : (distance * 8 * PKT_SIZE * MilliSecPerSec) / MilliSecond (bitrate);
    }

    // Compute the number of packets transmitted during a given duration in milliseconds

    TSDUCKDLL inline PacketCounter PacketDistance (BitRate bitrate, MilliSecond duration)
    {
        return (PacketCounter (bitrate) * (duration >= 0 ? duration : -duration)) / (MilliSecPerSec * 8 * PKT_SIZE);
    }

    // Compute the minimum number of TS packets required to transport a section.

    TSDUCKDLL inline PacketCounter SectionPacketCount (size_t section_size)
    {
        // The required size for a section is section_size + 1 (1 for pointer_field
        // in first packet). In each packet, the useable size is 184 bytes.
        return PacketCounter ((section_size + 184) / 184);
    }

    // Value of sync byte (first in TS packet)

    const uint8_t SYNC_BYTE = 0x47;

    // PES packet start code prefix (24 bits)

    const uint32_t PES_START = 0x000001;

    // PID field

    const size_t PID_BITS = 13;        // Size (in bits) of PID field
    const PID PID_MAX = 1 << PID_BITS; // Max number of PID's (=8192)

    // A bit mask for PID values. Useful to implement PID filtering.

    typedef std::bitset <PID_MAX> PIDSet;

    // These constants respectively contains no PID and all PID's.

    TSDUCKDLL extern const PIDSet NoPID;
    TSDUCKDLL extern const PIDSet AllPIDs;

    // Continuity counter (CC) value wraps at 16

    const size_t  CC_BITS = 4;    // Size (in bits) of CC field
    const uint8_t CC_MASK = 0x0F;
    const uint8_t CC_MAX  = 1 << CC_BITS;

    // Section version number value wraps at 32

    const size_t  SVERSION_BITS = 5;    // Size (in bits) of version field
    const uint8_t SVERSION_MASK = 0x1F;
    const uint8_t SVERSION_MAX  = 1 << SVERSION_BITS;

    // DVB Common Scrambling (DVB-CS) Control Word (CW)
    // All control words are 64-bit long.

    const size_t CW_BITS = 64;
    const size_t CW_BYTES = CW_BITS / 8;

    // Scrambling_control values (used in TS and PES packets headers)

    enum {
        SC_CLEAR        = 0,  // Not scrambled (MPEG-defined)
        SC_DVB_RESERVED = 1,  // Reserved for future use by DVB
        SC_EVEN_KEY     = 2,  // Scrambled with even key (DVB-defined)
        SC_ODD_KEY      = 3   // Scrambled with odd key (DVB-defined)
    };

    // Encoding of Modified Julian Dates (MJD).
    // The origin of MJD is 17 Nov 1858 00:00:00.
    // The UNIX epoch (1 Jan 1970) is 40587 days from julian time origin

    const uint32_t MJD_EPOCH = 40587;

    // Video macroblock size. Valid for:
    // - ISO 11172-2 (MPEG-1 video)
    // - ISO 13818-2 (MPEG-2 video)
    // - ISO 14496-10 (MPEG-4 Advanced Video Coding, AVC, ITU H.264)

    const size_t MACROBLOCK_WIDTH  = 16;
    const size_t MACROBLOCK_HEIGHT = 16;

    //---------------------------------------------------------------------
    // Predefined PID values
    //---------------------------------------------------------------------

    enum {

        // Valid in all MPEG contexts:

        PID_PAT     = 0x0000, // Program Association Table PAT
        PID_CAT     = 0x0001, // Conditional Access Table
        PID_TSDT    = 0x0002, // Transport Stream Description Table
        PID_NULL    = 0x1FFF, // Null packets (stuffing)

        // Valid in DVB context:

        PID_NIT     = 0x0010, // Network Information Table
        PID_SDT     = 0x0011, // Service Description Table
        PID_BAT     = 0x0011, // Bouquet Association Table
        PID_EIT     = 0x0012, // Event Information Table
        PID_RST     = 0x0013, // Running Status Table
        PID_TDT     = 0x0014, // Time & Date Table
        PID_TOT     = 0x0014, // Time Offset Table
        PID_NETSYNC = 0x0015, // Network synchronization
        PID_RNT     = 0x0016, // TV-Anytime
        PID_INBSIGN = 0x001C, // Inband Signalling
        PID_MEASURE = 0x001D, // Measurement
        PID_DIT     = 0x001E, // Discontinuity Information Table
        PID_SIT     = 0x001F  // Selection Information Table
    };

    //---------------------------------------------------------------------
    // MPEG clock representation:
    // - PCR (Program Clock Reference)
    // - PTS (Presentation Time Stamp)
    // - DTS (Decoding Time Stamp)
    //---------------------------------------------------------------------

    // MPEG-2 System Clock frequency, used by PCR (27 Mb/s)

    const uint32_t SYSTEM_CLOCK_FREQ = 27000000;

    // MPEG-2 System Clock subfrequency, used by PTS and DTS

    const uint32_t SYSTEM_CLOCK_SUBFACTOR = 300;
    const uint32_t SYSTEM_CLOCK_SUBFREQ = SYSTEM_CLOCK_FREQ / SYSTEM_CLOCK_SUBFACTOR;

    // PTS and DTS wrap up at 2**33.

    const uint64_t PTS_DTS_MASK  = TS_UCONST64 (0x00000001FFFFFFFF);
    const uint64_t PTS_DTS_SCALE = TS_UCONST64 (0x0000000200000000);

    // Check if PTS2 follows PTS1 after wrap up.

    TSDUCKDLL inline bool WrapUpPTS (uint64_t pts1, uint64_t pts2)
    {
        return pts2 < pts1 && (pts1 - pts2) > TS_UCONST64 (0x00000001F0000000);
    }

    // In MPEG video, B-frames are transported out-of-sequence.
    // Their PTS is typically lower than the previous D-frame or I-frame
    // in the transport. A "sequenced" PTS is one that is higher than
    // the previous sequenced PTS (with possible wrap up).

    TSDUCKDLL inline bool SequencedPTS (uint64_t pts1, uint64_t pts2)
    {
        return pts1 <= pts2 || WrapUpPTS (pts1, pts2);
    }

    //---------------------------------------------------------------------
    // Stream id values, as used in PES header
    //---------------------------------------------------------------------

    enum {
        SID_PSMAP      = 0xBC, // Program stream map
        SID_PRIV1      = 0xBD, // Private stream 1
        SID_PAD        = 0xBE, // Padding stream
        SID_PRIV2      = 0xBF, // Private stream 2
        SID_AUDIO      = 0xC0, // Audio stream, with number
        SID_AUDIO_MASK = 0x1F, // Mask to get audio stream number
        SID_VIDEO      = 0xE0, // Video stream, with number
        SID_VIDEO_MASK = 0x0F, // Mask to get video stream number
        SID_ECM        = 0xF0, // ECM stream
        SID_EMM        = 0xF1, // EMM stream
        SID_DSMCC      = 0xF2, // DSM-CC data
        SID_ISO13522   = 0xF3, // ISO 13522 (hypermedia)
        SID_H222_1_A   = 0xF4, // H.222.1 type A
        SID_H222_1_B   = 0xF5, // H.222.1 type B
        SID_H222_1_C   = 0xF6, // H.222.1 type C
        SID_H222_1_D   = 0xF7, // H.222.1 type D
        SID_H222_1_E   = 0xF8, // H.222.1 type E
        SID_ANCILLARY  = 0xF9, // Ancillary stream
        SID_MP4_SLPACK = 0xFA, // MPEG-4 SL-packetized stream
        SID_MP4_FLEXM  = 0xFB, // MPEG-4 FlexMux stream
        SID_METADATA   = 0xFC, // MPEG-7 metadata stream
        SID_EXTENDED   = 0xFD, // Extended stream id
        SID_RESERVED   = 0xFE, // Reserved value
        SID_PSDIR      = 0xFF, // Program stream directory
        SID_MAX        = 0x100 // Max number of stream ids
    };

    // Check if a SID value indicates a video stream
    TSDUCKDLL inline bool IsVideoSID (uint8_t sid)
    {
        return (sid & ~SID_VIDEO_MASK) == SID_VIDEO;
    }

    // Check if a SID value indicates an audio stream
    TSDUCKDLL inline bool IsAudioSID (uint8_t sid)
    {
        return (sid & ~SID_AUDIO_MASK) == SID_AUDIO;
    }

    // Check if a SID value indicates a PES packet with long header
    TSDUCKDLL bool IsLongHeaderSID (uint8_t sid);

    //---------------------------------------------------------------------
    // PES start code values
    //---------------------------------------------------------------------

    enum {
        PST_PICTURE         = 0x00,
        PST_SLICE_MIN       = 0x01,
        PST_SLICE_MAX       = 0xAF,
        PST_RESERVED_B0     = 0xB0,
        PST_RESERVED_B1     = 0xB1,
        PST_USER_DATA       = 0xB2,
        PST_SEQUENCE_HEADER = 0xB3,
        PST_SEQUENCE_ERROR  = 0xB4,
        PST_EXTENSION       = 0xB5,
        PST_RESERVED_B6     = 0xB6,
        PST_SEQUENCE_END    = 0xB7,
        PST_GROUP           = 0xB8,
        PST_SYSTEM_MIN      = 0xB9,  // Stream id values (SID_*)
        PST_SYSTEM_MAX      = 0xFF,
    };

    //---------------------------------------------------------------------
    // Aspect ratio values (in MPEG-1/2 video sequence header)
    //---------------------------------------------------------------------

    enum {
        AR_SQUARE = 1,  // 1/1
        AR_4_3    = 2,  // 4/3
        AR_16_9   = 3,  // 16/9
        AR_221    = 4,  // 2.21/1
    };

    //---------------------------------------------------------------------
    // Chroma format values (in MPEG-1/2 video sequence header)
    //---------------------------------------------------------------------

    enum {
        CHROMA_MONO = 0,  // Monochrome
        CHROMA_420  = 1,  // 4:2:0
        CHROMA_422  = 2,  // 4:2:2
        CHROMA_444  = 3,  // 4:4:4
    };

    //---------------------------------------------------------------------
    // AVC access unit types
    //---------------------------------------------------------------------

    enum {
        AVC_AUT_NON_IDR      =  1, // Coded slice of a non-IDR picture
        AVC_AUT_SLICE_A      =  2, // Coded slice data partition A
        AVC_AUT_SLICE_B      =  3, // Coded slice data partition B
        AVC_AUT_SLICE_C      =  4, // Coded slice data partition C
        AVC_AUT_IDR          =  5, // Coded slice of an IDR picture
        AVC_AUT_SEI          =  6, // Supplemental enhancement information (SEI)
        AVC_AUT_SEQPARAMS    =  7, // Sequence parameter set
        AVC_AUT_PICPARAMS    =  8, // Picture parameter set
        AVC_AUT_DELIMITER    =  9, // Access unit delimiter
        AVC_AUT_END_SEQUENCE = 10, // End of sequence
        AVC_AUT_END_STREAM   = 11, // End of stream
        AVC_AUT_FILLER       = 12, // Filler data
        AVC_AUT_SEQPARAMSEXT = 13, // Sequence parameter set extension
        AVC_AUT_PREFIX       = 14, // Prefix NAL unit in scalable extension
        AVC_AUT_SUBSETPARAMS = 15, // Subset sequence parameter set
        AVC_AUT_SLICE_NOPART = 19, // Coded slice without partitioning
        AVC_AUT_SLICE_SCALE  = 20, // Coded slice in scalable extension
    };

    //---------------------------------------------------------------------
    // Stream type values, as used in the PMT
    //---------------------------------------------------------------------

    enum {
        ST_MPEG1_VIDEO = 0x01, // MPEG-1 Video
        ST_MPEG2_VIDEO = 0x02, // MPEG-2 Video
        ST_MPEG1_AUDIO = 0x03, // MPEG-1 Audio
        ST_MPEG2_AUDIO = 0x04, // MPEG-2 Audio
        ST_PRIV_SECT   = 0x05, // MPEG-2 Private sections
        ST_PES_PRIV    = 0x06, // MPEG-2 PES private data
        ST_MHEG        = 0x07, // MHEG
        ST_DSMCC       = 0x08, // DSM-CC
        ST_MPEG2_ATM   = 0x09, // MPEG-2 over ATM
        ST_DSMCC_MPE   = 0x0A, // DSM-CC Multi-Protocol Encapsulation
        ST_DSMCC_UN    = 0x0B, // DSM-CC User-to-Network messages
        ST_DSMCC_SD    = 0x0C, // DSM-CC Stream Descriptors
        ST_DSMCC_SECT  = 0x0D, // DSM-CC Sections
        ST_MPEG2_AUX   = 0x0E, // MPEG-2 Auxiliary
        ST_AAC_AUDIO   = 0x0F, // Advanced Audio Coding (ISO 13818-7)
        ST_MPEG4_VIDEO = 0x10, // MPEG-4 Video
        ST_MPEG4_AUDIO = 0x11, // MPEG-4 Audio
        ST_MPEG4_PES   = 0x12, // MPEG-4 SL or FlexMux in PES packets
        ST_MPEG4_SECT  = 0x13, // MPEG-4 SL or FlexMux in sections
        ST_DSMCC_DLOAD = 0x14, // DSM-CC Synchronized Download Protocol
        ST_MDATA_PES   = 0x15, // MPEG-7 MetaData in PES packets
        ST_MDATA_SECT  = 0x16, // MPEG-7 MetaData in sections
        ST_MDATA_DC    = 0x17, // MPEG-7 MetaData in DSM-CC Data Carousel
        ST_MDATA_OC    = 0x18, // MPEG-7 MetaData in DSM-CC Object Carousel
        ST_MDATA_DLOAD = 0x19, // MPEG-7 MetaData in DSM-CC Sync Downl Proto
        ST_MPEG2_IPMP  = 0x1A, // MPEG-2 IPMP stream
        ST_AVC_VIDEO   = 0x1B, // AVC video
        ST_IPMP        = 0x7F, // IPMP stream
        ST_AC3_AUDIO   = 0x81, // AC-3 Audio (ATSC only)
        ST_EAC3_AUDIO  = 0x87, // Enhanced-AC-3 Audio (ATSC only)
    };

    // Check if an ST value indicates a PES stream
    TSDUCKDLL bool IsPES (uint8_t st);

    // Check if an ST value indicates a video stream
    TSDUCKDLL bool IsVideoST (uint8_t st);

    // Check if an ST value indicates an audio stream
    TSDUCKDLL bool IsAudioST (uint8_t st);

    // Check if an ST value indicates a stream carrying sections
    TSDUCKDLL bool IsSectionST (uint8_t st);

    //---------------------------------------------------------------------
    // PSI, SI and data sections and tables
    //---------------------------------------------------------------------

    const size_t MAX_DESCRIPTOR_SIZE = 257; // 255 + 2-byte header

    const size_t SHORT_SECTION_HEADER_SIZE = 3;
    const size_t LONG_SECTION_HEADER_SIZE  = 8;
    const size_t SECTION_CRC32_SIZE        = 4;
    const size_t MAX_PSI_SECTION_SIZE      = 1024;
    const size_t MAX_PRIVATE_SECTION_SIZE  = 4096;

    const size_t MIN_SHORT_SECTION_SIZE = SHORT_SECTION_HEADER_SIZE;
    const size_t MIN_LONG_SECTION_SIZE  = LONG_SECTION_HEADER_SIZE + SECTION_CRC32_SIZE;

    const size_t MAX_PSI_SHORT_SECTION_PAYLOAD_SIZE = MAX_PSI_SECTION_SIZE - SHORT_SECTION_HEADER_SIZE;
    const size_t MAX_PSI_LONG_SECTION_PAYLOAD_SIZE  = MAX_PSI_SECTION_SIZE - LONG_SECTION_HEADER_SIZE - SECTION_CRC32_SIZE;

    const size_t MAX_PRIVATE_SHORT_SECTION_PAYLOAD_SIZE = MAX_PRIVATE_SECTION_SIZE - SHORT_SECTION_HEADER_SIZE;
    const size_t MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE  = MAX_PRIVATE_SECTION_SIZE - LONG_SECTION_HEADER_SIZE - SECTION_CRC32_SIZE;

    //---------------------------------------------------------------------
    // Table identification (TID) values
    //---------------------------------------------------------------------

    enum {

        // Valid in all MPEG contexts:

        TID_PAT           = 0x00, // Program Association Table PAT
        TID_CAT           = 0x01, // Conditional Access Table
        TID_PMT           = 0x02, // Program Map Table
        TID_TSDT          = 0x03, // Transport Stream Description Table
        TID_MP4SDT        = 0x04, // MPEG-4 Scene Description Table
        TID_MP4ODT        = 0x05, // MPEG-4 Object Descriptor Table
        TID_MDT           = 0x06, // MetaData Table
        TID_DSMCC_MPE     = 0x3A, // DSM-CC Multi-Protocol Encapsulated data
        TID_DSMCC_UNM     = 0x3B, // DSM-CC User-to-Network Messages
        TID_DSMCC_DDM     = 0x3C, // DSM-CC Download Data Messages
        TID_DSMCC_SD      = 0x3D, // DSM-CC Stream Descriptors
        TID_DSMCC_PD      = 0x3E, // DSM-CC Private Data
        TID_NULL          = 0xFF, // Reserved, end of TS packet PSI payload

        // Valid in DVB context:

        TID_NIT_ACT       = 0x40, // Network Information Table - Actual netw
        TID_NIT_OTH       = 0x41, // Network Information Table - Other netw
        TID_SDT_ACT       = 0x42, // Service Description Table - Actual TS
        TID_SDT_OTH       = 0x46, // Service Description Table - Other TS
        TID_BAT           = 0x4A, // Bouquet Association Table
        TID_EIT_PF_ACT    = 0x4E, // EIT present/following - Actual network
        TID_EIT_PF_OTH    = 0x4F, // EIT present/following - Other network
        TID_EIT_S_ACT_MIN = 0x50, // EIT schedule - Actual network
        TID_EIT_S_ACT_MAX = 0x5F, // EIT schedule - Actual network
        TID_EIT_S_OTH_MIN = 0x60, // EIT schedule - Other network
        TID_EIT_S_OTH_MAX = 0x6F, // EIT schedule - Other network
        TID_TDT           = 0x70, // Time & Date Table
        TID_RST           = 0x71, // Running Status Table
        TID_ST            = 0x72, // Stuffing Table
        TID_TOT           = 0x73, // Time Offset Table
        TID_RNT           = 0x74, // Resolution Notification T. (TV-Anytime)
        TID_CT            = 0x75, // Container Table (TV-Anytime)
        TID_RCT           = 0x76, // Related Content Table (TV-Anytime)
        TID_CIT           = 0x77, // Content Identifier Table (TV-Anytime)
        TID_MPE_FEC       = 0x78, // MPE-FEC Table (Data Broadcasting)
        TID_DIT           = 0x7E, // Discontinuity Information Table
        TID_SIT           = 0x7F, // Selection Information Table

        TID_ECM_80        = 0x80, // ECM
        TID_ECM_81        = 0x81, // ECM
        TID_EMM_FIRST     = 0x82, // Start of EMM range
        TID_EMM_LAST      = 0x8F, // End of EMM range

        // Ranges by type

        TID_EIT_MIN       = 0x4E, // EIT, first TID
        TID_EIT_MAX       = 0x6F, // EIT, last TID
        TID_CAS_FIRST     = 0x80, // Start of CAS range
        TID_CAS_LAST      = 0x8F, // End of CAS range

        // Valid in SafeAccess CAS context:

        TID_SA_CECM_82    = 0x82, // Complementary ECM
        TID_SA_CECM_83    = 0x83, // Complementary ECM
        TID_SA_EMM_STB_U  = 0x84, // STB or CI-CAM unique EMM
        TID_SA_EMM_STB_G  = 0x85, // STB global EMM
        TID_SA_EMM_A      = 0x86, // Global EMM ("all")
        TID_SA_EMM_U      = 0x87, // Unique EMM
        TID_SA_EMM_S      = 0x88, // Group EMM ("shared")
        TID_SA_EMM_CAM_G  = 0x89, // CI-CAM global EMM
        TID_SA_RECM_8A    = 0x8A, // Record ECM
        TID_SA_RECM_8B    = 0x8B, // Record ECM
        TID_SA_EMM_T      = 0x8F, // Technical EMM

        // Valid in Logiways context:

        TID_LW_DMT        = 0x90, // Download Marker Table
        TID_LW_BDT        = 0x91, // Binary Data Table
        TID_LW_VIT        = 0x92, // VoD Information Table
        TID_LW_VCT        = 0x93, // VoD Command Table

        // Valid in MediaGuard CAS context:
    
        TID_MG_EMM_U      = 0x82,
        TID_MG_EMM_A      = 0x83,
        TID_MG_EMM_I      = 0x85,
        TID_MG_EMM_G      = 0x84,
        TID_MG_EMM_C      = 0x86,
        TID_MG_EMM_CG     = 0x89,

        TID_MAX           = 0x100 // Number of possible TID values
    };

    //---------------------------------------------------------------------
    // Private data specifier (PDS) values
    //---------------------------------------------------------------------

    enum {
        PDS_NAGRA     = 0x00000009,
        PDS_NAGRA_2   = 0x0000000A,
        PDS_NAGRA_3   = 0x0000000B,
        PDS_NAGRA_4   = 0x0000000C,
        PDS_NAGRA_5   = 0x0000000D,
        PDS_TPS       = 0x00000010,
        PDS_EACEM     = 0x00000028,
        PDS_EICTA     = PDS_EACEM,
        PDS_LOGIWAYS  = 0x000000A2,
        PDS_CANALPLUS = 0x000000C0,
        PDS_EUTELSAT  = 0x0000055F,
    };

    //---------------------------------------------------------------------
    // Descriptor tag values (descriptor identification, DID)
    //---------------------------------------------------------------------

    enum {

        // Valid in all MPEG contexts:

        DID_VIDEO               = 0x02, // video_stream_descriptor
        DID_AUDIO               = 0x03, // audio_stream_descriptor
        DID_HIERARCHY           = 0x04, // hierarchy_descriptor
        DID_REGISTRATION        = 0x05, // registration_descriptor
        DID_DATA_ALIGN          = 0x06, // data_stream_alignment_descriptor
        DID_TGT_BG_GRID         = 0x07, // target_background_grid_descriptor
        DID_VIDEO_WIN           = 0x08, // video_window_descriptor
        DID_CA                  = 0x09, // CA_descriptor
        DID_LANGUAGE            = 0x0A, // ISO_639_language_descriptor
        DID_SYS_CLOCK           = 0x0B, // system_clock_descriptor
        DID_MUX_BUF_USE         = 0x0C, // multiplex_buffer_utilization_desc
        DID_COPYRIGHT           = 0x0D, // copyright_descriptor
        DID_MAX_BITRATE         = 0x0E, // maximum bitrate descriptor
        DID_PRIV_DATA_IND       = 0x0F, // private data indicator descriptor
        DID_SMOOTH_BUF          = 0x10, // smoothing buffer descriptor
        DID_STD                 = 0x11, // STD_descriptor
        DID_IBP                 = 0x12, // IBP descriptor
                                        // Values 0x13 to 0x1A defined by DSM-CC
        DID_MPEG4_VIDEO         = 0x1B, // MPEG-4_video_descriptor
        DID_MPEG4_AUDIO         = 0x1C, // MPEG-4_audio_descriptor
        DID_IOD                 = 0x1D, // IOD_descriptor
        DID_SL                  = 0x1E, // SL_descriptor
        DID_FMC                 = 0x1F, // FMC_descriptor
        DID_EXT_ES_ID           = 0x20, // External_ES_id_descriptor
        DID_MUXCODE             = 0x21, // MuxCode_descriptor
        DID_FMX_BUFFER_SIZE     = 0x22, // FmxBufferSize_descriptor
        DID_MUX_BUFFER          = 0x23, // MultiplexBuffer_descriptor
        DID_CONTENT_LABELING    = 0x24, // Content_labeling_descriptor
        DID_METADATA_ASSOC      = 0x25, // Metadata_association_descriptor
        DID_METADATA            = 0x26, // Metadata_descriptor
        DID_METADATA_STD        = 0x27, // Metadata_STD_descriptor
        DID_AVC_VIDEO           = 0x28, // AVC_video_descriptor
        DID_MPEG2_IPMP          = 0x29, // MPEG-2_IPMP_descriptor
        DID_AVC_TIMING_HRD      = 0x2A, // AVC_timing_and_HRD_descriptor

        // Valid in DVB context:

        DID_NETWORK_NAME        = 0x40, // network_name_descriptor
        DID_SERVICE_LIST        = 0x41, // service_list_descriptor
        DID_STUFFING            = 0x42, // stuffing_descriptor
        DID_SAT_DELIVERY        = 0x43, // satellite_delivery_system_desc
        DID_CABLE_DELIVERY      = 0x44, // cable_delivery_system_descriptor
        DID_VBI_DATA            = 0x45, // VBI_data_descriptor
        DID_VBI_TELETEXT        = 0x46, // VBI_teletext_descriptor
        DID_BOUQUET_NAME        = 0x47, // bouquet_name_descriptor
        DID_SERVICE             = 0x48, // service_descriptor
        DID_COUNTRY_AVAIL       = 0x49, // country_availability_descriptor
        DID_LINKAGE             = 0x4A, // linkage_descriptor
        DID_NVOD_REFERENCE      = 0x4B, // NVOD_reference_descriptor
        DID_TIME_SHIFT_SERVICE  = 0x4C, // time_shifted_service_descriptor
        DID_SHORT_EVENT         = 0x4D, // short_event_descriptor
        DID_EXTENDED_EVENT      = 0x4E, // extended_event_descriptor
        DID_TIME_SHIFT_EVENT    = 0x4F, // time_shifted_event_descriptor
        DID_COMPONENT           = 0x50, // component_descriptor
        DID_MOSAIC              = 0x51, // mosaic_descriptor
        DID_STREAM_ID           = 0x52, // stream_identifier_descriptor
        DID_CA_ID               = 0x53, // CA_identifier_descriptor
        DID_CONTENT             = 0x54, // content_descriptor
        DID_PARENTAL_RATING     = 0x55, // parental_rating_descriptor
        DID_TELETEXT            = 0x56, // teletext_descriptor
        DID_TELEPHONE           = 0x57, // telephone_descriptor
        DID_LOCAL_TIME_OFFSET   = 0x58, // local_time_offset_descriptor
        DID_SUBTITLING          = 0x59, // subtitling_descriptor
        DID_TERREST_DELIVERY    = 0x5A, // terrestrial_delivery_system_desc
        DID_MLINGUAL_NETWORK    = 0x5B, // multilingual_network_name_desc
        DID_MLINGUAL_BOUQUET    = 0x5C, // multilingual_bouquet_name_desc
        DID_MLINGUAL_SERVICE    = 0x5D, // multilingual_service_name_desc
        DID_MLINGUAL_COMPONENT  = 0x5E, // multilingual_component_descriptor
        DID_PRIV_DATA_SPECIF    = 0x5F, // private_data_specifier_descriptor
        DID_SERVICE_MOVE        = 0x60, // service_move_descriptor
        DID_SHORT_SMOOTH_BUF    = 0x61, // short_smoothing_buffer_descriptor
        DID_FREQUENCY_LIST      = 0x62, // frequency_list_descriptor
        DID_PARTIAL_TS          = 0x63, // partial_transport_stream_desc
        DID_DATA_BROADCAST      = 0x64, // data_broadcast_descriptor
        DID_SCRAMBLING          = 0x65, // scrambling_descriptor
        DID_DATA_BROADCAST_ID   = 0x66, // data_broadcast_id_descriptor
        DID_TRANSPORT_STREAM    = 0x67, // transport_stream_descriptor
        DID_DSNG                = 0x68, // DSNG_descriptor
        DID_PDC                 = 0x69, // PDC_descriptor
        DID_AC3                 = 0x6A, // AC-3_descriptor
        DID_ANCILLARY_DATA      = 0x6B, // ancillary_data_descriptor
        DID_CELL_LIST           = 0x6C, // cell_list_descriptor
        DID_CELL_FREQ_LINK      = 0x6D, // cell_frequency_link_descriptor
        DID_ANNOUNCE_SUPPORT    = 0x6E, // announcement_support_descriptor
        DID_APPLI_SIGNALLING    = 0x6F, // application_signalling_descriptor
        DID_ADAPTFIELD_DATA     = 0x70, // adaptation_field_data_descriptor
        DID_SERVICE_ID          = 0x71, // service_identifier_descriptor
        DID_SERVICE_AVAIL       = 0x72, // service_availability_descriptor
        DID_DEFAULT_AUTHORITY   = 0x73, // default_authority_descriptor
        DID_RELATED_CONTENT     = 0x74, // related_content_descriptor
        DID_TVA_ID              = 0x75, // TVA_id_descriptor
        DID_CONTENT_ID          = 0x76, // content_identifier_descriptor
        DID_TIME_SLICE_FEC_ID   = 0x77, // time_slice_fec_identifier_desc
        DID_ECM_REPETITION_RATE = 0x78, // ECM_repetition_rate_descriptor
        DID_S2_SAT_DELIVERY     = 0x79, // S2_satellite_delivery_system_descriptor
        DID_ENHANCED_AC3        = 0x7A, // enhanced_AC-3_descriptor
        DID_DTS                 = 0x7B, // DTS_descriptor
        DID_AAC                 = 0x7C, // AAC_descriptor
        DID_XAIT_LOCATION       = 0x7D, // XAIT_location_descriptor (DVB-MHP)
        DID_FTA_CONTENT_MGMT    = 0x7E, // FTA_content_management_descriptor
        DID_EXTENSION           = 0x7F, // extension_descriptor

        // Valid in ATSC context:

        DID_ATSC_STUFFING       = 0X80, // stuffing_descriptor
        DID_AC3_AUDIO_STREAM    = 0x81, // ac3_audio_stream_descriptor
        DID_ATSC_PID            = 0x85, // program_identifier_descriptor
        DID_CAPTION             = 0x86, // caption_service_descriptor
        DID_CONTENT_ADVIS       = 0x87, // content_advisory_descriptor
        DID_EXT_CHAN_NAME       = 0xA0, // extended_channel_name_descriptor
        DID_SERV_LOCATION       = 0xA1, // service_location_descriptor
        DID_ATSC_TIME_SHIFT     = 0xA2, // time_shifted_event_descriptor
        DID_COMPONENT_NAME      = 0xA3, // component_name_descriptor
        DID_ATSC_DATA_BRDCST    = 0xA4, // data_broadcast_descriptor
        DID_PID_COUNT           = 0xA5, // pid_count_descriptor
        DID_DOWNLOAD            = 0xA6, // download_descriptor
        DID_MPROTO_ENCAPS       = 0xA7, // multiprotocol_encapsulation_desc

        // Valid after PDS_LOGIWAYS private_data_specifier

        DID_LW_SUBSCRIPTION      = 0x81, // subscription_descriptor
        DID_LW_SCHEDULE          = 0xB0, // schedule_descriptor
        DID_LW_PRIV_COMPONENT    = 0xB1, // private_component_descriptor
        DID_LW_PRIV_LINKAGE      = 0xB2, // private_linkage_descriptor
        DID_LW_CHAPTER           = 0xB3, // chapter_descriptor
        DID_LW_DRM               = 0xB4, // DRM_descriptor
        DID_LW_VIDEO_SIZE        = 0xB5, // video_size_descriptor
        DID_LW_EPISODE           = 0xB6, // episode_descriptor
        DID_LW_PRICE             = 0xB7, // price_descriptor
        DID_LW_ASSET_REFERENCE   = 0xB8, // asset_reference_descriptor
        DID_LW_CONTENT_CODING    = 0xB9, // content_coding_descriptor
        DID_LW_VOD_COMMAND       = 0xBA, // vod_command_descriptor
        DID_LW_DELETION_DATE     = 0xBB, // deletion_date_descriptor
        DID_LW_PLAY_LIST         = 0xBC, // play_list_descriptor
        DID_LW_PLAY_LIST_ENTRY   = 0xBD, // play_list_entry_descriptor
        DID_LW_ORDER_CODE        = 0xBE, // order_code_descriptor
        DID_LW_BOUQUET_REFERENCE = 0xBF, // bouquet_reference_descriptor

        // Valid after PDS_EUTELSAT private_data_specifier

        DID_EUTELSAT_CHAN_NUM   = 0x83, // eutelsat_channel_number_descriptor

        // Valid after PDS_EACEM/EICTA private_data_specifier

        DID_LOGICAL_CHANNEL_NUM = 0x83, // logical_channel_number_descriptor
        DID_PREF_NAME_LIST      = 0x84, // preferred_name_list_descriptor
        DID_PREF_NAME_ID        = 0x85, // preferred_name_identifier_descriptor
        DID_EACEM_STREAM_ID     = 0x86, // eacem_stream_identifier_descriptor
        DID_HD_SIMULCAST_LCN    = 0x88, // HD_simulcast_logical_channel_number_descriptor

        // Valid after PDS_CANALPLUS private_data_specifier

        DID_DTG_STREAM_IND      = 0x80, // DTG_Stream_indicator_descriptor
        DID_PIO_OFFSET_TIME     = 0X80, // pio_offset_time_descriptor
        DID_LOGICAL_CHANNEL_81  = 0x81, // logical_channel_descriptor
        DID_PRIVATE2            = 0x82, // private_descriptor2
        DID_LOGICAL_CHANNEL     = 0x83, // logical_channel_descriptor
        DID_PIO_CONTENT         = 0x83, // pio_content_descriptor
        DID_PIO_LOGO            = 0x84, // pio_logo_descriptor
        DID_ADSL_DELIVERY       = 0x85, // adsl_delivery_system_descriptor
        DID_PIO_FEE             = 0x86, // pio_fee_descriptor
        DID_PIO_EVENT_RANGE     = 0x88, // pio_event_range_descriptor
        DID_PIO_COPY_MANAGEMENT = 0x8B, // pio_copy_management_descriptor
        DID_PIO_COPY_CONTROL    = 0x8C, // pio_copy_control_descriptor
        DID_PIO_PPV             = 0x8E, // pio_ppv_descriptor
        DID_PIO_STB_SERVICE_ID  = 0x90, // pio_stb_service_id_descriptor
        DID_PIO_MASKING_SERV_ID = 0x91, // pio_masking_service_id_descriptor
        DID_PIO_STB_SERVMAP_UPD = 0x92, // pio_stb_service_map_update_desc
        DID_NEW_SERVICE_LIST    = 0x93, // new_service_list_descriptor
        DID_MESSAGE_NAGRA       = 0x94, // message_descriptor_Nagra
        DID_ITEM_EVENT          = 0xA1, // item_event_descriptor
        DID_ITEM_ZAPPING        = 0xA2, // item_zapping_descriptor
        DID_APPLI_MESSAGE       = 0xA3, // appli_message_descriptor
        DID_LIST                = 0xA4, // list_descriptor
        DID_KEY_LIST            = 0xB0, // key_list_descriptor
        DID_PICTURE_SIGNALLING  = 0xB1, // picture_signalling_descriptor
        DID_COUNTER_BB          = 0xBB, // counter_descriptor
        DID_DATA_COMPONENT_BD   = 0xBD, // data_component_descriptor
        DID_SYSTEM_MGMT_BE      = 0xBE, // system_management_descriptor
        DID_VO_LANGUAGE         = 0xC0, // vo_language_descriptor
        DID_DATA_LIST           = 0xC1, // data_list_descriptor
        DID_APPLI_LIST          = 0xC2, // appli_list_descriptor
        DID_MESSAGE             = 0xC3, // message_descriptor
        DID_FILE                = 0xC4, // file_descriptor
        DID_RADIO_FORMAT        = 0xC5, // radio_format_descriptor
        DID_APPLI_STARTUP       = 0xC6, // appli_startup_descriptor
        DID_PATCH               = 0xC7, // patch_descriptor
        DID_LOADER              = 0xC8, // loader_descriptor
        DID_CHANNEL_MAP_UPDATE  = 0xC9, // channel_map_update_descriptor
        DID_PPV                 = 0xCA, // ppv_descriptor
        DID_COUNTER_CB          = 0xCB, // counter_descriptor
        DID_OPERATOR_INFO       = 0xCC, // operator_info_descriptor
        DID_SERVICE_DEF_PARAMS  = 0xCD, // service_default_parameters_desc
        DID_FINGER_PRINTING     = 0xCE, // finger_printing_descriptor
        DID_FINGER_PRINTING_V2  = 0xCF, // finger_printing_descriptor_v2
        DID_CONCEALED_GEO_ZONES = 0xD0, // concealed_geo_zones_descriptor
        DID_COPY_PROTECTION     = 0xD1, // copy_protection_descriptor
        DID_MG_SUBSCRIPTION     = 0xD3, // subscription_descriptor
        DID_CABLE_BACKCH_DELIV  = 0xD4, // cable_backchannel_delivery_system
        DID_INTERACT_SNAPSHOT   = 0xD5, // Interactivity_snapshot_descriptor
        DID_ICON_POSITION       = 0xDC, // icon_position_descriptor
        DID_ICON_PIXMAP         = 0xDD, // icon_pixmap_descriptor
        DID_ZONE_COORDINATE     = 0xDE, // Zone_coordinate_descriptor
        DID_HD_APP_CONTROL_CODE = 0xDF, // HD_application_control_code_desc
        DID_EVENT_REPEAT        = 0xE0, // Event_Repeat_descriptor
        DID_PPV_V2              = 0xE1, // PPV_V2_descriptor
        DID_HYPERLINK_REF       = 0xE2, // Hyperlink_ref_descriptor
        DID_SHORT_SERVICE       = 0xE4, // Short_service_descriptor
        DID_OPERATOR_TELEPHONE  = 0xE5, // Operator_telephone_descriptor
        DID_ITEM_REFERENCE      = 0xE6, // Item_reference_descriptor
        DID_MH_PARAMETERS       = 0xE9, // MH_Parameters_descriptor
        DID_LOGICAL_REFERENCE   = 0xED, // Logical_reference_descriptor
        DID_DATA_VERSION        = 0xEE, // Data_Version_descriptor
        DID_SERVICE_GROUP       = 0xEF, // Service_group_descriptor
        DID_STREAM_LOC_TRANSP   = 0xF0, // Stream_Locator_Transport_desc
        DID_DATA_LOCATOR        = 0xF1, // Data_Locator_descriptor
        DID_RESIDENT_APP        = 0xF2, // resident_application_descriptor
        DID_RESIDENT_APP_SIGNAL = 0xF3, // Resident_Application_Signalling
        DID_MH_LOGICAL_REF      = 0xF8, // MH_Logical_Reference_descriptor
        DID_RECORD_CONTROL      = 0xF9, // record_control_descriptor
        DID_CMPS_RECORD_CONTROL = 0xFA, // cmps_record_control_descriptor
        DID_EPISODE             = 0xFB, // episode_descriptor
        DID_CMP_SELECTION       = 0xFC, // CMP_Selection_descriptor
        DID_DATA_COMPONENT_FD   = 0xFD, // data_component_descriptor
        DID_SYSTEM_MGMT_FE      = 0xFE, // system_management_descriptor
    };

    //---------------------------------------------------------------------
    // Extended descriptor tag values (in extension_descriptor)
    //---------------------------------------------------------------------

    enum {
        EDID_IMAGE_ICON         = 0x00, // image_icon_descriptor
        EDID_CPCM_DELIVERY_SIG  = 0x01, // cpcm_delivery_signalling_descriptor
        EDID_CP                 = 0x02, // CP_descriptor
        EDID_CP_IDENTIFIER      = 0x03, // CP_identifier_descriptor
        EDID_T2_DELIVERY        = 0x04, // T2_delivery_system_descriptor
        EDID_SH_DELIVERY        = 0x05, // SH_delivery_system_descriptor
        EDID_SUPPL_AUDIO        = 0x06, // supplementary_audio_descriptor
        EDID_NETW_CHANGE_NOTIFY = 0x07, // network_change_notify_descriptor
        EDID_MESSAGE            = 0x08, // message_descriptor
        EDID_TARGET_REGION      = 0x09, // target_region_descriptor
        EDID_TARGET_REGION_NAME = 0x0A, // target_region_name_descriptor
        EDID_SERVICE_RELOCATED  = 0x0B, // service_relocated_descriptor
    };

    //---------------------------------------------------------------------
    // Linkage type values (in linkage_descriptor)
    //---------------------------------------------------------------------

    enum {
        LINKAGE_INFO            = 0x01, // Information service
        LINKAGE_EPG             = 0x02, // EPG service
        LINKAGE_CA_REPLACE      = 0x03, // CA replacement service
        LINKAGE_TS_NIT_BAT      = 0x04, // TS containing complet network/bouquet SI
        LINKAGE_SERVICE_REPLACE = 0x05, // Service replacement service
        LINKAGE_DATA_BROADCAST  = 0x06, // Data broadcast service
        LINKAGE_RCS_MAP         = 0x07, // RCS map
        LINKAGE_HAND_OVER       = 0x08, // Mobile hand-over
        LINKAGE_SSU             = 0x09, // System software update service
        LINKAGE_SSU_TABLE       = 0x0A, // TS containing SSU BAT or NIT
        LINKAGE_IP_NOTIFY       = 0x0B, // IP/MAC notification service
        LINKAGE_INT_BAT_NIT     = 0x0C, // TS containing INT BAT or NIT
    };

    //---------------------------------------------------------------------
    // Data broadcast id values (in data_broadcast[_id]_descriptor)
    //---------------------------------------------------------------------

    enum {
        DBID_DATA_PIPE            = 0x0001, // Data pipe
        DBID_ASYNC_DATA_STREAM    = 0x0002, // Asynchronous data stream
        DBID_SYNC_DATA_STREAM     = 0x0003, // Synchronous data stream
        DBID_SYNCED_DATA_STREAM   = 0x0004, // Synchronised data stream
        DBID_MPE                  = 0x0005, // Multi protocol encapsulation
        DBID_DATA_CSL             = 0x0006, // Data Carousel
        DBID_OBJECT_CSL           = 0x0007, // Object Carousel
        DBID_ATM                  = 0x0008, // DVB ATM streams
        DBID_HP_ASYNC_DATA_STREAM = 0x0009, // Higher Protocols based on asynchronous data streams
        DBID_SSU                  = 0x000A, // System Software Update service [TS 102 006]
        DBID_IPMAC_NOTIFICATION   = 0x000B, // IP/MAC Notification service [EN 301 192]
        DBID_MHP_OBJECT_CSL       = 0x00F0, // MHP Object Carousel
        DBID_MHP_MPE              = 0x00F1, // Reserved for MHP Multi Protocol Encapsulation
        DBID_EUTELSAT_DATA_PIPE   = 0x0100, // Eutelsat Data Piping
        DBID_EUTELSAT_DATA_STREAM = 0x0101, // Eutelsat Data Streaming
        DBID_SAGEM_IP             = 0x0102, // SAGEM IP encapsulation in MPEG-2 PES packets
        DBID_BARCO_DATA_BRD       = 0x0103, // BARCO Data Broadcasting
        DBID_CIBERCITY_MPE        = 0x0104, // CyberCity Multiprotocol Encapsulation
        DBID_CYBERSAT_MPE         = 0x0105, // CyberSat Multiprotocol Encapsulation
        DBID_TDN                  = 0x0106, // The Digital Network
        DBID_OPENTV_DATA_CSL      = 0x0107, // OpenTV Data Carousel
        DBID_PANASONIC            = 0x0108, // Panasonic
        DBID_KABEL_DEUTSCHLAND    = 0x0109, // Kabel Deutschland
        DBID_TECHNOTREND          = 0x010A, // TechnoTrend Gorler GmbH
        DBID_MEDIAHIGHWAY_SSU     = 0x010B, // NDS France Technologies system software download
        DBID_GUIDE_PLUS           = 0x010C, // GUIDE Plus+ Rovi Corporation
        DBID_ACAP_OBJECT_CSL      = 0x010D, // ACAP Object Carousel
        DBID_MICRONAS             = 0x010E, // Micronas Download Stream
        DBID_POLSAT               = 0x0110, // Televizja Polsat
        DBID_DTG                  = 0x0111, // UK DTG
        DBID_SKYMEDIA             = 0x0112, // SkyMedia
        DBID_INTELLIBYTE          = 0x0113, // Intellibyte DataBroadcasting
        DBID_TELEWEB_DATA_CSL     = 0x0114, // TeleWeb Data Carousel
        DBID_TELEWEB_OBJECT_CSL   = 0x0115, // TeleWeb Object Carousel
        DBID_TELEWEB              = 0x0116, // TeleWeb
        DBID_BBC                  = 0x0117, // BBC
        DBID_ELECTRA              = 0x0118, // Electra Entertainment Ltd
        DBID_BBC_2_3              = 0x011A, // BBC 2 - 3
        DBID_TELETEXT             = 0x011B, // Teletext
        DBID_SKY_DOWNLOAD_1_5     = 0x0120, // Sky Download Streams 1-5
        DBID_ICO                  = 0x0121, // ICO mim
        DBID_CIPLUS_DATA_CSL      = 0x0122, // CI+ Data Carousel
        DBID_HBBTV                = 0x0123, // HBBTV Carousel
        DBID_ROVI_PREMIUM         = 0x0124, // Premium Content from Rovi Corporation
        DBID_MEDIA_GUIDE          = 0x0125, // Media Guide from Rovi Corporation
        DBID_INVIEW               = 0x0126, // InView Technology Ltd
        DBID_BOTECH               = 0x0130, // Botech Elektronik SAN. ve TIC. LTD.STI.
        DBID_SCILLA_PUSHVOD_CSL   = 0x0131, // Scilla Push-VOD Carousel
        DBID_CANAL_PLUS           = 0x0140, // Canal+
        DBID_OIPF_OBJECT_CSL      = 0x0150, // OIPF Object Carousel - Open IPTV Forum
        DBID_4TV                  = 0x4444, // 4TV Data Broadcast
        DBID_NOKIA_IP_SSU         = 0x4E4F, // Nokia IP based software delivery
        DBID_BBG_DATA_CSL         = 0xBBB1, // BBG Data Caroussel
        DBID_BBG_OBJECT_CSL       = 0xBBB2, // BBG Object Caroussel
        DBID_BBG                  = 0xBBBB, // Bertelsmann Broadband Group
   };

    //---------------------------------------------------------------------
    // Private linkage type values (in private_linkage_descriptor)
    //---------------------------------------------------------------------

    enum {
        LW_LINKAGE_PUSHVOD   = 0x81, // Push VoD download service
        LW_LINKAGE_CIPLUSREV = 0x82, // CI+ revocation list download service
    };

    //---------------------------------------------------------------------
    // Private component content type values (in private_component_desc)
    //---------------------------------------------------------------------

    enum {
        LW_COMP_GENERIC          = 0x00, // Generic, defined by context
        LW_COMP_DMT              = 0x01, // Download synchronization (DMT)
        LW_COMP_PUSHVOD_METADATA = 0x02, // Push VoD metadata (BDT, VIT)
        LW_COMP_PUSHVOD_PMT      = 0x03, // Push VoD "Content PMT"
        LW_COMP_CIPLUS_RSD       = 0x04, // CI+ RSD carousel
    };

    //---------------------------------------------------------------------
    // Scheduled operation type values (in schedule_descriptor)
    //---------------------------------------------------------------------

    enum {
        LW_SCHED_GENERIC        = 0x00,  // Generic, defined by context
        LW_SCHED_PUSHVOD        = 0x01,  // Push VoD download
        LW_SCHED_CIPLUS_REV     = 0x02,  // CI+ revocation lists download
        LW_SCHED_PLANNED_REBOOT = 0x03,  // Planned set-top box reboot
    };

    //---------------------------------------------------------------------
    // Schedule type values (in schedule_descriptor)
    //---------------------------------------------------------------------

    enum {
        LW_SCHED_NEVER     = 0,  // Never scheduled
        LW_SCHED_PERMANENT = 1,  // Permanently active
        LW_SCHED_DAILY     = 2,  // Daily scheduled, same time every day
        LW_SCHED_LIST      = 3,  // Play-list of specific times & duration
    };

    //---------------------------------------------------------------------
    // Binary data type (in BDT)
    //---------------------------------------------------------------------

    enum {
        LW_BDT_TYPE_GENERIC       = 0x00,  // Generic, defined by context
        LW_BDT_TYPE_POSTER        = 0x01,  // Poster image
        LW_BDT_TYPE_VOD_INFO      = 0x02,  // VoD information
        LW_BDT_TYPE_THUMBNAIL     = 0x03,  // Thumbnail image
        LW_BDT_TYPE_ADVERTISEMENT = 0x04,  // Advertisement image
        LW_BDT_TYPE_WALLPAPER     = 0x05,  // Background image
        LW_BDT_TYPE_APPLICATION   = 0x06,  // Generic application
        LW_BDT_TYPE_SSU           = 0x07,  // System software update
        LW_BDT_TYPE_APP_DATA      = 0x08,  // Generic application data
        LW_BDT_TYPE_MAX           = 0x08
    };

    //---------------------------------------------------------------------
    // Binary data format (in BDT)
    //---------------------------------------------------------------------

    enum {
        LW_BDT_GENERIC        = 0x00,  // Generic, raw binary data
        LW_BDT_JPEG           = 0x01,
        LW_BDT_PNG            = 0x02,
        LW_BDT_BMP            = 0x03,
        LW_BDT_GIF            = 0x04,
        LW_BDT_XML_PLAIN      = 0x05,  // XML
        LW_BDT_XML_COMPRESSED = 0x06,  // XML, RFC 1650 (zlib) compressed
        LW_BDT_ELF            = 0x07,  // ELF binary
        LW_BDT_JFFS2          = 0x08,  // JFFS2 file system image
        LW_BDT_JFFS2_MAGELLO  = 0x09,  // JFFS2 image with proprietary Magello encapsulation
        LW_BDT_LOGFS          = 0x0A,  // LogFS file system image
        LW_BDT_UBIFS          = 0x0B,  // UBIFS file system image
        LW_BDT_YAFFS1         = 0x0C,  // YAFFS1 file system image
        LW_BDT_YAFFS2         = 0x0D,  // YAFFS2 file system image
        LW_BDT_SQUASH_FS      = 0x0E,  // SquashFS file system image
        LW_BDT_ISO_9660       = 0x0F,  // ISO 9660 file system image
        LW_BDT_ZIP            = 0x10,  // ZIP container
        LW_BDT_RAR            = 0x11,  // RAR container
        LW_BDT_MAX            = 0x11
    };

    //---------------------------------------------------------------------
    // Asset reference type (in asset_reference_descriptor)
    //---------------------------------------------------------------------

    enum {
        LW_ASSET_REF_GENERIC              = 0x00,
        LW_ASSET_REF_PROMOTED_ASSET       = 0x01,
        LW_ASSET_REF_PURCHASEABLE_PACKAGE = 0x02,
    };

    //---------------------------------------------------------------------
    // DVB-assigned Bouquet Identifier values
    //---------------------------------------------------------------------

    enum {
        BID_TVNUMERIC          = 0x0086,  // TV Numeric on French TNT network
        BID_TVNUMERIC_EUTELSAT = 0xC030,  // TV Numeric on Eutelsat network
        BID_TVNUMERIC_ASTRA    = 0xC031,  // TV Numeric on Astra network
    };

    //---------------------------------------------------------------------
    // DVB-assigned Network Identifier values
    //---------------------------------------------------------------------

    enum {
        NID_TNT_FRANCE = 0x20FA,  // French national terrestrial network
    };

    //---------------------------------------------------------------------
    // IEEE-assigned Organizationally Unique Identifier (OUI) values
    //---------------------------------------------------------------------

    enum {
        OUI_DVB      = 0x00015A,  // Digital Video Broadcasting
        OUI_SKARDIN  = 0x001222,  // Skardin (UK)
        OUI_LOGIWAYS = 0x002660,  // Logiways
    };
}
