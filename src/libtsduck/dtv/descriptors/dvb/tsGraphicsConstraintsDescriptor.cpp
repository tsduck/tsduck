//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsGraphicsConstraintsDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

void ts::GraphicsConstraintsDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 5);
    buf.putBit(can_run_without_visible_ui);
    buf.putBit(handles_configuration_changed);
    buf.putBit(handles_externally_controlled_video);
    buf.putBytes(graphics_configuration);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::GraphicsConstraintsDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(5);
    can_run_without_visible_ui = buf.getBool();
    handles_configuration_changed = buf.getBool();
    handles_externally_controlled_video = buf.getBool();
    buf.getBytes(graphics_configuration);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::GraphicsConstraintsDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        buf.skipBits(5);
        disp << margin << "Can run without visible UI: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Handles configuration changed: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Handles externally controlled video: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp.displayPrivateData(u"Graphics configuration", buf, NPOS, margin);
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
