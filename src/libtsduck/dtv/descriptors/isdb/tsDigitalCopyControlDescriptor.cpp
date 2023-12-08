//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDigitalCopyControlDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"digital_copy_control_descriptor"
#define MY_CLASS ts::DigitalCopyControlDescriptor
#define MY_DID ts::DID_ISDB_COPY_CONTROL
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DigitalCopyControlDescriptor::DigitalCopyControlDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::DigitalCopyControlDescriptor::clearContent()
{
    digital_recording_control_data = 0;
    user_defined = 0;
    maximum_bitrate.reset();
    components.clear();
}

ts::DigitalCopyControlDescriptor::DigitalCopyControlDescriptor(DuckContext& duck, const Descriptor& desc) :
    DigitalCopyControlDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DigitalCopyControlDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(digital_recording_control_data, 2);
    buf.putBit(maximum_bitrate.has_value());
    buf.putBit(!components.empty());
    buf.putBits(user_defined, 4);
    if (maximum_bitrate.has_value()) {
        buf.putUInt8(maximum_bitrate.value());
    }
    if (!components.empty()) {
        buf.pushWriteSequenceWithLeadingLength(8); // component_control_length
        for (const auto& it : components) {
            buf.putUInt8(it.component_tag);
            buf.putBits(it.digital_recording_control_data, 2);
            buf.putBit(it.maximum_bitrate.has_value());
            buf.putBit(1);
            buf.putBits(it.user_defined, 4);
            if (it.maximum_bitrate.has_value()) {
                buf.putUInt8(it.maximum_bitrate.value());
            }
        }
        buf.popState(); // update component_control_length
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DigitalCopyControlDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(digital_recording_control_data, 2);
    bool bitrate_flag = buf.getBool();
    bool comp_flag = buf.getBool();
    buf.getBits(user_defined, 4);
    if (bitrate_flag) {
        maximum_bitrate = buf.getUInt8();
    }
    if (comp_flag) {
        buf.pushReadSizeFromLength(8); // component_control_length
        while (buf.canRead()) {
            Component comp;
            comp.component_tag = buf.getUInt8();
            buf.getBits(comp.digital_recording_control_data, 2);
            bitrate_flag = buf.getBool();
            buf.skipBits(1);
            buf.getBits(comp.user_defined, 4);
            if (bitrate_flag) {
                comp.maximum_bitrate = buf.getUInt8();
            }
            components.push_back(comp);
        }
        buf.popState(); // component_control_length
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DigitalCopyControlDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Recording control: " << DataName(MY_XML_NAME, u"CopyControl", buf.getBits<uint8_t>(2), NamesFlags::DECIMAL_FIRST) << std::endl;
        const bool bitrate_flag = buf.getBool();
        const bool comp_flag = buf.getBool();
        disp << margin << UString::Format(u"User-defined: 0x%1X (%<d)", {buf.getBits<uint8_t>(4)}) << std::endl;

        if (bitrate_flag && buf.canReadBytes(1)) {
            // Bitrate unit is 1/4 Mb/s.
            const uint32_t mbr = buf.getUInt8();
            disp << margin << UString::Format(u"Maximum bitrate: %d (%'d b/s)", {mbr, mbr * 250000}) << std::endl;
        }
        if (comp_flag) {
            buf.pushReadSizeFromLength(8); // component_control_length
            while (buf.canReadBytes(2)) {
                disp << margin << UString::Format(u"- Component tag: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
                disp << margin << "  Recording control: " << DataName(MY_XML_NAME, u"CopyControl", buf.getBits<uint8_t>(2), NamesFlags::DECIMAL_FIRST) << std::endl;
                const bool bflag = buf.getBool();
                buf.skipBits(1);
                disp << margin << UString::Format(u"  User-defined: 0x%1X (%<d)", {buf.getBits<uint8_t>(4)}) << std::endl;
                if (bflag && buf.canReadBytes(1)) {
                    const uint32_t mbr = buf.getUInt8();
                    disp << margin << UString::Format(u"  Maximum bitrate: %d (%'d b/s)", {mbr, mbr * 250000}) << std::endl;
                }
            }
            buf.popState(); // component_control_length
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DigitalCopyControlDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"digital_recording_control_data", digital_recording_control_data, false);
    root->setIntAttribute(u"user_defined", user_defined, false);
    root->setOptionalIntAttribute(u"maximum_bitrate", maximum_bitrate, false);
    for (const auto& it : components) {
        xml::Element* e = root->addElement(u"component_control");
        e->setIntAttribute(u"component_tag", it.component_tag, false);
        e->setIntAttribute(u"digital_recording_control_data", it.digital_recording_control_data, false);
        e->setIntAttribute(u"user_defined", it.user_defined, false);
        e->setOptionalIntAttribute(u"maximum_bitrate", it.maximum_bitrate, false);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DigitalCopyControlDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcomp;
    bool ok =
        element->getIntAttribute(digital_recording_control_data, u"digital_recording_control_data", true, 0, 0x00, 0x03) &&
        element->getIntAttribute(user_defined, u"user_defined", false, 0, 0x00, 0x0F) &&
        element->getOptionalIntAttribute(maximum_bitrate, u"maximum_bitrate") &&
        element->getChildren(xcomp, u"component_control");

    for (size_t i = 0; ok && i < xcomp.size(); ++i) {
        Component comp;
        ok = xcomp[i]->getIntAttribute(comp.component_tag, u"component_tag", true) &&
             xcomp[i]->getIntAttribute(comp.digital_recording_control_data, u"digital_recording_control_data", true, 0, 0x00, 0x03) &&
             xcomp[i]->getIntAttribute(comp.user_defined, u"user_defined", false, 0, 0x00, 0x0F) &&
             xcomp[i]->getOptionalIntAttribute(comp.maximum_bitrate, u"maximum_bitrate");
        components.push_back(comp);
    }
    return ok;
}
