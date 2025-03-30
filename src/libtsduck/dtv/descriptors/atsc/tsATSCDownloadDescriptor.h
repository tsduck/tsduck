//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC download_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ATSC download_descriptor.
    //! @see ATSC A/90, 12.2.3.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL ATSCDownloadDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint32_t download_id = 0;                 //!< Download id.
        uint32_t carousel_period = 0;             //!< Carousel period in milliseconds.
        uint32_t control_msg_time_out_value = 0;  //!< Control timeout in milliseconds.

        //!
        //! Default constructor.
        //!
        ATSCDownloadDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ATSCDownloadDescriptor(DuckContext& duck, const Descriptor& bin);

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
