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
//!  Generic DVB-NIP (Native IP) definitions.
//!
//!  Relevant standards:
//!  - IETF RFC 5651: "Layered Coding Transport (LCT) Building Block",
//!    October 2009
//!  - IETF RFC 3695: "Compact Forward Error Correction (FEC) Schemes",
//!    February 2004
//!  - IETF RFC 5052: "Forward Error Correction (FEC) Building Block",
//!    August 2007
//!  - IETF RFC 5775: "Asynchronous Layered Coding (ALC) Protocol Instantiation",
//!    April 2010
//!  - IETF RFC 3926: "FLUTE - File Delivery over Unidirectional Transport",
//!    October 2004 (FLUTE v1)
//!  - ETSI TS 103 876 V1.1.1 (2024-09), Digital Video Broadcasting (DVB);
//!    Native IP Broadcasting
//!
//!  Note: FLUTE v2 is defined in RFC 6726. It is not backwards compatible with
//!  FLUTE v1. DVB-NIP uses FLUTE v1.
//!
//!  DVB-NIP can be carried over GSE or MPE. TSDuck can only analyze DVB-NIP
//!  over MPE because MPE is encapsulated into TS while GSE is native to
//!  DVB-S2 or DVB-T2. A DVB-NIP stream is encapsulated into one single MPE
//!  stream. This MPE stream must be properly declared into the PMT of a
//!  service. This service must have one single MPE stream. A TS may carry
//!  several DVB-NIP streams but they must be in distinct services, one per
//!  DVB-NIP stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPSocketAddress.h"

namespace ts {
    //!
    //! DVB-NIP signalling TSI Transport Session Identifier (LCT) value.
    //! @see ETSI TS 103 876, section 8.2.2
    //!
    constexpr uint64_t NIP_SIGNALLING_TSI = 0;

    //!
    //! LCT Transport Object Identifier for FLUTE File Delivery Table (FDT).
    //! @see IETF RFC 3926, section 3.3
    //!
    constexpr uint64_t FLUTE_FDT_TOI = 0;

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
        HET_NACI           =  68,  //!< NIP Actual Carrier Information (DVB-NIP, ETSI TS 103 876).
        HET_MAX_VAR_SIZE   = 127,  //!< Max type value for variable-size header extensions.
        HET_MIN_FIXED_SIZE = 128,  //!< Min type value for fixed-size header extensions (24-bit payload).
        HET_FDT            = 192,  //!< FDT Instance Header (FLUTE, RFC 3926).
        HET_CENC           = 193,  //!< FDT Instance Content Encoding extension (FLUTE, RFC 3926).
        HET_MAX_FIXED_SIZE = 255,  //!< Max type value for fixed-size header extensions (24-bit payload).
    };

    //!
    //! Representation of a Layered Coding Transport (LCT) header.
    //! @see IETF RFC 5651
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL LCTHeader
    {
    public:
        bool      valid = false;              //!< The LCT Header was successfully parsed.
        uint8_t   lct_version = 0;            //!< LCT protocol version (4 bits).
        uint8_t   psi = 0;                    //!< Protocol-Specific Indication (2 bits).
        bool      close_session = false;      //!< Close Session flag.
        bool      close_object = false;       //!< Close Object flag.
        uint8_t   codepoint = 0;              //!< Codepoint identifier. Contains the FEC Encoding ID in FLUTE (RFC 3926, 5.1).
        ByteBlock cci {};                     //!< Congestion control information.
        uint64_t  tsi = 0;                    //!< Transport Session Identifier.
        uint64_t  toi = 0;                    //!< Transport Object Identifier (low 64 bits).
        uint64_t  toi_high = 0;               //!< Transport Object Identifier (high 64 bits).
        size_t    tsi_length = 0;             //!< Length in bytes of TSI field.
        size_t    toi_length = 0;             //!< Length in bytes of TOI field.
        std::map<uint8_t, ByteBlock> ext {};  //!< Header extensions, indexed by type (HET).

        //!
        //! Default constructor.
        //!
        LCTHeader() = default;

        //!
        //! Clear the content of a binary LCT header.
        //!
        void clear();

        //!
        //! Deserialize a binary LCT header.
        //! @param [in,out] addr Address of binary area. Updated to point after LCT header.
        //! @param [in,out] size Size of binary area. Updated with remaining size after deserializing the LCT header.
        //! @return True on success, false on error. Same as @a valid field.
        //!
        bool deserialize(const uint8_t*& addr, size_t& size);
    };

    //!
    //! Representation of the DVB-NIP Actual Carrier Information from LCT header extension HET_NACI.
    //! @see ETSI TS 103 876, section 8.7.3
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL NIPActualCarrierInformation
    {
    public:
        bool     valid = false;                //!< The informatio was successfully parsed.
        uint16_t nip_network_id = 0;           //!< NIPNetworkID
        uint16_t nip_carrier_id = 0;           //!< NIPCarrierID
        uint16_t nip_link_id = 0;              //!< NIPLinkID
        uint16_t nip_service_id = 0;           //!< NIPServiceID
        UString  nip_stream_provider_name {};  //!< NIPStreamProviderName

        //!
        //! Default constructor.
        //!
        NIPActualCarrierInformation() = default;

        //!
        //! Clear the content of a structure.
        //!
        void clear();

        //!
        //! Deserialize the structure from a binary area.
        //! @param [in] addr Address of binary area.
        //! @param [in] size Size of binary area.
        //! @return True on success, false on error. Same as @a valid field.
        //!
        bool deserialize(const uint8_t* addr, size_t size);

        //!
        //! Deserialize the structure from a HET_NACI LCT header extension.
        //! @param [in] lct LCT header.
        //! @return True on success, false on error or not present in LCT header. Same as @a valid field.
        //!
        bool deserialize(const LCTHeader& lct);
    };
}
