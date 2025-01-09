//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsModulation.h"

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
        uint64_t frequency = 0;    //!< Frequency in Hz (warning: coded in 100 Hz units in descriptor).
        uint8_t  FEC_outer = 0;    //!< FEC outer, 4 bits.
        uint8_t  modulation = 0;   //!< Modulation type, 8 bits.
        uint64_t symbol_rate = 0;  //!< Symbol rate in symbols/s (warning: coded in 100 sym/s units in descriptor).
        uint8_t  FEC_inner = 0;    //!< FEC inner, 4 bits.

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

        //!
        //! Translate the binary value in FEC_inner as a InnerFEC enumeration value.
        //! @return The corresponding InnerFEC enumeration value.
        //!
        InnerFEC getInnerFEC() const { return translate(FEC_inner, ToInnerFEC(), FEC_AUTO); }

        //!
        //! Translate the binary value in modulation as a Modulation enumeration value.
        //! @return The corresponding Modulation enumeration value.
        //!
        Modulation getModulation() const { return translate(modulation, ToModulation(), QAM_AUTO); }

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
        // Thread-safe init-safe static data patterns.
        static const Names& ModulationNames();
        static const Names& OuterFecNames();
        static const Names& InnerFecNames();
        static const std::map<int, InnerFEC>& ToInnerFEC();
        static const std::map<int, Modulation>& ToModulation();
    };
}
