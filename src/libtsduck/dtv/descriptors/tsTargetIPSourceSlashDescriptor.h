//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a target_IP_source_slash_descriptor (INT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsIPv4Address.h"
#include "tsIPUtils.h"

namespace ts {
    //!
    //! Representation of a target_IP_source_slash_descriptor (INT specific).
    //!
    //! This descriptor cannot be present in other tables than an INT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI EN 301 192, 8.4.5.10
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TargetIPSourceSlashDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Structure of an address entry in the descriptor.
        //!
        class Address
        {
        public:
            IPv4Address IPv4_source_addr;         //!< IPv4 source address.
            uint8_t   IPv4_source_slash_mask;   //!< Number of bits in source network mask.
            IPv4Address IPv4_dest_addr;           //!< IPv4 destination address.
            uint8_t   IPv4_dest_slash_mask;     //!< Number of bits in destination network mask.

            //!
            //! Constructor
            //! @param [in] addr1 IPv4 source address.
            //! @param [in] mask1 Number of bits in source network mask.
            //! @param [in] addr2 IPv4 destination address.
            //! @param [in] mask2 Number of bits in destination network mask.
            //!
            Address(const IPv4Address& addr1 = IPv4Address(), uint8_t mask1 = 0, const IPv4Address& addr2 = IPv4Address(), uint8_t mask2 = 0);
        };

        // TargetIPSourceSlashDescriptor public members:
        std::vector<Address> addresses;  //!< IPv4 addresses

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 25;

        //!
        //! Default constructor.
        //!
        TargetIPSourceSlashDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TargetIPSourceSlashDescriptor(DuckContext& duck, const Descriptor& bin);

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
