//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a partial_transport_stream_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Representation of a partial_transport_stream_descriptor.
    //! @see ETSI EN 300 468, 7.2.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL PartialTransportStreamDescriptor : public AbstractDescriptor
    {
    public:
        // PartialTransportStreamDescriptor public members:
        uint32_t peak_rate = 0;                                                  //!< 22 bits
        uint32_t minimum_overall_smoothing_rate = UNDEFINED_SMOOTHING_RATE;      //!< 22 bits
        uint16_t maximum_overall_smoothing_buffer = UNDEFINED_SMOOTHING_BUFFER;  //!< 14 bits

        static const uint32_t UNDEFINED_SMOOTHING_RATE   = 0x3FFFFF;  //!< "undefined" value for @a minimum_overall_smoothing_rate.
        static const uint16_t UNDEFINED_SMOOTHING_BUFFER = 0x3FFF;    //!< "undefined" value for @a maximum_overall_smoothing_buffer.

        //!
        //! Default constructor.
        //!
        PartialTransportStreamDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        PartialTransportStreamDescriptor(DuckContext& duck, const Descriptor& bin);

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
