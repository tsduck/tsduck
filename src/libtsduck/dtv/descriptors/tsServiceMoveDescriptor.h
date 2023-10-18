//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a service_move_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a service_move_descriptor.
    //! @see ETSI EN 300 468, 6.2.34.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ServiceMoveDescriptor : public AbstractDescriptor
    {
    public:
        // ServiceMoveDescriptor public members:
        uint16_t new_original_network_id = 0;   //!< New original network id.
        uint16_t new_transport_stream_id = 0;   //!< New transport stream id.
        uint16_t new_service_id = 0;            //!< New service id.

        //!
        //! Default constructor.
        //!
        ServiceMoveDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ServiceMoveDescriptor(DuckContext& duck, const Descriptor& bin);

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
