//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an S2_satellite_delivery_system_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDeliverySystemDescriptor.h"

namespace ts {
    //!
    //! Representation of an S2_satellite_delivery_system_descriptor.
    //! @see ETSI EN 300 468, 6.2.13.3.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL S2SatelliteDeliverySystemDescriptor : public AbstractDeliverySystemDescriptor
    {
    public:
        // Public members:
        bool                    backwards_compatibility_indicator = false;  //!< Deprecated.
        uint8_t                 TS_GS_mode = 0;                             //!< See ETSI EN 300 468, 6.2.13.3.
        std::optional<uint32_t> scrambling_sequence_index {};               //!< See ETSI EN 300 468, 6.2.13.3, 18-bit value.
        std::optional<uint8_t>  input_stream_identifier {};                 //!< See ETSI EN 300 468, 6.2.13.3.
        std::optional<uint8_t>  timeslice_number {};                        //!< See ETSI EN 300 468, 6.2.13.3.

        //!
        //! Default constructor.
        //!
        S2SatelliteDeliverySystemDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        S2SatelliteDeliverySystemDescriptor(DuckContext& duck, const Descriptor& bin);

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
