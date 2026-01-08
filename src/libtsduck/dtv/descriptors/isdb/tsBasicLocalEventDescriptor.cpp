//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBasicLocalEventDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"basic_local_event_descriptor"
#define MY_CLASS    ts::BasicLocalEventDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_BASIC_LOCAL_EV, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::BasicLocalEventDescriptor::BasicLocalEventDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::BasicLocalEventDescriptor::clearContent()
{
    segmentation_mode = 0;
    start_time_NPT = 0;
    end_time_NPT = 0;
    start_time = cn::milliseconds::zero();
    duration = cn::milliseconds::zero();
    reserved_data.clear();
    component_tags.clear();
}

ts::BasicLocalEventDescriptor::BasicLocalEventDescriptor(DuckContext& duck, const Descriptor& desc) :
    BasicLocalEventDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::BasicLocalEventDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 4);
    buf.putBits(segmentation_mode, 4);
    buf.pushWriteSequenceWithLeadingLength(8); // segmentation_info_length
    if (segmentation_mode == 0) {
    }
    else if (segmentation_mode == 1) {
        buf.putBits(0xFF, 7);
        buf.putBits(start_time_NPT, 33);
        buf.putBits(0xFF, 7);
        buf.putBits(end_time_NPT, 33);
    }
    else if (segmentation_mode < 6) {
        buf.putSecondsBCD(start_time);
        buf.putSecondsBCD(duration);
        if (start_time.count() % 1000 != 0 || duration.count() % 1000 != 0) {
            buf.putBCD(start_time.count() % 1000, 3);
            buf.putBits(0xFF, 4);
            buf.putBCD(duration.count() % 1000, 3);
            buf.putBits(0xFF, 4);
        }
    }
    else {
        buf.putBytes(reserved_data);
    }
    buf.popState(); // update segmentation_info_length
    buf.putBytes(component_tags);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::BasicLocalEventDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(4);
    buf.getBits(segmentation_mode, 4);
    buf.pushReadSizeFromLength(8); // segmentation_info_length
    if (segmentation_mode == 0) {
    }
    else if (segmentation_mode == 1) {
        buf.skipBits(7);
        buf.getBits(start_time_NPT, 33);
        buf.skipBits(7);
        buf.getBits(end_time_NPT, 33);
    }
    else if (segmentation_mode < 6) {
        buf.getSecondsBCD(start_time);
        buf.getSecondsBCD(duration);
        if (buf.canRead()) {
            start_time += cn::milliseconds(buf.getBCD<cn::milliseconds::rep>(3));
            buf.skipBits(4);
            duration += cn::milliseconds(buf.getBCD<cn::milliseconds::rep>(3));
            buf.skipBits(4);
        }
    }
    else {
        buf.getBytes(reserved_data);
    }
    buf.popState(); // end of segmentation_info_length
    buf.getBytes(component_tags);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::BasicLocalEventDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        buf.skipBits(4);
        const uint8_t mode = buf.getBits<uint8_t>(4);
        disp << margin << "Segmentation mode: " << DataName(MY_XML_NAME, u"Mode", mode, NamesFlags::DEC_VALUE_NAME) << std::endl;
        buf.pushReadSizeFromLength(8); // segmentation_info_length
        if (mode == 0) {
        }
        else if (mode == 1) {
            if (buf.canReadBytes(10)) {
                buf.skipBits(7);
                disp << margin << UString::Format(u"Start time NPT: 0x%09X (%<d)", buf.getBits<uint64_t>(33)) << std::endl;
                buf.skipBits(7);
                disp << margin << UString::Format(u"End time NPT: 0x%09X (%<d)", buf.getBits<uint64_t>(33)) << std::endl;
            }
        }
        else if (mode < 6) {
            if (buf.canReadBytes(6)) {
                disp << margin << UString::Format(u"Start time: %02d", buf.getBCD<int>(2));
                disp << UString::Format(u":%02d", buf.getBCD<int>(2));
                disp << UString::Format(u":%02d", buf.getBCD<int>(2));
                const int hour = buf.getBCD<int>(2);
                const int min = buf.getBCD<int>(2);
                const int sec = buf.getBCD<int>(2);
                if (buf.canReadBytes(2)) {
                    disp << UString::Format(u".%03d", buf.getBCD<int>(3));
                    buf.skipBits(4);
                }
                disp << std::endl;
                disp << margin << UString::Format(u"Duration: %02d:%02d:%02d", hour, min, sec);
                if (buf.canReadBytes(2)) {
                    disp << UString::Format(u".%03d", buf.getBCD<int>(3));
                    buf.skipBits(4);
                }
                disp << std::endl;
            }
        }
        else {
            disp.displayPrivateData(u"Reserved data", buf, NPOS, margin);
        }
        disp.displayPrivateData(u"Extraneous segmentation info data", buf, NPOS, margin);
        buf.popState(); // end of segmentation_info_length
        while (buf.canRead()) {
            disp << margin << UString::Format(u"Component tag: %n", buf.getUInt8()) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::BasicLocalEventDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"segmentation_mode", segmentation_mode);
    if (segmentation_mode == 0) {
    }
    else if (segmentation_mode == 1) {
        root->setIntAttribute(u"start_time_NPT", start_time_NPT, true);
        root->setIntAttribute(u"end_time_NPT", end_time_NPT, true);
    }
    else if (segmentation_mode < 6) {
        root->setTimeAttribute(u"start_time", start_time);
        root->setTimeAttribute(u"duration", duration);
        if (start_time.count() % 1000 != 0 || duration.count() % 1000 != 0) {
            root->setAttribute(u"start_time_extension", UString::Format(u"%03d", start_time.count() % 1000));
            root->setAttribute(u"duration_extension", UString::Format(u"%03d", duration.count() % 1000));
        }
    }
    else {
        root->addHexaTextChild(u"reserved_data", reserved_data, true);
    }
    for (auto it : component_tags) {
        root->addElement(u"component")->setIntAttribute(u"tag", it, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::BasicLocalEventDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    cn::milliseconds::rep start_time_extension = 0;
    cn::milliseconds::rep duration_extension = 0;

    bool ok = element->getIntAttribute(segmentation_mode, u"segmentation_mode", true, 0, 0x00, 0x0F) &&
              element->getIntAttribute(start_time_NPT, u"start_time_NPT", segmentation_mode == 1, 0, 0, 0x00000001FFFFFFFF) &&
              element->getIntAttribute(end_time_NPT, u"end_time_NPT", segmentation_mode == 1, 0, 0, 0x00000001FFFFFFFF) &&
              element->getTimeAttribute(start_time, u"start_time", segmentation_mode > 1 && segmentation_mode < 6) &&
              element->getTimeAttribute(duration, u"duration", segmentation_mode > 1 && segmentation_mode < 6) &&
              element->getIntAttribute(start_time_extension, u"start_time_extension", false, 0) &&
              element->getIntAttribute(duration_extension, u"duration_extension", false, 0) &&
              element->getHexaTextChild(reserved_data, u"reserved_data", false);

    // Convert seconds to milliseconds.
    start_time += cn::milliseconds(start_time_extension);
    duration += cn::milliseconds(duration_extension);

    for (auto& xcomp : element->children(u"component", &ok)) {
        ok = xcomp.getIntAttribute(component_tags.emplace_back(), u"tag", true);
    }
    return ok;
}
