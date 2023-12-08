//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an update_descriptor (UNT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an update_descriptor (UNT specific).
    //!
    //! This descriptor cannot be present in other tables than a UNT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 102 006, 9.5.2.6
    //! @ingroup descriptor
    //!
    class TSDUCKDLL UpdateDescriptor : public AbstractDescriptor
    {
    public:
        // UpdateDescriptor public members:
        uint8_t   update_flag = 0;      //!< 2 bits
        uint8_t   update_method = 0;    //!< 4 bits
        uint8_t   update_priority = 0;  //!< 2 bits
        ByteBlock private_data {};      //!< Private data

        //!
        //! Default constructor.
        //!
        UpdateDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        UpdateDescriptor(DuckContext& duck, const Descriptor& bin);

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
