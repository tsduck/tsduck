//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a data_broadcast_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a data_broadcast_descriptor.
    //! @see ETSI EN 300 468, 6.2.11.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DataBroadcastDescriptor : public AbstractDescriptor
    {
    public:
        // Public members
        uint16_t  data_broadcast_id = 0;  //!< Data broadcast id.
        uint8_t   component_tag = 0;      //!< Component tag.
        ByteBlock selector_bytes {};      //!< Selector bytes.
        UString   language_code {};       //!< ISO-639 language code, 3 characters.
        UString   text {};                //!< Text description.

        //!
        //! Default constructor.
        //!
        DataBroadcastDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DataBroadcastDescriptor(DuckContext& duck, const Descriptor& bin);

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
