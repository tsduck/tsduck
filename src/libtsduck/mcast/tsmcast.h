//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtsduck mpeg
//!  Generic FLUTE definitions (File Delivery over Unidirectional Transport).
//!  @see IETF RFC 3926
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPSocketAddress.h"

namespace ts {
    //!
    //! Namespace for advanced forms of television over multicast.
    //!
    namespace mcast {
        //!
        //! LCT Transport Object Identifier for FLUTE File Delivery Table (FDT).
        //! @see IETF RFC 3926, section 3.3
        //!
        constexpr uint64_t FLUTE_FDT_TOI = 0;

        //!
        //! Invalid Transport Session Identifier (TSI) value, to be used as placeholder.
        //!
        constexpr uint64_t INVALID_TSI = std::numeric_limits<uint64_t>::max();

        //!
        //! DVB-NIP signalling TSI Transport Session Identifier (LCT) value.
        //! @see ETSI TS 103 876, section 8.2.2
        //!
        constexpr uint64_t NIP_SIGNALLING_TSI = 0;

        //!
        //! DVB-NIP signalling UDP port.
        //! @see ETSI TS 103 876, section 8.2.2
        //!
        constexpr uint16_t NIP_SIGNALLING_PORT = 3937;

        //!
        //! Get the DVB-NIP signalling IPv4 address and port (224.0.23.14, UDP port 3937).
        //! @return A constant reference to the DVB-NIP signalling IPv4address.
        //! @see ETSI TS 103 876, section 8.2.2
        //!
        TSDUCKDLL const IPSocketAddress& NIPSignallingAddress4();

        //!
        //! Get the DVB-NIP signalling IPv6 address and port (FF0X:0:0:0:0:0:0:12D, UDP port 3937).
        //! @return A constant reference to the DVB-NIP signalling IPv6 address (with scope bits set to zero).
        //! @see ETSI TS 103 876, section 8.2.2
        //!
        TSDUCKDLL const IPSocketAddress& NIPSignallingAddress6();

        //!
        //! FEC Encoding ID values for FLUTE and LCT.
        //! @see IETF RFC 3926, section 5.1.4
        //!
        enum : uint8_t {
            FEI_COMPACT_NOCODE =   0,  //!< Compact No-Code FEC (Fully-Specified)
            FEI_RAPTORQ        =   6,  //!< RaptorQ FEC Scheme (RFC 6330)
            FEI_EXPANDABLE     = 128,  //!< Small Block, Large Block and Expandable FEC (Under-Specified)
            FEI_SMALL_BLOCK    = 129,  //!< Small Block Systematic FEC (Under-Specified)
            FEI_COMPACT        = 130,  //!< Compact FEC (Under-Secified)
        };

        //!
        //! Header Extension Types (HET) for Layered Coding Transport (LCT).
        //!
        enum : uint8_t {
            HET_MIN_VAR_SIZE   =   0,  //!< Min type value for variable-size header extensions.
            HET_NOP            =   0,  //!< No-Operation extension.
            HET_AUTH           =   1,  //!< Packet authentication extension.
            HET_TIME           =   2,  //!< Time extension.
            HET_FTI            =  64,  //!< FEC Object Transmission Information extension (ALC, RFC 5775).
            HET_TOL48          =  67,  //!< ATSC Transport Object Length, 48-bit version (ATSC A/331, section A.3.8.1).
            HET_NACI           =  68,  //!< NIP Actual Carrier Information (DVB-NIP, ETSI TS 103 876).
            HET_MAX_VAR_SIZE   = 127,  //!< Max type value for variable-size header extensions.
            HET_MIN_FIXED_SIZE = 128,  //!< Min type value for fixed-size header extensions (24-bit payload).
            HET_FDT            = 192,  //!< FDT Instance Header (FLUTE, RFC 3926).
            HET_CENC           = 193,  //!< FDT Instance Content Encoding extension (FLUTE, RFC 3926).
            HET_TOL24          = 194,  //!< ATSC Transport Object Length, 24-bit version (ATSC A/331, section A.3.8.1).
            HET_MAX_FIXED_SIZE = 255,  //!< Max type value for fixed-size header extensions (24-bit payload).
        };

        //!
        //! Content encoding values in HET_CENC header of Layered Coding Transport (LCT).
        //!
        enum : uint8_t {
            CENC_NULL    = 0,  //!< No encoding, raw data.
            CENC_ZLIB    = 1,  //!< ZLib encoding [RFC1950].
            CENC_DEFLATE = 2,  //!< Deflate encoding [RFC1951].
            CENC_GZIP    = 3,  //!< GZip encoding [RFC1952].
        };

        //!
        //! Codepoint values in LCT headers with ROUTE protocol.
        //! "IS" stands for Initialization Segment of the media content such as the DASH Initialization Segment.
        //! @see RFC 9223, section 2.1
        //!
        enum : uint8_t {
            ROUTE_CP_NONE            =  0, //!< Reserved (not used)
            ROUTE_CP_NRT_FILE        =  1, //!< Non Real Time (NRT) - File Mode
            ROUTE_CP_NRT_ENTITY      =  2, //!< NRT - Entity Mode
            ROUTE_CP_NRT_UNSIGNED    =  3, //!< NRT - Unsigned Package Mode
            ROUTE_CP_NRT_SIGNED      =  4, //!< NRT - Signed Package Mode
            ROUTE_CP_NEW_IS_CHANGED  =  5, //!< New IS, timeline changed
            ROUTE_CP_NEW_IS_CONTINUE =  6, //!< New IS, timeline continued
            ROUTE_CP_REDUNDANT_IS    =  7, //!< Redundant IS
            ROUTE_CP_MEDIA_FILE      =  8, //!< Media Segment, File Mode
            ROUTE_CP_MEDIA_ENTITY    =  9, //!< Media Segment, Entity Mode
            ROUTE_CP_MEDIA_RAC       = 10, //!< Media Segment, File Mode with CMAF Random Access Chunk
        };

        //!
        //! Logical definition of a unicast file transport protocol.
        //!
        enum FileTransport {
            FT_UNKNOWN,   //!< Unrecognized protocol.
            FT_FLUTE,     //!< File Delivery over Unidirectional Transport, RFC 3926 (v1), RFC 6726 (v2).
            FT_ROUTE,     //!< Real-Time Transport Object Delivery over Unidirectional Transport, RFC 9223.
        };
    }
}
