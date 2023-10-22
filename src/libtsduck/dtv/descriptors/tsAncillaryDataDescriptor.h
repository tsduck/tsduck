//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a ancillary_data_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a ancillary_data_descriptor.
    //! @see ETSI EN 300 468, 6.2.2.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AncillaryDataDescriptor : public AbstractDescriptor
    {
    public:
        // AncillaryDataDescriptor public members:
        uint8_t ancillary_data_identifier = 0;  //!< Data identifier (bit field).

        //!
        //! Default constructor.
        //! @param [in] id Data identifier.
        //!
        AncillaryDataDescriptor(uint8_t id = 0);

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AncillaryDataDescriptor(DuckContext& duck, const Descriptor& bin);

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
