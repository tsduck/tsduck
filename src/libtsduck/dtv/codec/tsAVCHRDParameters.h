
//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  AVC HRD (Hypothetical Reference Decoder) parameters.
//!  AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractVideoStructure.h"

namespace ts {
    //!
    //! AVC HRD (Hypothetical Reference Decoder) parameters.
    //! @ingroup mpeg
    //!
    //! AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
    //!
    class TSDUCKDLL AVCHRDParameters: public AbstractVideoStructure
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef AbstractVideoStructure SuperClass;

        //!
        //! Constructor from a binary area.
        //! @param [in] data Address of binary data to analyze.
        //! @param [in] size Size in bytes of binary data to analyze.
        //!
        AVCHRDParameters(const uint8_t* data = nullptr, size_t size = 0);

        // Inherited methods
        virtual void clear() override;
        virtual bool parse(const uint8_t* data, size_t siz, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual bool parse(AVCParser&, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual std::ostream& display(std::ostream& strm, const UString& margin = UString(), int level = Severity::Info) const override;

        // HRD parameters fields.
        // See ISO/IEC 14496-10 sections E.1.2 and E.2.2.
        uint32_t cpb_cnt_minus1 = 0;                          //!< cpb_cnt_minus1
        uint8_t  bit_rate_scale = 0;                          //!< bit_rate_scale
        uint8_t  cpb_size_scale = 0;                          //!< cpb_size_scale
        std::vector<uint32_t> bit_rate_value_minus1 {};       //!< bit_rate_value_minus1
        std::vector<uint32_t> cpb_size_value_minus1 {};       //!< cpb_size_value_minus1
        std::vector<uint8_t>  cbr_flag {};                    //!< cbr_flag
        uint8_t  initial_cpb_removal_delay_length_minus1 = 0; //!< initial_cpb_removal_delay_length_minus1
        uint8_t  cpb_removal_delay_length_minus1 = 0;         //!< cpb_removal_delay_length_minus1
        uint8_t  dpb_output_delay_length_minus1 = 0;          //!< dpb_output_delay_length_minus1
        uint8_t  time_offset_length = 0;                      //!< time_offset_length
    };
}
