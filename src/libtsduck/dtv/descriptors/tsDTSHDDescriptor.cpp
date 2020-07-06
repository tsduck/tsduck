//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsDTSHDDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    substream_core(),
    substream_0(),
    substream_1(),
    substream_2(),
    substream_3(),
    additional_info()
{
}

ts::DTSHDDescriptor::DTSHDDescriptor(DuckContext& duck, const Descriptor& desc) :
    DTSHDDescriptor()
{
    deserialize(duck, desc);
}

void ts::DTSHDDescriptor::clearContent()
{
    substream_core.clear();
    substream_0.clear();
    substream_1.clear();
    substream_2.clear();
    substream_3.clear();
    additional_info.clear();
}

ts::DTSHDDescriptor::SubstreamInfo::SubstreamInfo() :
    channel_count(0),
    LFE(false),
    sampling_frequency(0),
    sample_resolution(false),
    asset_info()
{
}

ts::DTSHDDescriptor::AssetInfo::AssetInfo() :
    asset_construction(0),
    vbr(false),
    post_encode_br_scaling(false),
    bit_rate(0),
    component_type(),
    ISO_639_language_code()
{
}


//----------------------------------------------------------------------------
// Reset the content of this descriptor object.
//----------------------------------------------------------------------------

void ts::DTSHDDescriptor::reset()
{
    substream_core.clear();
    substream_0.clear();
    substream_1.clear();
    substream_2.clear();
    substream_3.clear();
    additional_info.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DTSHDDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    bbp->appendUInt8((substream_core.set() ? 0x80 : 0x00) |
                     (substream_0.set() ? 0x40 : 0x00) |
                     (substream_1.set() ? 0x20 : 0x00) |
                     (substream_2.set() ? 0x10 : 0x00) |
                     (substream_3.set() ? 0x08 : 0x00) |
                     0x07);
    SerializeSubstreamInfo(substream_core, *bbp);
    SerializeSubstreamInfo(substream_0, *bbp);
    SerializeSubstreamInfo(substream_1, *bbp);
    SerializeSubstreamInfo(substream_2, *bbp);
    SerializeSubstreamInfo(substream_3, *bbp);
    bbp->append(additional_info);
    serializeEnd(desc, bbp);
}

void ts::DTSHDDescriptor::SerializeSubstreamInfo(const Variable<SubstreamInfo>& info, ByteBlock& bb)
{
    if (info.set()) {
        const SubstreamInfo& si(info.value());

        // Place-holder for length field.
        const size_t len_index = bb.size();
        bb.enlarge(1);

        // There must be 1 to 8 asset_info.
        if (si.asset_info.empty() || si.asset_info.size() > 8) {
            // Invalid number of asset_info, enlarge the data too much
            // to ensure that the binary descriptor will be invalidated.
            bb.enlarge(MAX_DESCRIPTOR_SIZE);
        }
        else {
            // Serialize content.
            bb.appendUInt8(uint8_t((si.asset_info.size() - 1) << 5) | (si.channel_count & 0x1F));
            bb.appendUInt8((si.LFE ? 0x80 : 0x00) |
                           uint8_t((si.sampling_frequency & 0x0F) << 3) |
                           (si.sample_resolution ? 0x07 : 0x03));

            for (size_t i = 0; i < si.asset_info.size(); ++i) {
                const AssetInfo& ai(si.asset_info[i]);
                const bool language_code_flag = ai.ISO_639_language_code.set() && ai.ISO_639_language_code.value().size() == 3;
                bb.appendUInt8(uint8_t(ai.asset_construction << 3) |
                               (ai.vbr ? 0x04 : 0x00) |
                               (ai.post_encode_br_scaling ? 0x02 : 0x00) |
                               (ai.component_type.set() ? 0x01 : 0x00));
                bb.appendUInt16((language_code_flag ? 0x8000 : 0x0000) |
                                uint16_t((ai.bit_rate & 0x1FFF) << 2) |
                                0x0003);
                if (ai.component_type.set()) {
                    bb.appendUInt8(ai.component_type.value());
                }
                if (language_code_flag) {
                    SerializeLanguageCode(bb, ai.ISO_639_language_code.value());
                }
            }
        }

        // Update length field.
        bb[len_index] = uint8_t(bb.size() - len_index - 1);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DTSHDDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    reset();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 2 && data[0] == MY_EDID;

    if (_is_valid) {
        const uint8_t flags = data[1];
        data += 2; size -= 2;
        _is_valid =
            DeserializeSubstreamInfo(substream_core, (flags & 0x80) != 0, data, size) &&
            DeserializeSubstreamInfo(substream_0, (flags & 0x40) != 0, data, size) &&
            DeserializeSubstreamInfo(substream_1, (flags & 0x20) != 0, data, size) &&
            DeserializeSubstreamInfo(substream_2, (flags & 0x10) != 0, data, size) &&
            DeserializeSubstreamInfo(substream_3, (flags & 0x08) != 0, data, size);
        if (_is_valid) {
            additional_info.copy(data, size);
        }
    }
}

bool ts::DTSHDDescriptor::DeserializeSubstreamInfo(Variable<SubstreamInfo>& info, bool present, const uint8_t*& data, size_t& size)
{
    if (!present) {
        // Substream info not present
        info.clear();
        return true;
    }
    else {
        // Substream info is present, deserialize it.
        info = SubstreamInfo();
        SubstreamInfo& si(info.value());

        // Check required size.
        if (size < 3 || size < 1 + size_t(data[0]) || data[0] < 2) {
            return false;
        }

        // Immediately update size, use length to check the deserialization.
        size_t length = data[0] - 2;
        size_t num_assets = size_t((data[1] >> 5) & 0x07) + 1;
        si.channel_count = data[1] & 0x1F;
        si.LFE = (data[2] & 0x80) != 0;
        si.sampling_frequency = (data[2] >> 3) & 0x0F;
        si.sample_resolution = (data[2] & 0x40) != 0;
        data += 3;
        size -= length + 3;

        // Deserialize all asset info.
        while (num_assets > 0 && length >= 3) {
            --num_assets;

            // Add a new asset info.
            si.asset_info.resize(si.asset_info.size() + 1);
            AssetInfo& ai(si.asset_info.back());

            ai.asset_construction = (data[0] >> 3) & 0x1F;
            ai.vbr = (data[0] & 0x04) != 0;
            ai.post_encode_br_scaling = (data[0] & 0x02) != 0;
            const bool component_type_flag = (data[0] & 0x01) != 0;
            const bool language_code_flag = (data[1] & 0x80) != 0;
            ai.bit_rate = (GetUInt16(data + 1) >> 2) & 0x1FFF;
            data += 3; length -= 3;

            if (component_type_flag) {
                if (length < 1) {
                    return false;
                }
                else {
                    ai.component_type = data[0];
                    data++; length--;
                }
            }

            if (language_code_flag) {
                if (length < 3) {
                    return false;
                }
                else {
                    ai.ISO_639_language_code = DeserializeLanguageCode(data);
                    data += 3; length -= 3;
                }
            }
        }

        // Check that everything was deserialized.
        return num_assets == 0 && length == 0;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DTSHDDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    if (size >= 1) {
        const uint8_t flags = data[0];
        data++; size--;
        if (DisplaySubstreamInfo(display, (flags & 0x80) != 0, indent, u"core", data, size) &&
            DisplaySubstreamInfo(display, (flags & 0x40) != 0, indent, u"0", data, size) &&
            DisplaySubstreamInfo(display, (flags & 0x20) != 0, indent, u"1", data, size) &&
            DisplaySubstreamInfo(display, (flags & 0x10) != 0, indent, u"2", data, size) &&
            DisplaySubstreamInfo(display, (flags & 0x08) != 0, indent, u"3", data, size))
        {
            display.displayPrivateData(u"Additional information", data, size, indent);
            data += size; size = 0;
        }
    }
    display.displayExtraData(data, size, indent);
}

bool ts::DTSHDDescriptor::DisplaySubstreamInfo(TablesDisplay& display, bool present, int indent, const UString& name, const uint8_t*& data, size_t& size)
{
    // Check presence and required size.
    if (!present) {
        // Nothing to display, not an error.
        return true;
    }
    else if (size < 3 || size < 1 + size_t(data[0]) || data[0] < 2) {
        return false;
    }

    // Immediately update size, use length to check the deserialization.
    size_t length = data[0] - 2;
    size_t num_assets = size_t((data[1] >> 5) & 0x07) + 1;
    const uint8_t channel_count = data[1] & 0x1F;
    const bool LFE = (data[2] & 0x80) != 0;
    const uint8_t sampling_frequency = (data[2] >> 3) & 0x0F;
    const bool sample_resolution = (data[2] & 0x40) != 0;
    data += 3;
    size -= length + 3;

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    strm << margin << "Substream " << name << ":" << std::endl
         << margin << UString::Format(u"  Asset count: %d, channel count: %d", {num_assets, channel_count}) << std::endl
         << margin << UString::Format(u"  Low Frequency Effects (LFE): %s", {LFE}) << std::endl
         << margin << UString::Format(u"  Sampling frequency: %s", {NameFromSection(u"DTSHDSamplingFrequency", sampling_frequency, names::VALUE)}) << std::endl
         << margin << UString::Format(u"  Sample resultion > 16 bits: %s", {sample_resolution}) << std::endl;

    // Deserialize all asset info.
    int asset_index = -1;
    while (num_assets > 0 && length >= 3) {
        --num_assets;
        ++asset_index;

        const uint16_t asset_construction = (data[0] >> 3) & 0x1F;
        const bool vbr = (data[0] & 0x04) != 0;
        const bool post_encode_br_scaling = (data[0] & 0x02) != 0;
        const bool component_type_flag = (data[0] & 0x01) != 0;
        const bool language_code_flag = (data[1] & 0x80) != 0;
        const uint16_t bit_rate = (GetUInt16(data + 1) >> 2) & 0x1FFF;
        data += 3; length -= 3;

        strm << margin << UString::Format(u"  Asset %d:", {asset_index}) << std::endl
             << margin << UString::Format(u"    Construction: %s", {NameFromSection(u"DTSHDAssetConstruction", asset_construction + (asset_index == 0 ? 0 : 0x0100), names::VALUE)}) << std::endl
             << margin << UString::Format(u"    VBR: %s, post-encode bitrate scaling: %s", {vbr, post_encode_br_scaling}) << std::endl
             << margin << "    Bit rate: ";

        if (bit_rate == 0) {
            strm << "unknown";
        }
        else if (post_encode_br_scaling) {
            strm << (bit_rate >> 3) << "." << ((10 * (bit_rate & 0x07)) / 8) << " kb/s";
        }
        else {
            strm << bit_rate << " kb/s";
        }
        strm << std::endl;

        if (component_type_flag) {
            if (length < 1) {
                return false;
            }
            else {
                strm << margin << UString::Format(u"    Component type: 0x%X", {data[0]}) << std::endl
                     << margin << UString::Format(u"      %s", {(data[0] & 0x40) != 0 ? u"Full service" : u"Combined service"}) << std::endl
                     << margin << UString::Format(u"      Service type: %s", {NameFromSection(u"DTSHDServiceType", (data[0] >> 3) & 0x07, names::VALUE)}) << std::endl
                     << margin << UString::Format(u"      Number of channels: %s", {NameFromSection(u"DTSHDNumberOfChannels", data[0] & 0x07, names::VALUE)}) << std::endl;
                data++; length--;
            }
        }

        if (language_code_flag) {
            if (length < 3) {
                return false;
            }
            else {
                strm << margin << UString::Format(u"    Language code: \"%s\"", {DeserializeLanguageCode(data)}) << std::endl;
                data += 3; length -= 3;
            }
        }
    }

    // Check that everything was deserialized.
    return num_assets == 0 && length == 0;
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

void ts::DTSHDDescriptor::SubstreamInfoToXML(const Variable<SubstreamInfo>& info, const UString& name, xml::Element* parent)
{
    if (info.set()) {
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
            xai->setAttribute(u"ISO_639_language_code", ai.ISO_639_language_code.value(u""), true);
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

bool ts::DTSHDDescriptor::SubstreamInfoFromXML(Variable<SubstreamInfo>& info, const UString& name, const xml::Element* parent)
{
    // Get at most one element of this name.
    xml::ElementVector children;
    if (!parent->getChildren(children, name, 0, 1)) {
        return false;
    }

    if (children.empty()) {
        // Element not present
        info.clear();
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
            x->getIntAttribute<uint8_t>(si.channel_count, u"channel_count", true, 0, 0, 0x1F) &&
            x->getBoolAttribute(si.LFE, u"LFE", true) &&
            x->getIntAttribute<uint8_t>(si.sampling_frequency, u"sampling_frequency", true, 0, 0, 0x0F) &&
            x->getBoolAttribute(si.sample_resolution, u"sample_resolution", true) &&
            x->getChildren(xassets, u"asset_info", 1, 8);

        for (size_t i = 0; valid && i < xassets.size(); ++i) {
            si.asset_info.resize(si.asset_info.size() + 1);
            AssetInfo& ai(si.asset_info.back());
            valid =
                xassets[i]->getIntAttribute<uint8_t>(ai.asset_construction, u"asset_construction", true, 0, 0, 0x1F) &&
                xassets[i]->getBoolAttribute(ai.vbr, u"vbr", true) &&
                xassets[i]->getBoolAttribute(ai.post_encode_br_scaling, u"post_encode_br_scaling", true) &&
                xassets[i]->getIntAttribute<uint16_t>(ai.bit_rate, u"bit_rate", true, 0, 0, 0x1FFF) &&
                xassets[i]->getOptionalIntAttribute<uint8_t>(ai.component_type, u"component_type") &&
                xassets[i]->getOptionalAttribute(ai.ISO_639_language_code, u"ISO_639_language_code", 3, 3);
        }
        return valid;
    }
}
