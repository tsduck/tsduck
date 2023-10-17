//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a CP_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Representation of a CP_descriptor.
    //! @see ETSI EN 300 468, 6.4.2.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CPDescriptor : public AbstractDescriptor
    {
    public:
        // CPDescriptor public members:
        uint16_t  cp_id = 0;          //!< CP system id.
        PID       cp_pid = PID_NULL;  //!< PID for CP tables.
        ByteBlock private_data {};    //!< CP-specific private data.

        //!
        //! Default constructor.
        //! @param [in] cp_id CP system id.
        //! @param [in] cp_pid PID for CP tables.
        //!
        CPDescriptor(uint16_t cp_id = 0, PID cp_pid = PID_NULL);

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CPDescriptor(DuckContext& duck, const Descriptor& bin);

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
