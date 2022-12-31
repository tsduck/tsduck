
//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  HEVC profile, tier and level structure.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractVideoStructure.h"

namespace ts {
    //!
    //! HEVC profile, tier and level structure.
    //! @ingroup mpeg
    //! @see ITU-T Rec. H.265, 7.3.3
    //!
    class TSDUCKDLL HEVCProfileTierLevel: public AbstractVideoStructure
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef AbstractVideoStructure SuperClass;

        //!
        //! Constructor from a binary area.
        //! Note: the two parameters @a profilePresentFlag and @a maxNumSubLayersMinus1
        //! must be passed in the initializer list of the parse() methods.
        //! @param [in] data Address of binary data to analyze.
        //! @param [in] size Size in bytes of binary data to analyze.
        //! @param [in] profilePresentFlag HRD profile info is present (depends on parent structure).
        //! @param [in] maxNumSubLayersMinus1 Number of sub-layers minus 1 (depends on parent structure).
        //!
        HEVCProfileTierLevel(const uint8_t* data = nullptr, size_t size = 0, bool profilePresentFlag = false, size_t maxNumSubLayersMinus1 = 0);

        //!
        //! Get the profile value.
        //! @return The profile value.
        //!
        uint8_t profile() const;

        // Inherited methods
        virtual void clear() override;
        virtual bool parse(const uint8_t*, size_t, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual bool parse(AVCParser&, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual std::ostream& display(std::ostream& = std::cout, const UString& margin = UString(), int level = Severity::Info) const override;

        //!
        //! Sub-layer parameters.
        //! @see ITU-T Rec. H.265, 7.3.3
        //!
        class TSDUCKDLL SubLayerParams
        {
        public:
            SubLayerParams();                                          //!< Constructor.
            uint8_t sub_layer_profile_present_flag;                    //!< sub_layer_profile_present_flag
            uint8_t sub_layer_level_present_flag;                      //!< sub_layer_level_present_flag
            // if (sub_layer_profile_present_flag) {
                uint8_t sub_layer_profile_space;                       //!< sub_layer_profile_space
                uint8_t sub_layer_tier_flag;                           //!< sub_layer_tier_flag
                uint8_t sub_layer_profile_idc;                         //!< sub_layer_profile_idc
                std::bitset<32> sub_layer_profile_compatibility_flag;  //!< sub_layer_profile_compatibility_flag
                uint8_t sub_layer_progressive_source_flag;             //!< sub_layer_progressive_source_flag
                uint8_t sub_layer_interlaced_source_flag;              //!< sub_layer_interlaced_source_flag
                uint8_t sub_layer_non_packed_constraint_flag;          //!< sub_layer_non_packed_constraint_flag
                uint8_t sub_layer_frame_only_constraint_flag;          //!< sub_layer_frame_only_constraint_flag
                uint8_t sub_layer_max_12bit_constraint_flag;           //!< sub_layer_max_12bit_constraint_flag
                uint8_t sub_layer_max_10bit_constraint_flag;           //!< sub_layer_max_10bit_constraint_flag
                uint8_t sub_layer_max_8bit_constraint_flag;            //!< sub_layer_max_8bit_constraint_flag
                uint8_t sub_layer_max_422chroma_constraint_flag;       //!< sub_layer_max_422chroma_constraint_flag
                uint8_t sub_layer_max_420chroma_constraint_flag;       //!< sub_layer_max_420chroma_constraint_flag
                uint8_t sub_layer_max_monochrome_constraint_flag;      //!< sub_layer_max_monochrome_constraint_flag
                uint8_t sub_layer_intra_constraint_flag;               //!< sub_layer_intra_constraint_flag
                uint8_t sub_layer_one_picture_only_constraint_flag;    //!< sub_layer_one_picture_only_constraint_flag
                uint8_t sub_layer_lower_bit_rate_constraint_flag;      //!< sub_layer_lower_bit_rate_constraint_flag
                uint8_t sub_layer_max_14bit_constraint_flag;           //!< sub_layer_max_14bit_constraint_flag
                uint8_t sub_layer_inbld_flag;                          //!< sub_layer_inbld_flag
            // }
            // if (sub_layer_level_present_flag) {
                uint8_t sub_layer_level_idc;                           //!< sub_layer_level_idc
            // }
        };

        //
        // HRD profile_tier_level fields.
        // See ITU-T Rec. H.265, 7.3.3
        //
        bool profile_present_flag;                               //!< from parent structure
        // if (profile_present_flag) {
            uint8_t general_profile_space;                       //!< general_profile_space
            uint8_t general_tier_flag;                           //!< general_tier_flag
            uint8_t general_profile_idc;                         //!< general_profile_idc
            std::bitset<32> general_profile_compatibility_flag;  //!< general_profile_compatibility_flag
            uint8_t general_progressive_source_flag;             //!< general_progressive_source_flag
            uint8_t general_interlaced_source_flag;              //!< general_interlaced_source_flag
            uint8_t general_non_packed_constraint_flag;          //!< general_non_packed_constraint_flag
            uint8_t general_frame_only_constraint_flag;          //!< general_frame_only_constraint_flag
            uint8_t general_max_12bit_constraint_flag;           //!< general_max_12bit_constraint_flag
            uint8_t general_max_10bit_constraint_flag;           //!< general_max_10bit_constraint_flag
            uint8_t general_max_8bit_constraint_flag;            //!< general_max_8bit_constraint_flag
            uint8_t general_max_422chroma_constraint_flag;       //!< general_max_422chroma_constraint_flag
            uint8_t general_max_420chroma_constraint_flag;       //!< general_max_420chroma_constraint_flag
            uint8_t general_max_monochrome_constraint_flag;      //!< general_max_monochrome_constraint_flag
            uint8_t general_intra_constraint_flag;               //!< general_intra_constraint_flag
            uint8_t general_one_picture_only_constraint_flag;    //!< general_one_picture_only_constraint_flag
            uint8_t general_lower_bit_rate_constraint_flag;      //!< general_lower_bit_rate_constraint_flag
            uint8_t general_max_14bit_constraint_flag;           //!< general_max_14bit_constraint_flag
            uint8_t general_inbld_flag;                          //!< general_inbld_flag
        // }
        uint8_t general_level_idc;                               //!< general_level_idc
        // for (i = 0; i <= maxNumSubLayersMinus1; i++) {...}
        std::vector<SubLayerParams> sub_layers;                  //!< Per-sub-layer parameters
    };
}
