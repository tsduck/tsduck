//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB terrestrial_delivery_system_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {

    //!
    //! Representation of an ISDB terrestrial_delivery_system_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.31
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ISDBTerrestrialDeliverySystemDescriptor : public AbstractDescriptor
    {
    public:
        // ISDBTerrestrialDeliverySystemDescriptor public members:
        uint16_t              area_code = 0;          //!< Area code, 12 bits.
        uint8_t               guard_interval = 0;     //!< Guard interval, 2 bits.
        uint8_t               transmission_mode = 0;  //!< Transmission mode, 2 bits.
        std::vector<uint64_t> frequencies {};         //!< Frequencies in Hz (warning: coded in 1/7 MHz units in descriptor).

        //!
        //! Default constructor.
        //!
        ISDBTerrestrialDeliverySystemDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBTerrestrialDeliverySystemDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // The frequency in the descriptor is in units of 1/7 MHz.
        // Conversion functions:
        static uint64_t BinToHz(uint16_t bin) { return (1000000 * uint64_t(bin)) / 7; }
        static uint16_t HzToBin(uint64_t freq) { return uint16_t((7 * freq) / 1000000); }
    };
}
