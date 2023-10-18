//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB hierarchical_transmission_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Representation of an ISDB hierarchical_transmission_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.22
    //! @ingroup descriptor
    //!
    class TSDUCKDLL HierarchicalTransmissionDescriptor : public AbstractDescriptor
    {
    public:
        // HierarchicalTransmissionDescriptor public members:
        bool high_quality = false;      //!< True when high quality, false for low quality.
        PID  reference_PID = PID_NULL;  //!< Reference PID.

        //!
        //! Default constructor.
        //!
        HierarchicalTransmissionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        HierarchicalTransmissionDescriptor(DuckContext& duck, const Descriptor& bin);

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
