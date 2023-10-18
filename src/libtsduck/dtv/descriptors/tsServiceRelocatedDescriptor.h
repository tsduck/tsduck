//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a service_relocated_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a service_relocated_descriptor.
    //! @see ETSI EN 300 468, 6.4.9.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ServiceRelocatedDescriptor : public AbstractDescriptor
    {
    public:
        // ServiceRelocatedDescriptor public members:
        uint16_t old_original_network_id = 0;   //!< Old original network id.
        uint16_t old_transport_stream_id = 0;   //!< Old transport stream id.
        uint16_t old_service_id = 0;            //!< Old service id.

        //!
        //! Default constructor.
        //!
        ServiceRelocatedDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ServiceRelocatedDescriptor(DuckContext& duck, const Descriptor& bin);

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
