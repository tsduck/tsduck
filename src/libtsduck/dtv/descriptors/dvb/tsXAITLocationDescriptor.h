//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a xait_location_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a xait_location_descriptor.
    //! @see ETSI TS 102 727, 10.17.6.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL XAITLocationDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint16_t xait_original_network_id = 0;  //!< 16 bits.
        uint16_t xait_service_id = 0;           //!< 16 bits.
        uint8_t  xait_version_number = 0;       //!< 5 bits.
        uint8_t  xait_update_policy = 0;        //!< 3 bits.

        //!
        //! Default constructor.
        //!
        XAITLocationDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        XAITLocationDescriptor(DuckContext& duck, const Descriptor& bin);

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
