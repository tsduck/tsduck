//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DSM-CC NPT_endpoint_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {

    //!
    //! Representation of a DSM-CC NPT_endpoint_descriptor.
    //! @see ISO/IEC 13818-6, 8.1.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL NPTEndpointDescriptor : public AbstractDescriptor
    {
    public:
        // NPTEndpointDescriptor public members:
        uint64_t start_NPT = 0;   //!< 33 bits, start Normal Play Time (NPT).
        uint64_t stop_NPT = 0;    //!< 33 bits, stop Normal Play Time (NPT).

        //!
        //! Default constructor.
        //! @param [in] start Start NPT.
        //! @param [in] stop Stop NPT.
        //!
        NPTEndpointDescriptor(uint64_t start = 0, uint64_t stop = 0);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        NPTEndpointDescriptor(DuckContext& duck, const Descriptor& bin);

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
