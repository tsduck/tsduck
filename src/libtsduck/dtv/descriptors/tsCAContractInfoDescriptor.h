//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB CA_contract_info_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB CA_contract_info_descriptor.
    //! @see ARIB STD-B25, Part 1, 4.7.2
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CAContractInfoDescriptor : public AbstractDescriptor
    {
    public:
        // CAContractInfoDescriptor public members:
        uint16_t  CA_system_id = 0;              //!< Conditional access system id as defined in ARIB STD-B10, Part 2, Annex M.
        uint8_t   CA_unit_id = 0;                //!< 4 bits, billing unit group.
        ByteBlock component_tags {};             //!< One byte per component tag.
        ByteBlock contract_verification_info {}; //!< Contract description data.
        UString   fee_name {};                   //!< Contract description.

        //!
        //! Default constructor.
        //!
        CAContractInfoDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CAContractInfoDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
