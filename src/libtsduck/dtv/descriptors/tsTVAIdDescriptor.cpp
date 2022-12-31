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

#include "tsTVAIdDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"TVA_id_descriptor"
#define MY_CLASS ts::TVAIdDescriptor
#define MY_DID ts::DID_TVA_ID
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TVAIdDescriptor::TVAIdDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    TVA_ids()
{
}

ts::TVAIdDescriptor::TVAIdDescriptor(DuckContext& duck, const Descriptor& desc) :
    TVAIdDescriptor()
{
    deserialize(duck, desc);
}

void ts::TVAIdDescriptor::clearContent()
{
    TVA_ids.clear();
}

ts::TVAIdDescriptor::TVAId::TVAId() :
    TVA_id(0),
    running_status(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TVAIdDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : TVA_ids) {
        buf.putUInt16(it.TVA_id);
        buf.putBits(0xFF, 5);
        buf.putBits(it.running_status, 3);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TVAIdDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        TVAId ti;
        ti.TVA_id = buf.getUInt16();
        buf.skipBits(5);
        buf.getBits(ti.running_status, 3);
        TVA_ids.push_back(ti);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TVAIdDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"TVA id: 0x%X (%<d)", {buf.getUInt16()});
        buf.skipBits(5);
        disp << ", running status: " << DataName(MY_XML_NAME, u"RunningStatus", buf.getBits<uint8_t>(3), NamesFlags::DECIMAL_FIRST) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TVAIdDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : TVA_ids) {
        xml::Element* e = root->addElement(u"TVA");
        e->setIntAttribute(u"id", it.TVA_id, true);
        e->setIntAttribute(u"running_status", it.running_status);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TVAIdDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xtva;
    bool ok = element->getChildren(xtva, u"TVA", 0, MAX_ENTRIES);
    for (auto it = xtva.begin(); ok && it != xtva.end(); ++it) {
        TVAId ti;
        ok = (*it)->getIntAttribute(ti.TVA_id, u"id", true) &&
             (*it)->getIntAttribute(ti.running_status, u"running_status", true, 0, 0, 7);
        TVA_ids.push_back(ti);
    }
    return ok;
}
