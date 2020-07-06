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

#include "tsSIParameterDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsMJD.h"
TSDUCK_SOURCE;

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

void ts::SIParameterDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(parameter_version);
    EncodeMJD(update_time, bbp->enlarge(2), 2);  // date only
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt8(it->table_id);
        bbp->appendUInt8(uint8_t(it->table_description.size()));
        bbp->append(it->table_description);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SIParameterDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 3;
    entries.clear();

    if (_is_valid) {
        parameter_version = data[0];
        DecodeMJD(data + 1, 2, update_time);
        data += 3; size -= 3;

        while (size >= 2) {
            Entry e;
            e.table_id = data[0];
            const size_t len = std::min<size_t>(data[1], size - 2);
            e.table_description.copy(data + 2, len);
            entries.push_back(e);
            data += 2 + len; size -= 2 + len;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SIParameterDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 3) {
        const uint8_t version = data[0];
        Time update;
        DecodeMJD(data + 1, 2, update);
        data += 3; size -= 3;

        strm << margin << UString::Format(u"Parameter version: 0x%X (%d)", {version, version}) << std::endl
             << margin << "Update time: " << update.format(Time::DATE) << std::endl;

        while (size >= 2) {
            strm << margin << "- Table id: " << names::TID(duck, data[0], CASID_NULL, names::HEXA_FIRST) << std::endl;
            const size_t len = std::min<size_t>(data[1], size - 2);
            display.displayPrivateData(u"Table description", data + 2, len, indent + 2);
            data += 2 + len; size -= 2 + len;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SIParameterDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"parameter_version", parameter_version, true);
    root->setDateAttribute(u"update_time", update_time);
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"table");
        e->setIntAttribute(u"id", it->table_id, true);
        if (!it->table_description.empty()) {
            e->addHexaText(it->table_description);
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
        element->getIntAttribute<uint8_t>(parameter_version, u"parameter_version", true) &&
        element->getDateAttribute(update_time, u"update_time", true) &&
        element->getChildren(xtables, u"table");

    for (auto it = xtables.begin(); ok && it != xtables.end(); ++it) {
        Entry entry;
        ok = (*it)->getIntAttribute<uint8_t>(entry.table_id, u"id", true) &&
             (*it)->getHexaText(entry.table_description, 0, 255);
        entries.push_back(entry);
    }
    return ok;
}
