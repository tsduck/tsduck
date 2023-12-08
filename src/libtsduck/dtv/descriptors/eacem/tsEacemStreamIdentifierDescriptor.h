//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an eacem_stream_identifier_descriptor.
//!  This is a private descriptor, must be preceded by the EACEM/EICTA PDS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an eacem_stream_identifier_descriptor.
    //!
    //! This is a private descriptor, must be preceded by the EACEM/EICTA PDS.
    //! @see EACEM Technical Report Number TR-030, 9.2.11.2.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL EacemStreamIdentifierDescriptor : public AbstractDescriptor
    {
    public:
        // EacemStreamIdentifierDescriptor public members:
        uint8_t version = 0;  //!< EACEM version byte.

        //!
        //! Default constructor.
        //! @param [in] version EACEM version byte.
        //!
        EacemStreamIdentifierDescriptor(uint8_t version = 0);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        EacemStreamIdentifierDescriptor(DuckContext& duck, const Descriptor& bin);

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
