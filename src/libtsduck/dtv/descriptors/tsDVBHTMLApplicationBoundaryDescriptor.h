//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a dvb_html_application_boundary_descriptor (AIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a dvb_html_application_boundary_descriptor (AIT specific).
    //!
    //! This descriptor cannot be present in other tables than an AIT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 101 812, 10.10.3.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DVBHTMLApplicationBoundaryDescriptor : public AbstractDescriptor
    {
    public:
        // DVBHTMLApplicationBoundaryDescriptor public members:
        UString label {};                //!< Data label.
        UString regular_expression {};   //!< Regular expression to generate all URL's.

        //!
        //! Default constructor.
        //!
        DVBHTMLApplicationBoundaryDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DVBHTMLApplicationBoundaryDescriptor(DuckContext& duck, const Descriptor& bin);

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
