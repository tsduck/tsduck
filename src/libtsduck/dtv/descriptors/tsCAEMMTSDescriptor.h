//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB CA_EMM_TS_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB CA_EMM_TS_descriptor.
    //! @see ARIB STD-B25, Part 1, 4.7.1
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CAEMMTSDescriptor : public AbstractDescriptor
    {
    public:
        // CAEMMTSDescriptor public members:
        uint16_t CA_system_id = 0;         //!< Conditional access system id as defined in ARIB STD-B10, Part 2, Annex M.
        uint16_t transport_stream_id = 0;  //!< Transport stream id.
        uint16_t original_network_id = 0;  //!< Original network id.
        uint8_t  power_supply_period = 0;  //!< Power-on time in minutes.

        //!
        //! Default constructor.
        //!
        CAEMMTSDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CAEMMTSDescriptor(DuckContext& duck, const Descriptor& bin);

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
