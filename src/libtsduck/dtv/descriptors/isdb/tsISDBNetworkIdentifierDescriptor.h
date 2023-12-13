//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB network_identifier_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB network_identifier_descriptor.
    //! @see ARIB STD-B21, Part 2, 9.1.8.3
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ISDBNetworkIdentifierDescriptor : public AbstractDescriptor
    {

    public:
        // ISDBNetworkIdentifierDescriptor public members:
        UString     country_code {};   //!< Country code.
        uint16_t    media_type = 0;    //!< Media type (two ASCII letters, e.g. "AB", "AC", etc.)
        uint16_t    network_id = 0;    //!< Network identifier.
        ByteBlock   private_data {};   //!< Private data.

        //!
        //! Default constructor.
        //!
        ISDBNetworkIdentifierDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBNetworkIdentifierDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Enumerations for XML
        static const Enumeration MediaTypes;
    };
}
