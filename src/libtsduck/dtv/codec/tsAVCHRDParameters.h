
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
        uint32_t cpb_cnt_minus1;                          //!< cpb_cnt_minus1
        uint8_t  bit_rate_scale;                          //!< bit_rate_scale
        uint8_t  cpb_size_scale;                          //!< cpb_size_scale
        std::vector<uint32_t> bit_rate_value_minus1;      //!< bit_rate_value_minus1
        std::vector<uint32_t> cpb_size_value_minus1;      //!< cpb_size_value_minus1
        std::vector<uint8_t>  cbr_flag;                   //!< cbr_flag
        uint8_t  initial_cpb_removal_delay_length_minus1; //!< initial_cpb_removal_delay_length_minus1
        uint8_t  cpb_removal_delay_length_minus1;         //!< cpb_removal_delay_length_minus1
        uint8_t  dpb_output_delay_length_minus1;          //!< dpb_output_delay_length_minus1
        uint8_t  time_offset_length;                      //!< time_offset_length
    };
}
