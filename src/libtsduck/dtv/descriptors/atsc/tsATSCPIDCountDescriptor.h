//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC PID_count_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ATSC PID_count_descriptor.
    //! @see ATSC A/90, 11.6.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL ATSCPIDCountDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint16_t total_number_of_PIDs = 0;  //!< Total number of PID's.
        uint16_t min_number_of_PIDs = 0;    //!< Minimum number of PID's.

        //!
        //! Default constructor.
        //!
        ATSCPIDCountDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ATSCPIDCountDescriptor(DuckContext& duck, const Descriptor& bin);

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
