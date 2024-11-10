//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2024, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAVS3AudioDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"AVS3_audio_descriptor"
#define MY_CLASS ts::AVS3AudioDescriptor
#define MY_DID ts::DID_AVS3_AUDIO
#define MY_PDS ts::PDS_AVSAudio
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AVS3AudioDescriptor::AVS3AudioDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

void ts::AVS3AudioDescriptor::clearContent()
{
    audio_codec_id = 0;
    sampling_frequency_index = 0;
    sampling_frequency = 0;
    resolution = 0;
    coding_data = std::monostate {};
    additional_info.clear();
}


//----------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------

uint8_t ts::AVS3AudioDescriptor::fullrate_coding_type::content_type() const
{
    if (channel_num_index.has_value() && num_objects.has_value()) {
        return Mix_signal;
    }
    else if (channel_num_index.has_value()) {
        return Channel_signal;
    }
    else if (num_objects.has_value()) {
        return Object_signal;
    }
    else if (hoa_order.has_value()) {
        return HOA_signal;
    }
    return INVALID_CONTENT_TYPE;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AVS3AudioDescriptor::general_coding_type::serialize(PSIBuffer& buf) const
{
    buf.putBit(0);  // anc_data_index
    buf.putBits(coding_profile, 3);
    buf.putBits(bitrate_index, 4);
    buf.putBits(bitstream_type, 1);
    buf.putBits(channel_number_index, 7);
    buf.putUInt16(raw_frame_length);
}
void ts::AVS3AudioDescriptor::lossless_coding_type::serialize(PSIBuffer& buf, uint8_t _sampling_frequency_index) const
{
    if (_sampling_frequency_index == 0x0F) {
        buf.putUInt24(sampling_frequency);
    }
    buf.putBit(0);  // anc_data_index
    buf.putBits(coding_profile, 3);
    buf.putBits(0xFF, 4);
    buf.putUInt8(channel_number);
}

void ts::AVS3AudioDescriptor::fullrate_coding_type::serialize(PSIBuffer& buf) const
{
    buf.putBits(nn_type, 3);
    buf.putBits(0xFF, 1);
    const uint8_t _content_type = content_type();
    buf.putBits(_content_type, 4);
    switch (_content_type) {
        case Channel_signal:
            buf.putBits(channel_num_index.value_or(0), 7);
            buf.putBits(0xFF, 1);
            break;
        case Object_signal:
            buf.putBits(num_objects.value_or(0), 7);
            buf.putBits(0xFF, 1);
            break;
        case Mix_signal:
            buf.putBits(channel_num_index.value_or(0), 7);
            buf.putBits(0xFF, 1);
            buf.putBits(num_objects.value_or(0), 7);
            buf.putBits(0xFF, 1);
            break;
        case HOA_signal:
            buf.putBits(hoa_order.value_or(0), 4);
            buf.putBits(0xFF, 4);
            break;
        default:
            break;
    }
    buf.putUInt16(total_bitrate);
}

void ts::AVS3AudioDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(audio_codec_id, 4);
    buf.putBits(sampling_frequency_index, 4);
    switch (audio_codec_id) {
        case General_Coding:
            if (std::holds_alternative<general_coding_type>(coding_data)) {
                std::get<general_coding_type>(coding_data).serialize(buf);
            }
            break;
        case Lossless_Coding:
            if (std::holds_alternative<lossless_coding_type>(coding_data)) {
                std::get<lossless_coding_type>(coding_data).serialize(buf, sampling_frequency_index);
            }
            break;
        case Fullrate_Coding:
            if (std::holds_alternative<fullrate_coding_type>(coding_data)) {
                std::get<fullrate_coding_type>(coding_data).serialize(buf);
            }
            break;
        default:
            break;
    }
    buf.putBits(resolution, 2);
    buf.putBits(0xFF, 6);
    buf.putBytes(additional_info);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AVS3AudioDescriptor::general_coding_type::deserialize(PSIBuffer& buf)
{
    buf.skipBits(1);  // anc_data_index
    coding_profile = buf.getBits<uint8_t>(3);
    buf.getBits(bitrate_index, 4);
    bitstream_type = buf.getBits<uint8_t>(1);
    buf.getBits(channel_number_index, 7);
    raw_frame_length = buf.getUInt16();
}

void ts::AVS3AudioDescriptor::lossless_coding_type::deserialize(PSIBuffer& buf, uint8_t _sampling_frequency_index)
{
    if (_sampling_frequency_index == 0x0F) {
        sampling_frequency = buf.getUInt24();
    }
    buf.skipBits(1);  // anc_data_index
    coding_profile = buf.getBits<uint8_t>(3);
    buf.skipBits(4);
    channel_number = buf.getUInt8();
}

void ts::AVS3AudioDescriptor::fullrate_coding_type::deserialize(PSIBuffer& buf)
{
    buf.getBits(nn_type, 3);
    buf.skipBits(1);
    const uint8_t content_type = buf.getBits<uint8_t>(4);
    switch (content_type) {
        case Channel_signal:
            buf.getBits(channel_num_index, 7);
            buf.skipBits(1);
            break;
        case Object_signal:
            buf.getBits(num_objects, 7);
            buf.skipBits(1);
            break;
        case Mix_signal:
            buf.getBits(channel_num_index, 7);
            buf.skipBits(1);
            buf.getBits(num_objects, 7);
            buf.skipBits(1);
            break;
        case HOA_signal:
            buf.getBits(hoa_order, 4);
            buf.skipBits(4);
            break;
        default:
            break;
    }
    total_bitrate = buf.getUInt16();
}

void ts::AVS3AudioDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(audio_codec_id, 4);
    buf.getBits(sampling_frequency_index, 4);

    if (audio_codec_id == General_Coding) {
        general_coding_type gc(buf);
        coding_data = gc;
    }
    else if (audio_codec_id == Lossless_Coding) {
        lossless_coding_type lc(buf, sampling_frequency_index);
        coding_data = lc;
    }
    else if (audio_codec_id == Fullrate_Coding) {
        fullrate_coding_type fc(buf);
        coding_data = fc;
    }
    else {
        coding_data = std::monostate {};
    }
    resolution = buf.getBits<uint8_t>(2);
    buf.skipBits(6);
    buf.getBytes(additional_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AVS3AudioDescriptor::general_coding_type::display(TablesDisplay& disp, const UString& margin)
{
    disp << margin << "General High-rate Coding. Coding Profile: " << DataName(MY_XML_NAME, u"coding_profile", coding_profile, NamesFlags::VALUE);
    disp << ", Bitstream Type: " << GeneralBitstreamTypes.name(bitstream_type, true) << std::endl;
    disp << margin << "  "
         << "Bitrate: " << DataName(MY_XML_NAME, u"channel_bitrate", (channel_number_index << 8) | bitrate_index, NamesFlags::VALUE)
         << ", Raw Frame Length: " << raw_frame_length << std::endl;
}

void ts::AVS3AudioDescriptor::lossless_coding_type::display(TablesDisplay& disp, const UString& margin, uint8_t _sampling_frequency_index)
{
    if (_sampling_frequency_index == 0xF) {
        disp << ", Sampling Frequency (actual): " << sampling_frequency << " Hz" << std::endl;
    }
    else {
        disp << ", Sampling Frequency (index): " << DataName(MY_XML_NAME, u"sampling_frequency_index", _sampling_frequency_index, NamesFlags::VALUE) << std::endl;
    }
    disp << margin << "Lossless Coding. Coding Profile: " << DataName(MY_XML_NAME, u"coding_profile", coding_profile, NamesFlags::VALUE);
    disp << ", channel number: " << int(channel_number) << std::endl;
}

void ts::AVS3AudioDescriptor::fullrate_coding_type::display(TablesDisplay& disp, const UString& margin)
{
    const UString err_msg = u"**ERROR**";
    bool ok = true;
    disp << margin << "General Full-rate Coding. NN Type: " << DataName(MY_XML_NAME, u"nn_type", nn_type, NamesFlags::VALUE) << std::endl;
    disp << margin << "  ";
    switch (content_type()) {
        case Channel_signal:
            disp << "Channel Signal - "
                 << (channel_num_index.has_value() ? DataName(MY_XML_NAME, u"channel_number_idx", channel_num_index.value(), NamesFlags::VALUE) : err_msg);
            break;
        case Object_signal:
            disp << "Object Signal - "
                 << (num_objects.has_value() ? UString::Format(u"number of objects: %d", num_objects.value()) : err_msg);
            break;
        case Mix_signal:
            disp << "Mix Signal - "
                 << (channel_num_index.has_value() ? DataName(MY_XML_NAME, u"channel_number_idx", channel_num_index.value(), NamesFlags::VALUE) : err_msg)
                 << (num_objects.has_value() ? UString::Format(u", number of objects: %d", num_objects.value() + 1) : err_msg);
            break;
        case HOA_signal:
            disp << "HOA Signal - "
                 << (hoa_order.has_value() ? UString::Format(u"order: %d", hoa_order.value() + 1) : err_msg);
            break;
        default:
            disp << " ** Invalid content_type **";
            ok = false;
            break;
    }
    if (ok) {
        disp << ", total bitrate: " << total_bitrate;
    }
    disp << std::endl;
}


void ts::AVS3AudioDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        const uint8_t _codec_id = buf.getBits<uint8_t>(4);
        disp << margin << "Codec ID: " << DataName(MY_XML_NAME, u"audio_codec_id", _codec_id, NamesFlags::VALUE);
        const uint8_t _sampling_frequency_index = buf.getBits<uint8_t>(4);
        switch (_codec_id) {
            case General_Coding: {
                    disp << ", Sampling Frequency (index): " << DataName(MY_XML_NAME, u"sampling_frequency_index", _sampling_frequency_index, NamesFlags::VALUE) << std::endl;
                    general_coding_type gc(buf);
                    gc.display(disp, margin);
                }
                break;
            case Lossless_Coding: {
                    lossless_coding_type lc(buf, _sampling_frequency_index);
                    lc.display(disp, margin, _sampling_frequency_index);
                }
                break;
            case Fullrate_Coding: {
                    disp << ", Sampling Frequency (index): " << DataName(MY_XML_NAME, u"sampling_frequency_index", _sampling_frequency_index, NamesFlags::VALUE) << std::endl;
                    fullrate_coding_type fc(buf);
                    fc.display(disp, margin);
                }
                break;
            default:
                break;
        }
        disp << margin << "Resolution: " << DataName(MY_XML_NAME, u"resolution", buf.getBits<uint8_t>(2), NamesFlags::VALUE) << std::endl;
        buf.skipBits(6);
        disp.displayPrivateData(u"Additional information", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// Enumerations for XML
//----------------------------------------------------------------------------

const ts::Enumeration ts::AVS3AudioDescriptor::GeneralBitstreamTypes({
    {u"uniform", 0},
    {u"variable", 1},
});

const ts::Enumeration ts::AVS3AudioDescriptor::Resolutions({
    {u"8 bits", 0},
    {u"16 bits", 1},
    {u"24 bits", 2},
});

const ts::Enumeration ts::AVS3AudioDescriptor::CodingProfiles({
    {u"basic", 0},
    {u"object", 1},
    {u"HOA", 2},
});


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AVS3AudioDescriptor::general_coding_type::toXML(xml::Element* root) const
{
    root->setEnumAttribute(CodingProfiles, u"coding_profile", coding_profile);
    root->setIntAttribute(u"bitrate_index", bitrate_index, true);
    root->setEnumAttribute(GeneralBitstreamTypes, u"bitstream_type", bitstream_type);
    root->setIntAttribute(u"channel_number_index", channel_number_index, true);
    root->setIntAttribute(u"raw_frame_length", raw_frame_length);
}

void ts::AVS3AudioDescriptor::lossless_coding_type::toXML(xml::Element* root, uint8_t _sampling_frequency_index) const
{
    if (_sampling_frequency_index == 0xF) {
        root->setIntAttribute(u"sampling_frequency", sampling_frequency, true);
    }
    root->setEnumAttribute(CodingProfiles, u"coding_profile", coding_profile);
    root->setIntAttribute(u"channel_number", channel_number);
}

void ts::AVS3AudioDescriptor::fullrate_coding_type::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"nn_type", nn_type);
    root->setOptionalIntAttribute(u"channel_num_index", channel_num_index, true);
    root->setOptionalIntAttribute(u"num_objects", num_objects);
    root->setOptionalIntAttribute(u"hoa_order", hoa_order);
    root->setIntAttribute(u"total_bitrate", total_bitrate);
}

void ts::AVS3AudioDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"sampling_frequency_index", sampling_frequency_index, true);
    root->setEnumAttribute(Resolutions, u"resolution", resolution);

    if (std::holds_alternative<general_coding_type>(coding_data)) {
        std::get<general_coding_type>(coding_data).toXML(root->addElement(u"general_coding"));
    }
    else if (std::holds_alternative<lossless_coding_type>(coding_data)) {
        std::get<lossless_coding_type>(coding_data).toXML(root->addElement(u"lossless_coding"), sampling_frequency_index);
    }
    else if (std::holds_alternative<fullrate_coding_type>(coding_data)) {
        std::get<fullrate_coding_type>(coding_data).toXML(root->addElement(u"fullrate_coding"));
    }
    root->addHexaTextChild(u"additional_info", additional_info, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AVS3AudioDescriptor::general_coding_type::fromXML(const xml::Element* element)
{
    return element->getEnumAttribute(coding_profile, CodingProfiles, u"coding_profile", true) &&
           element->getIntAttribute(bitrate_index, u"bitrate_index", true, 0, 0, 15) &&
           element->getEnumAttribute(bitstream_type, GeneralBitstreamTypes, u"bitstream_type", true, 0) &&
           element->getIntAttribute(channel_number_index, u"channel_number_index", true, 0, 0, 127) &&
           element->getIntAttribute(raw_frame_length, u"raw_frame_length", true);
}

bool ts::AVS3AudioDescriptor::lossless_coding_type::fromXML(const xml::Element* element, uint8_t _sampling_frequency_index)
{
    xml::ElementVector anc_blocks;
    bool ok = element->getEnumAttribute(coding_profile, CodingProfiles, u"coding_profile", true) &&
              element->getIntAttribute(channel_number, u"channel_number", true) &&
              element->getIntAttribute(sampling_frequency, u"sampling_frequency", (_sampling_frequency_index == 0xF), 0, 0, 0x00FFFFFF);

    if (ok && (element->hasAttribute(u"sampling_frequency")) && (_sampling_frequency_index != 0xF)) {
        element->report().warning(u"sampling_frequency is ignored when sampling_frequency_index != 0xF in <%s>, line %d", element->name(), element->lineNumber());
    }
    return ok;
}

bool ts::AVS3AudioDescriptor::fullrate_coding_type::fromXML(const xml::Element* element)
{
    bool ok = element->getIntAttribute(nn_type, u"nn_type", true, 0, 0, 7) &&
              element->getOptionalIntAttribute(channel_num_index, u"channel_num_index", 0, 127) &&
              element->getOptionalIntAttribute(num_objects, u"num_objects", 0, 127) &&
              element->getOptionalIntAttribute(hoa_order, u"hoa_order", 0, 127) &&
              element->getIntAttribute(total_bitrate, u"total_bitrate", true);
    if (content_type() == INVALID_CONTENT_TYPE) {
        element->report().error(u"invalid combination of channel_num_index, num_objects, hoa_order is specified in <%s>, line %d", element->name(), element->lineNumber());
        ok = false;
    }
    return ok;
}

bool ts::AVS3AudioDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    ts::xml::ElementVector gce, lce, fce;
    bool ok = element->getIntAttribute(sampling_frequency_index, u"sampling_frequency_index", true) &&
              element->getEnumAttribute(resolution, Resolutions, u"resolution", true) &&
              element->getChildren(gce, u"general_coding", 0, 1) &&
              element->getChildren(lce, u"lossless_coding", 0, 1) &&
              element->getChildren(fce, u"fullrate_coding", 0, 1) &&
              element->getHexaTextChild(additional_info, u"additional_info", false);

    if (ok && (gce.size() + lce.size() + fce.size() > 1)) {
        element->report().error(u"only one of <general_coding>, <lossless_coding> or <fullrate_coding> is permitted in <%s>, line %d", element->name(), element->lineNumber());
        ok = false;
    }
    if (ok) {
        if (!gce.empty()) {
            audio_codec_id = General_Coding;
            general_coding_type gc;
            if (gc.fromXML(gce[0])) {
                coding_data = gc;
            }
            else {
                ok = false;
            }
        }
        else if (!lce.empty()) {
            audio_codec_id = Lossless_Coding;
            lossless_coding_type lc;
            if (lc.fromXML(lce[0], sampling_frequency_index)) {
                coding_data = lc;
            }
            else {
                ok = false;
            }
        }
        else if (!fce.empty()) {
            audio_codec_id = Fullrate_Coding;
            fullrate_coding_type fc;
            if (fc.fromXML(fce[0])) {
                coding_data = fc;
            }
            else {
                ok = false;
            }
        }
        else {
            element->report().error(u"one of <general_coding>, <lossless_coding> or <fullrate_coding> is required in <%s>, line %d", element->name(), element->lineNumber());
            coding_data = std::monostate {};
            ok = false;
        }
    }
    if (ok && (audio_codec_id == General_Coding || audio_codec_id == Fullrate_Coding) && (sampling_frequency_index > 0x8)) {
        element->report().error(u"sampling_frequency_index must be 0x0..0x8 for General Coding and Fullrate Coding, in <%s> line %d", element->name(), element->lineNumber());
        ok = false;
    }
    if (ok && (audio_codec_id == Lossless_Coding)) {
        if ((sampling_frequency_index > 0x8) && (sampling_frequency_index < 0xF)) {
            element->report().error(u"sampling_frequency_index must be 0x0..0x8 or 0xF for Lossless Coding, in <%s> line %d", element->name(), element->lineNumber());
            ok = false;
        }
    }
    return ok;
}
