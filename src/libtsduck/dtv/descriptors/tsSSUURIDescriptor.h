//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SSU_uri_descriptor (UNT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an SSU_uri_descriptor (UNT specific).
    //!
    //! This descriptor cannot be present in other tables than a UNT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 102 006, 9.5.2.15
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SSUURIDescriptor : public AbstractDescriptor
    {
    public:
        // SSUURIDescriptor public members:
        uint8_t max_holdoff_time = 0;      //!< Max holdoff time in minutes.
        uint8_t min_polling_interval = 0;  //!< Min polling interval in hours.
        UString uri {};                    //!< The URI.

        //!
        //! Default constructor.
        //!
        SSUURIDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SSUURIDescriptor(DuckContext& duck, const Descriptor& bin);

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
