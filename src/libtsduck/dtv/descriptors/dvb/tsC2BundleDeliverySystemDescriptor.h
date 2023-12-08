//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a C2_bundle_delivery_system_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDeliverySystemDescriptor.h"

namespace ts {
    //!
    //! Representation of a C2_bundle_delivery_system_descriptor.
    //!
    //! @see ETSI EN 300 468, 6.4.6.4
    //! @see C2DeliverySystemDescriptor
    //! @ingroup descriptor
    //!
    class TSDUCKDLL C2BundleDeliverySystemDescriptor : public AbstractDeliverySystemDescriptor
    {
    public:
        //!
        //! PLP entry.
        //!
        struct TSDUCKDLL Entry
        {
            Entry() = default;                             //!< Default constructor.
            uint8_t  plp_id = 0;                           //!< PLP id.
            uint8_t  data_slice_id = 0;                    //!< Data slice id.
            uint32_t C2_system_tuning_frequency = 0;       //!< Frequency in Hz.
            uint8_t  C2_system_tuning_frequency_type = 0;  //!< 2 bits
            uint8_t  active_OFDM_symbol_duration = 0;      //!< 3 bits
            uint8_t  guard_interval = 0;                   //!< 3 bits, guard interval.
            bool     master_channel = false;               //!< Use master clock.
        };

        //!
        //! List of PLP entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of entries to fit in 254 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 31;

        // C2BundleDeliverySystemDescriptor public members:
        EntryList entries {};  //!< The list of PLP entries.

        //!
        //! Default constructor.
        //!
        C2BundleDeliverySystemDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        C2BundleDeliverySystemDescriptor(DuckContext& duck, const Descriptor& bin);

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
    };
}
