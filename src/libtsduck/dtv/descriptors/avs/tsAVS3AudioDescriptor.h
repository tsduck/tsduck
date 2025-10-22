//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an AVS3_audio_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsPSIBuffer.h"
#include "tsTablesDisplay.h"
#include "tsxmlElement.h"
#include "tsByteBlock.h"
#include "tsAVS2AudioDescriptor.h"

namespace ts {
    //!
    //! Representation of an AVS3_audio_descriptor.
    //!
    //! @see AVS T/AI 109.7.
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL AVS3AudioDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! parametrs related to general audio coding - audio_codec_id = 0;
        //!
        class TSDUCKDLL general_coding_type
        {
        public:
            int      coding_profile = 0;        //!< 3 bits. The coding type used in the bitstream.
            uint8_t  bitrate_index = 0;         //!< 4 bits. Index to bitrates in tables A.10 to A.20 of T/AI 109.3
            int      bitstream_type = 0;        //!< 1 bit. Indicates whether the bitstream coding is uniform or non-uniform.
            uint8_t  channel_number_index = 0;  //!< 7 bits. Index to the channel configuration table (A.*) in T/AI 109.3
            uint16_t raw_frame_length = 0;      //!< Total length of the current frame in the bitstream.

            //!
            //! Default constructor.
            //!
            general_coding_type() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer.
            //!
            general_coding_type(PSIBuffer& buf) : general_coding_type() { deserialize(buf); }

            //! @cond nodoxygen
            // Delegated methods.
            void serialize(PSIBuffer& buf) const;
            void deserialize(PSIBuffer& buf);
            bool fromXML(const xml::Element* element);
            void toXML(xml::Element* root) const;
            void display(ts::TablesDisplay& display, const ts::UString& margin);
            //! @endcond
        };

        //!
        //! parametrs related to lossless audio coding - audio_codec_id = 1;
        //!
        class TSDUCKDLL lossless_coding_type
        {
        public:
            uint32_t sampling_frequency = 0;  //!< 24 bits. Ths sampling frequency (in Hz) when lookup table cannot be used
            int      coding_profile = 0;      //!< 3 bits. The coding type used in the bitstream.
            uint8_t  channel_number = 0;      //!< indicates the number of channels

            //!
            //! Default constructor.
            //!
            lossless_coding_type() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer.
            //! @param [in] _sampling_frequency_index the index of teh sampling frequency from the containing class
            //!
            lossless_coding_type(PSIBuffer& buf, uint8_t _sampling_frequency_index) :
                lossless_coding_type() { deserialize(buf, _sampling_frequency_index); }

            //! @cond nodoxygen
            // Delegated methods.
            void serialize(PSIBuffer& buf, uint8_t _sampling_frequency_index) const;
            void deserialize(PSIBuffer& buf, uint8_t _sampling_frequency_index);
            bool fromXML(const xml::Element* element, uint8_t _sampling_frequency_index);
            void toXML(xml::Element* root, uint8_t _sampling_frequency_index) const;
            void display(ts::TablesDisplay& display, const ts::UString& margin, uint8_t _sampling_frequency_index);
            //! @endcond
        };

        //!
        //! parameters related to general full rate audio coding - audio_codec_id = 2;
        //!
        class TSDUCKDLL fullrate_coding_type
        {
        public:
            uint8_t                nn_type = 0;            //!< 3 bits. Indicates the configuration of the neural network (basic or low complexity).
            std::optional<uint8_t> channel_num_index {};   //!< 7 bits. Index to the channel configuration table (A.*) in T/AI 109.3
            std::optional<uint8_t> num_objects {};         //!< 7 bits. The number of audio objects used in the audio sequence
            std::optional<uint8_t> hoa_order {};           //!< 4 bits. The HOA signal order (value + 1)
            uint16_t               total_bitrate = 0;      //!< The total bitrate, in kbit/s, according to the value of content_type

            //!
            //! Default constructor.
            //!
            fullrate_coding_type() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer.
            //!
            fullrate_coding_type(PSIBuffer& buf) : fullrate_coding_type() { deserialize(buf); }

            //! @cond nodoxygen
            // Delegated methods.
            void serialize(PSIBuffer& buf) const;
            void deserialize(PSIBuffer& buf);
            bool fromXML(const xml::Element* element);
            void toXML(xml::Element* root) const;
            void display(ts::TablesDisplay& display, const ts::UString& margin);
            //! @endcond

            //!
            //! Determine the content type (Mix_Signal, Channel_Signal, Object_Signal, HOA_Signal) according to
            //! the values specified in the decaration
            //! @returns the type of content (channel, object, hybrid or ambisonic) according to the data items specified
            //!
            uint8_t content_type() const;
        };

    public:
        // Public members:
        uint8_t  audio_codec_id = 0;            //!< 4 bits. the coding type in use
        uint8_t  sampling_frequency_index = 0;  //!< 4 bits. Index to the sampling frequency table (A.9) of T/AI 109.3
        uint32_t sampling_frequency = 0;        //!< 24 bits, only of sampling_frequency_index = 0xf, Raw sampling frequency.
        int      resolution = 0;                //!< 2 bits. Number of quantization bits in the input signal (8, 16, 24)

        std::variant<std::monostate, general_coding_type, lossless_coding_type, fullrate_coding_type> coding_data {}; //!< coding type specific data

        ByteBlock additional_info {};           //!< additional (non-standard) bytes carried in the descriptor

        //!
        //! Default constructor.
        //!
        AVS3AudioDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] desc A binary descriptor to deserialize.
        //!
        AVS3AudioDescriptor(DuckContext& duck, const Descriptor& desc) :
            AVS3AudioDescriptor() { deserialize(duck, desc); }

        // Inherited methods
        DeclareDisplayDescriptor();

        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

        static constexpr uint8_t General_Coding = 0x00;       //!< value for audio_codec_id when general high rate coding is used
        static constexpr uint8_t Lossless_Coding = 0x01;      //!< value for audio_codec_id when lossless coding is used
        static constexpr uint8_t Fullrate_Coding = 0x02;      //!< value for audio_codec_id when general full rate coding is used

        static constexpr uint8_t Channel_signal = 0x0;        //!< value for content_type when channel based audio is used
        static constexpr uint8_t Object_signal = 0x1;         //!< value for content_type when object based audio is used
        static constexpr uint8_t Mix_signal = 0x2;            //!< value for content_type when hybrid (mix of channels and objects) audio is used
        static constexpr uint8_t HOA_signal = 0x3;            //!< value for content_type when ambisonic audio is used
        static constexpr uint8_t INVALID_CONTENT_TYPE = 0xF;  //!< value for content_type when audio coding method cannot be determined

    private:
        friend class AVS2AudioDescriptor;
        // Thread-safe init-safe static data patterns.
        static const Names& GeneralBitstreamTypes();     //!< readable bitstream type values for XML
        static const Names& Resolutions();               //!< readable resolution values for XML
        static const Names& CodingProfiles();            //!< readable coding profiles for XML
    };
}
