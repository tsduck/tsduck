//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsJPEGXSVideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"JPEG_XS_video_descriptor"
#define MY_CLASS ts::JPEGXSVideoDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_JXS_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::JPEGXSVideoDescriptor::JPEGXSVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::JPEGXSVideoDescriptor::clearContent()
{
    descriptor_version = 0;
    horizontal_size = 0;
    vertical_size = 0;
    brat = 0;
    interlace_mode = 0;
    framerate_DEN = 1;
    framerate_NUM = 0;
    sample_bitdepth.reset();
    sampling_structure.reset();
    Ppih = 0;
    level = 0;
    sublevel = 0;
    max_buffer_size = 0;
    buffer_model_type = 2;
    colour_primaries = 0;
    transfer_characteristics = 0;
    matrix_coefficients = 0;
    video_full_range_flag = false;
    still_mode = false;
    mdm.reset();
    private_data.clear();
}

ts::JPEGXSVideoDescriptor::JPEGXSVideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    JPEGXSVideoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::JPEGXSVideoDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::JPEGXSVideoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(descriptor_version);
    buf.putUInt16(horizontal_size);
    buf.putUInt16(vertical_size);
    buf.putUInt32(brat);
    buf.putBits(interlace_mode, 2);
    buf.putBits(framerate_DEN, 6);
    buf.putUInt8(0);
    buf.putUInt16(framerate_NUM);
    bool valid_flag = sample_bitdepth.has_value() && sampling_structure.has_value();
    buf.putBit(valid_flag);
    if (valid_flag) {
        buf.putBits(0x00, 7);
        buf.putBits(sample_bitdepth.value(), 4);
        buf.putBits(sampling_structure.value(), 4);
    }
    else {
        buf.putBits(0x0000, 15);
    }
    buf.putUInt16(Ppih);
    buf.putUInt8(level);
    buf.putUInt8(sublevel);
    buf.putUInt32(max_buffer_size);
    buf.putUInt8(buffer_model_type);
    buf.putUInt8(colour_primaries);
    buf.putUInt8(transfer_characteristics);
    buf.putUInt8(matrix_coefficients);
    buf.putBit(video_full_range_flag);
    buf.putBits(0xFF, 7);
    buf.putBit(still_mode);
    buf.putBit(mdm.has_value());
    buf.putBits(0x00, 6);
    if (mdm.has_value()) {
        mdm.value().serialize(buf);
    }
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::JPEGXSVideoDescriptor::deserializePayload(PSIBuffer& buf)
{
    descriptor_version = buf.getUInt8();
    horizontal_size = buf.getUInt16();
    vertical_size = buf.getUInt16();
    brat = buf.getUInt32();
    interlace_mode = buf.getBits<uint8_t>(2);
    framerate_DEN = buf.getBits<uint8_t>(6);
    buf.skipBits(8);
    framerate_NUM = buf.getUInt16();
    bool valid_flag = buf.getBool();
    if (valid_flag) {
        buf.skipBits(7);
        sample_bitdepth = buf.getBits<uint8_t>(4);
        sampling_structure = buf.getBits<uint8_t>(4);
    }
    else {
        buf.skipBits(15);
    }
    Ppih = buf.getUInt16();
    level = buf.getUInt8();
    sublevel = buf.getUInt8();
    max_buffer_size = buf.getUInt32();
    buffer_model_type = buf.getUInt8();
    colour_primaries = buf.getUInt8();
    transfer_characteristics = buf.getUInt8();
    matrix_coefficients = buf.getUInt8();
    video_full_range_flag = buf.getBool();
    buf.skipBits(7);
    still_mode = buf.getBool();
    bool have_mdm = buf.getBool();
    buf.skipBits(6);
    if (have_mdm) {
        Mastering_Display_Metadata_type m(buf);
        mdm = m;
    }
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::JPEGXSVideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(28)) {
        disp << margin << "Descriptor version: " << int(buf.getUInt8());
        disp << ", horizontal size: " << buf.getUInt16() << ", vertical size: " << buf.getUInt16() << std::endl;
        disp << margin << "Max bitrate: " << buf.getUInt32() << "Mbit/s" << std::endl;
        disp << margin << "Interlace: " << DataName(MY_XML_NAME, u"interlace_mode", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        uint8_t denominator = buf.getBits<uint8_t>(6);
        buf.skipReservedBits(8, 0);
        disp << margin << "Framerate: " << buf.getUInt16() << "/" << DataName(MY_XML_NAME, u"framerate_DEN", denominator);
        bool _valid_flag = buf.getBool();
        if (_valid_flag) {
            buf.skipReservedBits(7, 0);
            disp << ", bitdepth: " << int(buf.getBits<uint8_t>(4)+1) << "bits";
            disp << ", structure: " << DataName(MY_XML_NAME, u"sampling_structure", buf.getBits<uint8_t>(4), NamesFlags::VALUE | NamesFlags::DECIMAL);
        }
        else {
            buf.skipReservedBits(15, 0);
        }
        disp << std::endl;
        uint16_t _Ppih = buf.getUInt16();
        disp << margin << "Profile: " << DataName(MY_XML_NAME, u"profile", _Ppih, NamesFlags::VALUE);
        disp << ", level: " << DataName(MY_XML_NAME, u"level", buf.getUInt8(), NamesFlags::VALUE);
        disp << ", sublevel: " << DataName(MY_XML_NAME, u"sublevel", buf.getUInt8(), NamesFlags::VALUE) << std::endl;
        disp << margin << "Max buffer size: " << buf.getUInt32();
        disp << ", buffer model: " << int(buf.getUInt8()) << std::endl;
        disp << margin << "Colour primaries: " << DataName(MY_XML_NAME, u"colour_primaries", buf.getUInt8(), NamesFlags::VALUE | NamesFlags::DECIMAL);
        disp << ", transfer characteristics: " << DataName(MY_XML_NAME, u"transfer_characteristics", buf.getUInt8(), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        disp << margin << "Matrix coefficients: " << DataName(MY_XML_NAME, u"matrix_coefficients", buf.getUInt8(), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        disp << margin << "Video full range: " << UString::TrueFalse(buf.getBool());
        buf.skipReservedBits(7);
        disp << ", still mode: " << UString::TrueFalse(buf.getBool()) << std::endl;
        bool mdm_flag = buf.getBool();
        buf.skipReservedBits(6, 0);

        if (mdm_flag) {
            Mastering_Display_Metadata_type t;
            t.display(disp, buf, margin);
        }

        disp.displayPrivateData(u"private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::JPEGXSVideoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"descriptor_version", descriptor_version);
    root->setIntAttribute(u"horizontal_size", horizontal_size);
    root->setIntAttribute(u"vertical_size", vertical_size);
    root->setIntAttribute(u"brat", brat);
    root->setIntAttribute(u"interlace_mode", interlace_mode);
    root->setIntEnumAttribute(FramerateDenominators, u"framerate_DEN", framerate_DEN);
    root->setIntAttribute(u"framerate_NUM", framerate_NUM);
    root->setOptionalIntAttribute(u"sample_bitdepth", sample_bitdepth);
    root->setOptionalIntAttribute(u"sampling_structure", sampling_structure);
    root->setIntAttribute(u"Ppih", Ppih, true);
    root->setIntAttribute(u"level", level, true);
    root->setIntAttribute(u"sublevel", sublevel, true);
    root->setIntAttribute(u"max_buffer_size", max_buffer_size);
    root->setIntAttribute(u"buffer_model_type", buffer_model_type);
    root->setIntAttribute(u"colour_primaries", colour_primaries);
    root->setIntAttribute(u"transfer_characteristics", transfer_characteristics);
    root->setIntAttribute(u"matrix_coefficients", matrix_coefficients);
    root->setBoolAttribute(u"video_full_range_flag", video_full_range_flag);
    root->setBoolAttribute(u"still_mode", still_mode);

    if (mdm.has_value()) {
        mdm.value().toXML(root->addElement(u"mdm"));
    }
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// Enumerations for XML.
//----------------------------------------------------------------------------

const ts::Enumeration ts::JPEGXSVideoDescriptor::FramerateDenominators({
    // Table A.8 of ISO/IEC 21122-3
    {u"1", 1},
    {u"1.001", 2},
});


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::JPEGXSVideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector mdms;
    bool ok =
        element->getIntAttribute(descriptor_version, u"descriptor_version", true, 0, 0x00, 0x00) &&
        element->getIntAttribute(horizontal_size, u"horizontal_size", true) &&
        element->getIntAttribute(vertical_size, u"vertical_size", true) &&
        element->getIntAttribute(brat, u"brat", true) &&
        element->getIntAttribute(interlace_mode, u"interlace_mode", true, 0, 0, 0x03) &&
        element->getIntEnumAttribute(framerate_DEN, FramerateDenominators, u"framerate_DEN", true) &&
        element->getIntAttribute(framerate_NUM, u"framerate_NUM", true) &&
        element->getOptionalIntAttribute(sample_bitdepth, u"sample_bitdepth", 0, 0xF) &&
        element->getOptionalIntAttribute(sampling_structure, u"sampling_structure", 0, 0xF) &&
        element->getIntAttribute(Ppih, u"Ppih", true) &&
        element->getIntAttribute(level, u"level", true) &&
        element->getIntAttribute(sublevel, u"sublevel", true) &&
        element->getIntAttribute(max_buffer_size, u"max_buffer_size", true) &&
        element->getIntAttribute(buffer_model_type, u"buffer_model_type", true, 0, 2, 2) &&
        element->getIntAttribute(colour_primaries, u"colour_primaries", true) &&
        element->getIntAttribute(transfer_characteristics, u"transfer_characteristics", true) &&
        element->getIntAttribute(matrix_coefficients, u"matrix_coefficients", true) &&
        element->getBoolAttribute(video_full_range_flag, u"video_full_range_flag", true) &&
        element->getBoolAttribute(still_mode, u"still_mode", true) &&
        element->getHexaTextChild(private_data, u"private_data", false) &&
        element->getChildren(mdms, u"mdm", 0, 1);

    if (ok) {
        if ((sample_bitdepth.has_value() && !sampling_structure.has_value()) ||
            (!sample_bitdepth.has_value() && sampling_structure.has_value())) {
            element->report().error(u"neither or both of sample_bitdepth and sampling_structure are to be signalled  in <%s> at line %d", {element->name(), element->lineNumber()});
            ok = false;
        }
    }

    if (ok) {
        if (!mdms.empty()) {
            Mastering_Display_Metadata_type newMDM;
            ok = newMDM.fromXML(mdms[0]);
            if (ok) {
                mdm = newMDM;
            }
        }
    }
    return ok;
}
