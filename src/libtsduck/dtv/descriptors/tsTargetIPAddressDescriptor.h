//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a target_IP_address_descriptor (INT/UNT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsIPv4Address.h"

namespace ts {
    //!
    //! Representation of a target_IP_address_descriptor (INT/UNT specific).
    //!
    //! This descriptor cannot be present in other tables than an INT or UNT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI EN 301 192, 8.4.5.8
    //! @see ETSI TS 102 006, 6.5.2.3
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TargetIPAddressDescriptor : public AbstractDescriptor
    {
    public:
        // TargetIPAddressDescriptor public members:
        IPv4Address       IPv4_addr_mask {};  //!< IPv4 address mask
        IPv4AddressVector IPv4_addr {};       //!< IPv4 addresses

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 62;

        //!
        //! Default constructor.
        //!
        TargetIPAddressDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TargetIPAddressDescriptor(DuckContext& duck, const Descriptor& bin);

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
