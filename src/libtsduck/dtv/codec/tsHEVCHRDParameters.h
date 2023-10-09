
//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  HEVC HRD (Hypothetical Reference Decoder) parameters.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractVideoStructure.h"

namespace ts {
    //!
    //! HEVC HRD (Hypothetical Reference Decoder) parameters.
    //! @ingroup mpeg
    //! @see ITU-T Rec. H.265, E.2.2
    //!
    class TSDUCKDLL HEVCHRDParameters: public AbstractVideoStructure
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef AbstractVideoStructure SuperClass;

        //!
        //! Constructor from a binary area.
        //! Note: the two parameters @a commonInfPresentFlag and @a maxNumSubLayersMinus1
        //! must be passed in the initializer list of the parse() methods.
        //! @param [in] data Address of binary data to analyze.
        //! @param [in] size Size in bytes of binary data to analyze.
        //! @param [in] commonInfPresentFlag HRD common info present (depends on parent structure).
        //! @param [in] maxNumSubLayersMinus1 Number of sub-layers minus 1 (depends on parent structure).
        //!
        HEVCHRDParameters(const uint8_t* data = nullptr, size_t size = 0, bool commonInfPresentFlag = false, size_t maxNumSubLayersMinus1 = 0);

        // Inherited methods
        virtual void clear() override;
        virtual bool parse(const uint8_t*, size_t, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual bool parse(AVCParser&, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual std::ostream& display(std::ostream& = std::cout, const UString& margin = UString(), int level = Severity::Info) const override;

        //!
        //! CPB parameter set in sub-layer HRD (Hypothetical Reference Decoder) parameters.
        //! @see ITU-T Rec. H.265 section E.2.3.
        //!
        class TSDUCKDLL CPBParams
        {
        public:
            CPBParams() = default;                      //!< Constructor.
            uint32_t bit_rate_value_minus1 = 0;         //!< bit_rate_value_minus1
            uint32_t cpb_size_value_minus1 = 0;         //!< cpb_size_value_minus1
            // if (sub_pic_hrd_params_present_flag) {   // from parent hrd_parameters structure
                uint32_t cpb_size_du_value_minus1 = 0;  //!< cpb_size_du_value_minus1
                uint32_t bit_rate_du_value_minus1 = 0;  //!< bit_rate_du_value_minus1
            // }
            uint8_t cbr_flag = 0;                       //!< cbr_flag
        };

        //!
        //! Sub-layer HRD (Hypothetical Reference Decoder) parameters.
        //! @see ITU-T Rec. H.265 section E.2.3.
        //!
        class TSDUCKDLL SubLayerParams
        {
        public:
            SubLayerParams() = default;                        //!< Constructor.
            uint8_t fixed_pic_rate_general_flag = 0;           //!< fixed_pic_rate_general_flag
            // if (!fixed_pic_rate_general_flag) {
                uint8_t fixed_pic_rate_within_cvs_flag = 0;    //!< fixed_pic_rate_within_cvs_flag
            // }
            // if (fixed_pic_rate_within_cvs_flag) {
                uint32_t elemental_duration_in_tc_minus1 = 0;  //!< elemental_duration_in_tc_minus1
            // } else {
                uint8_t low_delay_hrd_flag = 0;                //!< low_delay_hrd_flag
            // }
            // if (!low_delay_hrd_flag) {
                uint32_t cpb_cnt_minus1 = 0;                   //!< cpb_cnt_minus1
            // }
            // if (nal_hrd_parameters_present_flag) {
                // sub_layer_hrd_parameters
                std::vector<CPBParams> nal_hrd_parameters {};  //!< nal_hrd_parameters
            // }
            // if (vcl_hrd_parameters_present_flag) {
                // sub_layer_hrd_parameters
                std::vector<CPBParams> vcl_hrd_parameters {};  //!< vcl_hrd_parameters
            // }
        };

        //
        // HRD parameters fields.
        // See ITU-T Rec. H.265 section E.2.1.
        //
        bool common_inf_present_flag = false;                                 //!< from parent structure
        // if (common_inf_present_flag) {
            uint8_t nal_hrd_parameters_present_flag = 0;                      //!< nal_hrd_parameters_present_flag
            uint8_t vcl_hrd_parameters_present_flag = 0;                      //!< vcl_hrd_parameters_present_flag
            // if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
                uint8_t sub_pic_hrd_params_present_flag = 0;                  //!< sub_pic_hrd_params_present_flag
                // if (sub_pic_hrd_params_present_flag) {
                    uint8_t tick_divisor_minus2 = 0;                          //!< tick_divisor_minus2
                    uint8_t du_cpb_removal_delay_increment_length_minus1 = 0; //!< du_cpb_removal_delay_increment_length_minus1
                    uint8_t sub_pic_cpb_params_in_pic_timing_sei_flag = 0;    //!< uint8_t sub_pic_cpb_params_in_pic_timing_sei_flag
                    uint8_t dpb_output_delay_du_length_minus1 = 0;            //!< dpb_output_delay_du_length_minus1
                // }
                uint8_t bit_rate_scale = 0;                                   //!< bit_rate_scale
                uint8_t cpb_size_scale = 0;                                   //!< cpb_size_scale
                // if (sub_pic_hrd_params_present_flag) {
                    uint8_t cpb_size_du_scale = 0;                            //!< cpb_size_du_scale
                // }
                uint8_t initial_cpb_removal_delay_length_minus1 = 0;          //!< initial_cpb_removal_delay_length_minus1
                uint8_t au_cpb_removal_delay_length_minus1 = 0;               //!< au_cpb_removal_delay_length_minus1
                uint8_t dpb_output_delay_length_minus1 = 0;                   //!< dpb_output_delay_length_minus1
            // }
        // }
        // for (i = 0; i <= maxNumSubLayersMinus1; i++) {...}
            std::vector<SubLayerParams> sub_layers {};                        //!< Per-sub-layer parameters

    private:
        bool parse_sub_layer_hrd_parameters(AVCParser&, std::vector<CPBParams>&);
        void display_sub_layer_hrd_parameters(std::ostream&, const UString&, const std::vector<CPBParams>&) const;
    };
}
