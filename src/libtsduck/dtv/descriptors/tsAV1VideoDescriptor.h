//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
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
//!  Representation of an AV1_video_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Representation of an AV1_video_descriptor.
    //!
    //! @see https://aomediacodec.github.io/av1-mpeg2-ts/
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AV1VideoDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t             version;                                //!< 7 bits. version of the descriptor - must be 1
        uint8_t             seq_profile;                            //!< 3 bits. Specifies the features that can be used in the coded video sequence.
        uint8_t             seq_level_idx_0;                        //!< 3 bits. Specifies the level that the coded video sequence conforms to when operating point 0 is selected
        uint8_t             seq_tier_0;                             //!< 1 bit. Specifies the tier that the coded video sequence conforms to when operating point 0 is selected
        bool                high_bitdepth;                          //!< 1 bit. Together with twelve_bit and seq_profile, determine the bit depth.
        bool                twelve_bit;                             //!< 1 bit. Together with high_bitdepth and seq_profile, determine the bit depth.
        bool                monochrome;                             //!< 1 bit. When true indicates that the video does not contain U and V color planes. When false  indicates that the video contains Y, U, and V color planes.
        bool                chroma_subsampling_x;                   //!< 1 bit. Specifies the chroma subsampling format.
        bool                chroma_subsampling_y;                   //!< 1 bit. Specifies the chroma subsampling format.
        uint8_t             chroma_sample_position;                 //!< 2 bits. Specifies the sample position for subsampled streams
        uint8_t             HDR_WCG_idc;                            //!< 2 bits. Indicates the presence or absence of HDR and WCG components in the PID
        Variable<uint8_t>   initial_presentation_delay_minus_one;   //!< 4 bits. !!not used in MPEG2-TS!!

        //!
        //! Default constructor.
        //!
        AV1VideoDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AV1VideoDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    private:
        // Enumerations for XML.
        static const Enumeration ChromaSamplePosition;

        // provide a textual representation of the subsampling format
        static UString SubsamplingFormat(bool subsampling_x, bool subsampling_y, bool monochrome);

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
