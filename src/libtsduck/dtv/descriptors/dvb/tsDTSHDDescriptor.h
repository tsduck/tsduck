//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DTS_HD_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a DTS_HD_descriptor.
    //! @see ETSI EN 300 468, G.3.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DTSHDDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Asset information.
        //!
        class TSDUCKDLL AssetInfo
        {
        public:
            AssetInfo() = default;                                 //!< Default constructor.
            uint8_t                asset_construction = 0;         //!< 5 bits, bit mask.
            bool                   vbr = false;                    //!< Variable bitrate
            bool                   post_encode_br_scaling = false; //!< 13-bit value in bit_rate is encoded as 10.3 bits.
            uint16_t               bit_rate = 0;                   //!< 13 bits, bitrate code.
            std::optional<uint8_t> component_type {};              //!< Optional component type.
            std::optional<UString> ISO_639_language_code {};       //!< Optional 3-character language code.
        };

        //!
        //! Substream information.
        //!
        class TSDUCKDLL SubstreamInfo
        {
        public:
            SubstreamInfo() = default;                        //!< Default constructor.
            uint8_t                channel_count = 0;         //!< 5 bits, number of channels.
            bool                   LFE = false;               //!< LFE (Low Frequency Effects) present.
            uint8_t                sampling_frequency = 0;    //!< 4 bits, sampling frequency code.
            bool                   sample_resolution = false; //!< Sample resolution is more than 16 bits when true.
            std::vector<AssetInfo> asset_info {};             //!< From 1 to 8 asset_info
        };

        // DTSHDDescriptor public members:
        std::optional<SubstreamInfo> substream_core {};   //!< Optional core substream description.
        std::optional<SubstreamInfo> substream_0 {};      //!< Optional substream 0 description.
        std::optional<SubstreamInfo> substream_1 {};      //!< Optional substream 1 description.
        std::optional<SubstreamInfo> substream_2 {};      //!< Optional substream 2 description.
        std::optional<SubstreamInfo> substream_3 {};      //!< Optional substream 3 description.
        ByteBlock                    additional_info {};  //!< Reserved for future use.

        //!
        //! Default constructor.
        //!
        DTSHDDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DTSHDDescriptor(DuckContext& duck, const Descriptor& bin);

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
        // Conversions of substrean info structures.
        static void SerializeSubstreamInfo(const std::optional<SubstreamInfo>&, PSIBuffer&);
        static void DeserializeSubstreamInfo(std::optional<SubstreamInfo>&, bool present, PSIBuffer&);
        static void DisplaySubstreamInfo(TablesDisplay& display, bool present, const UString& margin, const UString& name, PSIBuffer&);
        static void SubstreamInfoToXML(const std::optional<SubstreamInfo>&, const UString& name, xml::Element* parent);
        static bool SubstreamInfoFromXML(std::optional<SubstreamInfo>&, const UString& name, const xml::Element* parent);
    };
}
