//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an application_usage_descriptor (AIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an application_usage_descriptor (AIT specific).
    //!
    //! This descriptor cannot be present in other tables than an AIT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 102 809, 5.3.5.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ApplicationUsageDescriptor : public AbstractDescriptor
    {
    public:
        // ApplicationUsageDescriptor public members:
        uint8_t usage_type = 0;  //!< Usage type.

        //!
        //! Default constructor.
        //! @param [in] type Usage type.
        //!
        ApplicationUsageDescriptor(uint8_t type = 0);

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ApplicationUsageDescriptor(DuckContext& duck, const Descriptor& bin);

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
