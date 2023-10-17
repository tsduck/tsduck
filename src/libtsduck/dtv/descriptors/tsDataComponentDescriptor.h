//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB data_component_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB data_component_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.20
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DataComponentDescriptor : public AbstractDescriptor
    {
    public:
        // DataComponentDescriptor public members:
        uint16_t  data_component_id = 0;              //!< Data component id as defined in ARIB STD-B10, Part 2, Annex J.
        ByteBlock additional_data_component_info {};  //!< Additional info, depends on id.

        //!
        //! Default constructor.
        //!
        DataComponentDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DataComponentDescriptor(DuckContext& duck, const Descriptor& bin);

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
