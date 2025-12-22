//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Layered Coding Transport (LCT) header.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsByteBlock.h"
#include "tsTime.h"
#include "tsmcast.h"
#include "tsmcastNIPActualCarrierInformation.h"
#include "tsmcastFECTransmissionInformation.h"
#include "tsmcastFECPayloadId.h"
#include "tsmcastFDTInstanceHeader.h"

namespace ts::mcast {
    //!
    //! Representation of a Layered Coding Transport (LCT) header.
    //! By extension, for use in context of FLUTE and DVB-NIP, the corresponding
    //! optional headers are added.
    //! @see IETF RFC 5651
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL LCTHeader : public StringifyInterface
    {
    public:
        FileTransport protocol = FT_UNKNOWN;  //!< Higher level protocol.
        uint8_t       lct_version = 0;        //!< LCT protocol version (4 bits).
        uint8_t       psi = 0;                //!< Protocol-Specific Indication (2 bits).
        bool          repair_packet = false;  //!< FEC repair packet, meaning not a source packet (ROUTE).
        bool          close_session = false;  //!< Close Session flag.
        bool          close_object = false;   //!< Close Object flag.
        uint8_t       codepoint = 0;          //!< Codepoint identifier.
        uint8_t       fec_encoding_id = 0;    //!< FEC Encoding ID.
        ByteBlock     cci {};                 //!< Congestion control information.
        uint64_t      tsi = 0;                //!< Transport Session Identifier.
        uint64_t      toi = 0;                //!< Transport Object Identifier (low 64 bits).
        uint64_t      toi_high = 0;           //!< Transport Object Identifier (high 64 bits).
        size_t        tsi_length = 0;         //!< Length in bytes of TSI field.
        size_t        toi_length = 0;         //!< Length in bytes of TOI field.
        std::optional<Time>     time {};      //!< Optional sender current time from header HET_TIME.
        std::optional<uint8_t>  cenc {};      //!< Optional content encoding algorithm from header HET_CENC.
        std::optional<uint64_t> tol {};       //!< Optional ATSC Transport Onject Length from header HET_TOL (24 or 48 bits).
        std::optional<FDTInstanceHeader>           fdt {};   //!< Optional FDT instance from header HET_FDT.
        std::optional<FECTransmissionInformation>  fti {};   //!< Optional FEC transmission information from header HET_FTI.
        std::optional<NIPActualCarrierInformation> naci {};  //!< Optional DVB-NIP carrier information from header HET_NACI.
        std::map<uint8_t, ByteBlock> ext {};  //!< Other header extensions, indexed by type (HET), when not deserialized in explicit fields.
        FECPayloadId                 fpi {};  //!< FEC Payload ID, following the LCT header.

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
        //! @param [in] protocol The expected file transport protocol.
        //! @return True on success, false on error. Same as @a valid field.
        //!
        bool deserialize(const uint8_t*& addr, size_t& size, FileTransport protocol);

        // Implementation of StringifyInterface (multi-line in this class).
        virtual UString toString() const override;
    };
}
