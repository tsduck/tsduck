//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of the DVB-NIP Actual Carrier Information from LCT header.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsUString.h"

namespace ts {

    class LCTHeader;

    //!
    //! Representation of the DVB-NIP Actual Carrier Information from LCT header extension HET_NACI.
    //! @see ETSI TS 103 876, section 8.7.3
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL NIPActualCarrierInformation : public StringifyInterface
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

        //!
        //! Comparison operator for use as index in maps.
        //! @param [in] other Another instance to compare.
        //! @return True is this instance is logically less that @a other.
        //!
        bool operator<(const NIPActualCarrierInformation& other) const;

        // Implementation of StringifyInterface.
        virtual UString toString() const override;

    private:
        // Index of all intergers.
        uint64_t index() const;
    };
}
