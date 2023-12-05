//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an JPEG_XS_video_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsMasteringDisplayMetadata.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an JPEG_XS_video_descriptor.
    //!
    //! @see ISO/IEC 13818-1 2.6.127, ITU-T Rec. H.222.0.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL JPEGXSVideoDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t  descriptor_version = 0;              //!< Version of the descriptor (only 0 is currently supported)
        uint16_t horizontal_size = 0;                 //!< Same a Wf parameter of JPEG XS codestream picture header - see ISO/IEC 21122-1.
        uint16_t vertical_size = 0;                   //!< ame a Hf parameter of JPEG XS codestream picture header - see ISO/IEC 21122-1.
        uint32_t brat = 0;                            //!< Maximum bitrate in Mbit/second - details in ISO/IEC 21122-3.
        uint8_t  interlace_mode = 0;                  //!< 2 bits. Specifies whether the original picture is progressive of interlaced according to table A.7 of ISO.IEC 21122-3.
        uint8_t  framerate_DEN = 1;                   //!< 6 bits. Framerate denominator code according to table A.8 of ISO/IEC 21122-3
        uint16_t framerate_NUM = 0;                   //!< Directly specifies the numerator value for the frame rate.
        std::optional<uint8_t> sample_bitdepth {};    //!< 4 bits. Specifies the ditdepth of the components minus 1 as defined in the ihdr BPC.
        std::optional<uint8_t> sampling_structure {}; //!< 4 bits. Specifies the sampling structire of the image according to table A.10 of ISO/IEC 21122-3.
        uint16_t Ppih = 0;                            //!< Profile of the elementary stream.
        uint8_t  level = 0;                           //!< Level  of the elementary stream.
        uint8_t  sublevel = 0;                        //!< Sublevel of the elementary stream.
        uint32_t max_buffer_size = 0;                 //!< Size of elementary stream buffer in Mbytes.
        uint8_t  buffer_model_type = 2;               //!< Smoothing buffer model type (currently only '2' is permitted)
        uint8_t  colour_primaries = 0;                //!< According to ISO./IEC 23091-2.
        uint8_t  transfer_characteristics = 0;        //!< According to ISO./IEC 23091-2.
        uint8_t  matrix_coefficients = 0;             //!< According to ISO./IEC 23091-2.
        bool     video_full_range_flag = false;       //!< According to ISO./IEC 23091-2.
        bool     still_mode = false;                  //!< Indicates that the video stream may contain JPEG XS still images.

        std::optional<Mastering_Display_Metadata_type> mdm {};           //!< Mastering Display Metadata
        ByteBlock                                      private_data {};  //!< Private data.

        //!
        //! Default constructor.
        //!
        JPEGXSVideoDescriptor();
        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        JPEGXSVideoDescriptor(DuckContext& duck, const Descriptor& bin);

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

    private:
        // Enumerations for XML.
        static const Enumeration FramerateDenominators;
    };
}
