//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a J2K_video_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsMasteringDisplayMetadata.h"

namespace ts {
    //!
    //! Representation of a J2K_video_descriptor.
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.80.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL J2KVideoDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! J2K stripe description.
        //!
        class TSDUCKDLL JPEGXS_Stripe_type
        {
            TS_DEFAULT_COPY_MOVE(JPEGXS_Stripe_type);
        public:
            uint8_t  strp_max_idx = 0;  //!< Maximum value of the stripe index.
            uint16_t strp_height = 0;   //!< Default vertical size of a stripe.
            //!
            //! Default constructor.
            //!
            JPEGXS_Stripe_type();
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            JPEGXS_Stripe_type(PSIBuffer& buf) : JPEGXS_Stripe_type() { deserialize(buf); }

            //! @cond nodoxygen
            void clearContent();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        //!
        //! J2K block description.
        //!
        class TSDUCKDLL JPEGXS_Block_type
        {
            TS_DEFAULT_COPY_MOVE(JPEGXS_Block_type);
        public:
            uint32_t full_horizontal_size = 0;  //!< Horizontal size of the entire video frame.
            uint32_t full_vertical_size = 0;    //!< Vertical size of the entire video frame.
            uint16_t blk_width = 0;             //!< Default width of a J2K block.
            uint16_t blk_height = 0;            //!< Default height of a J2K block.
            uint8_t  max_blk_idx_h = 0;         //!< Maximum value of the horizontal block index.
            uint8_t  max_blk_idx_v = 0;         //!< Minimum value of the horizontal block index.
            uint8_t  blk_idx_h = 0;             //!< Horizontal block index of current block.
            uint8_t  blk_idx_v = 0;             //!< Vertical block index of current block.
            //!
            //! Default constructor.
            //!
            JPEGXS_Block_type();
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            JPEGXS_Block_type(PSIBuffer& buf) : JPEGXS_Block_type() { deserialize(buf); }

            //! @cond nodoxygen
            void clearContent();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        // Public members:
        uint16_t                                       profile_and_level = 0;        //!< Same as J2K concept.
        uint32_t                                       horizontal_size = 0;          //!< Horizontal size of the frame or field in each access unit.
        uint32_t                                       vertical_size = 0;            //!< Vertical size of the frame or field in each access unit.
        uint32_t                                       max_bit_rate = 0;             //!< Same as J2K concept.
        uint32_t                                       max_buffer_size = 0;          //!< Same as J2K concept.
        uint16_t                                       DEN_frame_rate = 0;           //!< Same as J2K concept.
        uint16_t                                       NUM_frame_rate = 0;           //!< Same as J2K concept.
        std::optional<uint8_t>                         color_specification {};       //!< Same as J2K concept.
        bool                                           still_mode = false;           //!< Same as J2K concept.
        bool                                           interlaced_video = false;     //!< Same as J2K concept.
        std::optional<uint8_t>                         colour_primaries {};          //!< 8 bits. According to ISO./IEC 23091-2.
        std::optional<uint8_t>                         transfer_characteristics {};  //!< 8 bits. According to ISO./IEC 23091-2.
        std::optional<uint8_t>                         matrix_coefficients {};       //!< 8 bits. According to ISO./IEC 23091-2.
        std::optional<bool>                            video_full_range_flag {};     //!< Bool. According to ISO./IEC 23091-2.
        std::optional<JPEGXS_Stripe_type>              stripe {};                    //!< List of J2K stripes.
        std::optional<JPEGXS_Block_type>               block {};                     //!< List of J2K blocks.
        std::optional<Mastering_Display_Metadata_type> mdm {};                       //!< Mastering Display Metadata.
        ByteBlock                                      private_data {};              //!< Private data.

        //!
        //! Default constructor.
        //!
        J2KVideoDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        J2KVideoDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
