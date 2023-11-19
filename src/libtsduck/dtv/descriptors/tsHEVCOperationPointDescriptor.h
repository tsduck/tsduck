//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an HEVC_operation_point_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

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
            ES_in_OP_type() = default;             //!< Constructor
            bool    necessary_layer_flag = false;  //!< 1 bit
            bool    output_layer_flag = false;     //!< 1 bit
            uint8_t ptl_ref_idx = 0;               //!< 6 bits
        };

        //!
        //! Elementary stream.
        //!
        class ES_type {
        public:
            ES_type() = default;                   //!< Constructor
            bool    prepend_dependencies = false;  //!< 1 bit
            uint8_t ES_reference = 0;              //!< 6 bits
        };

        //!
        //! Operation point.
        //!
        class operation_point_type {
        public:
            operation_point_type() = default;                              //!< Constructor
            uint8_t                     target_ols = 0;                    //!< 8 bits.
            std::vector<ES_type>        ESs {};                            //!< Elementary streams.
            std::vector<ES_in_OP_type>  ESinOPs {};                        //!< Elementary streams in operation point.
            uint8_t                     constant_frame_rate_info_idc = 0;  //!< 2 bits
            uint8_t                     applicable_temporal_id = 0;        //!< 3 bits
            std::optional<uint16_t>     frame_rate_indicator {};           //!< 12 bits
            std::optional<uint32_t>     avg_bit_rate {};                   //!< 24 bits
            std::optional<uint32_t>     max_bit_rate {};                   //!< 24 bits
        };

        // public members:
        std::vector<ByteBlock>            profile_tier_level_infos {};  //!< ISO/IEC 13818-1 clause 2.6.100
        std::vector<operation_point_type> operation_points {};          //!< ISO/IEC 13818-1 clause 2.6.100

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
