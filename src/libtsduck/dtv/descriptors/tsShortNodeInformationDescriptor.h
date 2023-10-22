//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB short_node_information_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB short_node_information_descriptor.
    //! @see ARIB STD-B10, Part 3, 5.2.4
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ShortNodeInformationDescriptor : public AbstractDescriptor
    {
    public:
        // ShortNodeInformationDescriptor public members:
        UString ISO_639_language_code {};  //!< Language code.
        UString node_name {};              //!< Node name.
        UString text {};                   //!< Information text.

        //!
        //! Default constructor.
        //!
        ShortNodeInformationDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ShortNodeInformationDescriptor(DuckContext& duck, const Descriptor& bin);

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
