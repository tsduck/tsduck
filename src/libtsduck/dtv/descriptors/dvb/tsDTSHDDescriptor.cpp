//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDTSHDDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DTS_HD_descriptor"
#define MY_CLASS ts::DTSHDDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_DTS_HD_AUDIO
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTSHDDescriptor::DTSHDDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DTSHDDescriptor::DTSHDDescriptor(DuckContext& duck, const Descriptor& desc) :
    DTSHDDescriptor()
{
    deserialize(duck, desc);
}

void ts::DTSHDDescriptor::clearContent()
{
    substream_core.reset();
    substream_0.reset();
    substream_1.reset();
    substream_2.reset();
    substream_3.reset();
    additional_info.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::DTSHDDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DTSHDDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(substream_core.has_value());
    buf.putBit(substream_0.has_value());
    buf.putBit(substream_1.has_value());
    buf.putBit(substream_2.has_value());
    buf.putBit(substream_3.has_value());
    buf.putBits(0xFF, 3);

    SerializeSubstreamInfo(substream_core, buf);
    SerializeSubstreamInfo(substream_0, buf);
    SerializeSubstreamInfo(substream_1, buf);
    SerializeSubstreamInfo(substream_2, buf);
    SerializeSubstreamInfo(substream_3, buf);
    buf.putBytes(additional_info);
}

void ts::DTSHDDescriptor::SerializeSubstreamInfo(const std::optional<SubstreamInfo>& info, PSIBuffer& buf)
{
    if (info.has_value()) {
        const SubstreamInfo& si(info.value());
        buf.pushWriteSequenceWithLeadingLength(8);  // start write sequence

        // There must be 1 to 8 asset_info.
        if (si.asset_info.empty() || si.asset_info.size() > 8) {
            buf.setUserError();
        }
        else {
            // Serialize content.
            buf.putBits(si.asset_info.size() - 1, 3);
            buf.putBits(si.channel_count, 5);
            buf.putBit(si.LFE);
            buf.putBits(si.sampling_frequency, 4);
            buf.putBit(si.sample_resolution);
            buf.putBits(0xFF, 2);
            for (size_t i = 0; i < si.asset_info.size(); ++i) {
                const AssetInfo& ai(si.asset_info[i]);
                buf.putBits(ai.asset_construction, 5);
                buf.putBit(ai.vbr);
                buf.putBit(ai.post_encode_br_scaling);
                buf.putBit(ai.component_type.has_value());
                buf.putBit(ai.ISO_639_language_code.has_value());
                buf.putBits(ai.bit_rate, 13);
                buf.putBits(0xFF, 2);
                if (ai.component_type.has_value()) {
                    buf.putUInt8(ai.component_type.value());
                }
                if (ai.ISO_639_language_code.has_value()) {
                    buf.putLanguageCode(ai.ISO_639_language_code.value());
                }
            }
        }
        buf.popState();  // end write sequence
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DTSHDDescriptor::deserializePayload(PSIBuffer& buf)
{
    const bool substream_core_flag = buf.getBool();
    const bool substream_0_flag = buf.getBool();
    const bool substream_1_flag = buf.getBool();
    const bool substream_2_flag = buf.getBool();
    const bool substream_3_flag = buf.getBool();
    buf.skipBits(3);

    DeserializeSubstreamInfo(substream_core, substream_core_flag, buf);
    DeserializeSubstreamInfo(substream_0, substream_0_flag, buf);
    DeserializeSubstreamInfo(substream_1, substream_1_flag, buf);
    DeserializeSubstreamInfo(substream_2, substream_2_flag, buf);
    DeserializeSubstreamInfo(substream_3, substream_3_flag, buf);
    buf.getBytes(additional_info);
}

void ts::DTSHDDescriptor::DeserializeSubstreamInfo(std::optional<SubstreamInfo>& info, bool present, PSIBuffer& buf)
{
    if (present) {
        info = SubstreamInfo();
        SubstreamInfo& si(info.value());
        buf.pushReadSizeFromLength(8); // start read sequence

        const size_t num_assets = buf.getBits<size_t>(3) + 1;
        buf.getBits(si.channel_count, 5);
        si.LFE = buf.getBool();
        buf.getBits(si.sampling_frequency, 4);
        si.sample_resolution = buf.getBool();
        buf.skipBits(2);

        // Deserialize all asset info.
        while (buf.canRead()) {

            // Add a new asset info.
            si.asset_info.resize(si.asset_info.size() + 1);
            AssetInfo& ai(si.asset_info.back());

            buf.getBits(ai.asset_construction, 5);
            ai.vbr = buf.getBool();
            ai.post_encode_br_scaling = buf.getBool();
            const bool component_type_flag = buf.getBool();
            const bool language_code_flag = buf.getBool();
            buf.getBits(ai.bit_rate, 13);
            buf.skipBits(2);
            if (component_type_flag) {
                ai.component_type = buf.getUInt8();
            }
            if (language_code_flag) {
                ai.ISO_639_language_code = buf.getLanguageCode();
            }
        }

        // Check that the number of assets matches
        if (si.asset_info.size() != num_assets) {
            buf.setUserError();
        }
        buf.popState();  // end read sequence
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DTSHDDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    const bool substream_core_flag = buf.getBool();
    const bool substream_0_flag = buf.getBool();
    const bool substream_1_flag = buf.getBool();
    const bool substream_2_flag = buf.getBool();
    const bool substream_3_flag = buf.getBool();
    buf.skipBits(3);

    DisplaySubstreamInfo(disp, substream_core_flag, margin, u"core", buf);
    DisplaySubstreamInfo(disp, substream_0_flag, margin, u"0", buf);
    DisplaySubstreamInfo(disp, substream_1_flag, margin, u"1", buf);
    DisplaySubstreamInfo(disp, substream_2_flag, margin, u"2", buf);
    DisplaySubstreamInfo(disp, substream_3_flag, margin, u"3", buf);
    disp.displayPrivateData(u"Additional information", buf, NPOS, margin);
}

void ts::DTSHDDescriptor::DisplaySubstreamInfo(TablesDisplay& disp, bool present, const UString& margin, const UString& name, PSIBuffer& buf)
{
    if (present && buf.canReadBytes(3)) {
        disp << margin << "Substream " << name << ":" << std::endl;
        buf.pushReadSizeFromLength(8); // start read sequence
        const size_t num_assets = buf.getBits<size_t>(3) + 1;
        disp << margin << UString::Format(u"  Asset count: %d, channel count: %d", {num_assets, buf.getBits<uint8_t>(5)}) << std::endl;
        disp << margin << UString::Format(u"  Low Frequency Effects (LFE): %s", {buf.getBool()}) << std::endl;
        disp << margin << UString::Format(u"  Sampling frequency: %s", {DataName(MY_XML_NAME, u"SamplingFrequency", buf.getBits<uint8_t>(4), NamesFlags::VALUE)}) << std::endl;
        disp << margin << UString::Format(u"  Sample resolution > 16 bits: %s", {buf.getBool()}) << std::endl;
        buf.skipBits(2);

        // Display all asset info.
        for (size_t asset_index = 0; asset_index < num_assets && buf.canReadBytes(3); ++asset_index) {
            disp << margin << UString::Format(u"  Asset %d:", {asset_index}) << std::endl;
            disp << margin << "    Construction: "
                 << DataName(MY_XML_NAME, u"AssetConstruction", buf.getBits<uint8_t>(5) + (asset_index == 0 ? 0 : 0x0100), NamesFlags::VALUE)
                 << std::endl;
            disp << margin << UString::Format(u"    VBR: %s", {buf.getBool()});
            const bool br_scaling = buf.getBool();
            disp << UString::Format(u", post-encode bitrate scaling: %s", {br_scaling}) << std::endl;
            const bool component_type_flag = buf.getBool();
            const bool language_code_flag = buf.getBool();
            const uint16_t bit_rate = buf.getBits<uint16_t>(13);
            buf.skipBits(2);

            disp << margin << "    Bit rate: ";
            if (bit_rate == 0) {
                disp << "unknown";
            }
            else if (br_scaling) {
                disp << (bit_rate >> 3) << "." << ((10 * (bit_rate & 0x07)) / 8) << " kb/s";
            }
            else {
                disp << bit_rate << " kb/s";
            }
            disp << std::endl;

            if (component_type_flag && buf.canReadBytes(1)) {
                const uint8_t type = buf.getUInt8();
                disp << margin << UString::Format(u"    Component type: 0x%X", {type}) << std::endl;
                disp << margin << UString::Format(u"      %s", {(type & 0x40) != 0 ? u"Full service" : u"Combined service"}) << std::endl;
                disp << margin << UString::Format(u"      Service type: %s", {DataName(MY_XML_NAME, u"ServiceType", (type >> 3) & 0x07, NamesFlags::VALUE)}) << std::endl;
                disp << margin << UString::Format(u"      Number of channels: %s", {DataName(MY_XML_NAME, u"NumberOfChannels", type & 0x07, NamesFlags::VALUE)}) << std::endl;
            }
            if (language_code_flag && buf.canReadBytes(3)) {
                disp << margin << "    Language code: \"" << buf.getLanguageCode() << "\"" << std::endl;
            }
        }
        disp.displayPrivateData(u"Extraneous substream data", buf, NPOS, margin + u"  ");
        buf.popState();  // end read sequence
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DTSHDDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    SubstreamInfoToXML(substream_core, u"substream_core", root);
    SubstreamInfoToXML(substream_0, u"substream_0", root);
    SubstreamInfoToXML(substream_1, u"substream_1", root);
    SubstreamInfoToXML(substream_2, u"substream_2", root);
    SubstreamInfoToXML(substream_3, u"substream_3", root);
    if (!additional_info.empty()) {
        root->addHexaTextChild(u"additional_info", additional_info);
    }
}

void ts::DTSHDDescriptor::SubstreamInfoToXML(const std::optional<SubstreamInfo>& info, const UString& name, xml::Element* parent)
{
    if (info.has_value()) {
        const SubstreamInfo& si(info.value());
        xml::Element* e = parent->addElement(name);
        e->setIntAttribute(u"channel_count", uint8_t(si.channel_count & 0x1F), false);
        e->setBoolAttribute(u"LFE", si.LFE);
        e->setIntAttribute(u"sampling_frequency", uint8_t(si.sampling_frequency & 0x0F), true);
        e->setBoolAttribute(u"sample_resolution", si.sample_resolution);
        for (size_t i = 0; i < si.asset_info.size() && i < 8; ++i) {
            const AssetInfo& ai(si.asset_info[i]);
            xml::Element* xai = e->addElement(u"asset_info");
            xai->setIntAttribute(u"asset_construction", uint8_t(ai.asset_construction & 0x1F), true);
            xai->setBoolAttribute(u"vbr", ai.vbr);
            xai->setBoolAttribute(u"post_encode_br_scaling", ai.post_encode_br_scaling);
            xai->setIntAttribute(u"bit_rate", uint16_t(ai.bit_rate & 0x1FFF), false);
            xai->setOptionalIntAttribute(u"component_type", ai.component_type, true);
            xai->setAttribute(u"ISO_639_language_code", ai.ISO_639_language_code.value_or(u""), true);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DTSHDDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return SubstreamInfoFromXML(substream_core, u"substream_core", element) &&
           SubstreamInfoFromXML(substream_0, u"substream_0", element) &&
           SubstreamInfoFromXML(substream_1, u"substream_1", element) &&
           SubstreamInfoFromXML(substream_2, u"substream_2", element) &&
           SubstreamInfoFromXML(substream_3, u"substream_3", element) &&
           element->getHexaTextChild(additional_info, u"additional_info", false);
}

bool ts::DTSHDDescriptor::SubstreamInfoFromXML(std::optional<SubstreamInfo>& info, const UString& name, const xml::Element* parent)
{
    // Get at most one element of this name.
    xml::ElementVector children;
    if (!parent->getChildren(children, name, 0, 1)) {
        return false;
    }

    if (children.empty()) {
        // Element not present
        info.reset();
        return true;
    }
    else {
        // Element present once.
        assert(children.size() == 1);

        info = SubstreamInfo();
        SubstreamInfo& si(info.value());
        const xml::Element* const x = children[0];
        xml::ElementVector xassets;

        bool valid =
            x->getIntAttribute(si.channel_count, u"channel_count", true, 0, 0, 0x1F) &&
            x->getBoolAttribute(si.LFE, u"LFE", true) &&
            x->getIntAttribute(si.sampling_frequency, u"sampling_frequency", true, 0, 0, 0x0F) &&
            x->getBoolAttribute(si.sample_resolution, u"sample_resolution", true) &&
            x->getChildren(xassets, u"asset_info", 1, 8);

        for (size_t i = 0; valid && i < xassets.size(); ++i) {
            si.asset_info.resize(si.asset_info.size() + 1);
            AssetInfo& ai(si.asset_info.back());
            valid =
                xassets[i]->getIntAttribute(ai.asset_construction, u"asset_construction", true, 0, 0, 0x1F) &&
                xassets[i]->getBoolAttribute(ai.vbr, u"vbr", true) &&
                xassets[i]->getBoolAttribute(ai.post_encode_br_scaling, u"post_encode_br_scaling", true) &&
                xassets[i]->getIntAttribute(ai.bit_rate, u"bit_rate", true, 0, 0, 0x1FFF) &&
                xassets[i]->getOptionalIntAttribute(ai.component_type, u"component_type") &&
                xassets[i]->getOptionalAttribute(ai.ISO_639_language_code, u"ISO_639_language_code", 3, 3);
        }
        return valid;
    }
}
