//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a xait_pid_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Representation of a xait_pid_descriptor.
    //! @see ETSI TS 102 727, 10.17.3.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL XAITPIDDescriptor : public AbstractDescriptor
    {
    public:
        // XAITPIDDescriptor public members:
        PID xait_PID = PID_NULL;  //!< PID for the XAIT.

        //!
        //! Default constructor.
        //! @param [in] pid PID for the XAIT.
        //!
        XAITPIDDescriptor(PID pid = PID_NULL);

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        XAITPIDDescriptor(DuckContext& duck, const Descriptor& bin);

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
