//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-, Paul Higgs
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an AVS3_video_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Representation of an AVS3_video_descriptor.
    //!
    //! @see T/AI 109-6.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AVS3VideoDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t   profile_id;                        //!< 8 bits.
        uint8_t   level_id;                          //!< 8 bits.
        bool      multiple_frame_rate_flag;          //!< 1 bit.
        uint8_t   frame_rate_code;                   //!< 4 bits.
        uint8_t   sample_precision;                  //!< 3 bits.
        uint8_t   chroma_format;                     //!< 2 bits.
        bool      temporal_id_flag;                  //!< 1 bit.
        bool      td_mode_flag;                      //!< 1 bit.
        bool      library_stream_flag;               //!< 1 bit.
        bool      library_picture_enable_flag;       //!< 1 bit.
        uint8_t   colour_primaries;                  //!< 8 bits.
        uint8_t   transfer_characteristics;          //!< 8 bits.
        uint8_t   matrix_coefficients;               //!< 8 bits.
		
        //!
        //! Default constructor.
        //!
        AVS3VideoDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AVS3VideoDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    private:
        static UString Avs3Profile(uint8_t pi);
        static UString Avs3Level(uint8_t i);
        static UString AVS3FrameRate(uint16_t fr);
        static UString AVS3SamplePrecision(uint16_t sp);
        static UString Avs3ChromaFormat(uint16_t cf);

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
