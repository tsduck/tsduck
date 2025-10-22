//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an AVS2_audio_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsPSIBuffer.h"
#include "tsTablesDisplay.h"
#include "tsxmlElement.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an AVS2_audio_descriptor.
    //!
    //! @see AVS T/AI 109.7.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL AVS2AudioDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! information specific to version 1 of AVS2 audio
        //! used when avs_info_flag == 1
        //!
        class TSDUCKDLL avs_version_info
        {
        public:
            uint8_t  audio_codec_id = 0;       //!< 4 bits. the audio coding method (common or lossless)
            uint8_t  coding_profile = 0;       //!< 3 bits.  basic framework or object metadata based framework
            uint8_t  bitrate_index = 0;        //!< 4 bits. index to bitrate tables (A.11~A.13) in GB/T33475.3
            int      bitstream_type = 0;       //!< 1 bit.  the bitsream type : 0 = uniform, 1 = variable
            uint16_t raw_frame_length = 0;     //!< according to Appendix A.2 of GB/T33475.3
            uint8_t  resolution = 0;           //!< 2 bits. the number of bits per sample 0=8, 1=16, 3=24

            //!
            //! Default constructor
            //!
            avs_version_info() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer.
            //!
            avs_version_info(PSIBuffer& buf) : avs_version_info() { deserialize(buf); }

            //! @cond nodoxygen
            // Delegated methods.
            void serialize(PSIBuffer& buf) const;
            void deserialize(PSIBuffer& buf);
            bool fromXML(const xml::Element* element);
            void toXML(xml::Element* root) const;
            static void display(ts::TablesDisplay& display, PSIBuffer& buf, const ts::UString& margin, uint8_t _num_channels);
            //! @endcond
        };

        // Public members:
        uint8_t num_channels = 0;       //!< The number of channels in the AVS2 audio strema
        uint8_t sample_rate_index = 0;  //!< 4 bits.  index to sample rate table (C.3) in GB/T33475.3

        std::optional<UString> description {};          //!< description of the AVS2 audio stream
        std::optional<UString> language {};             //!< 3-byte language of the audio stream
        std::optional<avs_version_info> avs_version {}; //!< version specific information

        ByteBlock additional_info {};                   //!< additional (non-standardised) information

        //!
        //! Default constructor.
        //!
        AVS2AudioDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] desc A binary descriptor to deserialize.
        //!
        AVS2AudioDescriptor(DuckContext& duck, const Descriptor& desc) :
            AVS2AudioDescriptor() { deserialize(duck, desc); }

        // Inherited methods
        DeclareDisplayDescriptor();

        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Thread-safe init-safe static data patterns.
        static const Names& CodingProfiles();
    };
}
