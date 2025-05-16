//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a SMPTE anc_data_descriptor.
//!  This is a MPEG private descriptor, must be preceded by the VANC REGID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a SMPTE anc_data_descriptor.
    //!
    //! This is a MPEG private descriptor, must be preceded by the VANC REGID.
    //! @see SMPTE ST 2038, 4.1.2
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL SMPTEAncDataDescriptor : public AbstractDescriptor
    {
    public:
        // SMPTEAncDataDescriptor public members:
        ByteBlock descriptor {}; //!< Additional information, optional and currently undefined.

        //!
        //! Default constructor.
        //!
        SMPTEAncDataDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SMPTEAncDataDescriptor(DuckContext& duck, const Descriptor& bin);

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
