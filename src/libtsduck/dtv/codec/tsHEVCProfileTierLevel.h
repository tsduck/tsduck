
//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            SubLayerParams() = default;                                    //!< Constructor.
            uint8_t sub_layer_profile_present_flag = 0;                    //!< sub_layer_profile_present_flag
            uint8_t sub_layer_level_present_flag = 0;                      //!< sub_layer_level_present_flag
            // if (sub_layer_profile_present_flag) {
                uint8_t sub_layer_profile_space = 0;                       //!< sub_layer_profile_space
                uint8_t sub_layer_tier_flag = 0;                           //!< sub_layer_tier_flag
                uint8_t sub_layer_profile_idc = 0;                         //!< sub_layer_profile_idc
                std::bitset<32> sub_layer_profile_compatibility_flag {};   //!< sub_layer_profile_compatibility_flag
                uint8_t sub_layer_progressive_source_flag = 0;             //!< sub_layer_progressive_source_flag
                uint8_t sub_layer_interlaced_source_flag = 0;              //!< sub_layer_interlaced_source_flag
                uint8_t sub_layer_non_packed_constraint_flag = 0;          //!< sub_layer_non_packed_constraint_flag
                uint8_t sub_layer_frame_only_constraint_flag = 0;          //!< sub_layer_frame_only_constraint_flag
                uint8_t sub_layer_max_12bit_constraint_flag = 0;           //!< sub_layer_max_12bit_constraint_flag
                uint8_t sub_layer_max_10bit_constraint_flag = 0;           //!< sub_layer_max_10bit_constraint_flag
                uint8_t sub_layer_max_8bit_constraint_flag = 0;            //!< sub_layer_max_8bit_constraint_flag
                uint8_t sub_layer_max_422chroma_constraint_flag = 0;       //!< sub_layer_max_422chroma_constraint_flag
                uint8_t sub_layer_max_420chroma_constraint_flag = 0;       //!< sub_layer_max_420chroma_constraint_flag
                uint8_t sub_layer_max_monochrome_constraint_flag = 0;      //!< sub_layer_max_monochrome_constraint_flag
                uint8_t sub_layer_intra_constraint_flag = 0;               //!< sub_layer_intra_constraint_flag
                uint8_t sub_layer_one_picture_only_constraint_flag = 0;    //!< sub_layer_one_picture_only_constraint_flag
                uint8_t sub_layer_lower_bit_rate_constraint_flag = 0;      //!< sub_layer_lower_bit_rate_constraint_flag
                uint8_t sub_layer_max_14bit_constraint_flag = 0;           //!< sub_layer_max_14bit_constraint_flag
                uint8_t sub_layer_inbld_flag = 0;                          //!< sub_layer_inbld_flag
            // }
            // if (sub_layer_level_present_flag) {
                uint8_t sub_layer_level_idc = 0;                           //!< sub_layer_level_idc
            // }
        };

        //
        // HRD profile_tier_level fields.
        // See ITU-T Rec. H.265, 7.3.3
        //
        bool profile_present_flag = false;                           //!< from parent structure
        // if (profile_present_flag) {
            uint8_t general_profile_space = 0;                       //!< general_profile_space
            uint8_t general_tier_flag = 0;                           //!< general_tier_flag
            uint8_t general_profile_idc = 0;                         //!< general_profile_idc
            std::bitset<32> general_profile_compatibility_flag {};   //!< general_profile_compatibility_flag
            uint8_t general_progressive_source_flag = 0;             //!< general_progressive_source_flag
            uint8_t general_interlaced_source_flag = 0;              //!< general_interlaced_source_flag
            uint8_t general_non_packed_constraint_flag = 0;          //!< general_non_packed_constraint_flag
            uint8_t general_frame_only_constraint_flag = 0;          //!< general_frame_only_constraint_flag
            uint8_t general_max_12bit_constraint_flag = 0;           //!< general_max_12bit_constraint_flag
            uint8_t general_max_10bit_constraint_flag = 0;           //!< general_max_10bit_constraint_flag
            uint8_t general_max_8bit_constraint_flag = 0;            //!< general_max_8bit_constraint_flag
            uint8_t general_max_422chroma_constraint_flag = 0;       //!< general_max_422chroma_constraint_flag
            uint8_t general_max_420chroma_constraint_flag = 0;       //!< general_max_420chroma_constraint_flag
            uint8_t general_max_monochrome_constraint_flag = 0;      //!< general_max_monochrome_constraint_flag
            uint8_t general_intra_constraint_flag = 0;               //!< general_intra_constraint_flag
            uint8_t general_one_picture_only_constraint_flag = 0;    //!< general_one_picture_only_constraint_flag
            uint8_t general_lower_bit_rate_constraint_flag = 0;      //!< general_lower_bit_rate_constraint_flag
            uint8_t general_max_14bit_constraint_flag = 0;           //!< general_max_14bit_constraint_flag
            uint8_t general_inbld_flag = 0;                          //!< general_inbld_flag
        // }
        uint8_t general_level_idc = 0;                               //!< general_level_idc
        // for (i = 0; i <= maxNumSubLayersMinus1; i++) {...}
        std::vector<SubLayerParams> sub_layers {};                   //!< Per-sub-layer parameters
    };
}
