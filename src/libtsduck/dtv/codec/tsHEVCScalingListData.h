
//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  HEVC scaling list data structure.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractVideoStructure.h"

namespace ts {
    //!
    //! HEVC scaling list data structure.
    //! @ingroup mpeg
    //! @see ITU-T Rec. H.265, 7.3.4 and 7.4.5
    //!
    class TSDUCKDLL HEVCScalingListData: public AbstractVideoStructure
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
        HEVCScalingListData(const uint8_t* data = nullptr, size_t size = 0);

        // Inherited methods
        virtual bool parse(const uint8_t*, size_t, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual bool parse(AVCParser&, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual std::ostream& display(std::ostream& = std::cout, const UString& margin = UString(), int level = Severity::Info) const override;

        //!
        //! Scaling data entry.
        //!
        class TSDUCKDLL Scaling
        {
        public:
            Scaling() = default;                                      //!< Constructor.
            uint8_t scaling_list_pred_mode_flag = 0;                  //!< scaling_list_pred_mode_flag
            // if (!scaling_list_pred_mode_flag) {
                uint32_t scaling_list_pred_matrix_id_delta = 0;       //!< scaling_list_pred_matrix_id_delta
            // } else {
                // if (sizeId > 1) {
                    int32_t scaling_list_dc_coef_minus8 = 0;          //!< scaling_list_dc_coef_minus8
                // }
                // for (i = 0; i < coefNum; i++) {
                    std::vector<int32_t> scaling_list_delta_coef {};  //!< scaling_list_delta_coef
                // }
            // }
        };

        //! HEVC scaling list data structure.
        std::array<std::array<Scaling, 6>, 4> list {};
    };
}
