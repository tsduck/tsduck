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

#include "tsApplicationIconsDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    icon_locator(),
    icon_flags(0),
    reserved_future_use()
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

void ts::ApplicationIconsDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    const UString margin(indent, ' ');

    if (size > 0) {
        disp << margin << "Icon locator: \"" << disp.duck().decodedWithByteLength(data, size) << "\"" << std::endl;
        if (size >= 2) {
            const uint16_t flags = GetUInt16(data);
            disp << margin << UString::Format(u"Icon flags: 0x%X", {flags}) << std::endl;
            for (uint16_t mask = 0x0001; mask != 0; mask <<= 1) {
                if ((flags & mask) != 0) {
                    disp << margin << "  - " << NameFromSection(u"ApplicationIconFlags", mask) << std::endl;
                }
            }
            disp.displayPrivateData(u"Reserved bytes", data + 2, size - 2, margin);
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
