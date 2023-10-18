//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an IP/MAC_stream_location_descriptor (INT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an IP/MAC_stream_location_descriptor (INT specific).
    //!
    //! This descriptor cannot be present in other tables than an INT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI EN 301 192, 8.4.5.14.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL IPMACStreamLocationDescriptor : public AbstractDescriptor
    {
    public:
        // IPMACStreamLocationDescriptor public members:
        uint16_t network_id = 0;           //!< Network id
        uint16_t original_network_id = 0;  //!< Original network id
        uint16_t transport_stream_id = 0;  //!< Transport stream id
        uint16_t service_id = 0;           //!< Service id
        uint8_t  component_tag = 0;        //!< Component tag

        //!
        //! Default constructor.
        //!
        IPMACStreamLocationDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        IPMACStreamLocationDescriptor(DuckContext& duck, const Descriptor& bin);

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
