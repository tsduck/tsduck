//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a C2_delivery_system_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDeliverySystemDescriptor.h"

namespace ts {
    //!
    //! Representation of a C2_delivery_system_descriptor.
    //!
    //! @see ETSI EN 300 468, 6.4.6.1
    //! @ingroup descriptor
    //!
    class TSDUCKDLL C2DeliverySystemDescriptor : public AbstractDeliverySystemDescriptor
    {
    public:
        // Public members:
        uint8_t  plp_id = 0;                           //!< PLP id.
        uint8_t  data_slice_id = 0;                    //!< Data slice id.
        uint32_t C2_system_tuning_frequency = 0;       //!< Frequency in Hz.
        uint8_t  C2_system_tuning_frequency_type = 0;  //!< 2 bits
        uint8_t  active_OFDM_symbol_duration = 0;      //!< 3 bits
        uint8_t  guard_interval = 0;                   //!< 3 bits, guard interval

        //!
        //! Default constructor.
        //!
        C2DeliverySystemDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        C2DeliverySystemDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Enumerations for XML. Also used in class C2BundleDeliverySystemDescriptor.
        friend class C2BundleDeliverySystemDescriptor;
        static const ts::Enumeration C2GuardIntervalNames;
    };
}
