//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsApplicationIconsDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"application_icons_descriptor"
#define MY_CLASS ts::ApplicationIconsDescriptor
#define MY_DID ts::DID_AIT_APP_ICONS
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ApplicationIconsDescriptor::ApplicationIconsDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

void ts::ApplicationIconsDescriptor::clearContent()
{
    icon_locator.clear();
    icon_flags = 0;
    reserved_future_use.clear();
}

ts::ApplicationIconsDescriptor::ApplicationIconsDescriptor(DuckContext& duck, const Descriptor& desc) :
    ApplicationIconsDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ApplicationIconsDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putStringWithByteLength(icon_locator);
    buf.putUInt16(icon_flags);
    buf.putBytes(reserved_future_use);
}

void ts::ApplicationIconsDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getStringWithByteLength(icon_locator);
    icon_flags = buf.getUInt16();
    buf.getBytes(reserved_future_use);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ApplicationIconsDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Icon locator: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        if (buf.canReadBytes(2)) {
            const uint16_t flags = buf.getUInt16();
            disp << margin << UString::Format(u"Icon flags: 0x%X", {flags}) << std::endl;
            for (uint16_t mask = 0x0001; mask != 0; mask <<= 1) {
                if ((flags & mask) != 0) {
                    disp << margin << "  - " << DataName(MY_XML_NAME, u"IconFlags", mask) << std::endl;
                }
            }
            disp.displayPrivateData(u"Reserved bytes", buf, NPOS, margin);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ApplicationIconsDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"icon_locator", icon_locator);
    root->setIntAttribute(u"icon_flags", icon_flags, true);
    root->addHexaTextChild(u"reserved_future_use", reserved_future_use, true);
}

bool ts::ApplicationIconsDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(icon_locator, u"icon_locator", true) &&
           element->getIntAttribute(icon_flags, u"icon_flags", true) &&
           element->getHexaTextChild(reserved_future_use, u"reserved_future_use");
}
