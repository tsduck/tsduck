//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB connected_transmission_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB connected_transmission_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.41
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ISDBConnectedTransmissionDescriptor: public AbstractDescriptor {
    public:
        // ISDBLDTLinkageDescriptor public members:
        uint16_t  connected_transmission_group_id = 0;       //!< the label identifying the connected transmission group.
        uint8_t   segment_type = 0;                          //!< 2 bits. indicates segment type in accordance with table 6-80. The number of layers and the layer of each segment type are shown in the table.
        uint8_t   modulation_type_A = 0;                     //!< 2 bits.
        uint8_t   modulation_type_B = 0;                     //!< 2 bits.
        uint8_t   modulation_type_C = 0;                     //!< 2 bits.
        ByteBlock addtional_connected_transmission_info {};  //!< additional information specified in the operational guidelines of service providers.

        //!
        //! Default constructor.
        //!
        ISDBConnectedTransmissionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBConnectedTransmissionDescriptor(DuckContext& duck, const Descriptor& bin);

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
}  // namespace ts
