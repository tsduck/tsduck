//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB service_prominence_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a DVB service_prominence_descriptor.
    //! @see ETSI EN 300 468, clause 6.4.18.
    //! @ingroup descriptor
    //!
    //! Note: SOGI = Service Of General Interest
    //!
    class TSDUCKDLL DVBServiceProminenceDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Definition of a service prominence region.
        //!
        class SOGI_region_type {
        public:
            SOGI_region_type() = default;                      //!< Constructor.
            std::optional<UString>  country_code {};           //!< ETSI EN 300 468, clause 6.4.18.
            std::optional<uint8_t>  primary_region_code {};    //!< ETSI EN 300 468, clause 6.4.18.
            std::optional<uint8_t>  secondary_region_code {};  //!< ETSI EN 300 468, clause 6.4.18.
            std::optional<uint16_t> tertiary_region_code {};   //!< ETSI EN 300 468, clause 6.4.18.
        };

        //!
        //! Definition of a service of general interest indication
        //! prominence values (flag & priority) and applicable regions.
        //!
        class SOGI_type {
        public:
            SOGI_type() = default;                            //!< Constructor.
            bool                          SOGI_flag = false;  //!< ETSI EN 300 468, clause 6.4.18.
            uint16_t                      SOGI_priority = 0;  //!< ETSI EN 300 468, clause 6.4.18.
            std::optional<uint16_t>       service_id {};      //!< ETSI EN 300 468, clause 6.4.18.
            std::vector<SOGI_region_type> regions {};         //!< ETSI EN 300 468, clause 6.4.18.
        };

        // Public members:
        std::vector<SOGI_type> SOGI_list {};     //!< List of SOGI.
        ByteBlock              private_data {};  //!< Private data bytes.

        //!
        //! Default constructor.
        //!
        DVBServiceProminenceDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DVBServiceProminenceDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
