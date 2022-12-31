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

#include "tsSIParameterDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"SI_parameter_descriptor"
#define MY_CLASS ts::SIParameterDescriptor
#define MY_DID ts::DID_ISDB_SI_PARAMETER
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SIParameterDescriptor::SIParameterDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    parameter_version(0),
    update_time(),
    entries()
{
}

void ts::SIParameterDescriptor::clearContent()
{
    parameter_version = 0;
    update_time.clear();
    entries.clear();
}

ts::SIParameterDescriptor::SIParameterDescriptor(DuckContext& duck, const Descriptor& desc) :
    SIParameterDescriptor()
{
    deserialize(duck, desc);
}

ts::SIParameterDescriptor::Entry::Entry() :
    table_id(TID_NULL),
    table_description()
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SIParameterDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(parameter_version);
    buf.putMJD(update_time, 2);  // 2 bytes: date only
    for (const auto& it : entries) {
        buf.putUInt8(it.table_id);
        buf.putUInt8(uint8_t(it.table_description.size()));
        buf.putBytes(it.table_description);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SIParameterDescriptor::deserializePayload(PSIBuffer& buf)
{
    parameter_version = buf.getUInt8();
    update_time = buf.getMJD(2); // 2 bytes: date only
    while (buf.canRead()) {
        Entry e;
        e.table_id = buf.getUInt8();
        const size_t len = buf.getUInt8();
        buf.getBytes(e.table_description, len);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SIParameterDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"Parameter version: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp << margin << "Update time: " << buf.getMJD(2).format(Time::DATE) << std::endl;
        while (buf.canReadBytes(2)) {
            disp << margin << "- Table id: " << names::TID(disp.duck(), buf.getUInt8(), CASID_NULL, NamesFlags::HEXA_FIRST) << std::endl;
            disp.displayPrivateData(u"Table description", buf, buf.getUInt8(), margin + u"  ");
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SIParameterDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"parameter_version", parameter_version, true);
    root->setDateAttribute(u"update_time", update_time);
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"table");
        e->setIntAttribute(u"id", it.table_id, true);
        if (!it.table_description.empty()) {
            e->addHexaText(it.table_description);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SIParameterDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xtables;
    bool ok =
        element->getIntAttribute(parameter_version, u"parameter_version", true) &&
        element->getDateAttribute(update_time, u"update_time", true) &&
        element->getChildren(xtables, u"table");

    for (auto it = xtables.begin(); ok && it != xtables.end(); ++it) {
        Entry entry;
        ok = (*it)->getIntAttribute(entry.table_id, u"id", true) &&
             (*it)->getHexaText(entry.table_description, 0, 255);
        entries.push_back(entry);
    }
    return ok;
}
