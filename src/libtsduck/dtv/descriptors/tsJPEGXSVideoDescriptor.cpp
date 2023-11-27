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
    frat = 0;
    schar = 0;
    Ppih = 0;
    Plev = 0;
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
    buf.putUInt32(frat);  // TODO: put as progressive/interlaced flag + 31 bits framerate
    buf.putUInt16(schar);
    buf.putUInt16(Ppih);
    buf.putUInt16(Plev);
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
    frat = buf.getUInt32();
    schar = buf.getUInt16();
    Ppih = buf.getUInt16();
    Plev = buf.getUInt16();
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
        disp << margin << "Max bitrate: " << buf.getUInt32();
        disp << ", framerate: " << buf.getUInt32();
        disp << ", sample characteristics: " << buf.getUInt16() << std::endl;
        disp << margin << "Profile: " << buf.getUInt16();
        disp << ", level: " << buf.getUInt16();
        disp << ", max buffer size: " << buf.getUInt32();
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
    root->setIntAttribute(u"descriptor_version", descriptor_version, false);
    root->setIntAttribute(u"horizontal_size", horizontal_size, false);
    root->setIntAttribute(u"vertical_size", vertical_size, false);
    root->setIntAttribute(u"brat", brat, false);
    root->setIntAttribute(u"frat", frat, false);
    root->setIntAttribute(u"schar", schar, false);
    root->setIntAttribute(u"Ppih", Ppih, false);
    root->setIntAttribute(u"Plev", Plev, false);
    root->setIntAttribute(u"max_buffer_size", max_buffer_size, false);
    root->setIntAttribute(u"buffer_model_type", buffer_model_type, false);
    root->setIntAttribute(u"colour_primaries", colour_primaries, false);
    root->setIntAttribute(u"transfer_characteristics", transfer_characteristics, false);
    root->setIntAttribute(u"matrix_coefficients", matrix_coefficients, false);
    root->setBoolAttribute(u"video_full_range_flag", video_full_range_flag);
    root->setBoolAttribute(u"still_mode", still_mode);

    if (mdm.has_value()) {
        mdm.value().toXML(root->addElement(u"mdm"));
    }
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::JPEGXSVideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector mdms;
    bool ok =
        element->getIntAttribute(descriptor_version, u"descriptor_version", true, 0, 0x00, 0x00) &&
        element->getIntAttribute(horizontal_size, u"horizontal_size", true, 0) &&
        element->getIntAttribute(vertical_size, u"vertical_size", true, 0) &&
        element->getIntAttribute(brat, u"brat", true, 0) &&
        element->getIntAttribute(frat, u"frat", true, 0) &&
        element->getIntAttribute(schar, u"schar", true, 0) &&
        element->getIntAttribute(Ppih, u"Ppih", true, 0) &&
        element->getIntAttribute(Plev, u"Plev", true, 0) &&
        element->getIntAttribute(max_buffer_size, u"max_buffer_size", true, 0) &&
        element->getIntAttribute(buffer_model_type, u"buffer_model_type", true, 0, 2, 2) &&
        element->getIntAttribute(colour_primaries, u"colour_primaries", true, 0) &&
        element->getIntAttribute(transfer_characteristics, u"transfer_characteristics", true, 0) &&
        element->getIntAttribute(matrix_coefficients, u"matrix_coefficients", true, 0) &&
        element->getBoolAttribute(video_full_range_flag, u"video_full_range_flag", true) &&
        element->getBoolAttribute(still_mode, u"still_mode", true) &&
        element->getHexaTextChild(private_data, u"private_data", false) &&
        element->getChildren(mdms, u"mdm", 0, 1);

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
