//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a cable_delivery_system_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDeliverySystemDescriptor.h"

namespace ts {
    //!
    //! Representation of a cable_delivery_system_descriptor.
    //! @see ETSI EN 300 468, 6.2.13.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CableDeliverySystemDescriptor : public AbstractDeliverySystemDescriptor
    {
    public:
        // SatelliteDeliverySystemDescriptor public members:
        uint64_t frequency;          //!< Frequency in Hz (warning: coded in 100 Hz units in descriptor).
        uint8_t  FEC_outer;          //!< FEC outer, 4 bits.
        uint8_t  modulation;         //!< Modulation type, 8 bits.
        uint64_t symbol_rate;        //!< Symbol rate in symbols/s (warning: coded in 100 sym/s units in descriptor).
        uint8_t  FEC_inner;          //!< FEC inner, 4 bits.

        //!
        //! Default constructor.
        //!
        CableDeliverySystemDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CableDeliverySystemDescriptor(DuckContext& duck, const Descriptor& bin);

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
