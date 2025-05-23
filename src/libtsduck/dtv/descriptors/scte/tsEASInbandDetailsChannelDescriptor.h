//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SCTE 18 EAS_inband_details_channel_descriptor
//!  (specific to a Cable Emergency Alert Table).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an SCTE 18 EAS_inband_details_channel_descriptor (specific to a Cable Emergency Alert Table).
    //!
    //! This descriptor cannot be present in other tables than a Cable Emergency Alert Table).
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see SCTE 18, 5.1.1
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL EASInbandDetailsChannelDescriptor : public AbstractDescriptor
    {
    public:
        // EASInbandDetailsChannelDescriptor public members:
        uint8_t  details_RF_channel = 0;      //!< RF channel number of the carrier.
        uint16_t details_program_number = 0;  //!< Program number, aka service id.

        //!
        //! Default constructor.
        //!
        EASInbandDetailsChannelDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        EASInbandDetailsChannelDescriptor(DuckContext& duck, const Descriptor& bin);

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
