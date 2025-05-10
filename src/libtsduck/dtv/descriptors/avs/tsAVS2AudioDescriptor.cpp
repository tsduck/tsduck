//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAVS2AudioDescriptor.h"
#include "tsAVS3AudioDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsDVBCharTableUTF16.h"

#define MY_XML_NAME u"AVS2_audio_descriptor"
#define MY_CLASS    ts::AVS2AudioDescriptor
#define MY_EDID     ts::EDID::PrivateMPEG(ts::DID_AVS2_AUDIO, ts::REGID_AVSAudio)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AVS2AudioDescriptor::AVS2AudioDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::AVS2AudioDescriptor::clearContent()
{
    num_channels = 0;
    sample_rate_index = 0;
    description.reset();
    language.reset();
    avs_version.reset();
    additional_info.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AVS2AudioDescriptor::avs_version_info::serialize(PSIBuffer& buf) const
{
    buf.putBits(audio_codec_id, 4);
    buf.putBit(0);  // anc_data_index
    buf.putBits(coding_profile, 3);
    if (audio_codec_id == 0) {
        buf.putBits(bitrate_index, 4);
        buf.putBits(bitstream_type, 1);
        buf.putBits(0xFF, 3);
        buf.putUInt16(raw_frame_length);
    }
    buf.putBits(resolution, 2);
    buf.putBits(0xFF, 6);
}

void ts::AVS2AudioDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(num_channels);
    buf.putBits(sample_rate_index, 4);
    buf.putBit(avs_version.has_value());  // avs_version_flag
    buf.putBit(description.has_value());  // text_present_flag
    buf.putBit(language.has_value());     // language_present_flag
    buf.putBits(0x00, 1);
    if (description.has_value()) {
        buf.putStringWithByteLength(description.value(), 0, NPOS, &ts::DVBCharTableUTF16::RAW_UTF_16);
    }
    if (language.has_value()) {
        buf.putLanguageCode(language.value());
    }
    if (avs_version.has_value()) {
        avs_version.value().serialize(buf);
    }
    buf.putBytes(additional_info);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AVS2AudioDescriptor::avs_version_info::deserialize(PSIBuffer& buf)
{
    audio_codec_id = buf.getBits<uint8_t>(4);
    buf.skipBits(1);   // anc_data_index
    coding_profile = buf.getBits<uint8_t>(3);
    if (audio_codec_id == 0) {
        bitrate_index = buf.getBits<uint8_t>(4);
        bitstream_type = buf.getBit();
        buf.skipBits(3);
        raw_frame_length = buf.getUInt16();
    }
    resolution = buf.getBits<uint8_t>(2);
    buf.skipBits(6);
}

void ts::AVS2AudioDescriptor::deserializePayload(PSIBuffer& buf)
{
    num_channels = buf.getUInt8();
    buf.getBits(sample_rate_index, 4);
    const bool avs_version_flag = buf.getBool();
    const bool text_present_flag = buf.getBool();
    const bool language_present_flag = buf.getBool();
    buf.skipBits(1);
    if (text_present_flag) {
        description = buf.getStringWithByteLength(&ts::DVBCharTableUTF16::RAW_UTF_16);
    }
    if (language_present_flag) {
        language = buf.getLanguageCode();
    }
    if (avs_version_flag) {
        avs_version_info version(buf);
        avs_version = version;
    }
    buf.getBytes(additional_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AVS2AudioDescriptor::avs_version_info::display(TablesDisplay& disp, PSIBuffer& buf, const ts::UString& margin, uint8_t _num_channels)
{
    uint8_t _audio_codec_id = buf.getBits<uint8_t>(4);
    disp << margin << "Codec id: " << DataName(MY_XML_NAME, u"audio_codec_id", _audio_codec_id, NamesFlags::NAME_VALUE | NamesFlags::DECIMAL);
    buf.skipBits(1);   // anc_data_index
    disp << ", Coding profile: " << DataName(MY_XML_NAME, u"coding_profile", buf.getBits<uint8_t>(3), NamesFlags::NAME_VALUE | NamesFlags::DECIMAL);
    uint8_t _bitrate_index = 0, _bitstream_type = 0;
    uint16_t _raw_frame_length = 0;
    if (_audio_codec_id == AVS3AudioDescriptor::General_Coding) {
        _bitrate_index = buf.getBits<uint8_t>(4);
        _bitstream_type = buf.getBit();
        buf.skipReservedBits(3);
        _raw_frame_length = buf.getUInt16();
    }
    disp << ", Resolution: " << DataName(MY_XML_NAME, u"resolution", buf.getBits<uint8_t>(2), NamesFlags::NAME_VALUE | NamesFlags::DECIMAL) << std::endl;
    buf.skipReservedBits(6);
    if (_audio_codec_id == AVS3AudioDescriptor::General_Coding) {
        disp << margin << "Bitrate: " << DataName(MY_XML_NAME, u"bitrate_index", _bitrate_index, NamesFlags::NAME_VALUE);
        disp << ", Bitstream type: " << DataName(MY_XML_NAME, u"bitstream_type", _bitstream_type, NamesFlags::NAME_VALUE | NamesFlags::DECIMAL);
        disp << ", Raw frame length: " << _raw_frame_length << std::endl;
    }
}

void ts::AVS2AudioDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        uint8_t _num_channels = buf.getUInt8();
        disp << margin << "Channels: " << uint16_t(_num_channels);
        disp << ", Sample rate (Hz): " << DataName(MY_XML_NAME, u"sample_rate_index", buf.getBits<uint8_t>(4), NamesFlags::NAME_VALUE | NamesFlags::DECIMAL) << std::endl;
        const bool avs_version_flag = buf.getBool();
        const bool text_present_flag = buf.getBool();
        const bool language_present_flag = buf.getBool();
        buf.skipReservedBits(1, 0);
        if (text_present_flag) {
            disp << margin << "Description: " << buf.getStringWithByteLength(&ts::DVBCharTableUTF16::RAW_UTF_16) << std::endl;
        }
        if (language_present_flag) {
            disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        }
        if (avs_version_flag) {
            avs_version_info::display(disp, buf, margin, _num_channels);
        }
        disp.displayPrivateData(u"Additional information", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// Thread-safe init-safe static data patterns.
//----------------------------------------------------------------------------

const ts::Names& ts::AVS2AudioDescriptor::CodingProfiles()
{
    static const Names data({
        {u"basic",  0},
        {u"object", 1},
    });
    return data;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AVS2AudioDescriptor::avs_version_info::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"audio_codec_id", audio_codec_id);
    root->setEnumAttribute(CodingProfiles(), u"coding_profile", coding_profile);
    root->setEnumAttribute(AVS3AudioDescriptor::Resolutions(), u"resolution", resolution);
    if (audio_codec_id == AVS3AudioDescriptor::General_Coding) {
        root->setIntAttribute(u"bitrate_index", bitrate_index, true);
        root->setEnumAttribute(AVS3AudioDescriptor::GeneralBitstreamTypes(), u"bitstream_type", bitstream_type);
        root->setIntAttribute(u"raw_frame_length", raw_frame_length);
    }
}

void ts::AVS2AudioDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"num_channels", num_channels);
    root->setIntAttribute(u"sample_rate_index", sample_rate_index);
    if (description.has_value()) {
        root->setAttribute(u"description", description.value());
    }
    if (language.has_value()) {
        root->setAttribute(u"language", language.value());
    }
    if (avs_version.has_value()) {
        avs_version.value().toXML(root->addElement(u"version_info"));
    }
    root->addHexaTextChild(u"additional_info", additional_info, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AVS2AudioDescriptor::avs_version_info::fromXML(const xml::Element* element)
{
    bool ok = element->getIntAttribute(audio_codec_id, u"audio_codec_id", true, 0, 0, 15) &&
              element->getEnumAttribute(coding_profile, CodingProfiles(), u"coding_profile", true) &&
              element->getEnumAttribute(resolution, AVS3AudioDescriptor::Resolutions(), u"resolution", true);
    if (ok && (audio_codec_id == AVS3AudioDescriptor::General_Coding)) {
        ok = element->getIntAttribute(bitrate_index, u"bitrate_index", true, 0, 0, 0x0f) &&
             element->getEnumAttribute(bitstream_type, AVS3AudioDescriptor::GeneralBitstreamTypes(), u"bitstream_type", true) &&
             element->getIntAttribute(raw_frame_length, u"raw_frame_length", true);
    }
    if ((audio_codec_id != AVS3AudioDescriptor::General_Coding) && (element->hasAttribute(u"bitrate_index") || element->hasAttribute(u"bitstream_type") || element->hasAttribute(u"raw_frame_length"))) {
        element->report().warning(u"bitrate_index, bitstream_type and raw_frame_length attributes are only applicable for audio_codec_id=0, in <%s>, line %d", element->name(), element->lineNumber());
    }
    return ok;
}

bool ts::AVS2AudioDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector version_info;
    bool ok = element->getIntAttribute(num_channels, u"num_channels", true) &&
              element->getIntAttribute(sample_rate_index, u"sample_rate_index", true, 0, 0, 12) &&
              element->getOptionalAttribute(description, u"description", 0, 255) &&
              element->getOptionalAttribute(language, u"language", 3, 3) &&
              element->getChildren(version_info, u"version_info", 0, 1) &&
              element->getHexaTextChild(additional_info, u"additional_info");
    if (!version_info.empty()) {
        avs_version_info vi;
        if (vi.fromXML(version_info[0])) {
            avs_version = vi;
        }
        else {
            ok = false;
        }
    }
    return ok;
}
