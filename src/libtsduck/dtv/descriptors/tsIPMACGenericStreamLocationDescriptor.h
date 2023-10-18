//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an IP/MAC_generic_stream_location_descriptor (INT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an IP/MAC_generic_stream_location_descriptor (INT specific).
    //!
    //! This descriptor cannot be present in other tables than an INT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI EN 301 192, 8.4.5.15.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL IPMACGenericStreamLocationDescriptor : public AbstractDescriptor
    {
    public:
        // IPMACGenericStreamLocationDescriptor public members:
        uint16_t  interactive_network_id = 0;  //!< Delivery system id.
        uint8_t   modulation_system_type = 0;  //!< Type of modulation.
        uint16_t  modulation_system_id = 0;    //!< System identifier, depending on modulation.
        uint16_t  PHY_stream_id = 0;           //!< Stream identifier, depending on modulation.
        ByteBlock selector_bytes {};           //!< Selector bytes.

        //!
        //! Default constructor.
        //!
        IPMACGenericStreamLocationDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        IPMACGenericStreamLocationDescriptor(DuckContext& duck, const Descriptor& bin);

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
