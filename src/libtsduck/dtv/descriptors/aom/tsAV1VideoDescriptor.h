//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an AV1_video_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

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
        uint8_t                version = 0;                               //!< 7 bits. version of the descriptor - must be 1
        uint8_t                seq_profile = 0;                           //!< 3 bits. Specifies the features that can be used in the coded video sequence.
        uint8_t                seq_level_idx_0 = 0;                       //!< 3 bits. Specifies the level that the coded video sequence conforms to when operating point 0 is selected
        uint8_t                seq_tier_0 = 0;                            //!< 1 bit. Specifies the tier that the coded video sequence conforms to when operating point 0 is selected
        bool                   high_bitdepth = false;                     //!< 1 bit. Together with twelve_bit and seq_profile, determine the bit depth.
        bool                   twelve_bit = false;                        //!< 1 bit. Together with high_bitdepth and seq_profile, determine the bit depth.
        bool                   monochrome = false;                        //!< 1 bit. When true indicates that the video does not contain U and V color planes. When false  indicates that the video contains Y, U, and V color planes.
        bool                   chroma_subsampling_x = false;              //!< 1 bit. Specifies the chroma subsampling format.
        bool                   chroma_subsampling_y = false;              //!< 1 bit. Specifies the chroma subsampling format.
        uint8_t                chroma_sample_position = 0;                //!< 2 bits. Specifies the sample position for subsampled streams
        uint8_t                HDR_WCG_idc = 0;                           //!< 2 bits. Indicates the presence or absence of HDR and WCG components in the PID
        std::optional<uint8_t> initial_presentation_delay_minus_one {};   //!< 4 bits. !!not used in MPEG2-TS!!

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

        // provide a textual representation of the subsampling format
        static UString SubsamplingFormat(bool subsampling_x, bool subsampling_y, bool monochrome);

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
