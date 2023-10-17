//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a CA_identifier_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a CA_identifier_descriptor.
    //! @see ETSI EN 300 468, 6.2.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CAIdentifierDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        std::vector<uint16_t> casids {}; //!< List of CA system ids.

        //!
        //! Default constructor
        //!
        CAIdentifierDescriptor();

        //!
        //! Constructor using a variable-length argument list.
        //! @param [in] casids Variable-length list of CA system ids.
        //!
        CAIdentifierDescriptor(std::initializer_list<uint16_t> casids);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CAIdentifierDescriptor(DuckContext& duck, const Descriptor& bin);

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
