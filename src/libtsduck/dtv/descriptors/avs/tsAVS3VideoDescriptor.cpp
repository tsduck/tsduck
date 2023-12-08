//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAVS3VideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsAlgorithm.h"

#define MY_XML_NAME u"AVS3_video_descriptor"
#define MY_CLASS ts::AVS3VideoDescriptor
#define MY_DID ts::DID_AVS3_VIDEO
#define MY_PDS ts::PDS_AVS
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

// T/AI 109.2 Table B.1
const std::vector<uint8_t> ts::AVS3VideoDescriptor::valid_profile_ids {
    0x20, 0x22, 0x30, 0x32
};

// T/AI 109.2 Table B.2
const std::vector<uint8_t> ts::AVS3VideoDescriptor::valid_level_ids {
    0x10, 0x12, 0x14, 0x20, 0x22,
    0x40, 0x42, 0x41, 0x43, 0x44, 0x46, 0x45, 0x47, 0x48, 0x4a, 0x49, 0x4b,
    0x50, 0x52, 0x51, 0x53, 0x54, 0x56, 0x55, 0x57, 0x58, 0x5a, 0x59, 0x5b,
    0x60, 0x62, 0x61, 0x63, 0x64, 0x66, 0x65, 0x67, 0x68, 0x6a, 0x69, 0x6b
};


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AVS3VideoDescriptor::AVS3VideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

void ts::AVS3VideoDescriptor::clearContent()
{
    profile_id = 0;
    level_id = 0;
    multiple_frame_rate_flag = false;
    frame_rate_code = 0;
    sample_precision = 0;
    chroma_format = 0;
    temporal_id_flag = false;
    td_mode_flag = false;
    library_stream_flag = false;
    library_picture_enable_flag = false;
    colour_primaries = 0;
    transfer_characteristics = 0;
    matrix_coefficients = 0;
}

ts::AVS3VideoDescriptor::AVS3VideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    AVS3VideoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AVS3VideoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(profile_id);
    buf.putUInt8(level_id);
    buf.putBit(multiple_frame_rate_flag);
    buf.putBits(frame_rate_code, 4);
    buf.putBits(sample_precision, 3);
    buf.putBits(chroma_format, 2);
    buf.putBit(temporal_id_flag);
    buf.putBit(td_mode_flag);
    buf.putBit(library_stream_flag);
    buf.putBit(library_picture_enable_flag);
    buf.putBits(0xFF, 2);
    buf.putUInt8(colour_primaries);
    buf.putUInt8(transfer_characteristics);
    buf.putUInt8(matrix_coefficients);
    buf.putBits(0xFF, 8);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AVS3VideoDescriptor::deserializePayload(PSIBuffer& buf)
{
    profile_id = buf.getUInt8();
    level_id = buf.getUInt8();
    multiple_frame_rate_flag = buf.getBool();
    buf.getBits(frame_rate_code, 4);
    buf.getBits(sample_precision, 3);
    buf.getBits(chroma_format, 2);
    temporal_id_flag = buf.getBool();
    td_mode_flag = buf.getBool();
    library_stream_flag = buf.getBool();
    library_picture_enable_flag = buf.getBool();
    buf.skipBits(2);
    colour_primaries = buf.getUInt8();
    transfer_characteristics = buf.getUInt8();
    matrix_coefficients = buf.getUInt8();
    buf.skipBits(8);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AVS3VideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(8)) {
        disp << margin << "Profile ID: " << DataName(MY_XML_NAME, u"profile", buf.getUInt8(), NamesFlags::VALUE);
        disp << ", Level ID: " << DataName(MY_XML_NAME, u"level", buf.getUInt8(), NamesFlags::VALUE);
        disp << ", Multiple frame rate: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Frame rate code: " << DataName(MY_XML_NAME, u"frame_rate", buf.getBits<uint8_t>(4), NamesFlags::VALUE | NamesFlags::DECIMAL);
        disp << ", Sample precision: " << DataName(MY_XML_NAME, u"sample_precision", buf.getBits<uint8_t>(3), NamesFlags::VALUE | NamesFlags::DECIMAL);
        disp << ", Chroma format: " << DataName(MY_XML_NAME, u"chroma_format", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        disp << margin << "Temporal ID: " << UString::TrueFalse(buf.getBool());
        disp << ", TD mode: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Library stream: " << UString::TrueFalse(buf.getBool());
        disp << ", Library picture enable: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipBits(2); // T/AI 109.6 is not explicit on the value for reserved bits
        const uint8_t cp = buf.getUInt8();
        const uint8_t tc = buf.getUInt8();
        const uint8_t mc = buf.getUInt8();
        disp << margin << UString::Format(u"Colour primaries: %d, Transfer characteristics: %d, Matrix coefficients: %d", {cp, tc, mc}) << std::endl;
        buf.skipBits(8); // T/AI 109.6 is not explicit on the value for reserved bits
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AVS3VideoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"profile_id", profile_id, true);
    root->setIntAttribute(u"level_id", level_id, true);
    root->setBoolAttribute(u"multiple_frame_rate_flag", multiple_frame_rate_flag);
    root->setIntAttribute(u"frame_rate_code", frame_rate_code);
    root->setIntAttribute(u"sample_precision", sample_precision);
    root->setIntAttribute(u"chroma_format", chroma_format);
    root->setBoolAttribute(u"temporal_id_flag", temporal_id_flag);
    root->setBoolAttribute(u"td_mode_flag", td_mode_flag);
    root->setBoolAttribute(u"library_stream_flag", library_stream_flag);
    root->setBoolAttribute(u"library_picture_enable_flag", library_picture_enable_flag);
    root->setIntAttribute(u"colour_primaries", colour_primaries);
    root->setIntAttribute(u"transfer_characteristics", transfer_characteristics);
    root->setIntAttribute(u"matrix_coefficients", matrix_coefficients);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AVS3VideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute(profile_id, u"profile_id", true, 0, 0x20, 0x32) &&
        element->getIntAttribute(level_id, u"level_id", true, 0, 0x10, 0x6B) &&
        element->getBoolAttribute(multiple_frame_rate_flag, u"multiple_frame_rate_flag", false) &&
        element->getIntAttribute(frame_rate_code, u"frame_rate_code", true, 0, 0x01, 0x0D) &&
        element->getIntAttribute(sample_precision, u"sample_precision", true, 0, 0x01, 0x02) &&
        element->getIntAttribute(chroma_format, u"chroma_format", true, 0, 0x01, 0x01) &&
        element->getBoolAttribute(temporal_id_flag, u"temporal_id_flag", false) &&
        element->getBoolAttribute(td_mode_flag, u"td_mode_flag", false) &&
        element->getBoolAttribute(library_stream_flag, u"library_stream_flag", false) &&
        element->getBoolAttribute(library_picture_enable_flag, u"library_picture_enable_flag", false) &&
        element->getIntAttribute(colour_primaries, u"colour_primaries", true, 0, 1, 9) &&
        element->getIntAttribute(transfer_characteristics, u"transfer_characteristics", true, 0, 1, 14) &&
        element->getIntAttribute(matrix_coefficients, u"matrix_coefficients", true, 0, 1, 9); // although 3 is 'reserved'

    if (!Contains(valid_profile_ids, profile_id)) {
        element->report().error(u"'%d' is not a valid profile_id in <%s>, line %d", {profile_id, element->name(), element->lineNumber()});
        ok = false;
    }
    if (!Contains(valid_level_ids, level_id)) {
        element->report().error(u"'%d' is not a valid level_id in <%s>, line %d", {level_id, element->name(), element->lineNumber()});
        ok = false;
    }
    return ok;
}
