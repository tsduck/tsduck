//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC multiprotocol_encapsulation_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ATSC multiprotocol_encapsulation_descriptor.
    //! @see ATSC A/90, 12.2.4.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL ATSCMultiprotocolEncapsulationDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t deviceId_address_range = 6;       //!< 3 bits.
        bool    deviceId_IP_mapping_flag = true;  //!< 1 bit.
        bool    alignment_indicator = false;      //!< 1 bit.
        uint8_t max_sections_per_datagram = 1;    //!< 8 bits.

        //!
        //! Default constructor.
        //!
        ATSCMultiprotocolEncapsulationDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ATSCMultiprotocolEncapsulationDescriptor(DuckContext& duck, const Descriptor& bin);

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
