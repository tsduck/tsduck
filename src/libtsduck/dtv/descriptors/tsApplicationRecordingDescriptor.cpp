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

#include "tsApplicationRecordingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    scheduled_recording(false),
    trick_mode_aware(false),
    time_shift(false),
    dynamic(false),
    av_synced(false),
    initiating_replay(false),
    labels(),
    component_tags(),
    private_data(),
    reserved_future_use()
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

void ts::ApplicationRecordingDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((scheduled_recording ? 0x80 : 0x00) |
                     (trick_mode_aware ? 0x40 : 0x00) |
                     (time_shift ? 0x20 : 0x00) |
                     (dynamic ? 0x10 : 0x00) |
                     (av_synced ? 0x08 : 0x00) |
                     (initiating_replay ? 0x04 : 0x00) |
                     0x03);
    bbp->appendUInt8(uint8_t(labels.size()));
    for (auto it = labels.begin(); it != labels.end(); ++it) {
        bbp->append(duck.encodedWithByteLength(it->label));
        bbp->appendUInt8(uint8_t(it->storage_properties << 6) | 0x3F);
    }
    bbp->appendUInt8(uint8_t(component_tags.size()));
    bbp->append(component_tags);
    bbp->appendUInt8(uint8_t(private_data.size()));
    bbp->append(private_data);
    bbp->append(reserved_future_use);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ApplicationRecordingDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    labels.clear();
    component_tags.clear();
    private_data.clear();
    reserved_future_use.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 4;

    // Flags in first byte.
    scheduled_recording = (data[0] & 0x80) != 0;
    trick_mode_aware = (data[0] & 0x40) != 0;
    time_shift = (data[0] & 0x20) != 0;
    dynamic = (data[0] & 0x10) != 0;
    av_synced = (data[0] & 0x08) != 0;
    initiating_replay = (data[0] & 0x04) != 0;

    // Labels
    uint8_t labelCount = data[1];
    data += 2;
    size -= 2;
    while (_is_valid && labelCount > 0) {
        _is_valid = size >= 1 && size >= size_t(data[0] + 2);
        if (_is_valid) {
            const size_t len = data[0];
            labels.push_back(RecodingLabel(duck.decoded(data + 1, len), (data[len + 1] >> 6) & 0x03));
            data += len + 2;
            size -= len + 2;
            labelCount--;
        }
    }

    // Component tags.
    _is_valid = _is_valid && size >= 1 && size >= size_t(1 + data[0]);
    if (_is_valid) {
        const size_t len = data[0];
        component_tags.copy(data + 1, len);
        data += len + 1;
        size -= len + 1;
    }

    // Private data.
    _is_valid = _is_valid && size >= 1 && size >= size_t(1 + data[0]);
    if (_is_valid) {
        const size_t len = data[0];
        private_data.copy(data + 1, len);
        data += len + 1;
        size -= len + 1;
    }

    // Reserved area.
    if (_is_valid) {
        reserved_future_use.copy(data, size);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ApplicationRecordingDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    // Flags in first byte.
    bool valid = size >= 1;
    if (valid) {
        strm << margin << "Scheduled recording: " << UString::TrueFalse((data[0] & 0x80) != 0) << std::endl
             << margin << "Trick mode aware: " << UString::TrueFalse((data[0] & 0x40) != 0) << std::endl
             << margin << "Time shift: " << UString::TrueFalse((data[0] & 0x20) != 0) << std::endl
             << margin << "Dynamic: " << UString::TrueFalse((data[0] & 0x10) != 0) << std::endl
             << margin << "Av synced: " << UString::TrueFalse((data[0] & 0x08) != 0) << std::endl
             << margin << "Initiating replay: " << UString::TrueFalse((data[0] & 0x04) != 0) << std::endl;
        data++; size--;
    }

    // Labels
    valid = valid && size >= 1;
    if (valid) {
        uint8_t labelCount = data[0];
        data++; size--;
        while (valid && labelCount > 0) {
            valid = size >= 1 && size >= size_t(data[0] + 2);
            if (valid) {
                const size_t len = data[0];
                strm << margin << UString::Format(u"Label: \"%s\", storage properties: 0x%X", {duck.decoded(data + 1, len), uint8_t((data[len + 1] >> 6) & 0x03)}) << std::endl;
                data += len + 2;
                size -= len + 2;
                labelCount--;
            }
        }
        valid = valid && labelCount == 0;
    }

    // Component tags.
    valid = valid && size >= 1 && size >= size_t(1 + data[0]);
    if (valid) {
        uint8_t count = data[0];
        data++; size--;
        while (count > 0) {
            strm << margin << UString::Format(u"Component tag: 0x%X (%d)", {data[0], data[0]}) << std::endl;
            data++; size--;
            count--;
        }
    }

    // Private data.
    valid = valid && size >= 1 && size >= size_t(1 + data[0]);
    if (valid) {
        uint8_t count = data[0];
        data++; size--;
        display.displayPrivateData(u"Private data", data, count, indent);
        data += count; size -= count;
        display.displayPrivateData(u"Reserved bytes", data, size, indent);
        size = 0;
    }

    display.displayExtraData(data, size, indent);
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

    for (auto it = labels.begin(); it != labels.end(); ++it) {
        xml::Element* e = root->addElement(u"label");
        e->setAttribute(u"label", it->label);
        e->setIntAttribute(u"storage_properties", it->storage_properties & 0x03);
    }
    for (auto it = component_tags.begin(); it != component_tags.end(); ++it) {
        root->addElement(u"component")->setIntAttribute(u"tag", *it, true);
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
             labelChildren[i]->getIntAttribute<uint8_t>(lab.storage_properties, u"storage_properties", true, 0, 0, 3);
        labels.push_back(lab);
    }

    for (size_t i = 0; ok && i < compChildren.size(); ++i) {
        uint8_t tag = 0;
        ok = compChildren[i]->getIntAttribute<uint8_t>(tag, u"tag", true);
        component_tags.push_back(tag);
    }
    return ok;
}
