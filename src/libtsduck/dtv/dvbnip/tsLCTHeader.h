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
#include "tsNIPActualCarrierInformation.h"
#include "tsFECTransmissionInformation.h"
#include "tsFECPayloadId.h"
#include "tsFDTInstanceHeader.h"
#include "tsByteBlock.h"
#include "tsTime.h"

namespace ts {
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
        Time      sender_current_time {};     //!< Optional sender current time from header HET_TIME. Time::Epoch if unset.
        NIPActualCarrierInformation naci {};  //!< Optional DVB-NIP carrier information from header HET_NACI.
        FDTInstanceHeader           fdt {};   //!< Optional FDT instance from header HET_FDT.
        FECTransmissionInformation  fti {};   //!< Optional FEC transmission information from header HET_FTI.
        FECPayloadId                fpi {};   //!< FEC Payload ID, following the LCT header.

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

        // Implementation of StringifyInterface (multi-line in this class).
        virtual UString toString() const override;
    };
}
