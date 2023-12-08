//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a target_IPv6_source_slash_descriptor (INT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsIPv6Address.h"

namespace ts {
    //!
    //! Representation of a target_IPv6_source_slash_descriptor (INT specific).
    //!
    //! This descriptor cannot be present in other tables than an INT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI EN 301 192, 8.4.5.13
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TargetIPv6SourceSlashDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Structure of an address entry in the descriptor.
        //!
        class Address
        {
        public:
            Address() = default;                    //!< Constructor
            IPv6Address IPv6_source_addr {};        //!< IPv6 source address.
            uint8_t     IPv6_source_slash_mask {};  //!< Number of bits in source network mask.
            IPv6Address IPv6_dest_addr {};          //!< IPv6 destination address.
            uint8_t     IPv6_dest_slash_mask {};    //!< Number of bits in destination network mask.
        };

        // TargetIPv6SourceSlashDescriptor public members:
        std::vector<Address> addresses {};  //!< IPv6 addresses

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 7;

        //!
        //! Default constructor.
        //!
        TargetIPv6SourceSlashDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TargetIPv6SourceSlashDescriptor(DuckContext& duck, const Descriptor& bin);

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
