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
        uint8_t             seq_profile;                            //!< 3 bits. same as value in Sequence Header OBU in the AV1 video stream.
        uint8_t             seq_level_idx_0;                        //!< 3 bits. same as value in Sequence Header OBU in the AV1 video stream.
        uint8_t             seq_tier_0;                             //!< 1 bit.
        bool                high_bitdepth;                          //!< 1 bit.
        bool                twelve_bit;                             //!< 1 bit.
        bool                monochrome;                             //!< 1 bit.
        bool                chroma_subsampling_x;                   //!< 1 bit.
        bool                chroma_subsampling_y;                   //!< 1 bit.
        uint8_t             chroma_sample_position;                 //!< 2 bits.
        uint8_t             HDR_WCG_idc;                            //!< 2 bits. indicates the presence or absence of HDR and WCG components in the PID
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

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
