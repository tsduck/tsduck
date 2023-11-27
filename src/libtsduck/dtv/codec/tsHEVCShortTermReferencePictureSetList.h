
//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  List of HEVC short-term reference picture sets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractVideoStructure.h"

namespace ts {
    //!
    //! List of HEVC short-term reference picture sets.
    //! @ingroup mpeg
    //! @see ITU-T Rec. H.265, 7.3.7 and 7.4.8.
    //!
    //! The HEVC structure st_ref_pic_set(stRpsIdx) is defined in 7.3.7 for index @a stRpsIdx.
    //! This index ranges from 0 to @a num_short_term_ref_pic_sets. The structure at index
    //! @a i may reference similar structures at lower indexes. So, a st_ref_pic_set cannot
    //! be used alone, it needs a reference to the previous instances.
    //!
    //! The structure st_ref_pic_set(stRpsIdx) is used in two structures:
    //! - In sequence parameter set (7.3.2.2), an array of index 0 to  @a num_short_term_ref_pic_sets - 1.
    //! - in slice segment header (7.3.6), last element, index @a num_short_term_ref_pic_sets.
    //!
    //! As a consequence, the st_ref_pic_set structure is not defined as an AbstractVideoStructure
    //! because it cannot be parsed alone. The AbstractVideoStructure is a vector of st_ref_pic_set.
    //! The vector structure is used because the st_ref_pic_set structure are accessed by index.
    //!
    //! The constructor parses a list of @a num_short_term_ref_pic_sets - 1 elements.
    //!
    //! Unlike other AbstractVideoStructure subclasses, the parse() methods do no clear the content
    //! and do not rebuild the complete object. The parse() methods only parse one st_ref_pic_set
    //! structure which is placed in an existing element of the vector. The parse() methods need
    //! one argument: the @a stRpsIdx index of the element to parse from memory.
    //!
    //! To completely rebuild a new HEVCShortTermReferencePictureSetList, use reset() and then
    //! parse each element is sequence.
    //!
    class TSDUCKDLL HEVCShortTermReferencePictureSetList: public AbstractVideoStructure
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef AbstractVideoStructure SuperClass;

        //!
        //! Constructor from a binary area.
        //! The constructor resizes the vector with @a num_short_term_ref_pic_sets + 1 elements
        //! and parses the elements indexes from 0 to @a num_short_term_ref_pic_sets. The last
        //! element is left uninitialized.
        //! @param [in] data Address of binary data to analyze.
        //! @param [in] size Size in bytes of binary data to analyze.
        //! @param [in] num_short_term_ref_pic_sets The corresponding HEVC parameter.
        //!
        HEVCShortTermReferencePictureSetList(const uint8_t* data = nullptr, size_t size = 0, uint32_t num_short_term_ref_pic_sets = 0);

        //!
        //! Reset this object, makes it valid, resize the vector with empty st_ref_pic_set structures.
        //! @param [in] num_short_term_ref_pic_sets The corresponding HEVC parameter.
        //!
        void reset(uint32_t num_short_term_ref_pic_sets);

        //!
        //! Get the @a num_short_term_ref_pic_sets parameter of the list of st_ref_pic_set structures.
        //! In practice, this is the size of the list of the ShortTermReferencePictureSet.
        //! The ShortTermReferencePictureSet index 0 to num_short_term_ref_pic_sets - 1 are stored
        //! in the HEVCSequenceParameterSet. The one with index num_short_term_ref_pic_sets, when
        //! present, is directly stored in the slice header.
        //! @return The @a num_short_term_ref_pic_sets parameter of the list of st_ref_pic_set structures.
        //!
        uint32_t num_short_term_ref_pic_sets() const { return uint32_t(list.size()); }

        // Inherited methods
        virtual void clear() override;
        virtual bool parse(const uint8_t*, size_t, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual bool parse(AVCParser&, std::initializer_list<uint32_t> = std::initializer_list<uint32_t>()) override;
        virtual std::ostream& display(std::ostream& = std::cout, const UString& margin = UString(), int level = Severity::Info) const override;

        //!
        //! HEVC short-term reference picture set.
        //! @see ITU-T Rec. H.265, 7.3.7 and 7.4.8.from
        //!
        class TSDUCKDLL ShortTermReferencePictureSet
        {
        public:
            //!
            //! Constructor.
            //!
            ShortTermReferencePictureSet() = default;
            //!
            //! Clear structure content.
            //!
            void clear();

            // Inline data:
            bool valid = false;                                     //!< This structure is valid.
            // if (stRpsIdx != 0) {
                uint8_t inter_ref_pic_set_prediction_flag = 0;      //!< inter_ref_pic_set_prediction_flag
            // }
            // if (inter_ref_pic_set_prediction_flag) {
                // if (stRpsIdx == num_short_term_ref_pic_sets) {
                    uint32_t delta_idx_minus1 = 0;                  //!< delta_idx_minus1
                // }
                uint8_t delta_rps_sign = 0;                         //!< delta_rps_sign
                uint32_t abs_delta_rps_minus1 = 0;                  //!< abs_delta_rps_minus1
                // for (j = 0; j <= NumDeltaPocs[RefRpsIdx]; j++)
                std::vector<uint8_t> used_by_curr_pic_flag {};      //!< used_by_curr_pic_flag
                // if (!used_by_curr_pic_flag) {
                    std::vector<uint8_t> use_delta_flag {};         //!< use_delta_flag
                // }
            // } else {
                uint32_t num_negative_pics = 0;                     //!< num_negative_pics
                uint32_t num_positive_pics = 0;                     //!< num_positive_pics
                // for (i = 0; i < num_negative_pics; i++) {
                std::vector<uint32_t> delta_poc_s0_minus1 {};       //!< delta_poc_minus1
                std::vector<uint8_t> used_by_curr_pic_s0_flag {};   //!< used_by_curr_pic_flag
                // for (i = 0; i < num_positive_pics; i++) {
                std::vector<uint32_t> delta_poc_s1_minus1 {};       //!< delta_poc_minus1
                std::vector<uint8_t> used_by_curr_pic_s1_flag {};   //!< used_by_curr_pic_flag
            // }

            // Synthetic variables:
            uint32_t             NumNegativePics = 0;  //!< ITU-T Rec. H.265, 7.4.8 (7-61, 7-63)
            uint32_t             NumPositivePics = 0;  //!< ITU-T Rec. H.265, 7.4.8 (7-62, 7-64)
            std::vector<uint8_t> UsedByCurrPicS0 {};   //!< ITU-T Rec. H.265, 7.4.8 (7-65)
            std::vector<uint8_t> UsedByCurrPicS1 {};   //!< ITU-T Rec. H.265, 7.4.8 (7-66)
            std::vector<int32_t> DeltaPocS0 {};        //!< ITU-T Rec. H.265, 7.4.8 (7-67)
            std::vector<int32_t> DeltaPocS1 {};        //!< ITU-T Rec. H.265, 7.4.8 (7-68)
            uint32_t             NumDeltaPocs = 0;     //!< ITU-T Rec. H.265, 7.4.8 (7-71)
        };

        //!
        //! The list of ShortTermReferencePictureSet is organized as a vector.
        //!
        std::vector<ShortTermReferencePictureSet> list {};
    };
}
