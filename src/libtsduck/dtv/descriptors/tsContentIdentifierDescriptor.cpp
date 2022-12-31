//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsContentIdentifierDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"content_identifier_descriptor"
#define MY_CLASS ts::ContentIdentifierDescriptor
#define MY_DID ts::DID_CONTENT_ID
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ContentIdentifierDescriptor::ContentIdentifierDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    crids()
{
}

ts::ContentIdentifierDescriptor::ContentIdentifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    ContentIdentifierDescriptor()
{
    deserialize(duck, desc);
}

void ts::ContentIdentifierDescriptor::clearContent()
{
    crids.clear();
}

ts::ContentIdentifierDescriptor::CRID::CRID() :
    crid_type(0),
    crid_location(0),
    crid_ref(0),
    crid()
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ContentIdentifierDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : crids) {
        buf.putBits(it.crid_type, 6);
        buf.putBits(it.crid_location, 2);
        if (it.crid_location == 0) {
            buf.putUTF8WithLength(it.crid);
        }
        else if (it.crid_location == 1) {
            buf.putUInt16(it.crid_ref);
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ContentIdentifierDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        CRID cr;
        buf.getBits(cr.crid_type, 6);
        buf.getBits(cr.crid_location, 2);
        if (cr.crid_location == 0) {
            buf.getUTF8WithLength(cr.crid);
        }
        else if (cr.crid_location == 1) {
            cr.crid_ref = buf.getUInt16();
        }
        crids.push_back(cr);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ContentIdentifierDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(1)) {
        disp << margin << "- CRID type: " << DataName(MY_XML_NAME, u"CRIDType", buf.getBits<uint8_t>(6), NamesFlags::HEXA_FIRST) << std::endl;
        const uint8_t loc = buf.getBits<uint8_t>(2);
        disp << margin << "  CRID location: " << DataName(MY_XML_NAME, u"CRIDLocation", loc, NamesFlags::DECIMAL_FIRST) << std::endl;
        if (loc == 0 && buf.canReadBytes(1)) {
            disp << margin << "  CRID: \"" << buf.getUTF8WithLength() << "\"" << std::endl;
        }
        else if (loc == 1 && buf.canReadBytes(2)) {
            disp << margin << UString::Format(u"  CRID reference: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ContentIdentifierDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : crids) {
        xml::Element* e = root->addElement(u"crid");
        e->setIntAttribute(u"crid_type", it.crid_type, true);
        e->setIntAttribute(u"crid_location", it.crid_location);
        if (it.crid_location == 0) {
            e->setAttribute(u"crid", it.crid);
        }
        else if (it.crid_location == 1) {
            e->setIntAttribute(u"crid_ref", it.crid_ref, true);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ContentIdentifierDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcrid;
    bool ok = element->getChildren(xcrid, u"crid");
    for (auto it = xcrid.begin(); ok && it != xcrid.end(); ++it) {
        CRID cr;
        ok = (*it)->getIntAttribute(cr.crid_type, u"crid_type", true, 0, 0, 0x3F) &&
             (*it)->getIntAttribute(cr.crid_location, u"crid_location", true, 0, 0, 3) &&
             (*it)->getIntAttribute(cr.crid_ref, u"crid_ref", cr.crid_location == 1) &&
             (*it)->getAttribute(cr.crid, u"crid", cr.crid_location == 0, UString(), 0, 255);
        crids.push_back(cr);
    }
    return ok;
}
