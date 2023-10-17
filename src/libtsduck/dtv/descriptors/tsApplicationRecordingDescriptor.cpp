//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsApplicationRecordingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"application_recording_descriptor"
#define MY_CLASS ts::ApplicationRecordingDescriptor
#define MY_DID ts::DID_AIT_APP_RECORDING
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ApplicationRecordingDescriptor::ApplicationRecordingDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ApplicationRecordingDescriptor::clearContent()
{
    scheduled_recording = false;
    trick_mode_aware = false;
    time_shift = false;
    dynamic = false;
    av_synced = false;
    initiating_replay = false;
    labels.clear();
    component_tags.clear();
    private_data.clear();
    reserved_future_use.clear();
}

ts::ApplicationRecordingDescriptor::ApplicationRecordingDescriptor(DuckContext& duck, const Descriptor& desc) :
    ApplicationRecordingDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ApplicationRecordingDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(scheduled_recording);
    buf.putBit(trick_mode_aware);
    buf.putBit(time_shift);
    buf.putBit(dynamic);
    buf.putBit(av_synced);
    buf.putBit(initiating_replay);
    buf.putBits(0xFF, 2);
    buf.putUInt8(uint8_t(labels.size()));
    for (const auto& it : labels) {
        buf.putStringWithByteLength(it.label);
        buf.putBits(it.storage_properties, 2);
        buf.putBits(0xFF, 6);
    }
    buf.putUInt8(uint8_t(component_tags.size()));
    buf.putBytes(component_tags);
    buf.putUInt8(uint8_t(private_data.size()));
    buf.putBytes(private_data);
    buf.putBytes(reserved_future_use);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ApplicationRecordingDescriptor::deserializePayload(PSIBuffer& buf)
{
    scheduled_recording = buf.getBool();
    trick_mode_aware = buf.getBool();
    time_shift = buf.getBool();
    dynamic = buf.getBool();
    av_synced = buf.getBool();
    initiating_replay = buf.getBool();
    buf.skipBits(2);
    const uint8_t label_count = buf.getUInt8();
    for (size_t i = 0; i < label_count && !buf.error(); ++i) {
        RecodingLabel rl;
        buf.getStringWithByteLength(rl.label);
        buf.getBits(rl.storage_properties, 2);
        buf.skipBits(6);
        labels.push_back(rl);
    }
    buf.pushReadSizeFromLength(8); // component_tag_list_length
    buf.getBytes(component_tags);
    buf.popState();
    buf.pushReadSizeFromLength(8); // private_length
    buf.getBytes(private_data);
    buf.popState();
    buf.getBytes(reserved_future_use);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ApplicationRecordingDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    // Flags in first byte.
    if (buf.canReadBytes(1)) {
        disp << margin << "Scheduled recording: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Trick mode aware: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Time shift: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Dynamic: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Av synced: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Initiating replay: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipBits(2);
    }

    // Labels
    if (buf.canReadBytes(1)) {
        uint8_t labelCount = buf.getUInt8();
        while (buf.canReadBytes(1) && labelCount > 0) {
            disp << margin << "Label: \"" << buf.getStringWithByteLength();
            disp << UString::Format(u"\", storage properties: 0x%X", {buf.getBits<uint8_t>(2)}) << std::endl;
            buf.skipBits(6);
            labelCount--;
        }
    }

    // Component tags.
    if (buf.canReadBytes(1)) {
        uint8_t count = buf.getUInt8();
        while (count > 0 && buf.canReadBytes(1)) {
            disp << margin << UString::Format(u"Component tag: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
            count--;
        }
    }

    // Private data.
    if (buf.canReadBytes(1)) {
        uint8_t count = buf.getUInt8();
        disp.displayPrivateData(u"Private data", buf, count, margin);
        disp.displayPrivateData(u"Reserved bytes", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ApplicationRecordingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"scheduled_recording", scheduled_recording);
    root->setBoolAttribute(u"trick_mode_aware", trick_mode_aware);
    root->setBoolAttribute(u"time_shift", time_shift);
    root->setBoolAttribute(u"dynamic", dynamic);
    root->setBoolAttribute(u"av_synced", av_synced);
    root->setBoolAttribute(u"initiating_replay", initiating_replay);

    for (const auto& it : labels) {
        xml::Element* e = root->addElement(u"label");
        e->setAttribute(u"label", it.label);
        e->setIntAttribute(u"storage_properties", it.storage_properties & 0x03);
    }
    for (auto it : component_tags) {
        root->addElement(u"component")->setIntAttribute(u"tag", it, true);
    }
    root->addHexaTextChild(u"private", private_data, true);
    root->addHexaTextChild(u"reserved_future_use", reserved_future_use, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ApplicationRecordingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector labelChildren;
    xml::ElementVector compChildren;
    bool ok =
        element->getBoolAttribute(scheduled_recording, u"scheduled_recording", true) &&
        element->getBoolAttribute(trick_mode_aware, u"trick_mode_aware", true) &&
        element->getBoolAttribute(time_shift, u"time_shift", true) &&
        element->getBoolAttribute(dynamic, u"dynamic", true) &&
        element->getBoolAttribute(av_synced, u"av_synced", true) &&
        element->getBoolAttribute(initiating_replay, u"initiating_replay", true) &&
        element->getChildren(labelChildren, u"label") &&
        element->getChildren(compChildren, u"component") &&
        element->getHexaTextChild(private_data, u"private") &&
        element->getHexaTextChild(reserved_future_use, u"reserved_future_use");

    for (size_t i = 0; ok && i < labelChildren.size(); ++i) {
        RecodingLabel lab;
        ok = labelChildren[i]->getAttribute(lab.label, u"label", true) &&
             labelChildren[i]->getIntAttribute(lab.storage_properties, u"storage_properties", true, 0, 0, 3);
        labels.push_back(lab);
    }

    for (size_t i = 0; ok && i < compChildren.size(); ++i) {
        uint8_t tag = 0;
        ok = compChildren[i]->getIntAttribute(tag, u"tag", true);
        component_tags.push_back(tag);
    }
    return ok;
}
