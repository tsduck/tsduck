//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a network_name_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Representation of a network_name_descriptor
    //! @see ETSI EN 300 468, 6.2.27.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL NetworkNameDescriptor : public AbstractDescriptor
    {
    public:
        // NetworkNameDescriptor public members:
        UString name {}; //!< Network name.

        //!
        //! Default constructor.
        //! @param [in] name Network name.
        //!
        NetworkNameDescriptor(const UString& name = UString());

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        NetworkNameDescriptor(DuckContext& duck, const Descriptor& bin);

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
