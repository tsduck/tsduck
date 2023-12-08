//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a URI_linkage_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a URI_linkage_descriptor.
    //! @see ETSI EN 300 468, 6.4.15.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL URILinkageDescriptor : public AbstractDescriptor
    {
    public:
        // URILinkageDescriptor public members:
        uint8_t   uri_linkage_type = 0;      //!< URI linkage type.
        UString   uri {};                    //!< The URI.
        uint16_t  min_polling_interval = 0;  //!< Valid when uri_linkage_type == 0x00 or 0x01.
        ByteBlock private_data {};           //!< Private data.

        //!
        //! Default constructor.
        //!
        URILinkageDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        URILinkageDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
