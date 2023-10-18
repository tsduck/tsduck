//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a service_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Representation of a service_descriptor.
    //! @see ETSI EN 300 468, 6.2.33.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ServiceDescriptor : public AbstractDescriptor
    {
    public:
        // ServiceDescriptor public members:
        uint8_t service_type = 0;  //!< Service type.
        UString provider_name {};  //!< Provider name.
        UString service_name {};   //!< Service name.

        //!
        //! Default constructor.
        //! @param [in] type Service type.
        //! @param [in] provider Provider name.
        //! @param [in] name Service name.
        //!
        ServiceDescriptor(uint8_t type = 0, const UString& provider = UString(), const UString& name = UString());

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ServiceDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();
        virtual DescriptorDuplication duplicationMode() const override;

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer& buf) const override;
        virtual void deserializePayload(PSIBuffer& buf) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
