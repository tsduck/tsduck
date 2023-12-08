//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a multilingual_component_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractMultilingualDescriptor.h"

namespace ts {
    //!
    //! Representation of a multilingual_component_descriptor.
    //! @see ETSI EN 300 468, 6.2.23.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MultilingualComponentDescriptor : public AbstractMultilingualDescriptor
    {
    public:
        // MultilingualComponentDescriptor fields, in addition to inherited "entries".
        uint8_t component_tag = 0;  //!< Component tag, aka. stream identifier.

        //!
        //! Default constructor.
        //!
        MultilingualComponentDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MultilingualComponentDescriptor(DuckContext& duck, const Descriptor& bin);

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
