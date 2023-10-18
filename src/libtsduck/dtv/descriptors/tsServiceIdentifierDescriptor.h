//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a service_identifier_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Representation of a service_identifier_descriptor.
    //! @see ETSI TS 102 809, 6.2.1.
    //! @see ETSI TS 101 812, 10.12.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ServiceIdentifierDescriptor : public AbstractDescriptor
    {
    public:
        // ServiceIdentifierDescriptor public members:
        UString identifier {}; //!< Service identifier.

        //!
        //! Default constructor.
        //! @param [in] id Service identifier.
        //!
        ServiceIdentifierDescriptor(const UString& id = UString());

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ServiceIdentifierDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();
        virtual DescriptorDuplication duplicationMode() const override;

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
