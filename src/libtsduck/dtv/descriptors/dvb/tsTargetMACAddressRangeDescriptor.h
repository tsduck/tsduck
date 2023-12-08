//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a target_MAC_address_range_descriptor (INT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsMACAddress.h"

namespace ts {
    //!
    //! Representation of a target_MAC_address_range_descriptor (INT specific).
    //!
    //! This descriptor cannot be present in other tables than an INT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI EN 301 192, 8.4.5.7
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TargetMACAddressRangeDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Structure of an address range entry in the descriptor.
        //!
        class Range
        {
        public:
            Range() = default;            //!< Constructor
            MACAddress MAC_addr_low {};   //!< First MAC address.
            MACAddress MAC_addr_high {};  //!< Last MAC address.
        };

        // TargetMACAddressRangeDescriptor public members:
        std::vector<Range> ranges {};  //!< MAC address ranges.

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 21;

        //!
        //! Default constructor.
        //!
        TargetMACAddressRangeDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TargetMACAddressRangeDescriptor(DuckContext& duck, const Descriptor& bin);

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
