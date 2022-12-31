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
//!  Representation of an HEVC_operation_point_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Representation of an Auxiliary_video_stream_descriptor.
    //! @see ISO/IEC 13818-1 | ITU-T H.222.0 clause 2.6.100
    //! @ingroup descriptor
    //!
    class TSDUCKDLL HEVCOperationPointDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! ES in operation point.
        //!
        class ES_in_OP_type {
        public:
            bool        necessary_layer_flag;  //!< 1 bit
            bool        output_layer_flag;     //!< 1 bit
            uint8_t     ptl_ref_idx;           //!< 6 bits

            ES_in_OP_type();        //!< Constructor
        };

        //!
        //! Elementary stream.
        //!
        class ES_type {
        public:
            bool        prepend_dependencies;  //!< 1 bit
            uint8_t     ES_reference;          //!< 6 bits

            ES_type();              //!< Constructor
        };

        //!
        //! Operation point.
        //!
        class operation_point_type {
        public:
            uint8_t                     target_ols;                     //!< 8 bits.
            std::vector<ES_type>        ESs;                            //!< Elementary streams.
            std::vector<ES_in_OP_type>  ESinOPs;                        //!< Elementary streams in operation point.
            uint8_t                     constant_frame_rate_info_idc;   //!< 2 bits
            uint8_t                     applicable_temporal_id;         //!< 3 bits
            Variable<uint16_t>          frame_rate_indicator;           //!< 12 bits
            Variable<uint32_t>          avg_bit_rate;                   //!< 24 bits
            Variable<uint32_t>          max_bit_rate;                   //!< 24 bits

            operation_point_type();         //!< Constructor
        };

        // public members:
        std::vector<ByteBlock>            profile_tier_level_infos;  //!< ISO/IEC 13818-1 clause 2.6.100
        std::vector<operation_point_type> operation_points;          //!< ISO/IEC 13818-1 clause 2.6.100

        //!
        //! Default constructor.
        //!
        HEVCOperationPointDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        HEVCOperationPointDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
