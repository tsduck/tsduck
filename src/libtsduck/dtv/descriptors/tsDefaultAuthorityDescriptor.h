//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a default_authority_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an RAR_over_IP_descriptor
    //! @see ETSI TS 102 323 clause 6.3.3 and clause 5.2.2 for interpretation.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DefaultAuthorityDescriptor: public AbstractDescriptor
    {
    public:

        std::string default_authority {};  //!< Default authority for this scope. Fully qualified name of the default authority according to the rules given by RFC 1591

        //!
        //! Default constructor.
        //!
        DefaultAuthorityDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DefaultAuthorityDescriptor(DuckContext& duck, const Descriptor& bin);

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
