//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2025, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an Auxiliary_video_stream_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an Auxiliary_video_stream_descriptor.
    //! @see ISO/IEC 13818-1 | ITU-T H.222.0 clause 2.6.74 and ISO/IEC 23002-3
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL AuxiliaryVideoStreamDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! SI message type.
        //!
        class TSDUCKDLL si_message_type
        {
            TS_DEFAULT_COPY_MOVE(si_message_type);
        public:
            //!
            //! ISO-23002-2 value coding.
            //!
            class TSDUCKDLL iso23002_2_value_coding
            {
                TS_DEFAULT_COPY_MOVE(iso23002_2_value_coding);
            private:
                uint16_t numFF_bytes = 0;  //!< number of FF bytes in the coded type
                uint8_t  last_byte = 0;    //!< last byte value (not FF) of the codec type
            public:
                //!
                //! Default constructor.
                //!
                iso23002_2_value_coding() = default;
                //!
                //! Initializing constructor.
                //! @param [in] initial_value the value to set in the coded representation
                //!
                iso23002_2_value_coding(uint32_t initial_value) { set_value(initial_value); }
                //!
                //! Constructor from a binary descriptor
                //! @param [in] buf A binary descriptor to deserialize.
                //!
                iso23002_2_value_coding(PSIBuffer& buf) { deserialize(buf); }

                //!
                //! Return the value represented in coded form
                //! @return decimal value
                //!
                uint32_t value() const { return uint32_t(numFF_bytes * 255) + last_byte; }

                //!
                //! set the coded form to the specified value
                //! @param [in] new_value value to be set
                //!
                void set_value(const uint32_t new_value) { numFF_bytes = uint16_t(new_value / 255); last_byte = uint8_t(new_value % 255); }

                //! @cond nodoxygen
                void clear() { numFF_bytes = 0; last_byte = 0; }
                void serialize(PSIBuffer&) const;
                void deserialize(PSIBuffer&);
                //! @endcond
            };

            //!
            //! Generic parameters type.
            //!
            class TSDUCKDLL generic_params_type
            {
                TS_DEFAULT_COPY_MOVE(generic_params_type);
            public:
                //!
                //! If it is TRUE, the auxiliary video data corresponds only to the bottom field of the primary video.
                //! If FALSE, the auxiliary video data corresponds only to the top field of the primary video.
                //! If aux_is_one_field is FALSE, aux_is_bottom_field is not applicable.
                std::optional<bool> aux_is_bottom_field {};
                //!
                //! If it is TRUE, any spatial re-sampling operation on the auxiliary video should be field-based.
                //! If it is FALSE, any spatial re-sampling operation on the auxiliary video should be frame-based.
                //! If aux_is_one_field is TRUE, aux_is_interlaced is inferred to be TRUE.
                std::optional<bool> aux_is_interlaced {};
                //!
                //! Horizontal position offsets of the auxiliary video data expressed in 1/16th sample position
                //! in the primary video spatial sampling grid.
                uint8_t             position_offset_h = 0;
                //!
                //! Vertical position offsets of the auxiliary video data expressed in 1/16th sample position
                //! in the primary video spatial sampling grid.
                uint8_t             position_offset_v = 0;

                //!
                //! Default constructor.
                //!
                generic_params_type() = default;
                //!
                //! Constructor from a binary descriptor
                //! @param [in] buf A binary descriptor to deserialize.
                //!
                generic_params_type(PSIBuffer& buf) : generic_params_type() { deserialize(buf); }

                //!
                //! Get the size, in bytes, of this generic_parameters() structure
                //! @return Size, in bytes, of this generic_parameters() structure
                //!
                uint32_t get_size() const { return 3; }

                //! @cond nodoxygen
                void serialize(PSIBuffer&) const;
                void deserialize(PSIBuffer&);
                void toXML(xml::Element*) const;
                bool fromXML(const xml::Element*);
                void display(TablesDisplay&, PSIBuffer&, const UString&);
                //! @endcond
            };

            //!
            //! Depth parameters type.
            //!
            //! kfar and knear specify the range of the depth information respectively behind and in front of the
            //! picture relatively to W. W represents the screen width at the receiver side. W and zp is expressed
            //! using the same distance units.
            //!
            class TSDUCKDLL depth_params_type
            {
                TS_DEFAULT_COPY_MOVE(depth_params_type);
            public:
                uint8_t nkfar = 0;  //!< the numerator of the parameter kfar.
                uint8_t nknear = 0; //!< the numerator of the parameter knear.

                //!
                //! Default constructor.
                //!
                depth_params_type() = default;
                //!
                //! Constructor from a binary descriptor
                //! @param [in] buf A binary descriptor to deserialize.
                //!
                depth_params_type(PSIBuffer& buf) { deserialize(buf); }

                //!
                //! Get the size, in bytes, of this depth_parameters() structure
                //! @return Size, in bytes, of this depth_parameters() structure
                //!
                uint32_t get_size() const { return 2; }

                //! @cond nodoxygen
                void serialize(PSIBuffer&) const;
                void deserialize(PSIBuffer&);
                void toXML(xml::Element*) const;
                bool fromXML(const xml::Element*);
                void display(TablesDisplay&, PSIBuffer&, const UString&);
                //! @endcond
            };

            //!
            //! Parallax parameters type.
            //!
            class TSDUCKDLL parallax_params_type
            {
                TS_DEFAULT_COPY_MOVE(parallax_params_type);
            public:
                uint16_t parallax_zero = 0;     //!< the value for which the parallax is null.
                uint16_t parallax_scale = 0;    //!< scaling factor that defines the dynamic range of the decoded parallax values.
                uint16_t dref = 0;              //!< the reference spectator's viewing distance given in cm.
                uint16_t wref = 0;              //!< the reference spectator's monitor width given in cm.

                //!
                //! Default constructor.
                //!
                parallax_params_type() = default;
                //!
                //! Constructor from a binary descriptor
                //! @param [in] buf A binary descriptor to deserialize.
                //!
                parallax_params_type(PSIBuffer& buf) { deserialize(buf); }

                //!
                //! Get the size, in bytes, of this parallax_parameters() structure
                //! @return Size, in bytes, of this parallax_parameters() structure
                //!
                uint32_t get_size() const { return 8; }

                //! @cond nodoxygen
                void serialize(PSIBuffer&) const;
                void deserialize(PSIBuffer&);
                void toXML(xml::Element*) const;
                bool fromXML(const xml::Element*);
                void display(TablesDisplay&, PSIBuffer&, const UString&);
                //! @endcond
            };

            // si_message_type public fields:
            iso23002_2_value_coding             payload_type {};        //!< The payload type of the SI message
            iso23002_2_value_coding             payload_size {};        //!< Size in bytes of a reserved SI message
            std::optional<generic_params_type>  generic_params {};      //!< Provide precise alignment of the auxiliary video with the primary one
            std::optional<depth_params_type>    depth_params {};        //!< Parameters related an an auxiliary video stream carrying a depth map.
            std::optional<parallax_params_type> parallax_params {};     //!< Parameters related to parralax informaion of an auxiliary video stream.
            std::optional<ByteBlock>            reserved_si_message {}; //!< Data reserved for future backward-compatible use by ISO/IEC.

            //!
            //! Default constructor.
            //!
            si_message_type() = default;
            //!
            //! Constructor from a binary descriptor
            //! @param [in] buf A binary descriptor to deserialize.
            //!
            si_message_type(PSIBuffer& buf) : si_message_type() { deserialize(buf); }

            //!
            //! Get the size, in bytes, of this SI message
            //! @return Size, in bytes, of this SI message
            //!
            uint32_t get_message_size() const;

            //! @cond nodoxygen
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

        // AuxiliaryVideoStreamDescriptor public members:
        uint8_t aux_video_codedstreamtype = 0;  //!< 8 bits, compression coding type
        //!
        //! From ISO/IEC 13818-1, clause 2.6.75
        //! si_rbsp() - Supplemental information RBSP as defined in ISO/IEC 23002-3. It shall contain
        //! at least one auxiliary video supplemental information (AVSI) message (also defined in ISO/IEC 23002-3).
        //! The type of auxiliary video is inferred from si_rbsp(). The total size of si_rbsp()
        //! shall not exceed 254 bytes.
        std::vector<si_message_type> si_messages {};

        //!
        //! Default constructor.
        //!
        AuxiliaryVideoStreamDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AuxiliaryVideoStreamDescriptor(DuckContext& duck, const Descriptor& bin);

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
