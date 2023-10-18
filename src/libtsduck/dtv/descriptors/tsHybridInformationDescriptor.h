//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB hybrid_information_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB hybrid_information_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.58
    //! @ingroup descriptor
    //!
    class TSDUCKDLL HybridInformationDescriptor : public AbstractDescriptor
    {
    public:
        // HybridInformationDescriptor public members:
        bool     has_location = false;   //!< A location is present in the descriptor.
        bool     location_type = false;  //!< Type: false = broadcast, true = connected
        uint8_t  format = 0;             //!< Location format, 4 bits.
        uint8_t  component_tag = 0;      //!< Service component tag (when has_location && !location_type).
        uint16_t module_id = 0;          //!< Module id (when has_location && !location_type).
        UString  URL {};                 //!< URL (when has_location && location_type).

        //!
        //! Default constructor.
        //!
        HybridInformationDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        HybridInformationDescriptor(DuckContext& duck, const Descriptor& bin);

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
