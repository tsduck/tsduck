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

#include "tsGraphicsConstraintsDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"graphics_constraints_descriptor"
#define MY_CLASS ts::GraphicsConstraintsDescriptor
#define MY_DID ts::DID_AIT_GRAPHICS_CONST
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::GraphicsConstraintsDescriptor::GraphicsConstraintsDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    can_run_without_visible_ui(false),
    handles_configuration_changed(false),
    handles_externally_controlled_video(false),
    graphics_configuration()
{
}

ts::GraphicsConstraintsDescriptor::GraphicsConstraintsDescriptor(DuckContext& duck, const Descriptor& desc) :
    GraphicsConstraintsDescriptor()
{
    deserialize(duck, desc);
}

void ts::GraphicsConstraintsDescriptor::clearContent()
{
    can_run_without_visible_ui = false;
    handles_configuration_changed = false;
    handles_externally_controlled_video = false;
    graphics_configuration.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::GraphicsConstraintsDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(0xF8 |
                     (can_run_without_visible_ui ? 0x04 : 0x00) |
                     (handles_configuration_changed ? 0x02 : 0x00) |
                     (handles_externally_controlled_video ? 0x01 : 0x00));
    bbp->append(graphics_configuration);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::GraphicsConstraintsDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    graphics_configuration.clear();
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    if (_is_valid) {
        can_run_without_visible_ui = (data[0] & 0x04) != 0;
        handles_configuration_changed = (data[0] & 0x02) != 0;
        handles_externally_controlled_video = (data[0] & 0x01) != 0;
        graphics_configuration.copy(data + 1, size - 1);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::GraphicsConstraintsDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        strm << margin << "Can run without visible UI: " << UString::TrueFalse((data[0] & 0x04) != 0) << std::endl
             << margin << "Handles configuration changed: " << UString::TrueFalse((data[0] & 0x02) != 0) << std::endl
             << margin << "Handles externally controlled video: " << UString::TrueFalse((data[0] & 0x01) != 0) << std::endl;
        display.displayPrivateData(u"Graphics configuration", data + 1, size - 1, indent);
    }
    else {
        display.displayExtraData(data, size, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::GraphicsConstraintsDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"can_run_without_visible_ui", can_run_without_visible_ui);
    root->setBoolAttribute(u"handles_configuration_changed", handles_configuration_changed);
    root->setBoolAttribute(u"handles_externally_controlled_video", handles_externally_controlled_video);
    if (!graphics_configuration.empty()) {
        root->addHexaTextChild(u"graphics_configuration", graphics_configuration);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::GraphicsConstraintsDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(can_run_without_visible_ui, u"can_run_without_visible_ui", true) &&
           element->getBoolAttribute(handles_configuration_changed, u"handles_configuration_changed", true) &&
           element->getBoolAttribute(handles_externally_controlled_video, u"handles_externally_controlled_video", true) &&
           element->getHexaTextChild(graphics_configuration, u"graphics_configuration", false, 0, MAX_DESCRIPTOR_SIZE - 1);
}
