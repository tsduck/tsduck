//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC dcc_arriving_request_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsATSCMultipleString.h"

namespace ts {
    //!
    //! Representation of an ATSC dcc_arriving_request_descriptor.
    //! @see ATSC A/65, section 6.9.11.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DCCArrivingRequestDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t            dcc_arriving_request_type = 0;  //!< Request type.
        ATSCMultipleString dcc_arriving_request_text {};   //!< Request name.

        //!
        //! Default constructor.
        //!
        DCCArrivingRequestDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DCCArrivingRequestDescriptor(DuckContext& duck, const Descriptor& bin);

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
