//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an audio_stream_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an audio_stream_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.4.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AudioStreamDescriptor : public AbstractDescriptor
    {
    public:
        // AudioStreamDescriptor public members:
        bool    free_format = false;          //!< Free format.
        uint8_t ID = 0;                       //!< 1 bit, ID value for the stream.
        uint8_t layer = 0;                    //!< 2 bits, audio layer.
        bool    variable_rate_audio = false;  //!< Has variable bitrate.

        //!
        //! Default constructor.
        //!
        AudioStreamDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AudioStreamDescriptor(DuckContext& duck, const Descriptor& bin);

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
