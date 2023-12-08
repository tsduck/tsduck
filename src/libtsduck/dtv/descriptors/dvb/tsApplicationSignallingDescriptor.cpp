//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsApplicationSignallingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"application_signalling_descriptor"
#define MY_CLASS ts::ApplicationSignallingDescriptor
#define MY_DID ts::DID_APPLI_SIGNALLING
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ApplicationSignallingDescriptor::ApplicationSignallingDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ApplicationSignallingDescriptor::clearContent()
{
    entries.clear();
}

ts::ApplicationSignallingDescriptor::ApplicationSignallingDescriptor(DuckContext& duck, const Descriptor& desc) :
    ApplicationSignallingDescriptor()
{
    deserialize(duck, desc);
}

ts::ApplicationSignallingDescriptor::Entry::Entry(uint16_t type, uint8_t version) :
    application_type(type),
    AIT_version_number(version)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ApplicationSignallingDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putBit(1);
        buf.putBits(it.application_type, 15);
        buf.putBits(0xFF, 3);
        buf.putBits(it.AIT_version_number, 5);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ApplicationSignallingDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry e;
        buf.skipBits(1);
        buf.getBits(e.application_type, 15);
        buf.skipBits(3);
        buf.getBits(e.AIT_version_number, 5);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ApplicationSignallingDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(3)) {
        buf.skipBits(1);
        disp << margin << UString::Format(u"Application type: %d (0x%<X)", {buf.getBits<uint16_t>(15)});
        buf.skipBits(3);
        disp << UString::Format(u", AIT Version: %d (0x%<X)", {buf.getBits<uint8_t>(5)}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ApplicationSignallingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"application");
        e->setIntAttribute(u"application_type", it.application_type, true);
        e->setIntAttribute(u"AIT_version_number", it.AIT_version_number, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ApplicationSignallingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"application", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute(entry.application_type, u"application_type", true, 0, 0x0000, 0x7FFF) &&
             children[i]->getIntAttribute(entry.AIT_version_number, u"AIT_version_number", true, 0, 0x00, 0x1F);
        entries.push_back(entry);
    }
    return ok;
}
