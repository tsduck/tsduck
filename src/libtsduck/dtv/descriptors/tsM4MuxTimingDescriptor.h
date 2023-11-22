//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a m4mux_timing_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a m4mux_timing_descriptor
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.54.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL M4MuxTimingDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint16_t FCR_ES_ID = 0;      //!< ES_ID associated with this clock reference stream.
        uint32_t FCRResolution = 0;  //!< Resolution of the object time base in cycles per second.
        uint8_t  FCRLength = 0;      //!< Length of the fmxClockReference field in M4Mux packets with index = 238.
        uint8_t  FmxRateLength = 0;  //!< Length of the fmxRate field in M4Mux packets with index = 238.

        //!
        //! Default constructor.
        //!
        M4MuxTimingDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        M4MuxTimingDescriptor(DuckContext& duck, const Descriptor& bin);

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
