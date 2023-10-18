//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a target_IP_slash_descriptor (INT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsIPv4Address.h"
#include "tsIPUtils.h"

namespace ts {
    //!
    //! Representation of a target_IP_slash_descriptor (INT specific).
    //!
    //! This descriptor cannot be present in other tables than an INT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI EN 301 192, 8.4.5.9
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TargetIPSlashDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Structure of an address entry in the descriptor.
        //!
        class Address
        {
        public:
            Address() = default;               //!< Constructor
            IPv4Address IPv4_addr {};          //!< IPv4 address.
            uint8_t     IPv4_slash_mask = 0;   //!< Number of bits in network mask.
        };

        // TargetIPSlashDescriptor public members:
        std::vector<Address> addresses {};  //!< IPv4 addresses

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 51;

        //!
        //! Default constructor.
        //!
        TargetIPSlashDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TargetIPSlashDescriptor(DuckContext& duck, const Descriptor& bin);

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
