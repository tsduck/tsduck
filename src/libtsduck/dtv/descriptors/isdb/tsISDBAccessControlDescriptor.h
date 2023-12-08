//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB access_control_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Representation of an ISDB access_control_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.54
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ISDBAccessControlDescriptor : public AbstractDescriptor
    {
    public:
        // ISDBAccessControlDescriptor public members:
        uint16_t  CA_system_id = 0;       //!< Conditional access system id as defined in ARIB STD-B10, Part 2, Annex M.
        uint8_t   transmission_type = 7;  //!< Transmission type (broadcast by default). Default is 7, broadcast route.
        PID       pid = PID_NULL;         //!< PID for CA tables (ECM or EMM).
        ByteBlock private_data {};        //!< CA-specific private data.

        //!
        //! Default constructor.
        //! @param [in] id CA system id.
        //! @param [in] pid PID for CA tables (ECM or EMM).
        //!
        ISDBAccessControlDescriptor(uint16_t id = 0, PID pid = PID_NULL);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBAccessControlDescriptor(DuckContext& duck, const Descriptor& bin);

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
